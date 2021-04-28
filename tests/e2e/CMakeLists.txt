# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

include_directories(${CMAKE_CURRENT_LIST_DIR}/../config_files)

add_compile_options(-DprojCOVERAGE_TEST=0 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\")

# Create FreeRTOS Lib

add_library(freertos
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/croutine.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/event_groups.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/list.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/queue.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/stream_buffer.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/tasks.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/timers.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/portable/MemMang/heap_3.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(freertos
  PUBLIC
    ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/include
    ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
    ${freertos-repo_SOURCE_DIR}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
)

# Create FreeRTOS Plus Lib

add_library(freertos_plus
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos/mbedtls_error.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/using_mbedtls.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/sockets_wrapper.c
)

target_include_directories(freertos_plus
  PUBLIC
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/include
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/coreMQTT/source/interface
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/include
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/backoff_algorithm/source/include
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/logging
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${CMAKE_CURRENT_LIST_DIR}/config_files
)

target_link_libraries(freertos_plus
  PUBLIC
    freertos
    mbedtls
)

# Create mbedTLS Lib

add_library(mbedtls
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aes.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aesni.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/arc4.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/aria.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1parse.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1write.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/base64.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/bignum.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/blowfish.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/camellia.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ccm.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/certs.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chacha20.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/chachapoly.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher_wrap.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/cmac.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ctr_drbg.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/debug.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/des.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/dhm.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdh.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdsa.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecjpake.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp_curves.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy_poll.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/gcm.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/havege.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hkdf.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/hmac_drbg.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md2.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md4.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/md5.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/memory_buffer_alloc.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/net_sockets.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/nist_kw.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/oid.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/padlock.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pem.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk_wrap.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs11.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs12.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs5.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkparse.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkwrite.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform_util.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/poly1305.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_driver_wrappers.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_se.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_storage.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_its_file.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ripemd160.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa_internal.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha1.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha256.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha512.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cache.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ciphersuites.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cli.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cookie.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_msg.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_srv.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ticket.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls13_keys.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/threading.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/timing.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/version_features.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_create.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crl.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crt.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_csr.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_crt.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_csr.c
  ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/library/xtea.c
)

target_include_directories(mbedtls
  PUBLIC
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    ${freertos-repo_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
)

target_link_libraries(mbedtls
  PRIVATE
    freertos
)

# Add coreMQTT Lib

add_library(coremqtt
  ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/core_mqtt_serializer.c
  ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/core_mqtt_state.c
  ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/core_mqtt.c
)

target_include_directories(coremqtt
  PUBLIC
    ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/include
    ${az_iot_middleware_freertos_SOURCE_DIR}/libraries/coreMQTT/source/interface
)

target_link_libraries(coremqtt
  PRIVATE
    freertos_plus
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
    middleware_config
    freertos
    freertos_plus
    coremqtt
    cmocka
    pcap
    pthread
    az::iot_middleware::freertos
)
