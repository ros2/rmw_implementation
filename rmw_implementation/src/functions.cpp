// Copyright 2016-2017 Open Source Robotics Foundation, Inc.
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

#include <cstddef>
#include <stdexcept>

#include <memory>
#include <string>

#include "rcutils/allocator.h"
#include "rcutils/format_string.h"
#include "rcutils/get_env.h"
#include "rcutils/types/string_array.h"

#include "rcpputils/find_library.hpp"
#include "rcpputils/get_env.hpp"
#include "rcpputils/shared_library.hpp"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/names_and_types.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/rmw.h"

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

std::shared_ptr<rcpputils::SharedLibrary>
get_library()
{
  static std::shared_ptr<rcpputils::SharedLibrary> lib;

  if (!lib) {
    std::string env_var = rcpputils::get_env_var("RMW_IMPLEMENTATION");
    if (env_var.empty()) {
      env_var = STRINGIFY(DEFAULT_RMW_IMPLEMENTATION);
    }
    std::string library_path = rcpputils::find_library_path(env_var);
    if (library_path.empty()) {
      RMW_SET_ERROR_MSG(
        ("failed to find shared library of rmw implementation. Searched " + env_var).c_str());
      return nullptr;
    }

    try {
      lib = std::make_shared<rcpputils::SharedLibrary>(library_path.c_str());
    } catch (const std::runtime_error & e) {
      RMW_SET_ERROR_MSG(
        ("failed to load shared library of rmw implementation: " + library_path +
        " Exception: " + std::string(e.what())).c_str());
      return nullptr;
    } catch (const std::bad_alloc & e) {
      RMW_SET_ERROR_MSG(
        ("failed to load shared library of rmw implementation " + library_path + ": " +
        std::string(e.what())).c_str());
      return nullptr;
    }
  }
  return lib;
}

void *
get_symbol(const char * symbol_name)
{
  std::shared_ptr<rcpputils::SharedLibrary> lib = get_library();

  if (!lib) {
    // error message set by get_library()
    return nullptr;
  }

  if (!lib->has_symbol(symbol_name)) {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char * msg = rcutils_format_string(
      allocator,
      "failed to resolve symbol '%s' in shared library '%s'", symbol_name,
      lib->get_library_path().c_str());
    if (msg) {
      RMW_SET_ERROR_MSG(msg);
      allocator.deallocate(msg, allocator.state);
    } else {
      RMW_SET_ERROR_MSG("failed to allocate memory for error message");
    }
    return nullptr;
  }
  return lib->get_symbol(symbol_name);
}

