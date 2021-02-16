// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#include "rmw/qos_profiles.h"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

TEST(CLASSNAME(TestQoSProfilesAreCompatible, RMW_IMPLEMENTATION), compatible) {
  rmw_ret_t ret;
  rmw_qos_compatibility_type_t compatible;
  // All of the provided profiles should be compatible with themselves
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_sensor_data, rmw_qos_profile_sensor_data, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_default, rmw_qos_profile_default, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_parameters, rmw_qos_profile_parameters, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_parameter_events, rmw_qos_profile_parameter_events, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_services_default, rmw_qos_profile_services_default, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_system_default, rmw_qos_profile_system_default, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatible, RMW_QOS_COMPATIBILITY_OK);
}

TEST(CLASSNAME(TestQoSProfilesAreCompatible, RMW_IMPLEMENTATION), error_on_unknown) {
  rmw_ret_t ret;
  rmw_qos_compatibility_type_t compatible;
  // Expect an error code if a policy is "unknown"
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_sensor_data, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_default, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_parameters, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_parameter_events, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_services_default, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
  ret = rmw_qos_profile_check_compatible(
    rmw_qos_profile_system_default, rmw_qos_profile_unknown, &compatible, nullptr, 0u);
  EXPECT_EQ(ret, RMW_RET_INVALID_ARGUMENT);
}
