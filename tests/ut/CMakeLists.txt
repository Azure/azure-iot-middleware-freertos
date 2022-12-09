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
if(UNIX)
    add_cmocka_test(azure_iot_jws_mbedtls_ut
      SOURCES
        main.c
        azure_iot_jws_mbedtls_ut.c
        azure_iot_cmocka_mqtt.c
        ${CMAKE_CURRENT_LIST_DIR}/mbedtls/mbedtls_freertos_port.c
        ${CMAKE_CURRENT_LIST_DIR}/../../ports/mbedTLS/azure_iot_jws_mbedtls.c
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
        ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
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
        ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/library/error.c
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
        ${CMAKE_CURRENT_LIST_DIR}/mbedtls
        ${CMAKE_CURRENT_LIST_DIR}/../config_files
        ${FREERTOS_DIRECTORY}/FreeRTOS/Source/include
        ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix
        ${FREERTOS_DIRECTORY}/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
        ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/Source/Utilities/mbedtls_freertos
        ${FREERTOS_DIRECTORY}/FreeRTOS-Plus/ThirdParty/mbedtls/include
    )
endif()
