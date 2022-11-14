# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 99)

# Add modules and include compiler options/switches
include(../cmake/build/compile_options.cmake)

# Add coreMQTT Lib
add_library(coremqtt
  ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT/source/core_mqtt_serializer.c
  ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT/source/core_mqtt_state.c
  ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT/source/core_mqtt.c
)

target_include_directories(coremqtt
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT/source/include
    ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT/source/interface
)

if(${USE_COREHTTP})
  # Add coreHTTP Lib
  add_library(corehttp
    ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreHTTP/source/core_http_client.c
    ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreHTTP/source/dependency/3rdparty/http_parser/http_parser.c
  )

  target_include_directories(corehttp
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreHTTP/source/dependency/3rdparty/http_parser
      ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreHTTP/source/include
      ${CMAKE_CURRENT_LIST_DIR}/../libraries/coreHTTP/source/interface
  )
endif()

# Add embedded SDK lib
add_subdirectory(../libraries/azure-sdk-for-c azure-sdk-for-c)

# Azure IoT FreeRTOS middleware Library
add_library(az_iot_middleware_freertos
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_adu_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_hub_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_provisioning_client.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_hub_client_properties.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_json_reader.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_json_writer.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot.c
  ${CMAKE_CURRENT_LIST_DIR}/azure_iot_message.c
)

target_link_libraries(az_iot_middleware_freertos
  PUBLIC
    az::iot::common
    az::iot::hub
    az::iot::provisioning
)

target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/interface
)

if(${USE_COREHTTP})
  add_library(azure_iot_core_http
      ${CMAKE_CURRENT_LIST_DIR}/../ports/coreHTTP/azure_iot_core_http.c
  )

  target_include_directories(azure_iot_core_http
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/interface
      ${CMAKE_CURRENT_LIST_DIR}/../ports/coreHTTP
  )

  target_link_libraries(azure_iot_core_http
    PUBLIC
      az_iot_middleware_freertos
      corehttp
  )

  add_library(az::iot_middleware::core_http ALIAS azure_iot_core_http)
endif()

# Check if custom mqtt port path is set, otherwise
# use default coreMQTT port
if(NOT( "${AZURE_IOT_MQTT_PORT}" STREQUAL "" ))
  target_include_directories(az_iot_middleware_freertos
    PRIVATE
      ${AZURE_IOT_MQTT_PORT}
  )
else()
  # Include coreMQTT port
  target_sources(az_iot_middleware_freertos
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/../ports/coreMQTT/azure_iot_core_mqtt.c
  )

  target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${CMAKE_CURRENT_LIST_DIR}/../ports/coreMQTT
  )

  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      coremqtt
  )

  if(${USE_COREHTTP})
    target_link_libraries(az_iot_middleware_freertos
      PUBLIC
        corehttp
    )
  endif()

  target_compile_definitions(az_iot_middleware_freertos
    PRIVATE
      MQTT_DO_NOT_USE_CUSTOM_CONFIG
  )
endif()

az_add_compile_options(az_iot_middleware_freertos)

add_library(az::iot_middleware::freertos ALIAS az_iot_middleware_freertos)
