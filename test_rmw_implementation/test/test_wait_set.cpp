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
#include "rcutils/testing/fault_injection.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

class CLASSNAME (TestWaitSet, RMW_IMPLEMENTATION) : public ::testing::Test
{
protected:
  void SetUp() override
  {
    options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
    ASSERT_STREQ("/", options.enclave);
    context = rmw_get_zero_initialized_context();
    ret = rmw_init(&options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    ret = rmw_init_options_fini(&options);
    EXPECT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
  }

  rmw_init_options_t options;
  rmw_context_t context;
};

// Macro to reserve memory of rmw_wait input variables
#define RESERVE_MEMORY(Type, internal_var_name, var_name, size) do { \
    var_name = (rmw_ ## Type ## _t *) allocator.allocate( \
      sizeof(rmw_ ## Type ## _t *) * size, allocator.state); \
    if (var_name == nullptr) { \
      var_name = nullptr; \
      break; \
    } \
    var_name->internal_var_name ## _count = size; \
    memset(static_cast<void *>(var_name), 0, sizeof(rmw_ ## Type ## _t *)); \
    var_name->internal_var_name ## s = static_cast<void **>(allocator.allocate( \
        sizeof(void *) * size, allocator.state)); \
    if (!var_name->internal_var_name ## s) { \
      allocator.deallocate(static_cast<void *>(var_name), allocator.state); \
      var_name->internal_var_name ## s = nullptr; \
      break; \
    } \
    memset(var_name->internal_var_name ## s, 0, sizeof(void *) * size); \
} while (0);

// Macro to free memory of rmw_wait input variables
#define FREE_MEMORY(var_name, internal_var_name) \
  if (var_name->internal_var_name ## s) { \
    allocator.deallocate(static_cast<void **>(var_name->internal_var_name ## s), allocator.state); \
    var_name->internal_var_name ## s = nullptr; \
    var_name->internal_var_name ## _count = 0; \
  } \
  if (var_name) { \
    allocator.deallocate(static_cast<void *>(var_name), allocator.state); \
    var_name = nullptr; \
  }

TEST_F(CLASSNAME(TestWaitSet, RMW_IMPLEMENTATION), rmw_create_wait_set)
{
  // Created a valid wait_set
  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, 0);
  ASSERT_NE(nullptr, wait_set) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Destroyed a valid wait_set
  rmw_ret_t ret = rmw_destroy_wait_set(wait_set);
  EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;

  // Try to create a wait_set using a invalid argument
  wait_set = rmw_create_wait_set(nullptr, 0);
  EXPECT_EQ(wait_set, nullptr) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Battle test rmw_create_wait_set.
  RCUTILS_FAULT_INJECTION_TEST(
  {
    wait_set = rmw_create_wait_set(&context, 0);

    int64_t count = rcutils_fault_injection_get_count();
    rcutils_fault_injection_set_count(RCUTILS_FAULT_INJECTION_NEVER_FAIL);

    if (wait_set != nullptr) {
      ret = rmw_destroy_wait_set(wait_set);
      EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;
    } else {
      rmw_reset_error();
    }
    rcutils_fault_injection_set_count(count);
  });
}

TEST_F(CLASSNAME(TestWaitSet, RMW_IMPLEMENTATION), rmw_wait)
{
  size_t number_of_subscriptions = 1;
  size_t number_of_guard_conditions = 1;
  size_t number_of_clients = 1;
  size_t number_of_services = 1;
  size_t number_of_events = 1;
  size_t num_conditions =
    number_of_subscriptions +
    number_of_guard_conditions +
    number_of_clients +
    number_of_services +
    number_of_events;

  // Created a valid wait_set
  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, num_conditions);
  ASSERT_NE(nullptr, wait_set);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rmw_ret_t ret = rmw_destroy_wait_set(wait_set);
    EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;
  });

  // Call rmw_wait with invalid arguments
  rmw_ret_t ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Created two timeout,
  // - Equal to zero: do not block -- check only for immediately available entities.
  // - 100ms: This is represents the maximum amount of time to wait for an entity to become ready.
  rmw_time_t timeout_argument = {0, 100000000};  // 100ms
  rmw_time_t timeout_argument_zero = {0, 0};

  // Reserve memory for all the rmw_wait input arguments
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rmw_subscriptions_t * subscriptions;
  rmw_guard_conditions_t * guard_conditions;
  rmw_services_t * services;
  rmw_clients_t * clients;
  rmw_events_t * events;
  RESERVE_MEMORY(subscriptions, subscriber, subscriptions, number_of_subscriptions);
  RESERVE_MEMORY(guard_conditions, guard_condition, guard_conditions, number_of_guard_conditions);
  RESERVE_MEMORY(services, service, services, number_of_services);
  RESERVE_MEMORY(clients, client, clients, number_of_clients);
  RESERVE_MEMORY(events, event, events, number_of_events);
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    FREE_MEMORY(subscriptions, subscriber);
    FREE_MEMORY(guard_conditions, guard_condition);
    FREE_MEMORY(clients, client);
    FREE_MEMORY(services, service);
    FREE_MEMORY(events, event);
  });

  // Used a valid wait_set
  ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set and subscription with 100ms timeout
  ret = rmw_wait(subscriptions, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set and subscription with no timeout
  ret = rmw_wait(
    subscriptions, nullptr, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription and guard_conditions with 100ms timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription and guard_conditions with no timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions and services with 100ms timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, nullptr, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions and services with no timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services and clients with 100ms timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, clients, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services and clients with no timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, clients, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services, clients and events with 100ms
  // timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, clients, events, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services, clients and events with no
  // timeout
  ret = rmw_wait(
    subscriptions, guard_conditions, services, clients, events, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  const char * implementation_identifier = wait_set->implementation_identifier;
  wait_set->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_wait(
    subscriptions, guard_conditions, services, clients, events, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_INCORRECT_RMW_IMPLEMENTATION) << rcutils_get_error_string().str;
  rmw_reset_error();

  wait_set->implementation_identifier = implementation_identifier;

  // Battle test rmw_wait.
  RCUTILS_FAULT_INJECTION_TEST(
  {
    ret = rmw_wait(
      subscriptions, guard_conditions, services, clients, events, wait_set,
      &timeout_argument);

    EXPECT_TRUE(RMW_RET_TIMEOUT == ret || RMW_RET_ERROR == ret);
    rmw_reset_error();
  });
}

TEST_F(CLASSNAME(TestWaitSet, RMW_IMPLEMENTATION), rmw_destroy_wait_set)
{
  // Try to destroy a nullptr
  rmw_ret_t ret = rmw_destroy_wait_set(nullptr);
  EXPECT_EQ(ret, RMW_RET_ERROR) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Created a valid wait set
  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, 1);
  ASSERT_NE(nullptr, wait_set);
  rmw_reset_error();

  // Keep the implementation_identifier
  const char * implementation_identifier = wait_set->implementation_identifier;

  // Use a invalid implementation_identifier
  wait_set->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_destroy_wait_set(wait_set);
  EXPECT_EQ(ret, RMW_RET_INCORRECT_RMW_IMPLEMENTATION) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Restored the identifier and destroy the wait_set
  wait_set->implementation_identifier = implementation_identifier;
  ret = rmw_destroy_wait_set(wait_set);
  EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;
}
