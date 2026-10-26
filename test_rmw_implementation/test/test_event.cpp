// Copyright 2023 Sony Group Corporation.
// Copyright 2026 Dennis Lanov
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

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/qos_profiles.h"
#include "rmw/rmw.h"

#include "test_msgs/msg/basic_types.h"

class TestEvent : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rmw_ret_t ret = rmw_init_options_init(&init_options, allocator);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    ret = rmw_enclave_options_copy("/", &allocator, &init_options.enclave);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ASSERT_STREQ("/", init_options.enclave);
    ret = rmw_init(&init_options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    constexpr char node_name[] = "my_test_event";
    constexpr char node_namespace[] = "/my_test_ns";
    node = rmw_create_node(&context, node_name, node_namespace);
    ASSERT_NE(nullptr, node) << rcutils_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_init_options_fini(&init_options);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  void wait_and_take_event(rmw_event_t * event, void * event_info)
  {
    rmw_events_t events;
    void * events_storage[1];
    events_storage[0] = event;
    events.events = events_storage;
    events.event_count = 1;

    // rmw_wait
    rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, 1);
    ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      EXPECT_EQ(
        RMW_RET_OK, rmw_destroy_wait_set(wait_set)) << rmw_get_error_string().str;
    });
    rmw_time_t timeout = {1, 0};  // 1000ms
    rmw_ret_t ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, &events, wait_set, &timeout);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

    bool taken;
    EXPECT_EQ(RMW_RET_OK, rmw_take_event(event, event_info, &taken));
    EXPECT_TRUE(taken);
  }

  rmw_init_options_t init_options{rmw_get_zero_initialized_init_options()};
  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
  rmw_publisher_options_t pub_options = rmw_get_default_publisher_options();
  rmw_subscription_options_t sub_options = rmw_get_default_subscription_options();
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  const char * topic_name = "/test_topic";
};

struct EventUserData
{
  std::shared_ptr<std::atomic_size_t> event_count;
};

extern "C"
{
void event_callback(const void * user_data, size_t number_of_events)
{
  (void)number_of_events;
  const struct EventUserData * data = static_cast<const struct EventUserData *>(user_data);
  ASSERT_NE(nullptr, data);
  (*data->event_count)++;
}
}

TEST_F(TestEvent, basic_publisher_matched_event) {
  // Notice: Not support connextdds since it doesn't support rmw_event_set_callback() interface
  if (std::string(rmw_get_implementation_identifier()).find("rmw_connextdds") == 0) {
    GTEST_SKIP();
  }

  rmw_publisher_t * pub =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
  });

  rmw_event_t pub_matched_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_publisher_event_init(&pub_matched_event, pub, RMW_EVENT_PUBLICATION_MATCHED);
  ASSERT_EQ(RMW_RET_OK, ret);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&pub_matched_event)) << rmw_get_error_string().str;
  });

  struct EventUserData matched_data;
  matched_data.event_count = std::make_shared<std::atomic_size_t>(0);
  ret = rmw_event_set_callback(&pub_matched_event, event_callback, &matched_data);
  ASSERT_EQ(RMW_RET_OK, ret);

  // to take event if there is no subscription
  {
    rmw_matched_status_t matched_status;
    bool taken;
    EXPECT_EQ(RMW_RET_OK, rmw_take_event(&pub_matched_event, &matched_status, &taken));
    EXPECT_EQ(0, matched_status.total_count);
    EXPECT_EQ(0, matched_status.total_count_change);
    EXPECT_EQ(0, matched_status.current_count);
    EXPECT_EQ(0, matched_status.current_count_change);
    EXPECT_TRUE(taken);
  }

  // test the matched event while a subscription coming
  rmw_subscription_t * sub1 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub1) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 1);

  rmw_subscription_t * sub2 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub2) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 2);

  // wait matched event
  rmw_matched_status_t matched_status;
  wait_and_take_event(&pub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(2, matched_status.total_count_change);
  EXPECT_EQ(2, matched_status.current_count);
  EXPECT_EQ(2, matched_status.current_count_change);

  // Next, check unmatched status change
  *matched_data.event_count = 0;

  // test the unmatched event while the subscription exiting
  ret = rmw_destroy_subscription(node, sub1);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 1);

  ret = rmw_destroy_subscription(node, sub2);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 2);

  // wait unmatched status change
  wait_and_take_event(&pub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(0, matched_status.current_count);
  EXPECT_EQ(-2, matched_status.current_count_change);
}

