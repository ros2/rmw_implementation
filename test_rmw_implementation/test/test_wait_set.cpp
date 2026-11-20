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

#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <tuple>

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"
#include "rcutils/testing/fault_injection.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/event.h"

#include "test_msgs/msg/basic_types.h"
#include "test_msgs/srv/basic_types.h"

class TestWaitSet : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rmw_init_options_t options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&options, allocator);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      rmw_ret_t ret = rmw_init_options_fini(&options);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    });
    ret = rmw_enclave_options_copy("/", &allocator, &options.enclave);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
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
  }

  rmw_context_t context;
};

TEST_F(TestWaitSet, rmw_create_wait_set)
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

    if (wait_set) {
      RCUTILS_NO_FAULT_INJECTION(
      {
        ret = rmw_destroy_wait_set(wait_set);
        EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;
      });
    } else {
      rmw_reset_error();
    }
  });
}

class TestWaitSetUse : public TestWaitSet
{
protected:
  void SetUp() override
  {
    TestWaitSet::SetUp();
    constexpr char node_name[] = "my_node";
    constexpr char node_namespace[] = "/my_ns";
    node = rmw_create_node(&context, node_name, node_namespace);
    ASSERT_NE(nullptr, node) << rmw_get_error_string().str;
    constexpr char topic_name[] = "/test";
    const rosidl_message_type_support_t * message_ts =
      ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
    rmw_subscription_options_t sub_options = rmw_get_default_subscription_options();
    sub = rmw_create_subscription(
      node, message_ts, topic_name, &rmw_qos_profile_default, &sub_options);
    ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
    gc = rmw_create_guard_condition(&context);
    ASSERT_NE(nullptr, gc) << rmw_get_error_string().str;
    constexpr char service_name[] = "/test";
    const rosidl_service_type_support_t * service_ts =
      ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
    srv = rmw_create_service(node, service_ts, service_name, &rmw_qos_profile_default);
    ASSERT_NE(nullptr, srv) << rmw_get_error_string().str;
    client = rmw_create_client(node, service_ts, service_name, &rmw_qos_profile_default);
    ASSERT_NE(nullptr, client) << rmw_get_error_string().str;
    rmw_ret_t ret = RMW_RET_OK;
    if (!rmw_event_type_is_supported(RMW_EVENT_LIVELINESS_CHANGED)) {
      ret = rmw_subscription_event_init(&event, sub, RMW_EVENT_PUBLICATION_MATCHED);
    } else {
      ret = rmw_subscription_event_init(&event, sub, RMW_EVENT_LIVELINESS_CHANGED);
    }
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_event_fini(&event);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_client(node, client);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_service(node, srv);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_subscription(node, sub);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_guard_condition(gc);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    TestWaitSet::TearDown();
  }

  rmw_node_t * node{nullptr};
  rmw_subscription_t * sub{nullptr};
  rmw_guard_condition_t * gc{nullptr};
  rmw_service_t * srv{nullptr};
  rmw_client_t * client{nullptr};
  rmw_event_t event{rmw_get_zero_initialized_event()};
};

// Macro to initialize and manage rmw_wait input arrays.
// Requires `size` to a constant expression.
#define INITIALIZE_ARRAY(var_name, internal_var_name, size) \
  void * var_name ## _storage[size]; \
  var_name.internal_var_name ## s = var_name ## _storage; \
  var_name.internal_var_name ## _count = size;