#ifdef __cplusplus
extern "C"
{
#endif

#define EXPAND(x) x

#define ARG_TYPES(...) __VA_ARGS__

#define ARG_VALUES_0(...)
#define ARG_VALUES_1(t1) v1
#define ARG_VALUES_2(t2, ...) v2, EXPAND(ARG_VALUES_1(__VA_ARGS__))
#define ARG_VALUES_3(t3, ...) v3, EXPAND(ARG_VALUES_2(__VA_ARGS__))
#define ARG_VALUES_4(t4, ...) v4, EXPAND(ARG_VALUES_3(__VA_ARGS__))
#define ARG_VALUES_5(t5, ...) v5, EXPAND(ARG_VALUES_4(__VA_ARGS__))
#define ARG_VALUES_6(t6, ...) v6, EXPAND(ARG_VALUES_5(__VA_ARGS__))
#define ARG_VALUES_7(t7, ...) v7, EXPAND(ARG_VALUES_6(__VA_ARGS__))

#define ARGS_0(...) __VA_ARGS__
#define ARGS_1(t1) t1 v1
#define ARGS_2(t2, ...) t2 v2, EXPAND(ARGS_1(__VA_ARGS__))
#define ARGS_3(t3, ...) t3 v3, EXPAND(ARGS_2(__VA_ARGS__))
#define ARGS_4(t4, ...) t4 v4, EXPAND(ARGS_3(__VA_ARGS__))
#define ARGS_5(t5, ...) t5 v5, EXPAND(ARGS_4(__VA_ARGS__))
#define ARGS_6(t6, ...) t6 v6, EXPAND(ARGS_5(__VA_ARGS__))
#define ARGS_7(t7, ...) t7 v7, EXPAND(ARGS_6(__VA_ARGS__))

#define CALL_SYMBOL(symbol_name, ReturnType, error_value, ArgTypes, arg_values) \
  if (!symbol_ ## symbol_name) { \
    /* only necessary for functions called before rmw_init */ \
    symbol_ ## symbol_name = get_symbol(#symbol_name); \
  } \
  if (!symbol_ ## symbol_name) { \
    /* error message set by get_symbol() */ \
    return error_value; \
  } \
  typedef ReturnType (* FunctionSignature)(ArgTypes); \
  FunctionSignature func = reinterpret_cast<FunctionSignature>(symbol_ ## symbol_name); \
  return func(arg_values);

// cppcheck-suppress preprocessorErrorDirective
#define RMW_INTERFACE_FN(name, ReturnType, error_value, _NR, ...) \
  void * symbol_ ## name = nullptr; \
  ReturnType name(EXPAND(ARGS_ ## _NR(__VA_ARGS__))) \
  { \
    CALL_SYMBOL( \
      name, ReturnType, error_value, ARG_TYPES(__VA_ARGS__), \
      EXPAND(ARG_VALUES_ ## _NR(__VA_ARGS__))); \
  }

RMW_INTERFACE_FN(
  rmw_get_implementation_identifier,
  const char *, nullptr,
  0, ARG_TYPES(void))

RMW_INTERFACE_FN(
  rmw_init_options_init,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_init_options_t *, rcutils_allocator_t))

RMW_INTERFACE_FN(
  rmw_init_options_copy,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_init_options_t *, rmw_init_options_t *))

RMW_INTERFACE_FN(
  rmw_init_options_fini,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_init_options_t *))

RMW_INTERFACE_FN(
  rmw_shutdown,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_context_t *))

RMW_INTERFACE_FN(
  rmw_context_fini,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_context_t *))

RMW_INTERFACE_FN(
  rmw_get_serialization_format,
  const char *, nullptr,
  0, ARG_TYPES(void))

RMW_INTERFACE_FN(
  rmw_create_node,
  rmw_node_t *, nullptr,
  5, ARG_TYPES(
    rmw_context_t *, const char *, const char *, size_t, bool))

RMW_INTERFACE_FN(
  rmw_destroy_node,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_node_t *))

RMW_INTERFACE_FN(
  rmw_node_assert_liveliness,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(const rmw_node_t *))

RMW_INTERFACE_FN(
  rmw_node_get_graph_guard_condition,
  const rmw_guard_condition_t *, nullptr,
  1, ARG_TYPES(const rmw_node_t *))

RMW_INTERFACE_FN(
  rmw_init_publisher_allocation,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rosidl_message_type_support_t *,
    const rosidl_message_bounds_t *,
    rmw_publisher_allocation_t *))

RMW_INTERFACE_FN(
  rmw_fini_publisher_allocation,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_publisher_allocation_t *))

RMW_INTERFACE_FN(
  rmw_create_publisher,
  rmw_publisher_t *, nullptr,
  5, ARG_TYPES(
    const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
    const rmw_qos_profile_t *, const rmw_publisher_options_t *))

RMW_INTERFACE_FN(
  rmw_destroy_publisher,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_publisher_t *))

RMW_INTERFACE_FN(
  rmw_borrow_loaned_message,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rmw_publisher_t *,
    const rosidl_message_type_support_t *,
    void **))

RMW_INTERFACE_FN(
  rmw_return_loaned_message_from_publisher,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, void *))

RMW_INTERFACE_FN(
  rmw_publish,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_publisher_t *, const void *, rmw_publisher_allocation_t *))

RMW_INTERFACE_FN(
  rmw_publish_loaned_message,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_publisher_t *, void *, rmw_publisher_allocation_t *))

RMW_INTERFACE_FN(
  rmw_publisher_count_matched_subscriptions,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, size_t *))

RMW_INTERFACE_FN(
  rmw_publisher_get_actual_qos,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, rmw_qos_profile_t *))

RMW_INTERFACE_FN(
  rmw_publisher_event_init,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(rmw_event_t *, const rmw_publisher_t *, rmw_event_type_t))

