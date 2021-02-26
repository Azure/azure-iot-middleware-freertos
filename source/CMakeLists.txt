# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 99)

# Azure IoT FreeRTOS Middleware Library
add_library(az_iot_middleware_freertos
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_hub_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_provisioning_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot.c
)

# Check if custom mqtt port path is set, otherwise
# use default coreMQTT port
if(AZURE_IOT_MQTT_PORT)
  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${AZURE_IOT_MQTT_PORT})
else()
  # Include coreMQTT port
  target_sources(az_iot_middleware_freertos
    PRIVATE
      ${az_iot_middleware_freertos_SOURCE_DIR}/port/coreMQTT/azure_iot_core_mqtt.c)

  target_compile_definitions(az_iot_middleware_freertos
    PRIVATE
      MQTT_DO_NOT_USE_CUSTOM_CONFIG)

  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${az_iot_middleware_freertos_SOURCE_DIR}/port/coreMQTT
      ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/include
      ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/interface)
endif()

target_include_directories(az_iot_middleware_freertos
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/interface
)

add_library(az_iot_middleware_freertos ALIAS az::iot_middleware::freertos)

target_compile_options(
    az_iot_middleware_freertos
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
