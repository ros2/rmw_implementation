// Copyright 2023 Sony Group Corporation.
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

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/rmw.h"

#include "test_msgs/msg/basic_types.h"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

class CLASSNAME (TestEvent, RMW_IMPLEMENTATION) : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rmw_init_options_t options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      rmw_ret_t ret = rmw_init_options_fini(&options);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    });
    options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
    ASSERT_STREQ("/", options.enclave);
    context = rmw_get_zero_initialized_context();
    ret = rmw_init(&options, &context);
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
  }

  rmw_context_t context;
  rmw_node_t * node{nullptr};
  rmw_publisher_options_t pub_options = rmw_get_default_publisher_options();
  rmw_subscription_options_t sub_options = rmw_get_default_subscription_options();
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  const char * topic_name = "/test_topic";
};

// Matched events do not exist in this distribution, so a BEST_EFFORT publisher and RELIABLE
// subscriptions are used to raise an event on the publisher each time a subscription is created.
TEST_F(
  CLASSNAME(TestEvent, RMW_IMPLEMENTATION),
  publisher_qos_incompatible_event_after_wait_set_entities_change)
{
  rmw_qos_profile_t pub_qos = rmw_qos_profile_default;
  pub_qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
  rmw_qos_profile_t sub_qos = rmw_qos_profile_default;
  sub_qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;

  rmw_publisher_t * pub = rmw_create_publisher(node, ts, topic_name, &pub_qos, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
  });

  rmw_event_t pub_event{rmw_get_zero_initialized_event()};
  rmw_ret_t ret = rmw_publisher_event_init(&pub_event, pub, RMW_EVENT_OFFERED_QOS_INCOMPATIBLE);
  if (RMW_RET_UNSUPPORTED == ret) {
    rmw_reset_error();
    GTEST_SKIP() << "RMW_EVENT_OFFERED_QOS_INCOMPATIBLE is not supported";
  }
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_event_fini(&pub_event)) << rmw_get_error_string().str;
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

  // Wait for the event with a wait set holding `used_gc_count` guard conditions, none of which
  // is triggered, so only the event may be reported as ready.
  auto wait_for_event = [&](size_t used_gc_count, int32_t expected_total_count) {
      void * gc_storage[gc_count];
      for (size_t i = 0; i < used_gc_count; ++i) {
        gc_storage[i] = gcs[i]->data;
      }
      rmw_guard_conditions_t guard_conditions;
      guard_conditions.guard_condition_count = used_gc_count;
      guard_conditions.guard_conditions = gc_storage;
      void * events_storage[1] = {&pub_event};
      rmw_events_t events;
      events.event_count = 1;
      events.events = events_storage;

      rmw_time_t timeout = {10, 0};  // Long enough that only a missed wakeup reaches it.
      ASSERT_EQ(
        RMW_RET_OK,
        rmw_wait(nullptr, &guard_conditions, nullptr, nullptr, &events, wait_set, &timeout)) <<
        rmw_get_error_string().str;
      EXPECT_NE(nullptr, events.events[0]);
      for (size_t i = 0; i < used_gc_count; ++i) {
        EXPECT_EQ(nullptr, guard_conditions.guard_conditions[i]) << "guard condition " << i;
      }

      rmw_qos_incompatible_event_status_t status{};
      bool taken = false;
      EXPECT_EQ(RMW_RET_OK, rmw_take_event(&pub_event, &status, &taken)) <<
        rmw_get_error_string().str;
      EXPECT_TRUE(taken);
      EXPECT_EQ(expected_total_count, status.total_count);
      EXPECT_EQ(RMW_QOS_POLICY_RELIABILITY, status.last_policy_kind);
    };

  rmw_subscription_t * sub1 =
    rmw_create_subscription(node, ts, topic_name, &sub_qos, &sub_options);
  ASSERT_NE(nullptr, sub1) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub1)) << rmw_get_error_string().str;
  });
  wait_for_event(1, 1);

  rmw_subscription_t * sub2 =
    rmw_create_subscription(node, ts, topic_name, &sub_qos, &sub_options);
  ASSERT_NE(nullptr, sub2) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_destroy_subscription(node, sub2)) << rmw_get_error_string().str;
  });
  // Wait with more entities than in the previous rmw_wait call.
  wait_for_event(gc_count, 2);
}
