# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.14)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Options for building the lib
set(config_directory CACHE STRING "The directory which has the FreeRTOSConfig.h and azure_iot_config.h")
set(freertos_directory CACHE STRING "The directory which has the source code for FreeRTOS (as structured on their Github repo)")
set(freertos_port_directory CACHE STRING "The directory which has the port layer for FreeRTOS")

# If samples and tests aren't being built, then the user needs to provide a directory which has the necessary configs.
if("${config_directory}" STREQUAL "")
  message(FATAL_ERROR "The library needs a FreeRTOSConfig.h and azure_iot_config.h directory. Please set the path to the directory with those files.")
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
  # Include port files for all targets
elseif(WIN32)
  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/MSVC-MingW)
elseif(UNIX)
  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)
endif()

# Add the middleware library
add_subdirectory(source/)