RMW_INTERFACE_FN(
  rmw_publish_serialized_message,
  rmw_ret_t, RMW_RET_ERROR,
  3,
  ARG_TYPES(
    const rmw_publisher_t *, const rmw_serialized_message_t *,
    rmw_publisher_allocation_t *))

RMW_INTERFACE_FN(
  rmw_get_serialized_message_size,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rosidl_message_type_support_t *,
    const rosidl_message_bounds_t *,
    size_t *))

RMW_INTERFACE_FN(
  rmw_publisher_assert_liveliness,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(const rmw_publisher_t *))

RMW_INTERFACE_FN(
  rmw_serialize,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const void *, const rosidl_message_type_support_t *, rmw_serialized_message_t *))

RMW_INTERFACE_FN(
  rmw_deserialize,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_serialized_message_t *, const rosidl_message_type_support_t *, void *))

RMW_INTERFACE_FN(
  rmw_init_subscription_allocation,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rosidl_message_type_support_t *,
    const rosidl_message_bounds_t *,
    rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_fini_subscription_allocation,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_create_subscription,
  rmw_subscription_t *, nullptr,
  5, ARG_TYPES(
    const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
    const rmw_qos_profile_t *, const rmw_subscription_options_t *))

RMW_INTERFACE_FN(
  rmw_destroy_subscription,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_subscription_t *))

RMW_INTERFACE_FN(
  rmw_subscription_count_matched_publishers,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_subscription_t *, size_t *))

RMW_INTERFACE_FN(
  rmw_subscription_get_actual_qos,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_subscription_t *, rmw_qos_profile_t *))

RMW_INTERFACE_FN(
  rmw_subscription_event_init,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(rmw_event_t *, const rmw_subscription_t *, rmw_event_type_t))

RMW_INTERFACE_FN(
  rmw_take,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_subscription_t *, void *, bool *, rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_sequence,
  rmw_ret_t, RMW_RET_ERROR,
  6, ARG_TYPES(
    const rmw_subscription_t *, size_t, rmw_message_sequence_t *,
    rmw_message_info_sequence_t *, size_t *, rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_with_info,
  rmw_ret_t, RMW_RET_ERROR,
  5,
  ARG_TYPES(
    const rmw_subscription_t *, void *, bool *, rmw_message_info_t *,
    rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_serialized_message,
  rmw_ret_t, RMW_RET_ERROR,
  4,
  ARG_TYPES(
    const rmw_subscription_t *, rmw_serialized_message_t *, bool *,
    rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_serialized_message_with_info,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_subscription_t *, rmw_serialized_message_t *, bool *, rmw_message_info_t *,
    rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_loaned_message,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(
    const rmw_subscription_t *, void **, bool *, rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_take_loaned_message_with_info,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_subscription_t *, void **, bool *, rmw_message_info_t *,
    rmw_subscription_allocation_t *))

RMW_INTERFACE_FN(
  rmw_return_loaned_message_from_subscription,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_subscription_t *, void *))

RMW_INTERFACE_FN(
  rmw_create_client,
  rmw_client_t *, nullptr,
  4, ARG_TYPES(
    const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
    const rmw_qos_profile_t *))

RMW_INTERFACE_FN(
  rmw_destroy_client,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_client_t *))

RMW_INTERFACE_FN(
  rmw_send_request,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_client_t *, const void *, int64_t *))

RMW_INTERFACE_FN(
  rmw_take_response,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_client_t *, rmw_request_id_t *, void *, bool *))

RMW_INTERFACE_FN(
  rmw_create_service,
  rmw_service_t *, nullptr,
  4, ARG_TYPES(
    const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
    const rmw_qos_profile_t *))

RMW_INTERFACE_FN(
  rmw_destroy_service,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_service_t *))

RMW_INTERFACE_FN(
  rmw_take_request,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *, bool *))

RMW_INTERFACE_FN(
  rmw_send_response,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *))

RMW_INTERFACE_FN(
  rmw_take_event,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_event_t *, void *, bool *))

RMW_INTERFACE_FN(
  rmw_create_guard_condition,
  rmw_guard_condition_t *, nullptr,
  1, ARG_TYPES(rmw_context_t *))

