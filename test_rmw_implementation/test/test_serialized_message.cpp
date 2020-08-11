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

#include "osrf_testing_tools_cpp/memory_tools/gtest_quickstart.hpp"

#include "rcutils/allocator.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "./allocator_testing_utils.h"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

class CLASSNAME (TestSerializedMessage, RMW_IMPLEMENTATION) : public ::testing::Test
{
};

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), bad_allocation_on_init) {
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  rcutils_allocator_t failing_allocator = get_failing_allocator();
  EXPECT_EQ(
    RMW_RET_BAD_ALLOC, rmw_serialized_message_init(
      &serialized_message, 1lu, &failing_allocator));
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), init_with_bad_arguments) {
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();

  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT, rmw_serialized_message_init(
      nullptr, 0lu, &default_allocator));
  rmw_reset_error();

  rcutils_allocator_t invalid_allocator = rcutils_get_zero_initialized_allocator();
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT, rmw_serialized_message_init(
      &serialized_message, 0lu, &invalid_allocator));
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), fini_with_bad_arguments) {
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, rmw_serialized_message_fini(nullptr));
  rmw_reset_error();

  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, rmw_serialized_message_fini(&serialized_message));
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), resize_with_bad_arguments) {
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, rmw_serialized_message_resize(nullptr, 1lu));
  rmw_reset_error();

  rmw_serialized_message_t zero_initialized_serialized_message =
    rmw_get_zero_initialized_serialized_message();
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_serialized_message_resize(&zero_initialized_serialized_message, 1lu));
  rmw_reset_error();

  rmw_serialized_message_t serialized_message =
    rmw_get_zero_initialized_serialized_message();
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 1lu, &default_allocator)) <<
    rmw_get_error_string().str;

  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_serialized_message_resize(&serialized_message, 0lu));
  rmw_reset_error();

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), bad_allocation_on_resize) {
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  rcutils_allocator_t failing_allocator = get_failing_allocator();
  ASSERT_EQ(
    RMW_RET_OK, rmw_serialized_message_init(
      &serialized_message, 0lu, &failing_allocator)) << rmw_get_error_string().str;

  EXPECT_EQ(
    RMW_RET_BAD_ALLOC,
    rmw_serialized_message_resize(&serialized_message, 1lu));
  rmw_reset_error();

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestSerializedMessage, RMW_IMPLEMENTATION), init_resize_fini) {
  rmw_serialized_message_t serialized_message = rmw_get_zero_initialized_serialized_message();
  rcutils_allocator_t default_allocator = rcutils_get_default_allocator();
  size_t serialized_message_size = 1lu;

  rmw_ret_t ret = rmw_serialized_message_init(
    &serialized_message, serialized_message_size, &default_allocator);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  EXPECT_NO_MEMORY_OPERATIONS(
  {
    ret = rmw_serialized_message_resize(&serialized_message, serialized_message_size);
  });
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(serialized_message.buffer_capacity, serialized_message_size);

  ret = rmw_serialized_message_resize(&serialized_message, 2 * serialized_message_size);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(serialized_message.buffer_capacity, 2 * serialized_message_size);

  EXPECT_EQ(RMW_RET_OK, rmw_serialized_message_fini(&serialized_message)) <<
    rmw_get_error_string().str;
}
