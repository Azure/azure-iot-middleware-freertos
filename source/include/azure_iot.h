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
    AZURE_IOT_FAILED,               ///< There was a failure.
} AzureIoTError_t;

typedef uint64_t( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey, uint32_t keyLength,
                                             const uint8_t * pData, uint32_t dataLength,
                                             uint8_t * pOutput, uint32_t outputLength,
                                             uint32_t * pBytesCopied );

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