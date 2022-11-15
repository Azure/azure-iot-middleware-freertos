# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.13)

project(az_iot_middleware_freertos_e2e_tests)

if(NOT UNIX)
  message(FATAL_ERROR "E2E tests must be run on Linux")
endif()

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../config_files)# Include config

add_compile_options(-DprojCOVERAGE_TEST=0 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\")

if("${FREERTOS_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "The E2E tests needs a FreeRTOS directory.")
endif()

include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/include)
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/logging)
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)

# Add source files and libs
add_subdirectory(../../../source source)

# Create FreeRTOS Lib
add_library(freertos
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/croutine.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/event_groups.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/list.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/queue.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/stream_buffer.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/tasks.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/timers.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/MemMang/heap_3.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
  ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(freertos
  PUBLIC
    ${FREERTOS_DIRECTORY}/FreeRTOS/Source/include
    ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
    ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
)

# Create FreeRTOS Plus Lib
add_library(freertos_plus
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos/mbedtls_error.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/using_mbedtls.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/sockets_wrapper.c
)

target_include_directories(freertos_plus
  PUBLIC
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/include
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/interface
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/include
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/include
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/logging
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${CMAKE_CURRENT_LIST_DIR}/config_files
)

target_link_libraries(freertos_plus
  PUBLIC
    freertos
    mbedtls
)

# Create mbedTLS Lib
add_library(mbedtls
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aes.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aesni.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/arc4.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aria.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1parse.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1write.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/base64.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/bignum.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/blowfish.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/camellia.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ccm.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/certs.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chacha20.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chachapoly.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher_wrap.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cmac.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ctr_drbg.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/debug.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/des.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/dhm.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdh.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdsa.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecjpake.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp_curves.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy_poll.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/gcm.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/havege.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hkdf.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hmac_drbg.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md2.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md4.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md5.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/memory_buffer_alloc.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/net_sockets.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/nist_kw.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/oid.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/padlock.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pem.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk_wrap.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs11.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs12.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs5.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkparse.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkwrite.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform_util.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/poly1305.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_driver_wrappers.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_se.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_storage.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_its_file.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ripemd160.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa_internal.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha1.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha256.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha512.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cache.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ciphersuites.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cli.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cookie.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_msg.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_srv.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ticket.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls13_keys.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/threading.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/timing.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version_features.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_create.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crl.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crt.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_csr.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_crt.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_csr.c
  ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/xtea.c
)

target_include_directories(mbedtls
  PUBLIC
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
)

target_link_libraries(mbedtls
  PRIVATE
    freertos
)

# Add E2E Test Executable
add_executable(azure_iot_e2e_adu_tests
  ${CMAKE_CURRENT_LIST_DIR}/../../../ports/mbedTLS/azure_iot_jws_mbedtls.c
  ${CMAKE_CURRENT_LIST_DIR}/device/mbedtls_freertos_port.c
  ${CMAKE_CURRENT_LIST_DIR}/device/e2e_device_commands.c
  ${CMAKE_CURRENT_LIST_DIR}/device/e2e_device_task.c
  ${CMAKE_CURRENT_LIST_DIR}/device/main.c
)

target_include_directories(azure_iot_e2e_adu_tests
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/device
)

target_link_libraries(azure_iot_e2e_adu_tests
  PRIVATE
    freertos
    freertos_plus
    coremqtt
    cmocka
    pcap
    pthread
    az::iot_middleware::freertos
    azure_iot_core_http
)

target_link_options(azure_iot_e2e_adu_tests
  PRIVATE
    -Xlinker -Map=azure_iot_e2e_adu_tests.map)
