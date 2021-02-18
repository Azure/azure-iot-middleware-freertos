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

typedef uint32_t ( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey,
                                              uint32_t keyLength,
                                              const uint8_t * pData,
                                              uint32_t dataLength,
                                              uint8_t * pOutput,
                                              uint32_t outputLength,
                                              uint32_t * pBytesCopied );

/**
 * @brief Initialize Azure IoT
 *
 */
AzureIoTError_t AzureIoTInit();

/**
 * @brief Initialize
 *
 * @param messageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pBuffer The pointer to the buffer.
 * @param writtenLength The length of the properties already written (if applicable).
 * @param bufferLength The length of \p bufferLength.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTMessagePropertiesInit( AzureIoTMessageProperties_t * messageProperties,
                                               uint8_t * pBuffer,
                                               uint32_t writtenLength,
                                               uint32_t bufferLength );

/**
 * @brief Append a property name and value
 *
 * @param messageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pName The name of the property to append.
 * @param nameLength The length of \p pName.
 * @param pValue The value of the property to append.
 * @param valueLength The length of \p pValue.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTMessagePropertiesAppend( AzureIoTMessageProperties_t * messageProperties,
                                                 uint8_t * pName,
                                                 uint32_t nameLength,
                                                 uint8_t * pValue,
                                                 uint32_t valueLength );

/**
 * @brief Find a property in the message property bag.
 *
 * @param messageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pName The name of the property to find.
 * @param nameLength Length of the property name.
 * @param pOutValue The output pointer to the value.
 * @param pOutValueLength The length of \p outValue.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTMessagePropertiesFind( AzureIoTMessageProperties_t * messageProperties,
                                               uint8_t * pName,
                                               uint32_t nameLength,
                                               uint8_t ** pOutValue,
                                               uint32_t * pOutValueLength );

/**
 * @brief As part of symmetric key authentication, HMAC256 a buffer of bytes and base64 encode the result.
 *
 * @note This is used within Azure IoT Hub and Device Provisioning APIs should a symmetric key be set.
 *
 * @param xAzureIoTHMACFunction The #AzureIoTGetHMACFunc_t function to use for HMAC256 hashing.
 * @param pKey A pointer to the base64 encoded key.
 * @param keySize The length of the \p pKey.
 * @param pMessage A pointer to the message to be hashed.
 * @param messageSize The length of \p pMessage.
 * @param pBuffer An intermediary buffer to put the base64 decoded key.
 * @param bufferLength The length of \p pBuffer.
 * @param pOutput The buffer into which the resulting HMAC256 hashed, base64 encoded message will
 * be placed.
 * @param pOutputLength The output length of \p pOutput.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTBase64HMACCalculate( AzureIoTGetHMACFunc_t xAzureIoTHMACFunction,
                                             const uint8_t * pKey,
                                             uint32_t keySize,
                                             const uint8_t * pMessage,
                                             uint32_t messageSize,
                                             uint8_t * pBuffer,
                                             uint32_t bufferLength,
                                             uint8_t ** pOutput,
                                             uint32_t * pOutputLength );

#endif /* AZURE_IOT_H */
