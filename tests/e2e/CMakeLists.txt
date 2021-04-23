
############################################# Add E2E Tests ################################################


set(FREERTOS_ROOT_PATH "${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS")
set(FREERTOS_PLUS_ROOT_PATH "${az_iot_middleware_freertos_SOURCE_DIR}/libraries/FreeRTOS/FreeRTOS-Plus")

include_directories(${CMAKE_CURRENT_LIST_DIR}/../config_files)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
add_compile_options(-m32 -ggdb3 -O0 -D_DEBUG -DprojCOVERAGE_TEST=0 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\" -D_WINDOWS_)
add_link_options(-ggdb3 -O0)

# Create FreeRTOS Lib

add_library(freertos
  ${FREERTOS_ROOT_PATH}/Source/croutine.c
  ${FREERTOS_ROOT_PATH}/Source/event_groups.c
  ${FREERTOS_ROOT_PATH}/Source/list.c
  ${FREERTOS_ROOT_PATH}/Source/queue.c
  ${FREERTOS_ROOT_PATH}/Source/stream_buffer.c
  ${FREERTOS_ROOT_PATH}/Source/tasks.c
  ${FREERTOS_ROOT_PATH}/Source/timers.c
  ${FREERTOS_ROOT_PATH}/Source/portable/MemMang/heap_3.c
  ${FREERTOS_ROOT_PATH}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
  ${FREERTOS_ROOT_PATH}/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(freertos
  PUBLIC
    ${FREERTOS_ROOT_PATH}/Source/include
    ${FREERTOS_ROOT_PATH}/Source/portable/ThirdParty/GCC/Posix
    ${FREERTOS_ROOT_PATH}/Source/portable/ThirdParty/GCC/Posix/utils
)

# Create FreeRTOS Plus Lib

add_library(freertos_plus
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/mbedtls_freertos/mbedtls_error.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/using_mbedtls.c
  ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/network_transport/freertos_plus_tcp/sockets_wrapper.c
)

target_include_directories(freertos_plus
  PUBLIC
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/coreMQTT/source/include
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/coreMQTT/source/interface
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/network_transport/freertos_plus_tcp
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls
    ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux
    ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/include
    ${FREERTOS_PLUS_ROOT_PATH}/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/backoff_algorithm/source/include
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/mbedtls_freertos
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/logging
    ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/include
    ${CMAKE_CURRENT_LIST_DIR}/config_files
)

target_link_libraries(freertos_plus
  PUBLIC
    freertos
    mbedtls
)

# Create mbedTLS Lib

add_library(mbedtls
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/aes.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/aesni.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/arc4.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/aria.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/asn1parse.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/asn1write.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/base64.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/bignum.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/blowfish.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/camellia.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ccm.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/certs.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/chacha20.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/chachapoly.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/cipher.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/cipher_wrap.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/cmac.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ctr_drbg.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/debug.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/des.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/dhm.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ecdh.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ecdsa.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ecjpake.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ecp.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ecp_curves.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/entropy.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/entropy_poll.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/error.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/gcm.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/havege.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/hkdf.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/hmac_drbg.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/md.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/md2.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/md4.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/md5.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/memory_buffer_alloc.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/net_sockets.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/nist_kw.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/oid.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/padlock.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pem.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pk.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pk_wrap.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pkcs11.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pkcs12.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pkcs5.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pkparse.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/pkwrite.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/platform.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/platform_util.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/poly1305.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_crypto.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_crypto_driver_wrappers.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_crypto_se.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_crypto_storage.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/psa_its_file.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ripemd160.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/rsa.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/rsa_internal.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/sha1.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/sha256.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/sha512.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_cache.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_ciphersuites.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_cli.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_cookie.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_msg.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_srv.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_ticket.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_tls.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/ssl_tls13_keys.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/threading.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/timing.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/version.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/version_features.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509_create.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509_crl.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509_crt.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509_csr.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509write_crt.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/x509write_csr.c
  ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/library/xtea.c
)

target_include_directories(mbedtls
  PUBLIC
    ${FREERTOS_PLUS_ROOT_PATH}/ThirdParty/mbedtls/include
    ${FREERTOS_PLUS_ROOT_PATH}/Source/Utilities/mbedtls_freertos
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