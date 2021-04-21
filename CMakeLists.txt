# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.13)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Add modules and include compiler options/switches
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")
# include(eng/cmake/global_compile_options.txt)

# Options for building the lib
option(build_tests "Build the tests for the Azure IoT Middleware for FreeRTOS" OFF)
set(config_directory CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h")

# If samples and tests aren't being built, then the user needs to provide a directory which has the necessary configs.
if(NOT build_tests AND "${config_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
endif()

# If testing, turn on relevant switches and add configurations.
if(build_tests)
  include(CTest)
  enable_testing()

  # Turn on the flags necessary for code coverage
  add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g3)
  add_link_options(-fprofile-arcs -ftest-coverage)

  # Set the port and config files
  set(AZURE_IOT_MQTT_PORT ${CMAKE_CURRENT_LIST_DIR}/tests)
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

# Tests (mocks require GNU)
if(${CMAKE_COMPILER_IS_GNUCC})
  set(MOCK_LINKER_OPTIONS "-Wl,--gc-sections")
  add_subdirectory(tests/)
elseif(build_tests AND NOT ${CMAKE_COMPILER_IS_GNUCC})
  message(FATAL_ERROR "Due to necessary linker flags (for mocking functions), testing is only available on GNU gcc")
endif()