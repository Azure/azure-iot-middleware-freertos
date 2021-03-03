# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project (az_iot_middleware_freertos_ut LANGUAGES C)

enable_testing()

set(REPO_ROOT "${PROJECT_SOURCE_DIR}/..")

# Use custom dummy MQTT port
set(AZURE_IOT_MQTT_PORT ${PROJECT_SOURCE_DIR})

# Include middleware code
include(${REPO_ROOT}/source/CMakeLists.txt)

# Embedded SDK
add_subdirectory(${REPO_ROOT}/libraries/azure-sdk-for-c azure-sdk-for-c)
target_link_libraries(az_iot_freertos_middleware PUBLIC az_iot_hub az_iot_provisioning)

# FreeRTOS Kernel include
include_directories("${REPO_ROOT}/libraries/FreeRTOS/FreeRTOS/Source/include")

if(WIN32)
  include_directories("${REPO_ROOT}/libraries/FreeRTOS/FreeRTOS/Source/portable/MSVC-MingW")
elseif(UNIX)
  include_directories("${REPO_ROOT}/libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix")
endif()

# Unit test include
include_directories("${PROJECT_SOURCE_DIR}/config_files")
include_directories("${PROJECT_SOURCE_DIR}")

# list of unit tests
set(unit_tests
  azure_iot_hub_client_ut
  azure_iot_provisioning_client_ut)

# initialization_unit_test
add_executable(
  azure_iot_hub_client_ut
  ${PROJECT_SOURCE_DIR}/main.c
  ${PROJECT_SOURCE_DIR}/azure_iot_cmocka_mqtt.c
  ${PROJECT_SOURCE_DIR}/azure_iot_hub_client_ut.c)
target_link_libraries(
  azure_iot_hub_client_ut PUBLIC cmocka az_iot_freertos_middleware)

add_executable(
  azure_iot_provisioning_client_ut
  ${PROJECT_SOURCE_DIR}/main.c
  ${PROJECT_SOURCE_DIR}/azure_iot_cmocka_mqtt.c
  ${PROJECT_SOURCE_DIR}/azure_iot_provisioning_client_ut.c)
target_link_libraries(
  azure_iot_provisioning_client_ut PUBLIC cmocka az_iot_freertos_middleware)

foreach(test ${unit_tests})
  target_link_options(
    ${test}
      PRIVATE
        -Wl,--gc-sections)
  target_compile_options(
    ${test}
      PRIVATE -Werror
              -Wall
              -Wextra
              -pedantic
              -fmessage-length=0
              -fsigned-char
              -ffunction-sections
              -fdata-sections
              -Wunused
              -Wuninitialized
              -Wmissing-declarations
              -Wconversion
              -Wpointer-arith
              -Wshadow
              -Wlogical-op
              -Wfloat-equal )
  add_test(${test} ${test})
endforeach()
