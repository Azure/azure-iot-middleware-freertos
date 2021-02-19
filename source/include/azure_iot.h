/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot.h
 *
 */

#ifndef AZURE_IOT_H
#define AZURE_IOT_H

/* AZURE_IOT_DO_NOT_USE_CUSTOM_CONFIG allows building the azure iot library
 * without a custom config. If a custom config is provided, the
 * AZURE_IOT_DO_NOT_USE_CUSTOM_CONFIG macro should not be defined. */
#ifndef AZURE_IOT_DO_NOT_USE_CUSTOM_CONFIG
    /* Include custom config file before other headers. */
    #include "azure_iot_config.h"
#endif

/* Include config defaults header to get default values of configs not
 * defined in azure_iot_mqtt_config.h file. */
#include "azure_iot_config_defaults.h"

#include "FreeRTOS.h"

#include "azure/az_iot.h"

typedef enum AzureIoTError
{
    AZURE_IOT_SUCCESS = 0,           /*/< Success. */
    AZURE_IOT_INVALID_ARGUMENT,      /*/< Input argument does not comply with the expected range of values. */
    AZURE_IOT_STATUS_OOM,            /*/< The system is out of memory. */
    AZURE_IOT_STATUS_ITEM_NOT_FOUND, /*/< The item was not found. */
    AZURE_IOT_FAILED,                /*/< There was a failure. */
} AzureIoTError_t;

/**
 * @brief The bag of properties associated with a message.
 *
 */
typedef struct AzureIoTHubClientMessageProperties
{
    struct
    {
        az_iot_message_properties properties;
    } _internal;
} AzureIoTMessageProperties_t;

typedef uint64_t ( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t ( * AzureIoTGetHMACFunc_t )( const uint8_t * pucKey,
                                              uint32_t ulKeyLength,
                                              const uint8_t * pucData,
                                              uint32_t ulDataLength,
                                              uint8_t * pucOutput,
                                              uint32_t ulOutputLength,
                                              uint32_t * pucBytesCopied );

/**
 * @brief Initialize Azure IoT
 *
 */
AzureIoTError_t AzureIoT_Init();

/**
 * @brief Initialize
 *
 * @param pxMessageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pucBuffer The pointer to the buffer.
 * @param ulWrittenLength The length of the properties already written (if applicable).
 * @param ulBufferLength The length of \p pucBuffer.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoT_MessagePropertiesInit( AzureIoTMessageProperties_t * pxMessageProperties,
                                                uint8_t * pucBuffer,
                                                uint32_t ulWrittenLength,
                                                uint32_t ulBufferLength );

/**
 * @brief Append a property name and value
 *
 * @param pxMessageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pucName The name of the property to append.
 * @param ulNameLength The length of \p pucName.
 * @param pucValue The value of the property to append.
 * @param ulValueLength The length of \p pucValue.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoT_MessagePropertiesAppend( AzureIoTMessageProperties_t * pxMessageProperties,
                                                  uint8_t * pucName,
                                                  uint32_t ulNameLength,
                                                  uint8_t * pucValue,
                                                  uint32_t ulValueLength );

/**
 * @brief Find a property in the message property bag.
 *
 * @param pxMessageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pucName The name of the property to find.
 * @param nameLength Length of the property name.
 * @param ppucOutValue The output pointer to the value.
 * @param pulOutValueLength The length of \p ppucOutValue.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoT_MessagePropertiesFind( AzureIoTMessageProperties_t * pxMessageProperties,
                                                uint8_t * pucName,
                                                uint32_t ulNameLength,
                                                uint8_t ** ppucOutValue,
                                                uint32_t * pulOutValueLength );

/**
 * @brief As part of symmetric key authentication, HMAC256 a buffer of bytes and base64 encode the result.
 *
 * @note This is used within Azure IoT Hub and Device Provisioning APIs should a symmetric key be set.
 *
 * @param xAzureIoTHMACFunction The #AzureIoTGetHMACFunc_t function to use for HMAC256 hashing.
 * @param pucKey A pointer to the base64 encoded key.
 * @param ulKeySize The length of the \p pucKey.
 * @param pucMessage A pointer to the message to be hashed.
 * @param ulMessageSize The length of \p pucMessage.
 * @param pucBuffer An intermediary buffer to put the base64 decoded key.
 * @param ulBufferLength The length of \p pucBuffer.
 * @param ppucOutput The buffer into which the resulting HMAC256 hashed, base64 encoded message will
 * be placed.
 * @param pulOutputLength The output length of \p ppucOutput.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoT_Base64HMACCalculate( AzureIoTGetHMACFunc_t xAzureIoTHMACFunction,
                                              const uint8_t * pucKey,
                                              uint32_t ulKeySize,
                                              const uint8_t * pucMessage,
                                              uint32_t ulMessageSize,
                                              uint8_t * pucBuffer,
                                              uint32_t ulBufferLength,
                                              uint8_t ** ppucOutput,
                                              uint32_t * pulOutputLength );

#endif /* AZURE_IOT_H */