TEST_F(TestEvent, basic_subscription_matched_event) {
  // Notice: Not support connextdds since it doesn't support rmw_event_set_callback() interface
  if (std::string(rmw_get_implementation_identifier()).find("rmw_connextdds") == 0) {
    GTEST_SKIP();
  }

  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub)) << rmw_get_error_string().str;
  });

  rmw_event_t sub_matched_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_subscription_event_init(
    &sub_matched_event,
    sub,
    RMW_EVENT_SUBSCRIPTION_MATCHED);
  ASSERT_EQ(RMW_RET_OK, ret);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&sub_matched_event)) << rmw_get_error_string().str;
  });

  struct EventUserData matched_data;
  matched_data.event_count = std::make_shared<std::atomic_size_t>(0);
  ret = rmw_event_set_callback(&sub_matched_event, event_callback, &matched_data);
  ASSERT_EQ(RMW_RET_OK, ret);

  // to take event if there is no publisher
  {
    rmw_matched_status_t matched_status;
    bool taken;
    EXPECT_EQ(RMW_RET_OK, rmw_take_event(&sub_matched_event, &matched_status, &taken));
    EXPECT_EQ(0, matched_status.total_count);
    EXPECT_EQ(0, matched_status.total_count_change);
    EXPECT_EQ(0, matched_status.current_count);
    EXPECT_EQ(0, matched_status.current_count_change);
    EXPECT_TRUE(taken);
  }

  // test the matched event while a publisher coming
  rmw_publisher_t * pub1 =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub1) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 1);

  rmw_publisher_t * pub2 =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub2) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 2);

  // wait matched event
  rmw_matched_status_t matched_status;
  wait_and_take_event(&sub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(2, matched_status.total_count_change);
  EXPECT_EQ(2, matched_status.current_count);
  EXPECT_EQ(2, matched_status.current_count_change);

  // Next, check unmatched status change
  *matched_data.event_count = 0;

  // test the unmatched event while the publisher exiting
  ret = rmw_destroy_publisher(node, pub1);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 1);

  ret = rmw_destroy_publisher(node, pub2);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(*matched_data.event_count, 2);

  // wait unmatched status change
  wait_and_take_event(&sub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(0, matched_status.current_count);
  EXPECT_EQ(-2, matched_status.current_count_change);
}

TEST_F(TestEvent, one_pub_multi_sub_connect_disconnect) {
  rmw_publisher_t * pub =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
  });

  rmw_event_t pub_matched_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_publisher_event_init(&pub_matched_event, pub, RMW_EVENT_PUBLICATION_MATCHED);
  ASSERT_EQ(RMW_RET_OK, ret);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&pub_matched_event)) << rmw_get_error_string().str;
  });

  // test the matched event while a subscription coming
  rmw_subscription_t * sub1 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub1) << rmw_get_error_string().str;

  rmw_subscription_t * sub2 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub2) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  rmw_matched_status_t matched_status;
  bool taken;
  EXPECT_EQ(RMW_RET_OK, rmw_take_event(&pub_matched_event, &matched_status, &taken));
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(2, matched_status.total_count_change);
  EXPECT_EQ(2, matched_status.current_count);
  EXPECT_EQ(2, matched_status.current_count_change);
  EXPECT_TRUE(taken);

  // test the unmatched status change while the subscription exiting
  ret = rmw_destroy_subscription(node, sub1);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  // wait unmatched status change
  wait_and_take_event(&pub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(1, matched_status.current_count);
  EXPECT_EQ(-1, matched_status.current_count_change);

  ret = rmw_destroy_subscription(node, sub2);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  wait_and_take_event(&pub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(0, matched_status.current_count);
  EXPECT_EQ(-1, matched_status.current_count_change);
}

TEST_F(TestEvent, one_sub_multi_pub_matched_unmatched_event) {
  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub)) << rmw_get_error_string().str;
  });

  rmw_event_t sub_matched_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_subscription_event_init(
    &sub_matched_event,
    sub,
    RMW_EVENT_SUBSCRIPTION_MATCHED);
  ASSERT_EQ(RMW_RET_OK, ret);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&sub_matched_event)) << rmw_get_error_string().str;
  });

  // test the matched event while a publisher coming
  rmw_publisher_t * pub1 =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub1) << rmw_get_error_string().str;

  rmw_publisher_t * pub2 =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub2) << rmw_get_error_string().str;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  rmw_matched_status_t matched_status;
  bool taken;
  EXPECT_EQ(RMW_RET_OK, rmw_take_event(&sub_matched_event, &matched_status, &taken));
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(2, matched_status.total_count_change);
  EXPECT_EQ(2, matched_status.current_count);
  EXPECT_EQ(2, matched_status.current_count_change);
  EXPECT_TRUE(taken);

  // test the unmatched status change while the publisher exiting
  ret = rmw_destroy_publisher(node, pub1);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  // wait unmatched status change
  wait_and_take_event(&sub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(1, matched_status.current_count);
  EXPECT_EQ(-1, matched_status.current_count_change);

  ret = rmw_destroy_publisher(node, pub2);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  wait_and_take_event(&sub_matched_event, &matched_status);
  EXPECT_EQ(2, matched_status.total_count);
  EXPECT_EQ(0, matched_status.total_count_change);
  EXPECT_EQ(0, matched_status.current_count);
  EXPECT_EQ(-1, matched_status.current_count_change);
}

