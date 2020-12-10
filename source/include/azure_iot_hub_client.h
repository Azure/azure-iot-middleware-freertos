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
 * @file azure_iot_hub_client.h
 * 
 */

#ifndef AZURE_IOT_HUB_CLIENT_H
#define AZURE_IOT_HUB_CLIENT_H

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure_iot.h"

#include "core_mqtt.h"
#include "logging_stack.h"
#include "transport_interface.h"

#ifndef azureIoTHUBDEFAULTTOKENTIMEOUTINSEC
#define azureIoTHUBDEFAULTTOKENTIMEOUTINSEC                     ( 60 * 60U )
#endif

#ifndef azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS
#define azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS             ( 60U )
#endif

#ifndef azureIoTHubClientCONNACK_RECV_TIMEOUT_MS
#define azureIoTHubClientCONNACK_RECV_TIMEOUT_MS                ( 1000U )
#endif

#define AZURE_IOT_HUB_C2D_MESSAGE                               ( 0x1 )
#define AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE                     ( 0x2 )
#define AZURE_IOT_HUB_DEVICE_TWIN_MESSAGE                       ( 0x3 )
#define AZURE_IOT_HUB_DESIRED_PROPERTY_MESSAGE                  ( 0x4 )

typedef struct AzureIoTHubClient * AzureIoTHubClientHandle_t;

typedef enum AzureIoTHubClientError
{
    AZURE_IOT_HUB_CLIENT_SUCCESS = 0,
    AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT,
    AZURE_IOT_HUB_CLIENT_STATUS_PENDING,
    AZURE_IOT_HUB_CLIENT_STATUS_OOM,
    AZURE_IOT_HUB_CLIENT_INIT_FAILED,
    AZURE_IOT_HUB_CLIENT_FAILED,
} AzureIoTHubClientError_t;

typedef uint64_t( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey, uint32_t keyLength,
                                             const uint8_t * pData, uint32_t dataLength,
                                             uint8_t * pOutput, uint32_t outputLength,
                                             uint32_t * pBytesCopied );

/*
 *  This holds the message type (IE telemetry, methods, etc) and
 *  the publish info which holds payload/length, the topic/length, QOS, dup, etc.
*/
typedef struct AzureIoTHubClientMessage
{
    uint8_t message_type;
    MQTTPublishInfo_t * pxPublishInfo;
    union
    {
        az_iot_hub_client_c2d_request c2d_request;
        az_iot_hub_client_method_request method_request;
        az_iot_hub_client_twin_response twin_response;
    } parsed_message;
} AzureIoTHubClientMessage_t;

/*
 *  The subscription has the details of the topic filter, length, and QOS.
 *  The process function is the function to call for a specific topic.
*/
typedef struct AzureIoTHubClientReceiveContext
{
    MQTTSubscribeInfo_t xMQTTSubscription;
    uint32_t ( * process_function )( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                     MQTTPublishInfo_t * pxPublishInfo );

    void * callback_context;
    void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context );
} AzureIoTHubClientReceiveContext_t;

typedef struct AzureIoTHubClient
{
    MQTTContext_t xMQTTContext;

    az_iot_hub_client iot_hub_client_core;

    const uint8_t * hostname;
    uint32_t hostnameLength;
    const uint8_t *deviceId;
    uint32_t deviceIdLength;
    const uint8_t * azure_iot_hub_client_symmetric_key;
    uint32_t azure_iot_hub_client_symmetric_key_length;

    uint32_t ( * azure_iot_hub_client_token_refresh )( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                                       uint64_t expiryTimeSecs, const uint8_t * key, uint32_t keyLen,
                                                       uint8_t * pSASBuffer, uint32_t sasBufferLen, uint32_t * pSaSLength );
    AzureIoTGetHMACFunc_t azure_iot_hub_client_hmac_function;
    AzureIoTGetCurrentTimeFunc_t azure_iot_hub_client_time_function;
    
    AzureIoTHubClientReceiveContext_t xReceiveContext[3];
} AzureIoTHubClient_t;

AzureIoTHubClientError_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                 const uint8_t * pHostname, uint32_t hostnameLength,
                                                 const uint8_t * pDeviceId, uint32_t deviceIdLength,
                                                 const uint8_t * pModuleId, uint32_t moduleIdLength,
                                                 uint8_t * pBuffer, uint32_t bufferLength,
                                                 AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                 const TransportInterface_t * pTransportInterface );


void AzureIoTHubClient_Deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

AzureIoTHubClientError_t AzureIoTHubClient_SymmetricKeySet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            const uint8_t * pSymmetricKey, uint32_t pSymmetricKeyLength, 
                                                            AzureIoTGetHMACFunc_t hmacFunction );

AzureIoTHubClientError_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    bool cleanSession, TickType_t xTimeoutTicks );

AzureIoTHubClientError_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

AzureIoTHubClientError_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          const char * pTelemetryData, uint32_t telemetryDataLength );

AzureIoTHubClientError_t AzureIoTHubClient_DoWork( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                   uint32_t timeoutMs );

AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context );

AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context );

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                             void * callback_context );

AzureIoTHubClientError_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMessage_t * message, uint32_t status,
                                                               const char * payload, uint32_t payloadLength );

#endif /* AZURE_IOT_HUB_CLIENT_H */