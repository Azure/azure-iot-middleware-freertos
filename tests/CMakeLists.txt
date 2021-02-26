# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project (az_iot_middleware_freertos_ut LANGUAGES C)

enable_testing()

# Use custom dummy MQTT port
set(AZURE_IOT_MQTT_PORT ${PROJECT_SOURCE_DIR})

# FreeRTOS Kernel include
include_directories("${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/include")

if(WIN32)
  include_directories("${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/portable/MSVC-MingW")
elseif(UNIX)
  include_directories("${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix")
endif()

# Unit test include
include_directories("${PROJECT_SOURCE_DIR}/config_files")
include_directories("${PROJECT_SOURCE_DIR}")

include(AddCMockaTest)

add_cmocka_test(azure_iot_hub_client_ut
  SOURCES
    main.c
    azure_iot_hub_client_ut.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
    ${MOCK_COMPILE_OPTIONS}
  LINK_LIBRARIES
    ${CMOCKA_LIBRARIES}
    az::iot_middleware::freertos
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
)

add_cmocka_test(azure_iot_provisioning_client_ut
  SOURCES
    main.c
    azure_iot_provisioning_client_ut.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
    ${MOCK_COMPILE_OPTIONS}
  LINK_LIBRARIES
    ${CMOCKA_LIBRARIES}
    az::iot_middleware::freertos
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
)

# list of unit tests
# set(unit_tests
#   azure_iot_hub_client_ut
#   azure_iot_provisioning_client_ut)

# foreach(test ${unit_tests})
#   target_link_options(
#     ${test}
#       PRIVATE
#         -Wl,--gc-sections)
#   add_test(${test} ${test})
# endforeach()
