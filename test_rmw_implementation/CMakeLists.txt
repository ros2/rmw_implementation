cmake_minimum_required(VERSION 3.5)

project(test_rmw_implementation)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(osrf_testing_tools_cpp REQUIRED)

  find_package(rcutils REQUIRED)
  find_package(rmw REQUIRED)
  find_package(rmw_implementation REQUIRED)
  find_package(rmw_implementation_cmake REQUIRED)

  macro(test_api)
    find_package(${rmw_implementation} REQUIRED)
    message(STATUS "Creating API tests for '${rmw_implementation}'")
    set(rmw_implementation_env_var RMW_IMPLEMENTATION=${rmw_implementation})

    ament_add_gtest(test_init_shutdown${target_suffix}
      test/test_init_shutdown.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_init_shutdown${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_init_shutdown${target_suffix}
      osrf_testing_tools_cpp rcutils rmw rmw_implementation
    )
  endmacro()

  call_for_each_rmw_implementation(test_api)

  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
