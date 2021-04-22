/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef E2E_DEVICE_PROCESS_COMMANDS_H
    #define E2E_DEVICE_PROCESS_COMMANDS_H

    #ifdef __cplusplus
        extern   "C" {
    #endif

    #include <stdio.h>
    #include "azure_iot_hub_client.h"
    #include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
    #include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
    #include "using_mbedtls.h"
/*-----------------------------------------------------------*/

    #define e2etestRETRY_MAX_ATTEMPTS                      ( 5U )
    #define e2etestRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )
    #define e2etestRETRY_BACKOFF_BASE_MS                   ( 500U )
    #define e2etestCONNACK_RECV_TIMEOUT_MS                 ( 1000U )
    #define e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )
    #define e2etestProvisioning_Registration_TIMEOUT_MS    ( 10U )
/*-----------------------------------------------------------*/

/**
 * Network context shared with Transport
 *
 */
    struct NetworkContext
    {
        TlsTransportParams_t * pParams;
    };

/**
 * Process all the test command send to device.
 *
 */
    uint32_t ulE2EDeviceProcessCommands( AzureIoTHubClient_t * xAzureIoTHubClientHandle );

/**
 * Callback use for Cloud messages
 *
 **/
    void vHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                              void * pvContext );

/**
 * Callback use for Direct method messages
 *
 **/
    void vHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
                              void * pvContext );

/**
 * Callback use for Twin messages
 *
 **/
    void vHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * pxMessage,
                                   void * pvContext );

/**
 * Connect with retries to hostname
 *
 **/
    TlsTransportStatus_t xConnectToServerWithBackoffRetries( const char * pcHostName,
                                                             uint32_t ulPort,
                                                             NetworkCredentials_t * pxNetworkCredentials,
                                                             NetworkContext_t * pxNetworkContext );

/**
 * Calculate HMAC
 *
 **/
    uint32_t ulCalculateHMAC( const uint8_t * pucKey,
                              uint32_t ulKeyLength,
                              const uint8_t * pucData,
                              uint32_t ulDataLength,
                              uint8_t * pucOutput,
                              uint32_t ulOutputLength,
                              uint32_t * pulBytesCopied );

/**
 * Get unix time
 *
 **/
    uint64_t ulGetUnixTime( void );

    #ifdef __cplusplus
        }
    #endif

#endif /* E2E_DEVICE_PROCESS_COMMANDS_H */