TEST_F(TestEvent, publisher_matched_event_after_wait_set_entities_change) {
  rmw_publisher_t * pub =
    rmw_create_publisher(node, ts, topic_name, &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
  });

  rmw_event_t pub_matched_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_publisher_event_init(&pub_matched_event, pub, RMW_EVENT_PUBLICATION_MATCHED);
  ASSERT_EQ(RMW_RET_OK, ret);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&pub_matched_event)) << rmw_get_error_string().str;
  });

  constexpr size_t gc_count = 3;
  rmw_guard_condition_t * gcs[gc_count];
  for (size_t i = 0; i < gc_count; ++i) {
    gcs[i] = rmw_create_guard_condition(&context);
    ASSERT_NE(nullptr, gcs[i]) << rmw_get_error_string().str;
  }
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    for (size_t i = 0; i < gc_count; ++i) {
      EXPECT_EQ(
        RMW_RET_OK, rmw_destroy_guard_condition(gcs[i])) << rmw_get_error_string().str;
    }
  });

  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, gc_count + 1);
  ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_wait_set(wait_set)) << rmw_get_error_string().str;
  });

  // Wait for a match with a wait set holding `used_gc_count` guard conditions, none of which is
  // triggered, so only the event may be reported as ready.
  auto wait_for_match = [&](size_t used_gc_count, size_t expected_current_count) {
      void * gc_storage[gc_count];
      for (size_t i = 0; i < used_gc_count; ++i) {
        gc_storage[i] = gcs[i]->data;
      }
      rmw_guard_conditions_t guard_conditions;
      guard_conditions.guard_condition_count = used_gc_count;
      guard_conditions.guard_conditions = gc_storage;
      void * events_storage[1] = {&pub_matched_event};
      rmw_events_t events;
      events.event_count = 1;
      events.events = events_storage;

      rmw_time_t timeout = {1, 0};  // 1000ms
      ASSERT_EQ(
        RMW_RET_OK,
        rmw_wait(nullptr, &guard_conditions, nullptr, nullptr, &events, wait_set, &timeout)) <<
        rmw_get_error_string().str;
      EXPECT_NE(nullptr, events.events[0]);
      for (size_t i = 0; i < used_gc_count; ++i) {
        EXPECT_EQ(nullptr, guard_conditions.guard_conditions[i]) << "guard condition " << i;
      }

      rmw_matched_status_t matched_status;
      bool taken = false;
      EXPECT_EQ(RMW_RET_OK, rmw_take_event(&pub_matched_event, &matched_status, &taken));
      EXPECT_TRUE(taken);
      EXPECT_EQ(expected_current_count, matched_status.current_count);
    };

  rmw_subscription_t * sub1 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub1) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub1)) << rmw_get_error_string().str;
  });
  wait_for_match(1, 1u);

  rmw_subscription_t * sub2 =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &sub_options);
  ASSERT_NE(nullptr, sub2) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub2)) << rmw_get_error_string().str;
  });
  // Wait with more entities than in the previous rmw_wait call.
  wait_for_match(gc_count, 2u);
}

namespace
{

// Long enough that only a missed wakeup reaches it.
constexpr rmw_time_t kEventTimeout = {10, 0};
// Used where rmw_wait is expected to time out.
constexpr rmw_time_t kShortTimeout = {0, 200000000};
constexpr int32_t kSampleValue = 930;

rmw_qos_profile_t reliable_qos_profile()
{
  rmw_qos_profile_t qos = rmw_qos_profile_default;
  qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
  qos.depth = 10;
  qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
  qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
  return qos;
}

rmw_qos_profile_t best_effort_qos_profile()
{
  rmw_qos_profile_t qos = reliable_qos_profile();
  qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
  return qos;
}

}  // namespace

