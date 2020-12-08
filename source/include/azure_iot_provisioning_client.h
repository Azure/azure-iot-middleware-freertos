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
 * @file azure_iot_provisioning_client.h
 * 
 */

#ifndef AZURE_IOT_PROVISIONING_CLIENT_H
#define AZURE_IOT_PROVISIONING_CLIENT_H

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure/az_iot.h"

#include "core_mqtt.h"
#include "logging_stack.h"
#include "transport_interface.h"

#ifndef azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS
#define azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS                ( 4 * 60U )
#endif

#ifndef azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS
#define azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS                   ( 1000U )
#endif

typedef struct AzureIoTProvisioningClient * AzureIoTProvisioningClientHandle_t;

typedef enum AzureIoTProvisioningClientError
{
    AZURE_IOT_PROVISIONING_CLIENT_SUCCESS = 0,
    AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT,
    AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING,
    AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED,
    AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR,
    AZURE_IOT_PROVISIONING_CLIENT_FAILED,
} AzureIoTProvisioningClientError_t;

typedef uint32_t( * AzureIoTGetCurrentTimeFunc_t)(void);

typedef struct AzureIoTProvisioningClient
{
    MQTTContext_t xMQTTContext;

    uint8_t * azure_iot_provisioning_client_last_response_payload;
    uint32_t azure_iot_provisioning_client_last_response_payload_length;
    uint8_t * azure_iot_provisioning_client_last_response_topic;
    uint32_t azure_iot_provisioning_client_last_response_topic_length;

    az_iot_provisioning_client_register_response register_response;
    
    az_iot_provisioning_client iot_dps_client_core;

    const char * pRegistrationId;
    uint32_t registrationIdLength;
    const char * pEndpoint;
    uint32_t endpointLength;
    const char * pIdScope;
    uint32_t idScopeLength;

    uint32_t workflowState;
    uint32_t lastOperationResult;
    uint32_t azure_iot_provisioning_client_req_timeout;
    AzureIoTGetCurrentTimeFunc_t getTimeFunction;
} AzureIoTProvisioningClient_t;

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTDPSClientHandle,
                                                                   const char * pEndpoint, uint32_t endpointLength,
                                                                   const char * pIdScope, uint32_t idScopeLength,
                                                                   const char * pRegistrationId, uint32_t registrationIdLength,
                                                                   uint8_t * pBuffer, uint32_t bufferLength,
                                                                   AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                                   const TransportInterface_t * pTransportInterface );


void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTDPSClientHandle );

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTDPSClientHandle );

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTDPSClientHandle,
                                                                     uint8_t * pHubHostname, uint32_t * pHostnameLength,
                                                                     uint8_t * pDeviceId, uint32_t * pDeviceIdLength );

#endif /* AZURE_IOT_PROVISIONING_CLIENT_H */