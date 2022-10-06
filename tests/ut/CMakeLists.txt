# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.13)

project(az_iot_middleware_freertos_ut_tests)

if(NOT UNIX AND NOT ${CMAKE_COMPILER_IS_GNUCC})
  message(FATAL_ERROR "Unit tests must be run on Linux with GCC")
endif()

include(CTest)
enable_testing()

# CMocka test functions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/modules")
include(AddCMockaTest)

# Turn on the flags necessary for code coverage
add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g3)
add_link_options(-fprofile-arcs -ftest-coverage)
set(MOCK_LINKER_OPTIONS "-Wl,--gc-sections")

if("${FREERTOS_DIRECTORY}" STREQUAL "")
  message(FATAL_ERROR "The Unit tests needs a FreeRTOS directory.")
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../config_files)
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/include)
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/logging)
include_directories(${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix)

# Set the port for MQTT
set(AZURE_IOT_MQTT_PORT ${CMAKE_CURRENT_LIST_DIR})

# Add source files and libs
add_subdirectory(../../source source)

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
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    ../../source # To test private functions
)

add_cmocka_test(azure_iot_adu_client_ut
  SOURCES
    main.c
    azure_iot_adu_client_ut.c
    azure_iot_cmocka_mqtt.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
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
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_cmocka_test(azure_iot_hub_client_properties_ut
  SOURCES
    main.c
    azure_iot_hub_client_properties_ut.c
    azure_iot_cmocka_mqtt.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_cmocka_test(azure_iot_json_reader_ut
  SOURCES
    main.c
    azure_iot_json_reader_ut.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

add_cmocka_test(azure_iot_json_writer_ut
  SOURCES
    main.c
    azure_iot_json_writer_ut.c
  COMPILE_OPTIONS
    ${DEFAULT_C_COMPILE_FLAGS}
  LINK_LIBRARIES
    cmocka
    az::iot_middleware::freertos
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
  LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
  INCLUDE_DIRECTORIES
    ${CMOCKA_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
)

# The jws needs mbedtls and threading, which uses pthreads
if(LINUX)
    add_cmocka_test(azure_iot_jws_mbedtls_ut
      SOURCES
        main.c
        azure_iot_jws_mbedtls_ut.c
        azure_iot_cmocka_mqtt.c
        ${CMAKE_CURRENT_LIST_DIR}/mbedtls/mbedtls_freertos_port.c
        ${CMAKE_CURRENT_LIST_DIR}/../../ports/mbedTLS/azure_iot_jws_mbedtls.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/croutine.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/event_groups.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/list.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/queue.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/stream_buffer.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/tasks.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/timers.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_3.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/aes.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/aesni.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/arc4.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/aria.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1parse.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/asn1write.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/base64.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/bignum.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/blowfish.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/camellia.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ccm.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/certs.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/chacha20.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/chachapoly.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/cipher_wrap.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/cmac.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ctr_drbg.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/debug.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/des.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/dhm.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdh.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecdsa.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecjpake.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ecp_curves.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/entropy_poll.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/gcm.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/havege.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/hkdf.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/hmac_drbg.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/md.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/md2.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/md4.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/md5.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/memory_buffer_alloc.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/net_sockets.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/nist_kw.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/oid.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/padlock.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pem.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pk_wrap.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs11.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs12.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkcs5.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkparse.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/pkwrite.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/platform_util.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/poly1305.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_se.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_crypto_storage.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/psa_its_file.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ripemd160.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/rsa_internal.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha1.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha256.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/sha512.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cache.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ciphersuites.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cli.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_cookie.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_msg.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_srv.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_ticket.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/ssl_tls13_keys.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/threading.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/timing.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/version.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/version_features.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_create.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crl.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_crt.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509_csr.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_crt.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/x509write_csr.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/xtea.c
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
      COMPILE_OPTIONS
        ${DEFAULT_C_COMPILE_FLAGS}
        -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
      LINK_LIBRARIES
        cmocka
        pthread
        az::iot_middleware::freertos
      LINK_OPTIONS ${MOCK_LINKER_OPTIONS}
      INCLUDE_DIRECTORIES
        ${CMOCKA_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../config_files
        ${CMAKE_CURRENT_LIST_DIR}/mbedtls
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/include
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
        ${CMAKE_CURRENT_LIST_DIR}/../../libraries/FreeRTOS/FreeRTOS-Plus/ThirdParty/mbedtls/include
    )
endif()
