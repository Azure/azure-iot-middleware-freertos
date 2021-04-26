# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 99)

# Azure IoT FreeRTOS Middleware Library
add_library(az_iot_middleware_freertos
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_hub_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_provisioning_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot.c
)

# The user needs to supply this
target_link_libraries(az_iot_middleware_freertos
  PUBLIC
    middleware_config)

target_link_libraries(az_iot_middleware_freertos
  PUBLIC
    az::iot::common
    az::iot::hub
    az::iot::provisioning
)

# Include port files
# TODO: Have the user select the port path to use
if(WIN32)
  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/portable/MSVC-MingW
  )
  elseif(UNIX)
  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
  )
endif()

# Include FreeRTOS headers
target_include_directories(az_iot_middleware_freertos
  PUBLIC
    ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/include
)

target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/interface
)

# Check if custom mqtt port path is set, otherwise
# use default coreMQTT port
if(AZURE_IOT_MQTT_PORT)
  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${AZURE_IOT_MQTT_PORT}
  )

  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      middleware_config
  )
else()
  # Include coreMQTT port
  target_sources(az_iot_middleware_freertos
    PRIVATE
      ${az_iot_middleware_freertos_SOURCE_DIR}/ports/coreMQTT/azure_iot_core_mqtt.c
  )

  target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${az_iot_middleware_freertos_SOURCE_DIR}/ports/coreMQTT
  )

  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      coremqtt
)

  target_compile_definitions(az_iot_middleware_freertos
    PRIVATE
      MQTT_DO_NOT_USE_CUSTOM_CONFIG
  )
endif()

add_library(az::iot_middleware::freertos ALIAS az_iot_middleware_freertos)