// Waits on the QoS events of a RELIABLE subscription, without the subscription itself
// in the wait set.
class TestEventWait : public TestEvent
{
protected:
  void SetUp() override
  {
    ASSERT_NO_FATAL_FAILURE(TestEvent::SetUp());
    wait_set = rmw_create_wait_set(&context, 0);
    ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;
    const rmw_qos_profile_t qos = reliable_qos_profile();
    sub = rmw_create_subscription(node, ts, wait_topic_name, &qos, &sub_options);
    ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    for (rmw_event_t & event : events) {
      EXPECT_EQ(RMW_RET_OK, rmw_event_fini(&event)) << rmw_get_error_string().str;
    }
    for (rmw_publisher_t * pub : publishers) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
    }
    if (sub) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_subscription(node, sub)) << rmw_get_error_string().str;
    }
    if (wait_set) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_wait_set(wait_set)) << rmw_get_error_string().str;
    }
    TestEvent::TearDown();
  }

  // Initializes the subscription events of the given types that this rmw implementation
  // supports. They are finalized in TearDown().
  void init_supported_events(const std::vector<rmw_event_type_t> & event_types)
  {
    // The events are not moved once initialized.
    events.reserve(event_types.size());
    for (rmw_event_type_t event_type : event_types) {
      if (!rmw_event_type_is_supported(event_type)) {
        continue;
      }
      events.push_back(rmw_get_zero_initialized_event());
      rmw_ret_t ret = rmw_subscription_event_init(&events.back(), sub, event_type);
      if (RMW_RET_OK != ret) {
        events.pop_back();
      }
      ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
  }

  rmw_publisher_t * create_publisher(const rmw_qos_profile_t & qos)
  {
    rmw_publisher_t * pub = rmw_create_publisher(node, ts, wait_topic_name, &qos, &pub_options);
    if (pub) {
      publishers.push_back(pub);
    }
    return pub;
  }

  // rmw_wait() sets the entries that are not ready to NULL, so the array is rebuilt on
  // every call. On return, `ready` holds the entries as rmw_wait() left them.
  rmw_ret_t wait_for_events(const rmw_time_t & timeout, std::vector<void *> & ready)
  {
    ready.clear();
    for (rmw_event_t & event : events) {
      ready.push_back(&event);
    }
    rmw_events_t rmw_events;
    rmw_events.event_count = ready.size();
    rmw_events.events = ready.data();
    return rmw_wait(nullptr, nullptr, nullptr, nullptr, &rmw_events, wait_set, &timeout);
  }

  bool wait_for_subscription(const rmw_time_t & timeout)
  {
    void * subscriber = sub->data;
    rmw_subscriptions_t subscriptions;
    subscriptions.subscriber_count = 1;
    subscriptions.subscribers = &subscriber;
    rmw_ret_t ret = rmw_wait(
      &subscriptions, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout);
    EXPECT_TRUE(RMW_RET_OK == ret || RMW_RET_TIMEOUT == ret) << rmw_get_error_string().str;
    return RMW_RET_OK == ret && nullptr != subscriber;
  }

  // Publishes until a sample is pending on the subscription, without taking it. A sample
  // published before the endpoints have matched may be dropped, so publishing is retried.
  void publish_unread_sample(rmw_publisher_t * pub)
  {
    test_msgs__msg__BasicTypes msg;
    ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&msg));
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      test_msgs__msg__BasicTypes__fini(&msg);
    });
    msg.int32_value = kSampleValue;
    constexpr rmw_time_t retry_timeout = {1, 0};
    for (int attempt = 0; attempt < 10; ++attempt) {
      ASSERT_EQ(RMW_RET_OK, rmw_publish(pub, &msg, nullptr)) << rmw_get_error_string().str;
      if (wait_for_subscription(retry_timeout)) {
        return;
      }
    }
    FAIL() << "published sample never became pending";
  }

  void expect_sample_pending_and_take_it()
  {
    EXPECT_TRUE(wait_for_subscription({0, 0}));
    test_msgs__msg__BasicTypes msg;
    ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&msg));
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      test_msgs__msg__BasicTypes__fini(&msg);
    });
    bool taken = false;
    EXPECT_EQ(RMW_RET_OK, rmw_take(sub, &msg, &taken, nullptr)) << rmw_get_error_string().str;
    EXPECT_TRUE(taken);
    EXPECT_EQ(kSampleValue, msg.int32_value);
  }

  // Whether a BEST_EFFORT publisher raises a REQUESTED_QOS_INCOMPATIBLE event on the
  // RELIABLE subscription with this rmw implementation.
  static bool reliability_mismatch_is_reported()
  {
    if (!rmw_event_type_is_supported(RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE)) {
      return false;
    }
    rmw_qos_compatibility_type_t compatibility;
    rmw_ret_t ret = rmw_qos_profile_check_compatible(
      best_effort_qos_profile(), reliable_qos_profile(), &compatibility, nullptr, 0);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    return RMW_RET_OK == ret && RMW_QOS_COMPATIBILITY_ERROR == compatibility;
  }

  // Creates a BEST_EFFORT publisher while an event-only wait on the REQUESTED_QOS_INCOMPATIBLE
  // event in events[0] runs on another thread, and expects the event to end that wait.
  void expect_incompatible_publisher_wakes_event_only_wait()
  {
    ASSERT_EQ(1u, events.size());
    std::vector<void *> ready;
    std::future<rmw_ret_t> waiter = std::async(
      std::launch::async, [this, &ready]() {return wait_for_events(kEventTimeout, ready);});

    ASSERT_NE(nullptr, create_publisher(best_effort_qos_profile())) <<
      rmw_get_error_string().str;

    ASSERT_EQ(std::future_status::ready, waiter.wait_for(std::chrono::seconds(20)));
    ASSERT_EQ(RMW_RET_OK, waiter.get()) << "the QoS event did not end the wait before it timed out";
    ASSERT_NE(nullptr, ready[0]) << "rmw_wait returned without the QoS event ready";

    rmw_requested_qos_incompatible_event_status_t status{};
    bool taken = false;
    ASSERT_EQ(RMW_RET_OK, rmw_take_event(&events[0], &status, &taken)) <<
      rmw_get_error_string().str;
    EXPECT_TRUE(taken);
    EXPECT_GE(status.total_count, 1);
    EXPECT_EQ(RMW_QOS_POLICY_RELIABILITY, status.last_policy_kind);
  }

  rmw_wait_set_t * wait_set{nullptr};
  rmw_subscription_t * sub{nullptr};
  std::vector<rmw_publisher_t *> publishers;
  std::vector<rmw_event_t> events;
  const char * wait_topic_name = "/test_event_wait";
};

