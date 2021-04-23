CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
BIN := sample_azure_iot_embedded_sdk

BUILD_DIR := build

FREERTOS_DIR_REL := ../../../libraries/FreeRTOS/FreeRTOS
FREERTOS_DIR := $(abspath $(FREERTOS_DIR_REL))

FREERTOS_PLUS_DIR_REL := ../../../libraries/FreeRTOS/FreeRTOS-Plus
FREERTOS_PLUS_DIR := $(abspath $(FREERTOS_PLUS_DIR_REL))

BSP_DIR_REL := ../../Common/stm32l4_l4+/driver/stm32l4_l4+_discovery
BSP_DIR := $(abspath $(BSP_DIR_REL))

AZURE_IOT_DIR_REL := ../../../
AZURE_IOT_DIR := $(abspath $(AZURE_IOT_DIR_REL))

INCLUDE_DIRS := -I.
INCLUDE_DIRS += -I${FREERTOS_DIR}/Source/include
INCLUDE_DIRS += -I${FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/include
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/interface
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/backoff_algorithm/source/include/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/mbedtls_freertos/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Source/Utilities/logging/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/ThirdParty/mbedtls/include
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/source/include
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/source/interface
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/ports/coreMQTT
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/libraries/azure-sdk-for-c/sdk/inc
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/demo/sample_azure_iot_embedded_sdk/common
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+

# BSP includes
INCLUDE_DIRS += -I${BSP_DIR}/BSP/B-L475E-IOT01
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/Common
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/es_wifi
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/hts221
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/lis3mdl
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/lps22hb
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/lsm6dsl
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/mx25r6435f
INCLUDE_DIRS += -I${BSP_DIR}/BSP/Components/vl53l0x
INCLUDE_DIRS += -I${BSP_DIR}/CMSIS/Include
INCLUDE_DIRS += -I${BSP_DIR}/CMSIS/Device/ST/STM32L4xx/Include

# ST HAL
INCLUDE_DIRS += -I${BSP_DIR}/STM32L4xx_HAL_Driver/Inc
INCLUDE_DIRS += -I${BSP_DIR}/STM32L4xx_HAL_Driver/Inc/Legacy

# BSP app specific includes
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/st_code

# Transport
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/transport
INCLUDE_DIRS += -I${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/transport/using_mbedtls

SOURCE_FILES := $(wildcard *.c)
SOURCE_FILES += $(wildcard ${FREERTOS_DIR}/Source/*.c)
SOURCE_FILES += ${AZURE_IOT_DIR}/ports/coreMQTT/azure_iot_core_mqtt.c
# Memory manager (use malloc() / free() )
SOURCE_FILES += ${FREERTOS_DIR}/Source/portable/MemMang/heap_5.c

# FreeRTOS port
SOURCE_FILES += ${FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F/port.c

# FreeRTOS TCP
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt_serializer.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt_state.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Application-Protocols/coreMQTT/source/core_mqtt.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Utilities/backoff_algorithm/source/backoff_algorithm.c
SOURCE_FILES += ${FREERTOS_PLUS_DIR}/Source/Utilities/mbedtls_freertos/mbedtls_error.c

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

# Transport port
SOURCE_FILES += ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/transport/using_mbedtls/using_mbedtls.c
SOURCE_FILES += ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/transport/sockets_wrapper.c

# ST code
SOURCE_FILES += $(wildcard ${BSP_DIR}/BSP/B-L475E-IOT01/*.c)
SOURCE_FILES += $(wildcard ${BSP_DIR}/BSP/Components/Common/*.c)
SOURCE_FILES += $(wildcard ${BSP_DIR}/BSP/Components/es_wifi/*.c)
SOURCE_FILES += $(wildcard ${BSP_DIR}/STM32L4xx_HAL_Driver/*.c)
SOURCE_FILES += $(wildcard ${BSP_DIR}/CMSIS/*.c)
SOURCE_FILES += $(wildcard ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/st_code/*.c)
ASSEMBLER_FILES += $(wildcard ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/st_code/*.s)

# Demo library.
SOURCE_FILES += ${AZURE_IOT_DIR}/demo/sample_azure_iot_embedded_sdk/DemoTasks/sample_azure_iot_embedded_sdk.c
SOURCE_FILES += ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/main.c
SOURCE_FILES += ${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/mbedtls_freertos_port.c

MCPU_FLAGS := -mthumb -mcpu=cortex-m4
VFP_FLAGS := 
SPEC_FLAGS := --specs=nosys.specs
ST_APP_FLAGS := -DUSE_HAL_DRIVER -DSTM32L475xx -DENABLE_IOT_INFO -DENABLE_IOT_ERROR -DSENSOR -DRFU -DUSE_OFFLOAD_SSL -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"
CFLAGS := $(MCPU_FLAGS) $(VFP_FLAGS) -std=gnu11 -g3 -O0 -ffunction-sections -Wall -fstack-usage  -mfpu=fpv4-sp-d16 -mfloat-abi=hard $(ST_APP_FLAGS)
LDFLAGS := $(MCPU_FLAGS) $(SPEC_FLAGS) -T"${AZURE_IOT_DIR}/demo/Common/stm32l4_l4+/STM32L475VGTx_FLASH.ld" -Wl,-Map="${BUILD_DIR}/${BIN}.map" -Wl,--gc-sections -static -z muldefs  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wl,--start-group -lc -lm -Wl,--end-group

OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

AS_FILES = $(ASSEMBLER_FILES:%.s=$(BUILD_DIR)/%.o)

DEP_FILE = $(OBJ_FILES:%.o=%.d)

${BIN} : $(BUILD_DIR)/$(BIN)

${BUILD_DIR}/${BIN} : ${OBJ_FILES} ${AS_FILES}
	-mkdir -p ${@D}
	$(CC) $^ $(CFLAGS) $(INCLUDE_DIRS) ${LDFLAGS} -o $@.elf
	$(OBJCOPY) -O binary $@.elf $@.bin

-include ${DEP_FILE}

${BUILD_DIR}/%.o : %.c
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) ${INCLUDE_DIRS} -MMD -c $< -o $@

${BUILD_DIR}/%.o : %.s
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)