RMW_INTERFACE_FN(
  rmw_destroy_guard_condition,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_guard_condition_t *))

RMW_INTERFACE_FN(
  rmw_trigger_guard_condition,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(const rmw_guard_condition_t *))

RMW_INTERFACE_FN(
  rmw_create_wait_set,
  rmw_wait_set_t *, nullptr,
  2, ARG_TYPES(rmw_context_t *, size_t))

RMW_INTERFACE_FN(
  rmw_destroy_wait_set,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_wait_set_t *))

RMW_INTERFACE_FN(
  rmw_wait,
  rmw_ret_t, RMW_RET_ERROR,
  7, ARG_TYPES(
    rmw_subscriptions_t *, rmw_guard_conditions_t *, rmw_services_t *, rmw_clients_t *,
    rmw_events_t *, rmw_wait_set_t *, const rmw_time_t *))

RMW_INTERFACE_FN(
  rmw_get_publisher_names_and_types_by_node,
  rmw_ret_t, RMW_RET_ERROR,
  6, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, const char *, const char *, bool,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_subscriber_names_and_types_by_node,
  rmw_ret_t, RMW_RET_ERROR,
  6, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, const char *, const char *, bool,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_service_names_and_types_by_node,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, const char *, const char *,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_client_names_and_types_by_node,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, const char *, const char *,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_topic_names_and_types,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, bool,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_service_names_and_types,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(
  rmw_get_node_names,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, rcutils_string_array_t *, rcutils_string_array_t *))

RMW_INTERFACE_FN(
  rmw_get_node_names_with_enclaves,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(
    const rmw_node_t *, rcutils_string_array_t *,
    rcutils_string_array_t *, rcutils_string_array_t *))

RMW_INTERFACE_FN(
  rmw_count_publishers,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const char *, size_t *))

RMW_INTERFACE_FN(
  rmw_count_subscribers,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const char *, size_t *))

RMW_INTERFACE_FN(
  rmw_get_gid_for_publisher,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, rmw_gid_t *))

RMW_INTERFACE_FN(
  rmw_compare_gids_equal,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_gid_t *, const rmw_gid_t *, bool *))

RMW_INTERFACE_FN(
  rmw_service_server_is_available,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const rmw_client_t *, bool *))

RMW_INTERFACE_FN(
  rmw_set_log_severity,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_log_severity_t))

RMW_INTERFACE_FN(
  rmw_get_publishers_info_by_topic,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_node_t *,
    rcutils_allocator_t *,
    const char *,
    bool,
    rmw_topic_endpoint_info_array_t *))

RMW_INTERFACE_FN(
  rmw_get_subscriptions_info_by_topic,
  rmw_ret_t, RMW_RET_ERROR,
  5, ARG_TYPES(
    const rmw_node_t *,
    rcutils_allocator_t *,
    const char *,
    bool,
    rmw_topic_endpoint_info_array_t *))

#define GET_SYMBOL(x) symbol_ ## x = get_symbol(#x);

