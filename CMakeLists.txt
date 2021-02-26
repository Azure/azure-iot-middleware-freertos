# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project(az_iot_middleware_freertos LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(eng/cmake/global_compile_options.txt)

# Sources/Libraries
add_subdirectory(source/)

# Tests (mocks require GNU)
if(${CMAKE_COMPILER_IS_GNUCC})
{
  add_subdirectory(tests/)
}