// An unread sample must not wake a wait on the subscription's QoS events when the
// subscription itself is not in the wait set. None of these events can be raised between
// compatible endpoints of the same type with the default deadline.
TEST_F(TestEventWait, unread_sample_does_not_wake_event_only_wait) {
  const std::vector<rmw_event_type_t> event_types = {
    RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE,
    RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE,
    RMW_EVENT_REQUESTED_DEADLINE_MISSED,
  };
  ASSERT_NO_FATAL_FAILURE(init_supported_events(event_types));
  if (events.empty()) {
    GTEST_SKIP() << "none of the subscription events used by this test are supported";
  }
  rmw_publisher_t * pub = create_publisher(reliable_qos_profile());
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));

  // Repeated, because the sample stays unread and must not wake any of these waits.
  for (int i = 0; i < 3; ++i) {
    std::vector<void *> ready;
    EXPECT_EQ(RMW_RET_TIMEOUT, wait_for_events(kShortTimeout, ready)) << "iteration " << i;
    for (size_t j = 0; j < ready.size(); ++j) {
      EXPECT_EQ(nullptr, ready[j]) << "event " << j << " reported ready, iteration " << i;
    }
  }

  ASSERT_NO_FATAL_FAILURE(expect_sample_pending_and_take_it());
}

TEST_F(TestEventWait, incompatible_qos_event_wakes_event_only_wait) {
  if (!reliability_mismatch_is_reported()) {
    GTEST_SKIP() << "BEST_EFFORT publishers are compatible with RELIABLE subscriptions";
  }
  ASSERT_NO_FATAL_FAILURE(init_supported_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_event_only_wait());
}

// Same as above while the subscription has an unread sample, which must neither end the
// wait early nor keep the QoS event from being reported.
TEST_F(TestEventWait, incompatible_qos_event_wakes_event_only_wait_with_unread_sample) {
  if (!reliability_mismatch_is_reported()) {
    GTEST_SKIP() << "BEST_EFFORT publishers are compatible with RELIABLE subscriptions";
  }
  ASSERT_NO_FATAL_FAILURE(init_supported_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  rmw_publisher_t * pub = create_publisher(reliable_qos_profile());
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_event_only_wait());

  ASSERT_NO_FATAL_FAILURE(expect_sample_pending_and_take_it());
}