TEST_F(TestWaitSetUse, rmw_wait)
{
  constexpr size_t number_of_subscriptions = 1u;
  constexpr size_t number_of_guard_conditions = 1u;
  constexpr size_t number_of_clients = 1u;
  constexpr size_t number_of_services = 1u;
  constexpr size_t number_of_events = 1u;
  constexpr size_t num_conditions =
    number_of_subscriptions +
    number_of_guard_conditions +
    number_of_clients +
    number_of_services +
    number_of_events;

  // Created a valid wait_set
  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, num_conditions);
  ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;
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

  rmw_subscriptions_t subscriptions;
  rmw_guard_conditions_t guard_conditions;
  rmw_services_t services;
  rmw_clients_t clients;
  rmw_events_t events;
  INITIALIZE_ARRAY(subscriptions, subscriber, number_of_subscriptions);
  INITIALIZE_ARRAY(guard_conditions, guard_condition, number_of_guard_conditions);
  INITIALIZE_ARRAY(services, service, number_of_services);
  INITIALIZE_ARRAY(clients, client, number_of_clients);
  INITIALIZE_ARRAY(events, event, number_of_events);

  const char * implementation_identifier = wait_set->implementation_identifier;
  wait_set->implementation_identifier = "not-an-rmw-implementation-identifier";
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  clients.clients[0] = client->data;
  events.events[0] = &event;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, &clients, &events, wait_set,
    &timeout_argument);
  wait_set->implementation_identifier = implementation_identifier;
  EXPECT_EQ(ret, RMW_RET_INCORRECT_RMW_IMPLEMENTATION) << rcutils_get_error_string().str;
  EXPECT_NE(nullptr, subscriptions.subscribers[0]);  // left unchanged
  EXPECT_NE(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_NE(nullptr, services.services[0]);
  EXPECT_NE(nullptr, clients.clients[0]);
  EXPECT_NE(nullptr, events.events[0]);
  rmw_reset_error();

  // Used a valid wait_set
  ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  ret = rmw_wait(nullptr, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Used a valid wait_set and subscription with 100ms timeout
  subscriptions.subscribers[0] = sub->data;
  ret = rmw_wait(&subscriptions, nullptr, nullptr, nullptr, nullptr, wait_set, &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  rmw_reset_error();

  // Used a valid wait_set and subscription with no timeout
  subscriptions.subscribers[0] = sub->data;
  ret = rmw_wait(
    &subscriptions, nullptr, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription and guard_conditions with 100ms timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription and guard_conditions with no timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, nullptr, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions and services with 100ms timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, nullptr, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions and services with no timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, nullptr, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services and clients with 100ms timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  clients.clients[0] = client->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, &clients, nullptr, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  EXPECT_EQ(nullptr, clients.clients[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services and clients with no timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  clients.clients[0] = client->data;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, &clients, nullptr, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  EXPECT_EQ(nullptr, clients.clients[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services, clients and events with 100ms
  // timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  clients.clients[0] = client->data;
  events.events[0] = &event;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, &clients, &events, wait_set,
    &timeout_argument);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  EXPECT_EQ(nullptr, clients.clients[0]);
  EXPECT_EQ(nullptr, events.events[0]);
  rmw_reset_error();

  // Used a valid wait_set, subscription, guard_conditions, services, clients and events with no
  // timeout
  subscriptions.subscribers[0] = sub->data;
  guard_conditions.guard_conditions[0] = gc->data;
  services.services[0] = srv->data;
  clients.clients[0] = client->data;
  events.events[0] = &event;
  ret = rmw_wait(
    &subscriptions, &guard_conditions, &services, &clients, &events, wait_set,
    &timeout_argument_zero);
  EXPECT_EQ(ret, RMW_RET_TIMEOUT) << rcutils_get_error_string().str;
  EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
  EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
  EXPECT_EQ(nullptr, services.services[0]);
  EXPECT_EQ(nullptr, clients.clients[0]);
  EXPECT_EQ(nullptr, events.events[0]);
  rmw_reset_error();

  // Battle test rmw_wait.
  RCUTILS_FAULT_INJECTION_TEST(
  {
    subscriptions.subscribers[0] = sub->data;
    guard_conditions.guard_conditions[0] = gc->data;
    services.services[0] = srv->data;
    clients.clients[0] = client->data;
    events.events[0] = &event;
    ret = rmw_wait(
      &subscriptions, &guard_conditions, &services, &clients, &events, wait_set,
      &timeout_argument);
    EXPECT_NE(RMW_RET_OK, ret);
    if (RMW_RET_TIMEOUT == ret) {
      EXPECT_EQ(nullptr, subscriptions.subscribers[0]);
      EXPECT_EQ(nullptr, guard_conditions.guard_conditions[0]);
      EXPECT_EQ(nullptr, services.services[0]);
      EXPECT_EQ(nullptr, clients.clients[0]);
      EXPECT_EQ(nullptr, events.events[0]);
    } else {
      rmw_reset_error();
    }
  });
}

class TestWaitSetSharedEntity : public TestWaitSetUse
{
protected:
  // Waits on either the subscription or the guard condition; reports whether it came back ready.
  static rmw_ret_t wait_on(
    rmw_wait_set_t * wait_set, rmw_subscription_t * subscription,
    rmw_guard_condition_t * guard_condition, rmw_time_t timeout, bool * ready)
  {
    void * sub_storage[1] = {subscription ? subscription->data : nullptr};
    void * gc_storage[1] = {guard_condition ? guard_condition->data : nullptr};
    rmw_subscriptions_t subscriptions{subscription ? 1u : 0u, sub_storage};
    rmw_guard_conditions_t guard_conditions{guard_condition ? 1u : 0u, gc_storage};
    rmw_ret_t ret = rmw_wait(
      &subscriptions, &guard_conditions, nullptr, nullptr, nullptr, wait_set, &timeout);
    *ready = (subscription ? sub_storage[0] : gc_storage[0]) != nullptr;
    return ret;
  }

  // One entity is in two wait sets. While wait set `blocked` waits on it, wait set `other`
  // waits on it and times out; triggering the entity afterwards must still wake `blocked`.
  void expect_blocked_wait_set_woken(
    rmw_subscription_t * subscription, rmw_guard_condition_t * guard_condition,
    const std::function<void()> & trigger)
  {
    using namespace std::chrono_literals;
    rmw_wait_set_t * blocked = rmw_create_wait_set(&context, 1);
    ASSERT_NE(nullptr, blocked) << rmw_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_wait_set(blocked)) << rmw_get_error_string().str;
    });
    rmw_wait_set_t * other = rmw_create_wait_set(&context, 1);
    ASSERT_NE(nullptr, other) << rmw_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_wait_set(other)) << rmw_get_error_string().str;
    });

    auto blocked_result = std::async(
      std::launch::async, [&]() {
        bool ready = false;
        rmw_ret_t ret = wait_on(blocked, subscription, guard_condition, {10, 0}, &ready);
        return std::make_tuple(ret, ready, std::chrono::steady_clock::now());
      });
    std::this_thread::sleep_for(500ms);

    bool other_ready = true;
    EXPECT_EQ(
      RMW_RET_TIMEOUT,
      wait_on(other, subscription, guard_condition, {0, 10000000}, &other_ready));
    EXPECT_FALSE(other_ready);

    const auto triggered_at = std::chrono::steady_clock::now();
    trigger();
    ASSERT_EQ(std::future_status::ready, blocked_result.wait_for(15s));
    const auto [ret, ready, woken_at] = blocked_result.get();
    EXPECT_EQ(RMW_RET_OK, ret);
    EXPECT_TRUE(ready);
    EXPECT_LT(woken_at - triggered_at, 5s);
  }
};

