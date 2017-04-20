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

#include "rcutils/types/string_array.h"

#include "Poco/SharedLibrary.h"

#include "rmw/error_handling.h"
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
    RMW_SET_ERROR_MSG("failed to resolve symbol in shared library of rmw implementation");
    return nullptr;
  }
  return lib->getSymbol(symbol_name);
}

#if __cplusplus
extern "C"
{
#endif

#define ARG_TYPES(...) __VA_ARGS__
#define ARG_VALUES(...) __VA_ARGS__

#define CALL_SYMBOL(symbol_name, ReturnType, error_value, ArgTypes, arg_values) \
  void * symbol = get_symbol(symbol_name); \
  if (!symbol) { \
    /* error message set by get_symbol() */ \
    return error_value; \
  } \
  typedef ReturnType (* FunctionSignature)(ArgTypes); \
  FunctionSignature func = reinterpret_cast<FunctionSignature>(symbol); \
  return func(arg_values);


const char *
rmw_get_implementation_identifier(void)
{
  CALL_SYMBOL(
    "rmw_get_implementation_identifier", const char *, nullptr,
    ARG_TYPES(void), ARG_VALUES());
}

rmw_ret_t
rmw_init(void)
{
  CALL_SYMBOL(
    "rmw_init", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(void), ARG_VALUES());
}

rmw_node_t *
rmw_create_node(const char * name, const char * namespace_, size_t domain_id)
{
  CALL_SYMBOL(
    "rmw_create_node", rmw_node_t *, nullptr,
    ARG_TYPES(const char *, const char *, size_t), ARG_VALUES(name, namespace_, domain_id));
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  CALL_SYMBOL(
    "rmw_destroy_node", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_node_t *), ARG_VALUES(node));
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  CALL_SYMBOL(
    "rmw_node_get_graph_guard_condition", const rmw_guard_condition_t *, nullptr,
    ARG_TYPES(const rmw_node_t *), ARG_VALUES(node));
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies)
{
  CALL_SYMBOL(
    "rmw_create_publisher", rmw_publisher_t *, nullptr,
    ARG_TYPES(
      const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
      const rmw_qos_profile_t *),
    ARG_VALUES(node, type_support, topic_name, qos_policies));
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  CALL_SYMBOL(
    "rmw_destroy_publisher", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_node_t *, rmw_publisher_t *),
    ARG_VALUES(node, publisher));
}

rmw_ret_t
rmw_publish(const rmw_publisher_t * publisher, const void * ros_message)
{
  CALL_SYMBOL(
    "rmw_publish", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_publisher_t *, const void *),
    ARG_VALUES(publisher, ros_message));
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  bool ignore_local_publications)
{
  CALL_SYMBOL(
    "rmw_create_subscription", rmw_subscription_t *, nullptr,
    ARG_TYPES(
      const rmw_node_t *, const rosidl_message_type_support_t *, const char *,
      const rmw_qos_profile_t *, bool),
    ARG_VALUES(node, type_support, topic_name, qos_policies, ignore_local_publications));
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  CALL_SYMBOL(
    "rmw_destroy_subscription", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_node_t *, rmw_subscription_t *),
    ARG_VALUES(node, subscription));
}

rmw_ret_t
rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken)
{
  CALL_SYMBOL(
    "rmw_take", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_subscription_t *, void *, bool *),
    ARG_VALUES(subscription, ros_message, taken));
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  CALL_SYMBOL(
    "rmw_take_with_info", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_subscription_t *, void *, bool *, rmw_message_info_t *),
    ARG_VALUES(subscription, ros_message, taken, message_info));
}

rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  CALL_SYMBOL(
    "rmw_create_client", rmw_client_t *, nullptr,
    ARG_TYPES(
      const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
      const rmw_qos_profile_t *),
    ARG_VALUES(node, type_support, service_name, qos_policies));
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  CALL_SYMBOL(
    "rmw_destroy_client", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_node_t *, rmw_client_t *),
    ARG_VALUES(node, client));
}

rmw_ret_t
rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  CALL_SYMBOL(
    "rmw_send_request", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_client_t *, const void *, int64_t *),
    ARG_VALUES(client, ros_request, sequence_id));
}

