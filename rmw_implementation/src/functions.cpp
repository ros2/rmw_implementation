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
#include <cstdio>
#include <cstring>

#include <list>
#include <string>

#include <fstream>
#include <sstream>

#include "rcutils/allocator.h"
#include "rcutils/format_string.h"
#include "rcutils/types/string_array.h"

#include "Poco/SharedLibrary.h"

#include "rmw/error_handling.h"
#include "rmw/names_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/rmw.h"

std::string get_env_var(const char * env_var)
{
  char * value = nullptr;
#ifndef _WIN32
  value = getenv(env_var);
#else
  size_t value_size;
  _dupenv_s(&value, &value_size, env_var);
#endif
  std::string value_str = "";
  if (value) {
    value_str = value;
#ifdef _WIN32
    free(value);
#endif
  }
  // printf("get_env_var(%s) = %s\n", env_var, value_str.c_str());
  return value_str;
}

std::list<std::string> split(const std::string & value, const char delimiter)
{
  std::list<std::string> list;
  std::istringstream ss(value);
  std::string s;
  while (std::getline(ss, s, delimiter)) {
    list.push_back(s);
  }
  // printf("split(%s) = %zu\n", value.c_str(), list.size());
  return list;
}

bool is_file_exist(const char * filename)
{
  std::ifstream h(filename);
  // printf("is_file_exist(%s) = %s\n", filename, h.good() ? "true" : "false");
  return h.good();
}

std::string find_library_path(const std::string & library_name)
{
  const char * env_var;
  char separator;
  const char * filename_prefix;
  const char * filename_extension;
#ifdef _WIN32
  env_var = "PATH";
  separator = ';';
  filename_prefix = "";
  filename_extension = ".dll";
#elif __APPLE__
  env_var = "DYLD_LIBRARY_PATH";
  separator = ':';
  filename_prefix = "lib";
  filename_extension = ".dylib";
#else
  env_var = "LD_LIBRARY_PATH";
  separator = ':';
  filename_prefix = "lib";
  filename_extension = ".so";
#endif
  std::string search_path = get_env_var(env_var);
  std::list<std::string> search_paths = split(search_path, separator);

  std::string filename = filename_prefix;
  filename += library_name + filename_extension;

  for (auto it : search_paths) {
    std::string path = it + "/" + filename;
    if (is_file_exist(path.c_str())) {
      return path;
    }
  }
  return "";
}

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

Poco::SharedLibrary *
get_library()
{
  static Poco::SharedLibrary * lib = nullptr;
  if (!lib) {
    std::string env_var = get_env_var("RMW_IMPLEMENTATION");
    if (env_var.empty()) {
      env_var = STRINGIFY(DEFAULT_RMW_IMPLEMENTATION);
    }
    std::string library_path = find_library_path(env_var);
    if (library_path.empty()) {
      RMW_SET_ERROR_MSG("failed to find shared library of rmw implementation");
      return nullptr;
    }
    try {
      lib = new Poco::SharedLibrary(library_path);
    } catch (...) {
      RMW_SET_ERROR_MSG("failed to load shared library of rmw implementation");
      return nullptr;
    }
  }
  return lib;
}

void *
get_symbol(const char * symbol_name)
{
  Poco::SharedLibrary * lib = get_library();
  if (!lib) {
    // error message set by get_library()
    return nullptr;
  }
  if (!lib->hasSymbol(symbol_name)) {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char * msg = rcutils_format_string(
      allocator,
      "failed to resolve symbol in shared library '%s'", lib->getPath().c_str());
    if (msg) {
      RMW_SET_ERROR_MSG(msg);
      allocator.deallocate(msg, allocator.state);
    } else {
      RMW_SET_ERROR_MSG("failed to allocate memory for error message")
    }
    return nullptr;
  }
  return lib->getSymbol(symbol_name);
}

