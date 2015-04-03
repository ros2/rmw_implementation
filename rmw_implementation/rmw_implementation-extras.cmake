# Copyright 2014-2015 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# copied from rmw_implementation/rmw_implementation-extras.cmake

include("${rmw_implementation_DIR}/get_available_rmw_implementations.cmake")
include("${rmw_implementation_DIR}/get_default_rmw_implementation.cmake")

get_default_rmw_implementation(_middleware_implementation)
find_package("${_middleware_implementation}" REQUIRED)

# TODO should never need definitions and include dirs?
list(APPEND rmw_implementation_DEFINITIONS ${${_middleware_implementation}_DEFINITIONS})
list(APPEND rmw_implementation_INCLUDE_DIRS ${${_middleware_implementation}_INCLUDE_DIRS})
list(APPEND rmw_implementation_LIBRARIES ${${_middleware_implementation}_LIBRARIES})
