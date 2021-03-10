# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(eng/cmake/global_compile_options.txt)

add_subdirectory(libraries/azure-sdk-for-c)

# Sources/Libraries
add_subdirectory(source/)

# Tests (mocks require GNU)
if(${CMAKE_COMPILER_IS_GNUCC})
{
  set(MOCK_COMPILE_OPTIONS "-Wl,--gc-sections")
  add_subdirectory(tests/)
}
