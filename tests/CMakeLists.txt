# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project (az_iot_middleware_freertos_ut LANGUAGES C)

enable_testing()

set(AZ_IOT_REPO_ROOT "${PROJECT_SOURCE_DIR}/..")

# GCC flags required via code coverage
add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g3)
add_link_options(-fprofile-arcs -ftest-coverage)

# FreeRTOS Kernel include
include_directories(${AZ_IOT_REPO_ROOT}/libraries/FreeRTOS/FreeRTOS/Source/include)

# port for FreeRTOS
if(UNIX)
    include_directories(${AZ_IOT_REPO_ROOT}/libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)
else()
    message( FATAL_ERROR "Tests only supported in linux." )
endif()

# Middleware source
add_library(az_iot_middleware_freertos
  ${AZ_IOT_REPO_ROOT}/source/azure_iot_provisioning_client.c
)

target_compile_options(az_iot_middleware_freertos
    PRIVATE
    -Werror
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
    -Wfloat-equal
)

target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/config_files
)

target_include_directories(az_iot_middleware_freertos
  PUBLIC
    ${AZ_IOT_REPO_ROOT}/source/include
    ${AZ_IOT_REPO_ROOT}/source/interface
)

# Embedded SDK
add_subdirectory(${AZ_IOT_REPO_ROOT}/libraries/azure-sdk-for-c azure-sdk-for-c)
target_link_libraries(az_iot_middleware_freertos PUBLIC az_iot_hub az_iot_provisioning)

# list of unit tests
set(unit_tests
  azure_iot_provisioning_client_ut
)

# initialization_unit_tests
add_executable(
  azure_iot_provisioning_client_ut
  ${PROJECT_SOURCE_DIR}/azure_iot_cmocka_mqtt.c
  ${PROJECT_SOURCE_DIR}/azure_iot_provisioning_client_ut.c
)

foreach(test ${unit_tests})
    target_sources(${test} PRIVATE ${PROJECT_SOURCE_DIR}/main.c)
    target_include_directories(${test}
        PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/config_files
    )
    target_compile_options(${test}
        PRIVATE
        -Werror
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
        -Wfloat-equal
    )
    target_link_libraries(${test} PRIVATE cmocka az_iot_middleware_freertos)
    target_link_options(${test} PRIVATE -Wl,--gc-sections)

    add_test(${test} ${test})
endforeach()