void prefetch_symbols(void)
{
  // get all symbols to avoid race conditions later since the passed
  // symbol name is expected to be a std::string which requires allocation
  GET_SYMBOL(rmw_get_implementation_identifier)
  GET_SYMBOL(rmw_init_options_init)
  GET_SYMBOL(rmw_init_options_copy)
  GET_SYMBOL(rmw_init_options_fini)
  GET_SYMBOL(rmw_shutdown)
  GET_SYMBOL(rmw_context_fini)
  GET_SYMBOL(rmw_get_serialization_format)
  GET_SYMBOL(rmw_create_node)
  GET_SYMBOL(rmw_destroy_node)
  GET_SYMBOL(rmw_node_assert_liveliness)
  GET_SYMBOL(rmw_node_get_graph_guard_condition)
  GET_SYMBOL(rmw_init_publisher_allocation);
  GET_SYMBOL(rmw_fini_publisher_allocation);
  GET_SYMBOL(rmw_create_publisher)
  GET_SYMBOL(rmw_destroy_publisher)
  GET_SYMBOL(rmw_borrow_loaned_message);
  GET_SYMBOL(rmw_return_loaned_message_from_publisher);
  GET_SYMBOL(rmw_publish)
  GET_SYMBOL(rmw_publish_loaned_message)
  GET_SYMBOL(rmw_publisher_count_matched_subscriptions)
  GET_SYMBOL(rmw_publisher_get_actual_qos);
  GET_SYMBOL(rmw_publisher_event_init)
  GET_SYMBOL(rmw_publish_serialized_message)
  GET_SYMBOL(rmw_publisher_assert_liveliness)
  GET_SYMBOL(rmw_get_serialized_message_size)
  GET_SYMBOL(rmw_serialize)
  GET_SYMBOL(rmw_deserialize)
  GET_SYMBOL(rmw_init_subscription_allocation)
  GET_SYMBOL(rmw_fini_subscription_allocation)
  GET_SYMBOL(rmw_create_subscription)
  GET_SYMBOL(rmw_destroy_subscription)
  GET_SYMBOL(rmw_subscription_count_matched_publishers);
  GET_SYMBOL(rmw_subscription_get_actual_qos);
  GET_SYMBOL(rmw_subscription_event_init)
  GET_SYMBOL(rmw_take)
  GET_SYMBOL(rmw_take_with_info)
  GET_SYMBOL(rmw_take_serialized_message)
  GET_SYMBOL(rmw_take_serialized_message_with_info)
  GET_SYMBOL(rmw_take_loaned_message)
  GET_SYMBOL(rmw_take_loaned_message_with_info)
  GET_SYMBOL(rmw_return_loaned_message_from_subscription)
  GET_SYMBOL(rmw_create_client)
  GET_SYMBOL(rmw_destroy_client)
  GET_SYMBOL(rmw_send_request)
  GET_SYMBOL(rmw_take_response)
  GET_SYMBOL(rmw_create_service)
  GET_SYMBOL(rmw_destroy_service)
  GET_SYMBOL(rmw_take_request)
  GET_SYMBOL(rmw_send_response)
  GET_SYMBOL(rmw_take_event)
  GET_SYMBOL(rmw_create_guard_condition)
  GET_SYMBOL(rmw_destroy_guard_condition)
  GET_SYMBOL(rmw_trigger_guard_condition)
  GET_SYMBOL(rmw_create_wait_set)
  GET_SYMBOL(rmw_destroy_wait_set)
  GET_SYMBOL(rmw_wait)
  GET_SYMBOL(rmw_get_publisher_names_and_types_by_node)
  GET_SYMBOL(rmw_get_subscriber_names_and_types_by_node)
  GET_SYMBOL(rmw_get_service_names_and_types_by_node)
  GET_SYMBOL(rmw_get_client_names_and_types_by_node)
  GET_SYMBOL(rmw_get_topic_names_and_types)
  GET_SYMBOL(rmw_get_service_names_and_types)
  GET_SYMBOL(rmw_get_node_names)
  GET_SYMBOL(rmw_get_node_names_with_enclaves)
  GET_SYMBOL(rmw_count_publishers)
  GET_SYMBOL(rmw_count_subscribers)
  GET_SYMBOL(rmw_get_gid_for_publisher)
  GET_SYMBOL(rmw_compare_gids_equal)
  GET_SYMBOL(rmw_service_server_is_available)
  GET_SYMBOL(rmw_set_log_severity)
  GET_SYMBOL(rmw_get_publishers_info_by_topic)
  GET_SYMBOL(rmw_get_subscriptions_info_by_topic)
}

void * symbol_rmw_init = nullptr;

rmw_ret_t
rmw_init(const rmw_init_options_t * options, rmw_context_t * context)
{
  prefetch_symbols();
  if (!symbol_rmw_init) {
    symbol_rmw_init = get_symbol("rmw_init");
  }
  if (!symbol_rmw_init) {
    return RMW_RET_ERROR;
  }

  typedef rmw_ret_t (* FunctionSignature)(const rmw_init_options_t *, rmw_context_t *);
  FunctionSignature func = reinterpret_cast<FunctionSignature>(symbol_rmw_init);
  return func(options, context);
}

#ifdef __cplusplus
}
#endif
