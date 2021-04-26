# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.13)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Add modules and include compiler options/switches
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(cmake/build/compile_options.cmake)

# Options for building the lib
option(build_ut_tests "Build the unit tests for the Azure IoT Middleware for FreeRTOS" OFF)
option(build_e2e_tests "Build the e2e tests for the Azure IoT Middleware for FreeRTOS" OFF)
set(config_directory CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h")

# If samples and tests aren't being built, then the user needs to provide a directory which has the necessary configs.
if(NOT (build_ut_tests OR build_e2e_tests) AND "${config_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
endif()

if(build_ut_tests AND build_e2e_tests)
  message(FATAL_ERROR "The library can only be built with one mqtt config. Unit tests and e2e tests must be built with different mqtt configs and therefore only one can be turned on at a time")
endif()

# If testing, turn on relevant switches and add configurations.
if(build_ut_tests)
  include(CTest)
  enable_testing()

  # Turn on the flags necessary for code coverage
  add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g3)
  add_link_options(-fprofile-arcs -ftest-coverage)

  # Set the port and config files
  set(AZURE_IOT_MQTT_PORT ${CMAKE_CURRENT_LIST_DIR}/tests/ut)
  set(config_directory ${CMAKE_CURRENT_LIST_DIR}/tests/config_files)
endif()

if(build_e2e_tests)
  include(CTest)
  enable_testing()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

  set(config_directory ${CMAKE_CURRENT_LIST_DIR}/tests/config_files)
endif()

# Create the configuration interface library which the middleware will link to
add_library(middleware_config INTERFACE)
target_include_directories(middleware_config
  INTERFACE
    ${config_directory}
)

# Add the embedded SDK library
add_subdirectory(libraries/azure-sdk-for-c)

# Add the middleware library
add_subdirectory(source/)

# Unit Tests (mocks require GNU)
if(build_ut_tests AND ${CMAKE_COMPILER_IS_GNUCC})
  set(MOCK_LINKER_OPTIONS "-Wl,--gc-sections")
  add_subdirectory(tests/ut)
elseif(build_ut_tests AND NOT ${CMAKE_COMPILER_IS_GNUCC})
  message(FATAL_ERROR "Due to necessary linker flags (for mocking functions), testing is only available on GNU gcc")
endif()

# E2E Tests
if(build_e2e_tests AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(FATAL_ERROR "E2E tests must be run on Linux")
elseif(build_e2e_tests)
  add_subdirectory(tests/e2e)
endif()
