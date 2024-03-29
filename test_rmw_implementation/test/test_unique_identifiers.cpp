// Copyright 2020 Open Source Robotics Foundation, Inc.
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
#include "rmw/rmw.h"

#include "test_msgs/msg/basic_types.h"
#include "test_msgs/srv/basic_types.h"

class TestUniqueIdentifierAPI : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rmw_init_options_t init_options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&init_options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      rmw_ret_t ret = rmw_init_options_fini(&init_options);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    });
    init_options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
    ASSERT_STREQ("/", init_options.enclave);
    ret = rmw_init(&init_options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    constexpr char node_name[] = "my_test_node";
    constexpr char node_namespace[] = "/my_test_ns";
    node = rmw_create_node(&context, node_name, node_namespace);
    ASSERT_NE(nullptr, node) << rcutils_get_error_string().str;
    rmw_publisher_options_t options = rmw_get_default_publisher_options();
    constexpr char topic_name[] = "/test0";
    pub = rmw_create_publisher(node, ts, topic_name, &qos_profile, &options);
    ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;

    constexpr char service_name[] = "/test_service0";
    client = rmw_create_client(node, srv_ts, service_name, &qos_profile);
    ASSERT_NE(nullptr, client) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_publisher(node, pub);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_client(node, client);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes)};
  const rosidl_service_type_support_t * srv_ts{
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes)};
  rmw_qos_profile_t qos_profile{rmw_qos_profile_default};
  rmw_publisher_t * pub{nullptr};
  rmw_client_t * client{nullptr};
};

TEST_F(TestUniqueIdentifierAPI, get_pub_gid_with_bad_args) {
  rmw_gid_t gid{};
  rmw_ret_t ret = rmw_get_gid_for_publisher(pub, &gid);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  rmw_gid_t expected_gid = gid;

  ret = rmw_get_gid_for_publisher(nullptr, &gid);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
  bool gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);

  ret = rmw_get_gid_for_publisher(pub, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
  gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);

  const char * implementation_identifier = pub->implementation_identifier;
  pub->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_get_gid_for_publisher(pub, &gid);
  pub->implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
  gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);
}


TEST_F(TestUniqueIdentifierAPI, get_client_gid_with_bad_args) {
  rmw_gid_t gid{};
  rmw_ret_t ret = rmw_get_gid_for_client(client, &gid);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  rmw_gid_t expected_gid = gid;

  ret = rmw_get_gid_for_client(nullptr, &gid);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
  bool gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);

  ret = rmw_get_gid_for_client(client, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();
  gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);

  const char * implementation_identifier = client->implementation_identifier;
  client->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_get_gid_for_client(client, &gid);
  client->implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
  gids_are_equal = false;
  ret = rmw_compare_gids_equal(&expected_gid, &gid, &gids_are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(gids_are_equal);
}

