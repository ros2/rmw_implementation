// Copyright 2026 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Regression test for an AB-BA lock-order inversion in the subscription
// QoS-event path.
//
// Two mutexes are involved, per subscription:
//   R = the DataReader's internal mutex
//   E = the rmw subscription event mutex
//
//   * Executor side (the take loop below):
//       rmw_take_event(MESSAGE_LOST) locks E, then queries the reader's
//       sample-lost status, which locks R.                          (E -> R)
//
//   * DDS receive side (driven by real sample loss):
//       the reader holds R while delivering on_sample_lost, which calls back
//       into rmw and locks E.                                       (R -> E)
//
// Run concurrently with actual SAMPLE_LOST traffic, these orderings deadlock on
// an affected implementation.
//
// To exercise it, all three must hold:
//   1. The MESSAGE_LOST event callback is armed via rmw_event_set_callback()
//      (this is what an events-driven executor does; it makes the implementation
//      deliver on_sample_lost under the reader mutex -- the R->E edge).
//   2. Real SAMPLE_LOST occurs. The CMake registration sets a profile that
//      disables intra-process delivery for rmw_fastrtps; intra-process delivery
//      hands samples over inline and never loses them. If no loss is generated
//      the test reports "scenario not exercised" rather than a misleading pass.
//   3. The take loop and the publisher flood run concurrently (done below).
//
// On an affected implementation the take loop wedges and the watchdog fails the
// test; on a fixed implementation the take loop keeps progressing and it passes.
//
// For deterministic detection, build the rmw implementation with ThreadSanitizer;
// TSAN reports the inversion the first time both orderings are observed.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <tuple>

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/rmw.h"

#include "test_msgs/msg/basic_types.h"

namespace
{
constexpr std::chrono::seconds kMaxRunTime{20};       // overall cap
constexpr std::chrono::seconds kStallWindow{3};       // no take progress => deadlock
constexpr std::chrono::milliseconds kDiscovery{500};  // endpoint matching
}  // namespace

class TestEventMessageLostDeadlock : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    init_options = rmw_get_zero_initialized_init_options();
    ASSERT_EQ(RMW_RET_OK, rmw_init_options_init(&init_options, allocator));
    init_options.enclave = rcutils_strdup("/", allocator);
    ASSERT_STREQ("/", init_options.enclave);
    ASSERT_EQ(RMW_RET_OK, rmw_init(&init_options, &context));
    node = rmw_create_node(&context, "test_event_message_lost_deadlock", "/");
    ASSERT_NE(nullptr, node);
  }

  void TearDown() override
  {
    if (node) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_node(node));
    }
    EXPECT_EQ(RMW_RET_OK, rmw_shutdown(&context));
    EXPECT_EQ(RMW_RET_OK, rmw_context_fini(&context));
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(&init_options));
  }

  rmw_init_options_t init_options{rmw_get_zero_initialized_init_options()};
  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
  rmw_publisher_options_t pub_options = rmw_get_default_publisher_options();
  rmw_subscription_options_t sub_options = rmw_get_default_subscription_options();
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  const char * topic_name = "/message_lost_deadlock";
};