TEST_F(TestWaitSetSharedEntity, guard_condition_in_two_wait_sets)
{
  expect_blocked_wait_set_woken(
    nullptr, gc, [this]() {
      EXPECT_EQ(RMW_RET_OK, rmw_trigger_guard_condition(gc)) << rmw_get_error_string().str;
    });
}

TEST_F(TestWaitSetSharedEntity, subscription_in_two_wait_sets)
{
  rmw_publisher_options_t pub_options = rmw_get_default_publisher_options();
  rmw_publisher_t * pub = rmw_create_publisher(
    node, ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes), "/test",
    &rmw_qos_profile_default, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(RMW_RET_OK, rmw_destroy_publisher(node, pub)) << rmw_get_error_string().str;
  });
  size_t matched = 0u;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (matched == 0u && std::chrono::steady_clock::now() < deadline) {
    ASSERT_EQ(RMW_RET_OK, rmw_subscription_count_matched_publishers(sub, &matched));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_EQ(1u, matched);

  test_msgs__msg__BasicTypes msg{};
  expect_blocked_wait_set_woken(
    sub, nullptr, [&]() {
      EXPECT_EQ(RMW_RET_OK, rmw_publish(pub, &msg, nullptr)) << rmw_get_error_string().str;
    });
}

TEST_F(TestWaitSet, rmw_destroy_wait_set)
{
  // Try to destroy a nullptr
  rmw_ret_t ret = rmw_destroy_wait_set(nullptr);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Created a valid wait set
  rmw_wait_set_t * wait_set = rmw_create_wait_set(&context, 1);
  ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;
  rmw_reset_error();

  // Keep the implementation_identifier
  const char * implementation_identifier = wait_set->implementation_identifier;

  // Use a invalid implementation_identifier
  wait_set->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_destroy_wait_set(wait_set);
  wait_set->implementation_identifier = implementation_identifier;
  EXPECT_EQ(ret, RMW_RET_INCORRECT_RMW_IMPLEMENTATION) << rcutils_get_error_string().str;
  rmw_reset_error();

  // Restored the identifier and destroy the wait_set
  ret = rmw_destroy_wait_set(wait_set);
  EXPECT_EQ(ret, RMW_RET_OK) << rcutils_get_error_string().str;
}
