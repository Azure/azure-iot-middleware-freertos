# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

project(az_iot_middleware_freertos_e2e_tests)

if(NOT UNIX)
  message(FATAL_ERROR "E2E tests must be run on Linux")
endif()

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

include_directories(${CMAKE_CURRENT_LIST_DIR}/../config_files)# Include config

add_compile_options(-DprojCOVERAGE_TEST=0 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\")

if("${freertos_repo_SOURCE_DIR}" STREQUAL "")
  message(FATAL_ERROR "The E2E tests needs a FreeRTOS directory.")
endif()

include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/include)
include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/logging)
include_directories(${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)

# Add source files and libs
add_subdirectory(../../source source)

# Create FreeRTOS Lib
add_library(freertos
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/croutine.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/event_groups.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/list.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/queue.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/stream_buffer.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/tasks.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/timers.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/MemMang/heap_3.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(freertos
  PUBLIC
    ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/include
    ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
    ${freertos_repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
)

# Create FreeRTOS Plus Lib
add_library(freertos_plus
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos/mbedtls_error.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/using_mbedtls.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/sockets_wrapper.c
)

target_include_directories(freertos_plus
  PUBLIC
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/include
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/interface
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/include
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/include
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/logging
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${CMAKE_CURRENT_LIST_DIR}/config_files
)

target_link_libraries(freertos_plus
  PUBLIC
    freertos
    mbedtls
)

# Create mbedTLS Lib
add_library(mbedtls
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aes.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aesni.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/arc4.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aria.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1parse.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1write.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/base64.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/bignum.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/blowfish.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/camellia.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ccm.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/certs.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chacha20.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chachapoly.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher_wrap.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cmac.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ctr_drbg.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/debug.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/des.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/dhm.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdh.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdsa.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecjpake.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp_curves.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy_poll.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/gcm.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/havege.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hkdf.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hmac_drbg.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md2.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md4.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md5.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/memory_buffer_alloc.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/net_sockets.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/nist_kw.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/oid.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/padlock.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pem.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk_wrap.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs11.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs12.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs5.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkparse.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkwrite.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform_util.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/poly1305.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_driver_wrappers.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_se.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_storage.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_its_file.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ripemd160.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa_internal.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha1.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha256.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha512.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cache.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ciphersuites.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cli.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cookie.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_msg.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_srv.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ticket.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls13_keys.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/threading.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/timing.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version_features.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_create.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crl.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crt.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_csr.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_crt.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_csr.c
  ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/xtea.c
)

target_include_directories(mbedtls
  PUBLIC
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${freertos_repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
)

target_link_libraries(mbedtls
  PRIVATE
    freertos
)

# Add E2E Test Executable
add_executable(azure_iot_e2e_tests
  ${CMAKE_CURRENT_LIST_DIR}/device/mbedtls_freertos_port.c
  ${CMAKE_CURRENT_LIST_DIR}/device/e2e_device_commands.c
  ${CMAKE_CURRENT_LIST_DIR}/device/e2e_device_task.c
  ${CMAKE_CURRENT_LIST_DIR}/device/main.c
)

target_include_directories(azure_iot_e2e_tests
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/device
)

target_link_libraries(azure_iot_e2e_tests
  PRIVATE
    freertos
    freertos_plus
    coremqtt
    cmocka
    pcap
    pthread
    az::iot_middleware::freertos
)