TEST_F(TestUniqueIdentifierAPI, compare_gids_with_bad_args) {
  rmw_gid_t gid{};
  rmw_ret_t ret = rmw_get_gid_for_publisher(pub, &gid);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  rmw_gid_t duplicate_gid = gid;

  bool result = false;
  ret = rmw_compare_gids_equal(nullptr, &duplicate_gid, &result);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  ret = rmw_compare_gids_equal(&gid, nullptr, &result);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  ret = rmw_compare_gids_equal(&gid, &duplicate_gid, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  const char * implementation_identifier = gid.implementation_identifier;
  gid.implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_compare_gids_equal(&gid, &duplicate_gid, &result);
  gid.implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();

  implementation_identifier = duplicate_gid.implementation_identifier;
  duplicate_gid.implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_compare_gids_equal(&gid, &duplicate_gid, &result);
  duplicate_gid.implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
}

TEST_F(TestUniqueIdentifierAPI, compare_gids) {
  rmw_gid_t gid{};
  rmw_ret_t ret = rmw_get_gid_for_publisher(pub, &gid);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  bool are_equal = false;
  ret = rmw_compare_gids_equal(&gid, &gid, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(are_equal);

  are_equal = false;
  rmw_gid_t duplicate_gid = gid;
  ret = rmw_compare_gids_equal(&gid, &duplicate_gid, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_TRUE(are_equal);
}

class TestUniqueIdentifiersForMultiplePublishers : public TestUniqueIdentifierAPI
{
protected:
  void SetUp() override
  {
    TestUniqueIdentifierAPI::SetUp();
    rmw_publisher_options_t options = rmw_get_default_publisher_options();
    constexpr char topic1_name[] = "/test0";
    first_pub_for_topic1 = rmw_create_publisher(
      node, ts, topic1_name, &qos_profile, &options);
    ASSERT_NE(nullptr, first_pub_for_topic1) << rmw_get_error_string().str;
    second_pub_for_topic1 = rmw_create_publisher(
      node, ts, topic1_name, &qos_profile, &options);
    ASSERT_NE(nullptr, second_pub_for_topic1) << rmw_get_error_string().str;
    pub_for_topic0 = pub;  // reuse
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_publisher(node, second_pub_for_topic1);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_publisher(node, first_pub_for_topic1);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    TestUniqueIdentifierAPI::TearDown();
  }

  rmw_publisher_t * first_pub_for_topic1{nullptr};
  rmw_publisher_t * second_pub_for_topic1{nullptr};
  rmw_publisher_t * pub_for_topic0{nullptr};
};

TEST_F(TestUniqueIdentifiersForMultiplePublishers, different_pubs) {
  rmw_gid_t gid_of_pub_for_topic0{};
  rmw_ret_t ret = rmw_get_gid_for_publisher(pub_for_topic0, &gid_of_pub_for_topic0);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  rmw_gid_t gid_of_first_pub_for_topic1{};
  ret = rmw_get_gid_for_publisher(first_pub_for_topic1, &gid_of_first_pub_for_topic1);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  rmw_gid_t gid_of_second_pub_for_topic1{};
  ret = rmw_get_gid_for_publisher(second_pub_for_topic1, &gid_of_second_pub_for_topic1);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  bool are_equal = true;
  ret = rmw_compare_gids_equal(&gid_of_pub_for_topic0, &gid_of_first_pub_for_topic1, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_FALSE(are_equal);

  are_equal = true;
  ret = rmw_compare_gids_equal(
    &gid_of_first_pub_for_topic1, &gid_of_second_pub_for_topic1, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_FALSE(are_equal);
}


class TestUniqueIdentifiersForMultipleClients : public TestUniqueIdentifierAPI
{
protected:
  void SetUp() override
  {
    TestUniqueIdentifierAPI::SetUp();
    constexpr char service_name[] = "/test_service1";
    first_client_for_service1 = rmw_create_client(
      node, srv_ts, service_name, &qos_profile);
    ASSERT_NE(nullptr, first_client_for_service1) << rmw_get_error_string().str;
    second_client_for_service1 = rmw_create_client(
      node, srv_ts, service_name, &qos_profile);
    ASSERT_NE(nullptr, second_client_for_service1) << rmw_get_error_string().str;
    client_for_service0 = client;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_client(node, first_client_for_service1);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_client(node, second_client_for_service1);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    TestUniqueIdentifierAPI::TearDown();
  }

  rmw_client_t * client_for_service0{nullptr};
  rmw_client_t * first_client_for_service1{nullptr};
  rmw_client_t * second_client_for_service1{nullptr};
};


TEST_F(TestUniqueIdentifiersForMultipleClients, different_clis) {
  rmw_gid_t gid_of_client_for_service0{};
  rmw_ret_t ret = rmw_get_gid_for_client(client_for_service0, &gid_of_client_for_service0);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  rmw_gid_t gid_of_first_client_for_service1{};
  ret = rmw_get_gid_for_client(first_client_for_service1, &gid_of_first_client_for_service1);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  rmw_gid_t gid_of_second_client_for_service1{};
  ret = rmw_get_gid_for_client(second_client_for_service1, &gid_of_second_client_for_service1);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  bool are_equal = true;
  ret = rmw_compare_gids_equal(
    &gid_of_client_for_service0, &gid_of_first_client_for_service1, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_FALSE(are_equal);

  are_equal = true;
  ret = rmw_compare_gids_equal(
    &gid_of_first_client_for_service1, &gid_of_second_client_for_service1, &are_equal);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_FALSE(are_equal);
}
