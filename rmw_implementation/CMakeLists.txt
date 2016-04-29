cmake_minimum_required(VERSION 2.8.3)

# must be not NONE since some rmw implementation require compiler checks
project(rmw_implementation)

find_package(ament_cmake REQUIRED)
find_package(rmw_implementation_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

get_default_rmw_implementation(RMW_IMPLEMENTATION)

ament_package(
  CONFIG_EXTRAS "rmw_implementation-extras.cmake.in"
)
