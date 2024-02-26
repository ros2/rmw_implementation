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

#include "rosidl_runtime_c/primitives_sequence_functions.h"

#include "rosidl_typesupport_cpp/message_type_support.hpp"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "test_msgs/msg/basic_types.h"
#include "test_msgs/msg/basic_types.hpp"

#include "test_msgs/msg/bounded_plain_sequences.h"
#include "test_msgs/msg/bounded_plain_sequences.hpp"

#include "./allocator_testing_utils.h"

TEST(TestSerializeDeserialize, get_serialization_format) {
  const char * serialization_format = rmw_get_serialization_format();
  EXPECT_NE(nullptr, serialization_format);
  EXPECT_STREQ(serialization_format, rmw_get_serialization_format());
}

TEST(TestSerializeDeserialize, serialize_with_bad_arguments) {
  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes)};
  test_msgs__msg__BasicTypes input_message{};
  ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&input_message));
  rcutils_allocator_t failing_allocator = get_failing_allocator();
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &failing_allocator)) << rmw_get_error_string().str;

  EXPECT_NE(RMW_RET_OK, rmw_serialize(&input_message, ts, &serialized_message));
  rmw_reset_error();

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;

  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &default_allocator)) << rmw_get_error_string().str;

  rosidl_message_type_support_t * non_const_ts =
    const_cast<rosidl_message_type_support_t *>(ts);
  const char * typesupport_identifier = non_const_ts->typesupport_identifier;
  non_const_ts->typesupport_identifier = "not-a-typesupport-identifier";

  EXPECT_NE(RMW_RET_OK, rmw_serialize(&input_message, non_const_ts, &serialized_message));
  rmw_reset_error();

  non_const_ts->typesupport_identifier = typesupport_identifier;

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}

TEST(TestSerializeDeserialize, clean_round_trip_for_c_message) {
  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes)};
  test_msgs__msg__BasicTypes input_message{};
  test_msgs__msg__BasicTypes output_message{};
  ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&input_message));
  ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&output_message));
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &default_allocator)) << rmw_get_error_string().str;

  // Make input_message not equal to output_message.
  input_message.bool_value = !output_message.bool_value;
  input_message.int16_value = output_message.int16_value - 1;
  input_message.uint32_value = output_message.uint32_value + 1000000;

  rmw_ret_t ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);

  ret = rmw_deserialize(&serialized_message, ts, &output_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(input_message.bool_value, output_message.bool_value);
  EXPECT_EQ(input_message.int16_value, output_message.int16_value);
  EXPECT_EQ(input_message.uint32_value, output_message.uint32_value);

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}

TEST(TestSerializeDeserialize, clean_round_trip_for_c_bounded_message) {
  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BoundedPlainSequences)};
  test_msgs__msg__BoundedPlainSequences input_message{};
  test_msgs__msg__BoundedPlainSequences output_message{};
  ASSERT_TRUE(test_msgs__msg__BoundedPlainSequences__init(&input_message));
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    test_msgs__msg__BoundedPlainSequences__fini(&input_message);
  });
  ASSERT_TRUE(test_msgs__msg__BoundedPlainSequences__init(&output_message));
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    test_msgs__msg__BoundedPlainSequences__fini(&output_message);
  });
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &default_allocator)) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_serialized_message_fini(
        &serialized_message)) << rmw_get_error_string().str;
  });

  // Make input_message not equal to output_message.
  ASSERT_TRUE(rosidl_runtime_c__bool__Sequence__init(&input_message.bool_values, 1));
  input_message.bool_values.data[0] = true;
  ASSERT_TRUE(rosidl_runtime_c__int16__Sequence__init(&input_message.int16_values, 1));
  input_message.int16_values.data[0] = -7;

  rmw_ret_t ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);

  // Adding more items should increase buffer length
  auto first_message_length = serialized_message.buffer_length;
  ASSERT_TRUE(rosidl_runtime_c__int32__Sequence__init(&input_message.int32_values, 1));
  input_message.int32_values.data[0] = -1;
  ASSERT_TRUE(rosidl_runtime_c__uint16__Sequence__init(&input_message.uint16_values, 1));
  input_message.uint16_values.data[0] = 125;

  ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);
  EXPECT_GT(serialized_message.buffer_length, first_message_length);

  ret = rmw_deserialize(&serialized_message, ts, &output_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(input_message.bool_values.size, output_message.bool_values.size);
  EXPECT_EQ(input_message.bool_values.data[0], output_message.bool_values.data[0]);
  EXPECT_EQ(input_message.int16_values.size, output_message.int16_values.size);
  EXPECT_EQ(input_message.int16_values.data[0], output_message.int16_values.data[0]);
  EXPECT_EQ(input_message.int32_values.size, output_message.int32_values.size);
  EXPECT_EQ(input_message.int32_values.data[0], output_message.int32_values.data[0]);
  EXPECT_EQ(input_message.uint16_values.size, output_message.uint16_values.size);
  EXPECT_EQ(input_message.uint16_values.data[0], output_message.uint16_values.data[0]);
}

TEST(TestSerializeDeserialize, clean_round_trip_for_cpp_message) {
  const rosidl_message_type_support_t * ts =
    rosidl_typesupport_cpp::get_message_type_support_handle<test_msgs::msg::BasicTypes>();
  test_msgs::msg::BasicTypes input_message{};
  test_msgs::msg::BasicTypes output_message{};
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &default_allocator)) << rmw_get_error_string().str;

  // Make input_message not equal to output_message.
  input_message.bool_value = !output_message.bool_value;
  input_message.int16_value = output_message.int16_value - 1;
  input_message.uint32_value = output_message.uint32_value + 1000000;

  rmw_ret_t ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);

  ret = rmw_deserialize(&serialized_message, ts, &output_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(input_message, output_message);

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}

TEST(TestSerializeDeserialize, clean_round_trip_for_cpp_bounded_message) {
  using TestMessage = test_msgs::msg::BoundedPlainSequences;
  const rosidl_message_type_support_t * ts =
    rosidl_typesupport_cpp::get_message_type_support_handle<TestMessage>();
  TestMessage input_message{};
  TestMessage output_message{};
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &default_allocator)) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(
      RMW_RET_OK, rmw_serialized_message_fini(
        &serialized_message)) << rmw_get_error_string().str;
  });

  // Make input_message not equal to output_message.
  input_message.bool_values.push_back(true);
  input_message.int16_values.push_back(-7);

  rmw_ret_t ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);

  // Adding more items should increase buffer length
  auto first_message_length = serialized_message.buffer_length;
  input_message.int32_values.push_back(-1);
  input_message.int32_values.push_back(583);
  input_message.uint16_values.push_back(125);

  ret = rmw_serialize(&input_message, ts, &serialized_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_NE(nullptr, serialized_message.buffer);
  EXPECT_GT(serialized_message.buffer_length, 0lu);
  EXPECT_GT(serialized_message.buffer_length, first_message_length);

  ret = rmw_deserialize(&serialized_message, ts, &output_message);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(input_message, output_message);
}

TEST(TestSerializeDeserialize, rmw_get_serialized_message_size)
{
  if (rmw_get_serialized_message_size(nullptr, nullptr, nullptr) != RMW_RET_UNSUPPORTED) {
    // TODO(anyone): Add tests here when the implementation it's supported
    GTEST_SKIP();
  }
}
