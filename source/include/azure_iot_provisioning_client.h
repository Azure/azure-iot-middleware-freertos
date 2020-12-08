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
#define azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS             ( 4*60U )
#endif

#ifndef azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS
#define azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS               ( 1000U )
#endif

#define AZURE_IOT_PROVISIONING_C2D_MESSAGE                               (0x1)
#define AZURE_IOT_PROVISIONING_DIRECT_METHOD_MESSAGE                     (0x2)
#define AZURE_IOT_PROVISIONING_DEVICE_TWIN_MESSAGE                       (0x3)
#define AZURE_IOT_PROVISIONING_DESIRED_PROPERTY_MESSAGE                  (0x4)

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

    /* TODO: make this dynamic */
    uint8_t azure_iot_provisioning_client_last_response[1000];
    uint32_t azure_iot_provisioning_client_last_response_length;
    uint8_t azure_iot_provisioning_client_last_response_topic[128];
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
                                                                     uint8_t * pDeviceId, uint32_t* pDeviceIdLength);

#endif /* AZURE_IOT_PROVISIONING_CLIENT_H */