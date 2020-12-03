/**
 * @file azure_iot_hub_client.c
 * @brief -------.
 */

#include <stdio.h>

#include "azure_iot_hub_client.h"

#include "azure/az_iot.h"

static char mqtt_user_name[128];
static char telemetry_topic[128];

static void prvMQTTProcessIncomingPublish( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                           MQTTPublishInfo_t * pxPublishInfo )
{
    uint32_t index;
    configASSERT( pxPublishInfo != NULL );

    for( index = 0; index < sizeof(xAzureIoTHubClientHandle->xReceiveContext); index++ )
    {
        if ( xAzureIoTHubClientHandle->xReceiveContext[index].process_function != NULL &&
             ( xAzureIoTHubClientHandle->xReceiveContext[index].process_function( xAzureIoTHubClientHandle, pxPublishInfo ) == AZURE_IOT_HUB_CLIENT_SUCCESS ) )
        {
            break;
        }
    }
}

static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    (void )pxIncomingPacket;
    (void )usPacketId;
}

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTHubClientHandle_t  xAzureIoTHubClientHandle = (AzureIoTHubClientHandle_t)pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( xAzureIoTHubClientHandle, pxDeserializedInfo->pPublishInfo );
    }
    else
    {
        prvMQTTProcessResponse( pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
}

static uint32_t azure_iot_hub_client_cloud_message_process( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            MQTTPublishInfo_t * pxPublishInfo )
{
    AzureIotHubClientMessage_t message;

    az_iot_hub_client_c2d_request out_request;
    az_span topic_span = az_span_create((uint8_t * )pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength);
    if (az_result_failed(az_iot_hub_client_c2d_parse_received_topic(&xAzureIoTHubClientHandle->iot_hub_client_core, topic_span, &out_request)))
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    LogInfo( ( "C2D message QoS : %d\n", pxPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    LogInfo( ( "C2D message topic: %.*s  with payload : %.*s",
                   pxPublishInfo->topicNameLength,
                   pxPublishInfo->pTopicName,
                   pxPublishInfo->payloadLength,
                   pxPublishInfo->pPayload ) );

    if (xAzureIoTHubClientHandle->xReceiveContext[0].callback)
    {
        message.message_type = AZURE_IOT_HUB_C2D_MESSAGE;
        message.pxPublishInfo = pxPublishInfo;
        xAzureIoTHubClientHandle->xReceiveContext[0].callback( &message,
                                                               xAzureIoTHubClientHandle->xReceiveContext[0].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_direct_method_process( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            MQTTPublishInfo_t * pxPublishInfo )
{
    AzureIotHubClientMessage_t message;

    az_iot_hub_client_method_request out_request;
    az_span topic_span = az_span_create((uint8_t * )pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength);
    if (az_result_failed(az_iot_hub_client_methods_parse_received_topic(&xAzureIoTHubClientHandle->iot_hub_client_core, topic_span, &out_request)))
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    LogInfo( ( "Direct method QoS : %d\n", pxPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    LogInfo( ( "Direct method topic: %.*s  with method payload : %.*s",
                   pxPublishInfo->topicNameLength,
                   pxPublishInfo->pTopicName,
                   pxPublishInfo->payloadLength,
                   pxPublishInfo->pPayload ) );

    if (xAzureIoTHubClientHandle->xReceiveContext[1].callback)
    {
        message.message_type = AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE;
        message.pxPublishInfo = pxPublishInfo;
        xAzureIoTHubClientHandle->xReceiveContext[1].callback( &message,
                                                               xAzureIoTHubClientHandle->xReceiveContext[1].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_device_twin_process( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          MQTTPublishInfo_t * pxPublishInfo )
{
    AzureIotHubClientMessage_t message;

    az_iot_hub_client_twin_response out_request;
    az_span topic_span = az_span_create((uint8_t * )pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength);
    if (az_result_failed(az_iot_hub_client_twin_parse_received_topic(&xAzureIoTHubClientHandle->iot_hub_client_core, topic_span, &out_request)))
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    LogInfo( ( "Twin Incoming QoS : %d\n", pxPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    LogInfo( ( "Twin topic: %.*s. with payload : %.*s",
                   pxPublishInfo->topicNameLength,
                   pxPublishInfo->pTopicName,
                   pxPublishInfo->payloadLength,
                   pxPublishInfo->pPayload ) );

    if ( xAzureIoTHubClientHandle->xReceiveContext[2].callback )
    {
        message.message_type = out_request.response_type;
        message.pxPublishInfo = pxPublishInfo;
        xAzureIoTHubClientHandle->xReceiveContext[2].callback( &message,
                                                               xAzureIoTHubClientHandle->xReceiveContext[2].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

AzureIotHubClientError_t azure_iot_hub_client_init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    const char * pHostname, uint32_t hostnameLength,
                                                    const char * pDeviceId, uint32_t deviceIdLength,
                                                    const char * pModuleId, uint32_t moduleIdLength,
                                                    uint8_t * pBuffer, uint32_t bufferLength,
                                                    AzureIotGetCurrentTimeFunc_t getTimeFunction,
                                                    const TransportInterface_t * pTransportInterface )
{
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;
    MQTTFixedBuffer_t xBuffer = { pBuffer, bufferLength };
    az_iot_hub_client_options options;

    memset( ( void * )xAzureIoTHubClientHandle, 0, sizeof(AzureIoTHubClient_t));

    /* Initialize Azure IoT Hub Client */
    az_span hostname_span = az_span_create((uint8_t * )pHostname, hostnameLength);
    az_span device_id_span = az_span_create((uint8_t * )pDeviceId, deviceIdLength);
    options = az_iot_hub_client_options_default();
    options.module_id = az_span_create((uint8_t * )pModuleId, moduleIdLength);

    if ( az_result_failed( az_iot_hub_client_init( &xAzureIoTHubClientHandle->iot_hub_client_core,
                                                   hostname_span, device_id_span, &options ) ) )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    /* Initialize MQTT library. */
    else if ( (xResult = MQTT_Init( &(xAzureIoTHubClientHandle -> xMQTTContext), pTransportInterface,
                                    getTimeFunction, prvEventCallback,
                                    &xBuffer ) ) != MQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->deviceId = pDeviceId;
        xAzureIoTHubClientHandle->deviceIdLength = deviceIdLength;
        xAzureIoTHubClientHandle->hostname = pHostname;
        xAzureIoTHubClientHandle->hostnameLength = hostnameLength;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }
    
    return ret;
}


void azure_iot_hub_deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    (void)xAzureIoTHubClientHandle;
}


AzureIotHubClientError_t azure_iot_hub_client_connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                       bool cleanSession, TickType_t xTimeoutTicks )
{
    MQTTConnectInfo_t xConnectInfo;
    AzureIotHubClientError_t ret;
    MQTTStatus_t xResult;
    bool xSessionPresent;

    ( void ) memset( ( void * ) &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    xConnectInfo.cleanSession = cleanSession;

    size_t mqtt_user_name_length;
    if(az_result_failed(az_iot_hub_client_get_user_name(&xAzureIoTHubClientHandle->iot_hub_client_core, mqtt_user_name, sizeof(mqtt_user_name), &mqtt_user_name_length)))
    {
        return AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    xConnectInfo.pClientIdentifier = xAzureIoTHubClientHandle->deviceId;
    xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTHubClientHandle -> deviceIdLength;
    xConnectInfo.pUserName = mqtt_user_name;
    xConnectInfo.userNameLength = (uint16_t)mqtt_user_name_length;
    xConnectInfo.pPassword = NULL;
    xConnectInfo.passwordLength = 0;
    xConnectInfo.keepAliveSeconds = azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS;

    /* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
     * is passed as NULL. */
    if ( ( xResult = MQTT_Connect( &(xAzureIoTHubClientHandle -> xMQTTContext),
                                   &xConnectInfo,
                                   NULL,
                                   xTimeoutTicks,
                                   &xSessionPresent ) ) != MQTTSuccess )
    {
        printf( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%s \r\n",
                  xAzureIoTHubClientHandle->hostnameLength, xAzureIoTHubClientHandle->hostname,
                  MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        /* Successfully established and MQTT connection with the broker. */
        printf( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTHubClientHandle->hostnameLength,
                  xAzureIoTHubClientHandle->hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;

    if ( ( xResult = MQTT_Disconnect( &(xAzureIoTHubClientHandle -> xMQTTContext) ) ) != MQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        printf( ( "Disconnecting the MQTT connection with %.*s.\r\n", xAzureIoTHubClientHandle->hostnameLength,
                  xAzureIoTHubClientHandle->hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_telemetry_send( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                              const char * pTelemetryData, uint32_t telemetryDataLength)
{
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    size_t telemetry_topic_length;
    if(az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(&xAzureIoTHubClientHandle->iot_hub_client_core,
                        NULL, telemetry_topic, sizeof(telemetry_topic), &telemetry_topic_length)))

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = MQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = telemetry_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) telemetry_topic_length;
    xMQTTPublishInfo.pPayload = pTelemetryData;
    xMQTTPublishInfo.payloadLength = telemetryDataLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( &(xAzureIoTHubClientHandle -> xMQTTContext) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if ( ( xResult = MQTT_Publish( &(xAzureIoTHubClientHandle -> xMQTTContext),
                                   &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != MQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED; 
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_do_work( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                       uint32_t timeoutMs )
{
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;

    /* Process incoming UNSUBACK packet from the broker. */
    if ( ( xResult = MQTT_ProcessLoop( &(xAzureIoTHubClientHandle -> xMQTTContext), timeoutMs ) ) != MQTTSuccess )
    {
        printf( ( "Failed to receive UNSUBACK packet from broker: ProcessLoopDuration=%u, Error=%s\r\n", timeoutMs,
                  MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_cloud_message_enable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                    void ( * callback ) (AzureIotHubClientMessage_t* message, void* context),
                                                                    void * callback_context )
{
    MQTTSubscribeInfo_t * pMQTTSubscription = &xAzureIoTHubClientHandle->xReceiveContext[0].xMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription->qos = MQTTQoS1;
    pMQTTSubscription->pTopicFilter = AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
    pMQTTSubscription->topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId(&(xAzureIoTHubClientHandle->xMQTTContext));
    
    printf( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &(xAzureIoTHubClientHandle -> xMQTTContext),
                                     pMQTTSubscription, 1,  usSubscribePacketIdentifier) ) != MQTTSuccess )
    {
        printf( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s",
                  MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->xReceiveContext[0].process_function = azure_iot_hub_client_cloud_message_process;
        xAzureIoTHubClientHandle->xReceiveContext[0].callback = callback;
        xAzureIoTHubClientHandle->xReceiveContext[0].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_direct_method_enable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                    void ( * callback ) (AzureIotHubClientMessage_t* message, void* context),
                                                                    void * callback_context )
{   
    MQTTSubscribeInfo_t * pMQTTSubscription = &xAzureIoTHubClientHandle->xReceiveContext[1].xMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription->qos = MQTTQoS0;
    pMQTTSubscription->pTopicFilter = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
    pMQTTSubscription->topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId(&(xAzureIoTHubClientHandle->xMQTTContext));
    
    printf( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &(xAzureIoTHubClientHandle -> xMQTTContext),
                                     pMQTTSubscription, 1,  usSubscribePacketIdentifier) ) != MQTTSuccess )
    {
        printf( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s",
                  MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->xReceiveContext[1].process_function = azure_iot_hub_client_direct_method_process;
        xAzureIoTHubClientHandle->xReceiveContext[1].callback = callback;
        xAzureIoTHubClientHandle->xReceiveContext[1].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIotHubClientError_t azure_iot_hub_client_device_twin_enable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                  void ( * callback ) (AzureIotHubClientMessage_t* message, void* context),
                                                                  void * callback_context )
{
    MQTTSubscribeInfo_t * pMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIotHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription = &xAzureIoTHubClientHandle->xReceiveContext[2].xMQTTSubscription;
    pMQTTSubscription[0].qos = MQTTQoS0;
    pMQTTSubscription[0].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    pMQTTSubscription[0].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
    pMQTTSubscription[1].qos = MQTTQoS0;
    pMQTTSubscription[1].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    pMQTTSubscription[1].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId(&(xAzureIoTHubClientHandle->xMQTTContext));
    
    printf( ( "Attempt to subscribe to the MQTT  %s and %s topics.\r\n",
                AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &(xAzureIoTHubClientHandle -> xMQTTContext),
                                     pMQTTSubscription, 2,  usSubscribePacketIdentifier) ) != MQTTSuccess )
    {
        printf( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s \r\n",
                  MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->xReceiveContext[2].process_function = azure_iot_hub_client_device_twin_process;
        xAzureIoTHubClientHandle->xReceiveContext[2].callback = callback;
        xAzureIoTHubClientHandle->xReceiveContext[2].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

/*-----------------------------------------------------------*/