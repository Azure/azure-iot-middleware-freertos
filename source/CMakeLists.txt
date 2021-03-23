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

# There needs to be some kind of find_library here for FreeRTOS so if someone already has it
# in a high project, it can be consumed here.

# find_library(afr_freertos AFR::freertos)

if(TARGET AFR::freertos)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      AFR::freertos
  )
else()
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

  target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS/Source/include
  )
endif()

if(TARGET AFR::core_mqtt)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      AFR::core_mqtt
  )
endif()

if(TARGET AFR::platform)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      AFR::platform
  )
endif()

if(TARGET AFR::backoff_algorithm)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      AFR::backoff_algorithm
  )
endif()

if(TARGET 3rdparty::mbedtls)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
    3rdparty::mbedtls
  )
endif()

if(TARGET AFR::freertos_plus_tcp)
  target_link_libraries(az_iot_middleware_freertos
    PUBLIC
      AFR::freertos_plus_tcp
  )
endif()

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
      ${az_iot_middleware_freertos_SOURCE_DIR}/port/coreMQTT/azure_iot_core_mqtt.c
  )

  target_include_directories(az_iot_middleware_freertos
    PUBLIC
      ${az_iot_middleware_freertos_SOURCE_DIR}/port/coreMQTT
)

  target_compile_definitions(az_iot_middleware_freertos
    PRIVATE
      MQTT_DO_NOT_USE_CUSTOM_CONFIG
  )
endif()

add_library(az::iot_middleware::freertos ALIAS az_iot_middleware_freertos)

# target_compile_options(
#     az_iot_middleware_freertos
#       PRIVATE -Werror
#               -Wall
#               -Wextra
#               -pedantic
#               -fmessage-length=0
#               -fsigned-char
#               -ffunction-sections
#               -fdata-sections
#               -Wunused
#               -Wuninitialized
#               -Wmissing-declarations
#               -Wconversion
#               -Wpointer-arith
#               -Wshadow
#               -Wlogical-op
#               -Wfloat-equal )
