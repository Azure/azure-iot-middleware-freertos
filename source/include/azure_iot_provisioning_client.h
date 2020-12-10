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

#include "azure_iot.h"

#include "core_mqtt.h"
#include "logging_stack.h"
#include "transport_interface.h"

#ifndef azureIoTPROVISIONINGDEFAULTTOKENTIMEOUTINSEC
#define azureIoTPROVISIONINGDEFAULTTOKENTIMEOUTINSEC                        ( 60 * 60U )
#endif

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
    AZURE_IOT_PROVISIONING_CLIENT_STATUS_OOM,
    AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED,
    AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR,
    AZURE_IOT_PROVISIONING_CLIENT_FAILED,
} AzureIoTProvisioningClientError_t;

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
    uint64_t azure_iot_provisioning_client_req_timeout;
    AzureIoTGetCurrentTimeFunc_t getTimeFunction;

    const uint8_t * azure_iot_provisioning_client_symmetric_key;
    uint32_t azure_iot_provisioning_client_symmetric_key_length;

    uint32_t ( * azure_iot_provisioning_client_token_refresh )( struct AzureIoTProvisioningClient * xAzureIoTProvisioningClientHandle,
                                                                uint64_t expiryTimeSecs, const uint8_t * key, uint32_t keyLen,
                                                                uint8_t * pSASBuffer, uint32_t sasBufferLen, uint32_t * pSaSLength );
    AzureIoTGetHMACFunc_t azure_iot_provisioning_client_hmac_function;
} AzureIoTProvisioningClient_t;

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                   const char * pEndpoint, uint32_t endpointLength,
                                                                   const char * pIdScope, uint32_t idScopeLength,
                                                                   const char * pRegistrationId, uint32_t registrationIdLength,
                                                                   uint8_t * pBuffer, uint32_t bufferLength,
                                                                   AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                                   const TransportInterface_t * pTransportInterface );


void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle );

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_SymmetricKeySet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                              const uint8_t * pSymmetricKey, uint32_t pSymmetricKeyLength,
                                                                              AzureIoTGetHMACFunc_t hmacFunction );

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle );

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                     uint8_t * pHubHostname, uint32_t * pHostnameLength,
                                                                     uint8_t * pDeviceId, uint32_t * pDeviceIdLength );

#endif /* AZURE_IOT_PROVISIONING_CLIENT_H */