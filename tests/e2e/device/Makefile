CC := gcc
BIN := e2e_device_exe

BUILD_DIR := build

FREERTOS_DIR_REL := ../../../libraries/FreeRTOS/FreeRTOS
FREERTOS_DIR := $(abspath $(FREERTOS_DIR_REL))

FREERTOS_PLUS_DIR_REL := ../../../libraries/FreeRTOS/FreeRTOS-Plus
FREERTOS_PLUS_DIR := $(abspath $(FREERTOS_PLUS_DIR_REL))

AZURE_IOT_DIR_REL := ../../../
AZURE_IOT_DIR := $(abspath $(AZURE_IOT_DIR_REL))

INCLUDE_DIRS := -I.
INCLUDE_DIRS += -I${FREERTOS_DIR}/Source/include
INCLUDE_DIRS += -I${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix
INCLUDE_DIRS += -I${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/include
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/interface
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/network_transport/freertos_plus_tcp/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/include/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/portable/Compiler/GCC/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/backoff_algorithm/source/include/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/mbedtls_freertos/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/logging/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/include
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/source/include
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/source/interface
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/port/coreMQTT
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/inc
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/tests/e2e/device/
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/tests/e2e/device/config

SOURCE_FILES += $(wildcard ${FREERTOS_DIR}/Source/*.c)
SOURCE_FILES += ${AZURE_IOT_DIR}/port/coreMQTT/azure_iot_core_mqtt.c
# Memory manager (use malloc() / free() )
SOURCE_FILES += ${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c
# posix port
SOURCE_FILES += ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
SOURCE_FILES += ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c

# FreeRTOS TCP
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DNS.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_DHCP.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_ARP.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_WIN.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Stream_Buffer.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/portable/BufferManagement/BufferAllocation_2.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_IP.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_TCP_IP.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_UDP_IP.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/FreeRTOS_Sockets.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/portable/NetworkInterface/linux/NetworkInterface.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Utilities/mbedtls_freertos/mbedtls_error.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/network_transport/freertos_plus_tcp/using_mbedtls/using_mbedtls.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/network_transport/freertos_plus_tcp/sockets_wrapper.c

# mbedtls library.
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/aes.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/aesni.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/arc4.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/aria.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/asn1parse.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/asn1write.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/base64.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/bignum.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/blowfish.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/camellia.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ccm.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/certs.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/chacha20.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/chachapoly.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/cipher.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/cipher_wrap.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/cmac.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ctr_drbg.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/debug.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/des.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/dhm.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ecdh.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ecdsa.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ecjpake.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ecp.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ecp_curves.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/entropy.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/entropy_poll.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/error.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/gcm.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/havege.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/hkdf.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/hmac_drbg.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/md.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/md2.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/md4.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/md5.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/memory_buffer_alloc.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/net_sockets.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/nist_kw.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/oid.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/padlock.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pem.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pk.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pk_wrap.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pkcs11.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pkcs12.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pkcs5.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pkparse.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/pkwrite.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/platform.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/platform_util.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/poly1305.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_crypto.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_crypto_driver_wrappers.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_crypto_se.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_crypto_slot_management.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_crypto_storage.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/psa_its_file.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ripemd160.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/rsa.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/rsa_internal.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/sha1.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/sha256.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/sha512.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_cache.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_ciphersuites.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_cli.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_cookie.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_msg.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_srv.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_ticket.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_tls.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/ssl_tls13_keys.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/threading.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/timing.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/version.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/version_features.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509_create.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509_crl.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509_crt.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509_csr.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509write_crt.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/x509write_csr.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/library/xtea.c

# Azure IoT SDK library.
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_json_reader.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_json_token.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_json_writer.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_log.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_precondition.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_span.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/core/az_context.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_common.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_c2d.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_methods.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_sas.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_telemetry.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_hub_client_twin.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_provisioning_client.c
SOURCE_FILES += ${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/src/azure/iot/az_iot_provisioning_client_sas.c
SOURCE_FILES += ${AZURE_IOT_DIR}/source/azure_iot_hub_client.c
SOURCE_FILES += ${AZURE_IOT_DIR}/source/azure_iot_provisioning_client.c
SOURCE_FILES += ${AZURE_IOT_DIR}/source/azure_iot.c

# Demo library.
SOURCE_FILES += ${AZURE_IOT_DIR}/tests/e2e/device/e2e_device_task.c
SOURCE_FILES += ${AZURE_IOT_DIR}/tests/e2e/device/e2e_device_commands.c
SOURCE_FILES += ${AZURE_IOT_DIR}/tests/e2e/device/main.c
SOURCE_FILES += ${AZURE_IOT_DIR}/tests/e2e/device/mbedtls_freertos_port.c

CFLAGS := -m32 -ggdb3 -O0 -D_DEBUG -DprojCOVERAGE_TEST=0 -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\" -D_WINDOWS_
LDFLAGS := -ggdb3 -O0 -pthread -lpcap -lcmocka

OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

DEP_FILE = $(OBJ_FILES:%.o=%.d)

${BIN} : $(BUILD_DIR)/$(BIN)

${BUILD_DIR}/${BIN} : ${OBJ_FILES}
	-mkdir -p ${@D}
	$(CC) $^ $(CFLAGS) $(INCLUDE_DIRS) ${LDFLAGS} -o $@


-include ${DEP_FILE}

${BUILD_DIR}/%.o : %.c
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) ${INCLUDE_DIRS} -MMD -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)







