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
  find_package(test_msgs REQUIRED)

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

    ament_add_gtest(test_init_options${target_suffix}
      test/test_init_options.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_init_options${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_init_options${target_suffix}
      osrf_testing_tools_cpp rcutils rmw rmw_implementation
    )

    ament_add_gtest(test_create_destroy_node${target_suffix}
      test/test_create_destroy_node.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_create_destroy_node${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_create_destroy_node${target_suffix}
      osrf_testing_tools_cpp rcutils rmw rmw_implementation
    )

    ament_add_gtest(test_publisher${target_suffix}
      test/test_publisher.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_publisher${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_publisher${target_suffix}
      osrf_testing_tools_cpp rcutils rmw rmw_implementation test_msgs
    )

    ament_add_gtest(test_subscription${target_suffix}
      test/test_subscription.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_subscription${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_subscription${target_suffix}
      osrf_testing_tools_cpp rcutils rmw rmw_implementation test_msgs
    )

    ament_add_gtest(test_serialize_deserialize${target_suffix}
      test/test_serialize_deserialize.cpp
      ENV ${rmw_implementation_env_var}
    )
    target_compile_definitions(test_serialize_deserialize${target_suffix}
      PUBLIC "RMW_IMPLEMENTATION=${rmw_implementation}")
    ament_target_dependencies(test_serialize_deserialize${target_suffix}
      rcutils rmw rmw_implementation test_msgs
    )
  endmacro()

  call_for_each_rmw_implementation(test_api)

  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