#if __cplusplus
extern "C"
{
#endif

#define EXPAND(x) x
#define ARG_TYPES(...) __VA_ARGS__

#define ARG_VALS0(...)
#define ARG_VALS1(t1) v1
#define ARG_VALS2(t2, ...) v2, EXPAND(ARG_VALS1(__VA_ARGS__))
#define ARG_VALS3(t3, ...) v3, EXPAND(ARG_VALS2(__VA_ARGS__))
#define ARG_VALS4(t4, ...) v4, EXPAND(ARG_VALS3(__VA_ARGS__))
#define ARG_VALS5(t5, ...) v5, EXPAND(ARG_VALS4(__VA_ARGS__))
#define ARG_VALS6(t6, ...) v6, EXPAND(ARG_VALS5(__VA_ARGS__))

#define ARGS0(...) __VA_ARGS__
#define ARGS1(t1) t1 v1
#define ARGS2(t2, ...) t2 v2, EXPAND(ARGS1(__VA_ARGS__))
#define ARGS3(t3, ...) t3 v3, EXPAND(ARGS2(__VA_ARGS__))
#define ARGS4(t4, ...) t4 v4, EXPAND(ARGS3(__VA_ARGS__))
#define ARGS5(t5, ...) t5 v5, EXPAND(ARGS4(__VA_ARGS__))
#define ARGS6(t6, ...) t6 v6, EXPAND(ARGS5(__VA_ARGS__))

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

#define FOO_INIT do {} while (0)

#define RMW_INTERFACE_FN_INIT(name, ReturnType, error_value, init_fn, _NR, ...) \
  void * symbol_ ## name = nullptr; \
  ReturnType name(EXPAND(ARGS ## _NR(__VA_ARGS__))) \
  { \
    init_fn; \
    CALL_SYMBOL(name, ReturnType, error_value, ARG_TYPES(__VA_ARGS__), \
      EXPAND(ARG_VALS ## _NR(__VA_ARGS__))); \
  }

#define RMW_INTERFACE_FN(name, ReturnType, error_value, _NR, ...) \
  RMW_INTERFACE_FN_INIT(name, ReturnType, error_value, FOO_INIT, _NR, __VA_ARGS__)

RMW_INTERFACE_FN(rmw_get_implementation_identifier,
  const char *, nullptr,
  0, ARG_TYPES(void))

RMW_INTERFACE_FN(rmw_create_node,
  rmw_node_t *, nullptr,
  4, ARG_TYPES(
    const char *, const char *, size_t,
    const rmw_node_security_options_t *))

RMW_INTERFACE_FN(rmw_destroy_node,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_node_t *))

RMW_INTERFACE_FN(rmw_node_get_graph_guard_condition,
  const rmw_guard_condition_t *, nullptr,
  1, ARG_TYPES(const rmw_node_t *))

RMW_INTERFACE_FN(rmw_create_publisher,
  rmw_publisher_t *, nullptr,
  4, ARG_TYPES(
    const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
    const rmw_qos_profile_t *))

RMW_INTERFACE_FN(rmw_destroy_publisher,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_publisher_t *))

RMW_INTERFACE_FN(rmw_publish,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, const void *))

RMW_INTERFACE_FN(rmw_create_subscription,
  rmw_subscription_t *, nullptr,
  5, ARG_TYPES(
    const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
    const rmw_qos_profile_t *, bool))

RMW_INTERFACE_FN(rmw_destroy_subscription,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_subscription_t *))

RMW_INTERFACE_FN(rmw_take,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_subscription_t *, void *, bool *))

RMW_INTERFACE_FN(rmw_take_with_info,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_subscription_t *, void *, bool *, rmw_message_info_t *))

RMW_INTERFACE_FN(rmw_create_client,
  rmw_client_t *, nullptr,
  4, ARG_TYPES(
    const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
    const rmw_qos_profile_t *))

RMW_INTERFACE_FN(rmw_destroy_client,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_client_t *))

RMW_INTERFACE_FN(rmw_send_request,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_client_t *, const void *, int64_t *))

RMW_INTERFACE_FN(rmw_take_response,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_client_t *, rmw_request_id_t *, void *, bool *))

RMW_INTERFACE_FN(rmw_create_service,
  rmw_service_t *, nullptr,
  4, ARG_TYPES(
    const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
    const rmw_qos_profile_t *))

RMW_INTERFACE_FN(rmw_destroy_service,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(rmw_node_t *, rmw_service_t *))

RMW_INTERFACE_FN(rmw_take_request,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *, bool *))

RMW_INTERFACE_FN(rmw_send_response,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *))

RMW_INTERFACE_FN(rmw_create_guard_condition,
  rmw_guard_condition_t *, nullptr,
  0, ARG_TYPES(void))

RMW_INTERFACE_FN(rmw_destroy_guard_condition,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_guard_condition_t *))

