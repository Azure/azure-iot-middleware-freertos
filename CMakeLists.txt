# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.14)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Add modules and include compiler options/switches
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(cmake/build/compile_options.cmake)

# Options for building the lib
option(build_ut_tests "Build the unit tests for the Azure IoT Middleware for FreeRTOS" OFF)
option(build_e2e_tests "Build the e2e tests for the Azure IoT Middleware for FreeRTOS" OFF)
set(config_directory CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h")
set(freertos_directory CACHE STRING "The directory which has the source code for FreeRTOS (as structured on their Github repo)")
set(freertos_port_directory CACHE STRING "The directory which has the port layer for FreeRTOS")

# If samples and tests aren't being built, then the user needs to provide a directory which has the necessary configs.
if(NOT (build_ut_tests OR build_e2e_tests) AND "${config_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
endif()

if(build_ut_tests AND build_e2e_tests)
  message(FATAL_ERROR "The library can only be built with one mqtt config. Unit tests and e2e tests must be built with different mqtt configs and therefore only one can be turned on at a time")
endif()

# If user has not supplied a FreeRTOS directory, fetch the content
if("${freertos_directory}" STREQUAL "")

  message("Fetching FreeRTOS - This could take up to a minute or so...")
  include(FetchContent)
  FetchContent_Declare(
    freertos_repo
    GIT_REPOSITORY        https://github.com/FreeRTOS/FreeRTOS.git
    GIT_TAG               c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
  )
  FetchContent_MakeAvailable(freertos_repo)

  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/include)
else()
  include_directories(${freertos_directory}/FreeRTOS/Source/include)
endif()

# Include user config directory
if(NOT( "${config_directory}" STREQUAL "" ))
  include_directories(${config_directory})
endif()

# Include user port directory
if(NOT( "${freertos_port_directory}" STREQUAL "" ))
  include_directories(${freertos_port_directory})
endif()

# If testing, turn on relevant switches and add configurations.
if(build_ut_tests OR build_e2e_tests)
  include(CTest)
  enable_testing()

  # Include port files for all targets
  if(WIN32)
    include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/MSVC-MingW)
  elseif(UNIX)
    include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)
  endif()

  # Include config files for all targets
  include_directories(${CMAKE_CURRENT_LIST_DIR}/tests/config_files)

  # Logging levels
  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/logging)

  if(build_ut_tests)
    # Turn on the flags necessary for code coverage
    add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g3)
    add_link_options(-fprofile-arcs -ftest-coverage)

    # Set the port and config files
    set(AZURE_IOT_MQTT_PORT ${CMAKE_CURRENT_LIST_DIR}/tests/ut)
  endif()

  if(build_e2e_tests)
    # Must build for 32 bit
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  endif()
endif()

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
