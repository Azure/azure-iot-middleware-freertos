# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

# FreeRTOS Kernel include
include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/include)

if(WIN32)
  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/MSVC-MingW)
elseif(UNIX)
  include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)
endif()

include(AddCMockaTest)

# Add UT

add_cmocka_test(azure_iot_ut
  SOURCES
    main.c
    azure_iot_ut.c
    azure_iot_cmocka_mqtt.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
    middleware_config
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_cmocka_test(azure_iot_hub_client_ut
  SOURCES
    main.c
    azure_iot_hub_client_ut.c
    azure_iot_cmocka_mqtt.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
    middleware_config
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_cmocka_test(azure_iot_provisioning_client_ut
  SOURCES
    main.c
    azure_iot_provisioning_client_ut.c
    azure_iot_cmocka_mqtt.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
    middleware_config
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)