RMW_INTERFACE_FN(rmw_trigger_guard_condition,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(const rmw_guard_condition_t *))

RMW_INTERFACE_FN(rmw_create_waitset,
  rmw_waitset_t *, nullptr,
  1, ARG_TYPES(size_t))

RMW_INTERFACE_FN(rmw_destroy_waitset,
  rmw_ret_t, RMW_RET_ERROR,
  1, ARG_TYPES(rmw_waitset_t *))

RMW_INTERFACE_FN(rmw_wait,
  rmw_ret_t, RMW_RET_ERROR,
  6, ARG_TYPES(
    rmw_subscriptions_t *, rmw_guard_conditions_t *, rmw_services_t *,
    rmw_clients_t *, rmw_waitset_t *, const rmw_time_t *))

RMW_INTERFACE_FN(rmw_get_topic_names_and_types,
  rmw_ret_t, RMW_RET_ERROR,
  4, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *, bool,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(rmw_get_service_names_and_types,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(
    const rmw_node_t *, rcutils_allocator_t *,
    rmw_names_and_types_t *))

RMW_INTERFACE_FN(rmw_get_node_names,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_node_t *, rcutils_string_array_t *))

RMW_INTERFACE_FN(rmw_count_publishers,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const char *, size_t *))

RMW_INTERFACE_FN(rmw_count_subscribers,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const char *, size_t *))

RMW_INTERFACE_FN(rmw_get_gid_for_publisher,
  rmw_ret_t, RMW_RET_ERROR,
  2, ARG_TYPES(const rmw_publisher_t *, rmw_gid_t *))

RMW_INTERFACE_FN(rmw_compare_gids_equal,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_gid_t *, const rmw_gid_t *, bool *))

RMW_INTERFACE_FN(rmw_service_server_is_available,
  rmw_ret_t, RMW_RET_ERROR,
  3, ARG_TYPES(const rmw_node_t *, const rmw_client_t *, bool *))

#define GET_SYMBOL(x) symbol_ ## x = get_symbol(#x)
void pre_fetch_symbol(void)
{
  // get all symbols to avoid race conditions later since the passed
  // symbol name is expected to be a std::string which requires allocation
  GET_SYMBOL(rmw_get_implementation_identifier);
  GET_SYMBOL(rmw_create_node);
  GET_SYMBOL(rmw_destroy_node);
  GET_SYMBOL(rmw_node_get_graph_guard_condition);
  GET_SYMBOL(rmw_create_publisher);
  GET_SYMBOL(rmw_destroy_publisher);
  GET_SYMBOL(rmw_publish);
  GET_SYMBOL(rmw_create_subscription);
  GET_SYMBOL(rmw_destroy_subscription);
  GET_SYMBOL(rmw_take);
  GET_SYMBOL(rmw_take_with_info);
  GET_SYMBOL(rmw_create_client);
  GET_SYMBOL(rmw_destroy_client);
  GET_SYMBOL(rmw_send_request);
  GET_SYMBOL(rmw_take_response);
  GET_SYMBOL(rmw_create_service);
  GET_SYMBOL(rmw_destroy_service);
  GET_SYMBOL(rmw_take_request);
  GET_SYMBOL(rmw_send_response);
  GET_SYMBOL(rmw_create_guard_condition);
  GET_SYMBOL(rmw_destroy_guard_condition);
  GET_SYMBOL(rmw_trigger_guard_condition);
  GET_SYMBOL(rmw_create_waitset);
  GET_SYMBOL(rmw_destroy_waitset);
  GET_SYMBOL(rmw_wait);
  GET_SYMBOL(rmw_get_topic_names_and_types);
  GET_SYMBOL(rmw_get_service_names_and_types);
  GET_SYMBOL(rmw_get_node_names);
  GET_SYMBOL(rmw_count_publishers);
  GET_SYMBOL(rmw_count_subscribers);
  GET_SYMBOL(rmw_get_gid_for_publisher);
  GET_SYMBOL(rmw_compare_gids_equal);
  GET_SYMBOL(rmw_service_server_is_available);
}

RMW_INTERFACE_FN_INIT(rmw_init,
  rmw_ret_t, RMW_RET_ERROR, pre_fetch_symbol(),
  0, ARG_TYPES(void))

#if __cplusplus
}
#endif
