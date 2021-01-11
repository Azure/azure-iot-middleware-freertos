/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/**
 * @file azure_iot.h
 * 
 */

#ifndef AZURE_IOT_H
#define AZURE_IOT_H

#include "FreeRTOS.h"

#include "azure/az_iot.h"

typedef enum AzureIoTError
{
    AZURE_IOT_SUCCESS = 0,          ///< Success.
    AZURE_IOT_INVALID_ARGUMENT,     ///< Input argument does not comply with the expected range of values.
    AZURE_IOT_STATUS_OOM,           ///< The system is out of memory.
    AZURE_IOT_STATUS_ITEM_NOT_FOUND,///< The item was not found.
    AZURE_IOT_FAILED,               ///< There was a failure.
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

typedef uint64_t( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey, uint32_t keyLength,
                                             const uint8_t * pData, uint32_t dataLength,
                                             uint8_t * pOutput, uint32_t outputLength,
                                             uint32_t * pBytesCopied );

/**
 * @brief Initialize 
 * 
 * @param messageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param buffer The pointer to the buffer.
 * @param writtenLength The length of the properties already written (if applicable).
 * @param bufferLength The length of \p bufferLength.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTMessagePropertiesInit(AzureIoTMessageProperties_t* messageProperties,
                                              uint8_t* buffer, uint32_t writtenLength,
                                              uint32_t bufferLength);

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
AzureIoTError_t AzureIoTMessagePropertiesAppend(AzureIoTMessageProperties_t* messageProperties,
                                                uint8_t* pName, uint32_t nameLength,
                                                uint8_t* pValue, uint32_t valueLength);

/**
 * @brief Find a property in the message property bag.
 * 
 * @param messageProperties The #AzureIoTMessageProperties_t* to use for the operation.
 * @param pName The name of the property to find.
 * @param outValue The output pointer to the value.
 * @param outValueLength The length of \p outValue.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTMessagePropertiesFind(AzureIoTMessageProperties_t* messageProperties,
                                              uint8_t* pName, uint8_t** outValue,
                                              uint32_t* outValueLength);

/**
 * @brief As part of symmetric key authentication, HMAC256 a buffer of bytes and base64 encode the result.
 * 
 * @note This is used within Azure IoT Hub and Device Provisioning APIs should a symmetric key be set.
 * 
 * @param xAzureIoTHMACFunction The #AzureIoTGetHMACFunc_t function to use for HMAC256 hashing.
 * @param key_ptr A pointer to the base64 encoded key.
 * @param key_size The length of the \p key_ptr.
 * @param message_ptr A pointer to the message to be hashed.
 * @param message_size The length of \p message_ptr.
 * @param buffer_ptr An intermediary buffer to put the base64 decoded key.
 * @param buffer_len The length of \p buffer_ptr.
 * @param output_pptr The buffer into which the resulting HMAC256 hashed, base64 encoded message will
 * be placed.
 * @param output_len_ptr The output length of \p output_pptr.
 * @return An #AzureIoTError_t with the result of the operation.
 */
AzureIoTError_t AzureIoTBase64HMACCalculate( AzureIoTGetHMACFunc_t xAzureIoTHMACFunction,
                                             const uint8_t *key_ptr, uint32_t key_size,
                                             const uint8_t *message_ptr, uint32_t message_size,
                                             uint8_t *buffer_ptr, uint32_t buffer_len,
                                             uint8_t **output_pptr, uint32_t *output_len_ptr );

#endif /* AZURE_IOT_H */