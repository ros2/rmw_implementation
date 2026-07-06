cmake_minimum_required(VERSION 3.20)

project(test_rmw_implementation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_ros REQUIRED)
  find_package(ament_cmake_ros_core REQUIRED)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(osrf_testing_tools_cpp REQUIRED)

  find_package(rcutils REQUIRED)
  find_package(rmw REQUIRED)
  find_package(rmw_dds_common REQUIRED)
  find_package(rmw_implementation REQUIRED)
  find_package(rmw_implementation_cmake REQUIRED)
  find_package(rosidl_runtime_c REQUIRED)
  find_package(test_msgs REQUIRED)

  # finding gtest once in the highest scope
  # prevents finding it repeatedly in each local scope
  ament_find_gtest()

  ament_add_gtest_executable(test_init_shutdown
    test/test_init_shutdown.cpp
  )
  target_link_libraries(test_init_shutdown
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_init_options
    test/test_init_options.cpp
  )
  target_link_libraries(test_init_options
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_create_destroy_node
    test/test_create_destroy_node.cpp
  )
  target_link_libraries(test_create_destroy_node
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_publisher
    test/test_publisher.cpp
  )
  target_link_libraries(test_publisher
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_subscription
    test/test_subscription.cpp
  )
  target_link_libraries(test_subscription
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_dds_common::rmw_dds_common_library
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_serialize_deserialize
    test/test_serialize_deserialize.cpp
  )
  target_link_libraries(test_serialize_deserialize
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rosidl_runtime_c::rosidl_runtime_c
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_publisher_allocator
    test/test_publisher_allocator.cpp
  )
  target_link_libraries(test_publisher_allocator
    ament_cmake_ros_core::ament_ros_defaults
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_subscription_allocator
    test/test_subscription_allocator.cpp
  )
  target_link_libraries(test_subscription_allocator
    ament_cmake_ros_core::ament_ros_defaults
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_wait_set
    test/test_wait_set.cpp
  )
  target_link_libraries(test_wait_set
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_graph_api
    test/test_graph_api.cpp
  )
  target_link_libraries(test_graph_api
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_unique_identifiers
    test/test_unique_identifiers.cpp
  )
  target_link_libraries(test_unique_identifiers
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_service
    test/test_service.cpp
  )
  target_link_libraries(test_service
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_client
    test/test_client.cpp
  )
  target_link_libraries(test_client
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_qos_profile_check_compatible
    test/test_qos_profile_check_compatible.cpp
  )
  target_link_libraries(test_qos_profile_check_compatible
    ament_cmake_ros_core::ament_ros_defaults
    rmw::rmw
    rmw_implementation::rmw_implementation
  )

  ament_add_gtest_executable(test_duration_infinite
    test/test_duration_infinite.cpp
  )
  target_link_libraries(test_duration_infinite
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  ament_add_gtest_executable(test_event
    test/test_event.cpp
  )
  target_link_libraries(test_event
    ament_cmake_ros_core::ament_ros_defaults
    osrf_testing_tools_cpp::memory_tools
    rmw::rmw
    rmw_implementation::rmw_implementation
    test_msgs::test_msgs
  )

  function(test_api)
    message(STATUS "Creating API tests for '${rmw_implementation}'")
    set(rmw_implementation_env_var RMW_IMPLEMENTATION=${rmw_implementation})

    ament_add_ros_isolated_gtest_test(test_init_shutdown
      TEST_NAME test_init_shutdown${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_init_options
      TEST_NAME test_init_options${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_create_destroy_node
      TEST_NAME test_create_destroy_node${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_publisher
      TEST_NAME test_publisher${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_subscription
      TEST_NAME test_subscription${target_suffix}
      TIMEOUT 120
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_serialize_deserialize
      TEST_NAME test_serialize_deserialize${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_publisher_allocator
      TEST_NAME test_publisher_allocator${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_subscription_allocator
      TEST_NAME test_subscription_allocator${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_wait_set
      TEST_NAME test_wait_set${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_graph_api
      TEST_NAME test_graph_api${target_suffix}
      TIMEOUT 120
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_unique_identifiers
      TEST_NAME test_unique_identifiers${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_service
      TEST_NAME test_service${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_client
      TEST_NAME test_client${target_suffix}
      TIMEOUT 120
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_qos_profile_check_compatible
      TEST_NAME test_qos_profile_check_compatible${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_duration_infinite
      TEST_NAME test_duration_infinite${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )

    ament_add_ros_isolated_gtest_test(test_event
      TEST_NAME test_event${target_suffix}
      ENV
        ${rmw_implementation_env_var}
    )
  endfunction()

  call_for_each_rmw_implementation(test_api)

  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
