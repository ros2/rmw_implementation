cmake_minimum_required(VERSION 3.5)

project(rmw_implementation)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

find_package(rcutils REQUIRED)

# provides FindPoco.cmake and Poco on platforms without it
find_package(poco_vendor)
find_package(Poco COMPONENTS Foundation)
find_package(rmw_implementation_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

get_default_rmw_implementation(RMW_IMPLEMENTATION)

get_available_rmw_implementations(RMW_IMPLEMENTATIONS)
message(STATUS "")
message(STATUS "Found these rmw implementations:")
foreach(impl IN LISTS RMW_IMPLEMENTATIONS)
  message(STATUS "  '${impl}'")
endforeach()

message(STATUS "")
message(STATUS "Using default rmw implementation: '${RMW_IMPLEMENTATION}'")
message(STATUS "")

# if only a single rmw impl. is available or poco is not available
# this package will directly reference the default rmw impl.
if(NOT RMW_IMPLEMENTATIONS MATCHES ";" OR NOT Poco_FOUND)
  set(RMW_IMPLEMENTATION_SUPPORTS_POCO FALSE)

else()
  set(RMW_IMPLEMENTATION_SUPPORTS_POCO TRUE)
  find_package(rmw REQUIRED)

  message(STATUS "Support runtime override with any of the available rmw implementations")
  message(STATUS "")
  link_directories(${Poco_LIBRARY_DIR})
  add_library(${PROJECT_NAME} SHARED
    src/functions.cpp)
  ament_target_dependencies(${PROJECT_NAME}
    "Poco"
    "rmw")
  target_compile_definitions(${PROJECT_NAME}
    PUBLIC "DEFAULT_RMW_IMPLEMENTATION=${RMW_IMPLEMENTATION}")
  target_link_libraries(${PROJECT_NAME})
  configure_rmw_library(${PROJECT_NAME})

  ament_export_libraries(${PROJECT_NAME})
  if(NOT WIN32)
    ament_export_libraries(pthread)
  endif()

  install(
    TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )
endif()

register_rmw_implementation(
  "c:rosidl_typesupport_c"
  "cpp:rosidl_typesupport_cpp"
)

# necessary since get_available_rmw_implementations excludes rmw_implementation
list(APPEND RMW_IMPLEMENTATIONS "rmw_implementation")
ament_package(
  CONFIG_EXTRAS "rmw_implementation-extras.cmake.in"
)
