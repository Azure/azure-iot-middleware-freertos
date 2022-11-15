# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.14)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Options for building the lib
set(FREERTOS_DIRECTORY CACHE STRING "The directory which has the source code for FreeRTOS (as structured on their Github repo).")
set(FREERTOS_PORT_DIRECTORY CACHE STRING "The directory which has the port layer for FreeRTOS.")
set(CONFIG_DIRECTORY CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h.")
option(USE_COREHTTP "Enables building coreHTTP and azure_iot_core_http" OFF)

# The user needs to provide a FreeRTOS directory
if("${FREERTOS_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "FREERTOS_DIRECTORY needed: the library needs a FreeRTOS directory. Please clone FreeRTOS separately or use an already downloaded copy and pass the path in this variable.")
endif()

# The user needs to provide a FreeRTOS port directory
if("${FREERTOS_PORT_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "FREERTOS_PORT_DIRECTORY needed: the library needs a FreeRTOS port directory. Please pass the path to your selected port in the cloned FreeRTOS repo.")
endif()

# The user needs to provide a directory which has the necessary configs.
if("${CONFIG_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "CONFIG_DIRECTORY needed: the library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please create a directory with those files and pass it in this variable.")
endif()

include_directories(${CONFIG_DIRECTORY})
include_directories(${FREERTOS_PORT_DIRECTORY})
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/include)

# Add the middleware library
add_subdirectory(source/)
