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
#define e2etestRETRY_MAX_ATTEMPTS                      ( 5U )
#define e2etestRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )
#define e2etestRETRY_BACKOFF_BASE_MS                   ( 500U )
#define e2etestCONNACK_RECV_TIMEOUT_MS                 ( 1000U )
#define e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )
#define e2etestProvisioning_Registration_TIMEOUT_MS    ( 10U )

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

uint32_t ulE2EDeviceProcessCommands( AzureIoTHubClient_t * xAzureIoTHubClientHandle );
void vHandleCloudMessage( AzureIoTHubClientCloudMessageRequest_t * message,
                          void * context );
void vHandleDirectMethod( AzureIoTHubClientMethodRequest_t * message,
                          void * context );
void vHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * message,
                               void * context );
TlsTransportStatus_t xConnectToServerWithBackoffRetries( const char * pHostName,
                                                         uint32_t port,
                                                         NetworkCredentials_t * pxNetworkCredentials,
                                                         NetworkContext_t * pxNetworkContext );
uint32_t ulCalculateHMAC( const uint8_t * pKey,
                          uint32_t keyLength,
                          const uint8_t * pData,
                          uint32_t dataLength,
                          uint8_t * pOutput,
                          uint32_t outputLength,
                          uint32_t * pBytesCopied );
uint64_t ulGetUnixTime( void );

#ifdef __cplusplus
    }
#endif

#endif /* E2E_DEVICE_PROCESS_COMMANDS_H */
