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

#include <atomic>
#include <chrono>
#include <thread>

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/rmw.h"

#include "test_msgs/msg/basic_types.h"

class TestEvent : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rmw_ret_t ret = rmw_init_options_init(&init_options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    init_options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
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