TEST_F(TestEventMessageLostDeadlock, take_event_does_not_deadlock_with_on_sample_lost)
{
  // Shallow, best-effort QoS so a non-draining reader under a publisher flood drops
  // samples and generates SAMPLE_LOST.
  rmw_qos_profile_t qos = rmw_qos_profile_default;
  qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
  qos.depth = 1;
  qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;

  rmw_publisher_t * pub = rmw_create_publisher(node, ts, topic_name, &qos, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  rmw_subscription_t * sub = rmw_create_subscription(node, ts, topic_name, &qos, &sub_options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;

  std::this_thread::sleep_for(kDiscovery);  // let endpoints match

  // Arm the MESSAGE_LOST event callback so the implementation delivers on_sample_lost
  // under the reader mutex -- the R->E edge of the inversion.
  rmw_event_t event{rmw_get_zero_initialized_event()};
  ASSERT_EQ(RMW_RET_OK, rmw_subscription_event_init(&event, sub, RMW_EVENT_MESSAGE_LOST))
    << rmw_get_error_string().str;

  std::atomic<uint64_t> events_seen{0};
  rmw_event_callback_t cb = [](const void * user_data, size_t /*number_of_events*/) {
      auto * counter = static_cast<std::atomic<uint64_t> *>(const_cast<void *>(user_data));
      counter->fetch_add(1, std::memory_order_relaxed);
    };
  ASSERT_EQ(RMW_RET_OK, rmw_event_set_callback(&event, cb, &events_seen))
    << rmw_get_error_string().str;

  std::atomic<bool> stop{false};
  std::atomic<uint64_t> take_iters{0};

  // Publisher flood: messages are never taken on the reader, so the depth-1 history
  // overruns and SAMPLE_LOST fires on the receive thread (which holds the reader mutex
  // when it invokes on_sample_lost).
  std::thread flood([&]() {
      test_msgs__msg__BasicTypes msg;
      test_msgs__msg__BasicTypes__init(&msg);
      while (!stop.load(std::memory_order_relaxed)) {
        // Failures are irrelevant here; the flood must keep running.
        std::ignore = rmw_publish(pub, &msg, nullptr);
      }
      test_msgs__msg__BasicTypes__fini(&msg);
    });

  // take_event loop: take_event locks the event mutex, then queries the reader's
  // sample-lost status (reader mutex) -- the E->R edge.
  std::thread taker([&]() {
      rmw_message_lost_status_t status;
      bool taken = false;
      while (!stop.load(std::memory_order_relaxed)) {
        // Failures are irrelevant here; the loop must keep taking.
        std::ignore = rmw_take_event(&event, &status, &taken);
        take_iters.fetch_add(1, std::memory_order_relaxed);
      }
    });

  // Watchdog: the deadlock wedges the taker (stuck acquiring the reader mutex), so its
  // iteration counter freezes while loss continues. The best-effort publisher keeps
  // running regardless, so we watch the taker specifically.
  const auto start = std::chrono::steady_clock::now();
  auto last_progress = start;
  uint64_t last_take = 0;
  bool deadlocked = false;

  while (std::chrono::steady_clock::now() - start < kMaxRunTime) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const uint64_t now_take = take_iters.load(std::memory_order_relaxed);
    if (now_take != last_take) {
      last_take = now_take;
      last_progress = std::chrono::steady_clock::now();
      continue;
    }
    // No take progress. Only call it a deadlock once loss has actually been generated,
    // i.e. the R->E edge is genuinely being exercised.
    if (events_seen.load(std::memory_order_relaxed) > 0 &&
      std::chrono::steady_clock::now() - last_progress > kStallWindow)
    {
      deadlocked = true;
      break;
    }
  }

  stop.store(true, std::memory_order_relaxed);

  if (deadlocked) {
    // The workers are wedged on the inverted mutexes and cannot be joined or recovered
    // (TearDown would also wedge destroying DDS entities). Detach and terminate non-zero
    // so the test is recorded as a failure instead of hanging.
    flood.detach();
    taker.detach();
    std::cerr
      << "\nDEADLOCK DETECTED: rmw_take_event(MESSAGE_LOST) (event mutex -> reader mutex) "
      "deadlocked against on_sample_lost (reader mutex -> event mutex).\n"
      << "  take_event iterations before stall: " << take_iters.load() << "\n"
      << "  sample-lost events observed: " << events_seen.load() << "\n";
    std::quick_exit(1);
  }

  flood.join();
  taker.join();

  EXPECT_EQ(RMW_RET_OK, rmw_event_fini(&event));
  EXPECT_EQ(RMW_RET_OK, rmw_destroy_subscription(node, sub));
  EXPECT_EQ(RMW_RET_OK, rmw_destroy_publisher(node, pub));

  // If no loss was ever generated, the R->E edge never ran, so the scenario was not
  // exercised and "no deadlock" proves nothing. This happens when intra-process delivery
  // was not disabled, or on an implementation that does not produce SAMPLE_LOST for a
  // depth-1 best-effort reader that never takes. Skip cleanly rather than reporting a
  // misleading pass or a failure unrelated to any deadlock.
  if (events_seen.load() == 0u) {
    GTEST_SKIP()
      << "No SAMPLE_LOST was generated, so the lock-order inversion was not exercised. "
      "Ensure intra-process delivery is disabled (see the profile in the CMake env).";
  }
}
