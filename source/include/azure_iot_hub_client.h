/**
 * @file azure_iot_hub_client.h
 * 
 */

#ifndef AZURE_IOT_HUB_CLIENT_H
#define AZURE_IOT_HUB_CLIENT_H

/* bool is defined in only C99+. */
#if defined( __cplusplus ) || ( defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901L ) )
    #include <stdbool.h>
#elif !defined( bool ) && !defined( false ) && !defined( true )
    #define bool     int8_t
    #define false    ( int8_t ) 0
    #define true     ( int8_t ) 1
#endif
/** @endcond */

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure/az_iot.h"

#include "core_mqtt.h"
#include "logging_stack.h"
#include "transport_interface.h"

#ifndef azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS
#define azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS             ( 60U )
#endif

#ifndef azureIoTHubClientCONNACK_RECV_TIMEOUT_MS
#define azureIoTHubClientCONNACK_RECV_TIMEOUT_MS               ( 1000U )
#endif

#define AZURE_IOT_HUB_C2D_MESSAGE                               (0x1)
#define AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE                     (0x2)
#define AZURE_IOT_HUB_DEVICE_TWIN_MESSAGE                       (0x3)
#define AZURE_IOT_HUB_DESIRED_PROPERTY_MESSAGE                  (0x4)

typedef struct AzureIoTHubClient* AzureIoTHubClientHandle_t;

typedef enum AzureIotHubClientError
{
    AZURE_IOT_HUB_CLIENT_SUCCESS = 0,
    AZURE_IOT_HUB_CLIENT_STATUS_PENDING,
    AZURE_IOT_HUB_CLIENT_INIT_FAILED,
    AZURE_IOT_HUB_CLIENT_FAILED,
} AzureIotHubClientError_t;

/*
 *  This holds the message type (IE telemetry, methods, etc) and
 *  the publish info which holds payload/length, the topic/length, QOS, dup, etc.
*/
typedef struct AzureIotHubClientMessage
{
    uint8_t message_type;
    MQTTPublishInfo_t * pxPublishInfo;
} AzureIotHubClientMessage_t;

/*
 *  The subscription has the details of the topic filter, length, and QOS.
 *  The process function is the function to call for a specific topic.
*/
typedef struct AzureIotHubClientReceiveContext
{
    MQTTSubscribeInfo_t xMQTTSubscription;
    uint32_t( * process_function)( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                   MQTTPublishInfo_t * pxPublishInfo );
} AzureIotHubClientReceiveContext_t;

typedef struct AzureIoTHubClient
{
    MQTTContext_t xMQTTContext;

    az_iot_hub_client iot_hub_client_core;

    const char * hostname;
    uint32_t hostnameLength;
    const char *deviceId;
    uint32_t deviceIdLength;

    AzureIotHubClientReceiveContext_t xReceiveContext[4];

    void* app_callback_context;
    void (*app_callback) (AzureIotHubClientMessage_t* message, void* context);
} AzureIoTHubClient_t;

AzureIotHubClientError_t azure_iot_hub_client_init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    const char * pHostname, uint32_t hostnameLength,
                                                    const char * pDeviceId, uint32_t deviceIdLength,
                                                    uint8_t * pBuffer, uint32_t bufferLength,
                                                    const TransportInterface_t * pTransportInterface );


void azure_iot_hub_deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );


AzureIotHubClientError_t azure_iot_hub_client_connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                       bool cleanSession, TickType_t xTimeoutTicks );

AzureIotHubClientError_t azure_iot_hub_client_disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

AzureIotHubClientError_t azure_iot_hub_client_telemetry_send( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                              const char * pTelemetryData, uint32_t telemetryDataLength );

AzureIotHubClientError_t azure_iot_hub_client_do_work( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                       uint32_t timeoutMs );

AzureIotHubClientError_t azure_iot_hub_client_cloud_message_enable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

AzureIotHubClientError_t azure_iot_hub_client_direct_method_enable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

AzureIotHubClientError_t azure_iot_hub_client_device_twin_enable(AzureIoTHubClientHandle_t xAzureIoTHubClientHandle);

AzureIotHubClientError_t azure_iot_hub_client_receive_callback_set( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                    void ( *callback ) (AzureIotHubClientMessage_t* message, void * context),
                                                                    void * context );

#endif /* AZURE_IOT_HUB_CLIENT_H */