rmw_ret_t
rmw_take_response(
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken)
{
  CALL_SYMBOL(
    "rmw_take_response", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_client_t *, rmw_request_id_t *, void *, bool *),
    ARG_VALUES(client, request_header, ros_response, taken));
}

rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  CALL_SYMBOL(
    "rmw_create_service", rmw_service_t *, nullptr,
    ARG_TYPES(
      const rmw_node_t *, const rosidl_service_type_support_t *, const char *,
      const rmw_qos_profile_t *),
    ARG_VALUES(node, type_support, service_name, qos_policies));
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  CALL_SYMBOL(
    "rmw_destroy_service", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_node_t *, rmw_service_t *),
    ARG_VALUES(node, service));
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken)
{
  CALL_SYMBOL(
    "rmw_take_request", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *, bool *),
    ARG_VALUES(service, request_header, ros_request, taken));
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  CALL_SYMBOL(
    "rmw_send_response", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_service_t *, rmw_request_id_t *, void *),
    ARG_VALUES(service, request_header, ros_response));
}

rmw_guard_condition_t *
rmw_create_guard_condition(void)
{
  CALL_SYMBOL(
    "rmw_create_guard_condition", rmw_guard_condition_t *, nullptr,
    ARG_TYPES(void), ARG_VALUES());
}

rmw_ret_t
rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  CALL_SYMBOL(
    "rmw_destroy_guard_condition", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_guard_condition_t *),
    ARG_VALUES(guard_condition));
}

rmw_ret_t
rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition)
{
  CALL_SYMBOL(
    "rmw_trigger_guard_condition", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_guard_condition_t *),
    ARG_VALUES(guard_condition));
}

rmw_waitset_t *
rmw_create_waitset(size_t max_conditions)
{
  CALL_SYMBOL(
    "rmw_create_waitset", rmw_waitset_t *, nullptr,
    ARG_TYPES(size_t),
    ARG_VALUES(max_conditions));
}

rmw_ret_t
rmw_destroy_waitset(rmw_waitset_t * waitset)
{
  CALL_SYMBOL(
    "rmw_destroy_waitset", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_waitset_t *),
    ARG_VALUES(waitset));
}

rmw_ret_t
rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_waitset_t * waitset,
  const rmw_time_t * wait_timeout)
{
  CALL_SYMBOL(
    "rmw_wait", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(
      rmw_subscriptions_t *, rmw_guard_conditions_t *, rmw_services_t *,
      rmw_clients_t *, rmw_waitset_t *, const rmw_time_t *),
    ARG_VALUES(
      subscriptions, guard_conditions, services,
      clients, waitset, wait_timeout));
}

rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  CALL_SYMBOL(
    "rmw_get_topic_names_and_types", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_node_t *, rmw_topic_names_and_types_t *),
    ARG_VALUES(node, topic_names_and_types));
}

rmw_ret_t
rmw_destroy_topic_names_and_types(
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  CALL_SYMBOL(
    "rmw_destroy_topic_names_and_types", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(rmw_topic_names_and_types_t *),
    ARG_VALUES(topic_names_and_types));
}

rmw_ret_t
rmw_get_node_names(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names)
{
  CALL_SYMBOL(
    "rmw_get_node_names", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_node_t *, rcutils_string_array_t *),
    ARG_VALUES(node, node_names));
}

rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  CALL_SYMBOL(
    "rmw_count_publishers", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_node_t *, const char *, size_t *),
    ARG_VALUES(node, topic_name, count));
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  CALL_SYMBOL(
    "rmw_count_subscribers", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_node_t *, const char *, size_t *),
    ARG_VALUES(node, topic_name, count));
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  CALL_SYMBOL(
    "rmw_get_gid_for_publisher", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(
      const rmw_publisher_t *, rmw_gid_t *),
    ARG_VALUES(publisher, gid));
}

rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  CALL_SYMBOL(
    "rmw_compare_gids_equal", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_gid_t *, const rmw_gid_t *, bool *),
    ARG_VALUES(gid1, gid2, result));
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  CALL_SYMBOL(
    "rmw_service_server_is_available", rmw_ret_t, RMW_RET_ERROR,
    ARG_TYPES(const rmw_node_t *, const rmw_client_t *, bool *),
    ARG_VALUES(node, client, is_available));
}

#if __cplusplus
}
#endif
