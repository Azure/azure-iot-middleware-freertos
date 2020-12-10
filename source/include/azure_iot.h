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
    AZURE_IOT_SUCCESS = 0,
    AZURE_IOT_INVALID_ARGUMENT,
    AZURE_IOT_STATUS_OOM,
    AZURE_IOT_FAILED,
} AzureIoTError_t;

typedef uint64_t( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey, uint32_t keyLength,
                                             const uint8_t * pData, uint32_t dataLength,
                                             uint8_t * pOutput, uint32_t outputLength,
                                             uint32_t * pBytesCopied );


AzureIoTError_t AzureIoTBase64HMACCalculate( AzureIoTGetHMACFunc_t xAzureIoTHMACFunction,
                                             const uint8_t *key_ptr, uint32_t key_size,
                                             const uint8_t *message_ptr, uint32_t message_size,
                                             uint8_t *buffer_ptr, uint32_t buffer_len,
                                             uint8_t **output_pptr, uint32_t *output_len_ptr );

#endif /* AZURE_IOT_H */