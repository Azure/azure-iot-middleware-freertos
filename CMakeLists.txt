# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.14)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Options for building the lib
set(freertos_directory CACHE STRING "The directory which has the source code for FreeRTOS (as structured on their Github repo).")
set(freertos_port_directory CACHE STRING "The directory which has the port layer for FreeRTOS.")
set(config_directory CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h.")

# The user needs to provide a FreeRTOS directory
if("${freertos_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOS directory.")
endif()

# The user needs to provide a FreeRTOS port directory
if("${freertos_port_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOS port directory.")
endif()

# The user needs to provide a directory which has the necessary configs.
if("${config_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
endif()

include_directories(${config_directory})
include_directories(${freertos_port_directory})
include_directories(${freertos_directory}/FreeRTOS/Source/include)

# Add the middleware library
add_subdirectory(source/)
