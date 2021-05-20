# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.14)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Options for building the lib
set(FREERTOS_DIRECTORY CACHE STRING "The directory which has the source code for FreeRTOS (as structured on their Github repo).")
set(FREERTOS_PORT_DIRECTORY CACHE STRING "The directory which has the port layer for FreeRTOS.")
set(CONFIG_DIRECTORY CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h.")

# The user needs to provide a FreeRTOS directory
if("${FREERTOS_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOS directory.")
endif()

# The user needs to provide a FreeRTOS port directory
if("${FREERTOS_PORT_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOS port directory.")
endif()

# The user needs to provide a directory which has the necessary configs.
if("${CONFIG_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
endif()

include_directories(${CONFIG_DIRECTORY})
include_directories(${FREERTOS_PORT_DIRECTORY})
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/include)

# Add the middleware library
add_subdirectory(source/)
