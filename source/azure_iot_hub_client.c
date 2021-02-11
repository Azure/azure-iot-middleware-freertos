/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client.c
 * @brief -------.
 */

#include "azure_iot_hub_client.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "azure/az_iot.h"

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND          ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK            ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/*
 * Indexes of the receive context buffer for each feature
 */
#define RECEIVE_CONTEXT_INDEX_C2D        0
#define RECEIVE_CONTEXT_INDEX_METHODS    1
#define RECEIVE_CONTEXT_INDEX_TWIN       2

/*
 * Topic subscribe state
 */
#define TOPIC_SUBSCRIBE_STATE_NONE       0
#define TOPIC_SUBSCRIBE_STATE_SUB        1
#define TOPIC_SUBSCRIBE_STATE_SUBACK     2

static char mqtt_user_name[ 128 ];
static char mqtt_password[ 256 ];
static char telemetry_topic[ 128 ];
static char method_topic[ 128 ];
static char device_twin_reported_topic[ 128 ];
static char device_twin_get_topic[ 128 ];

static void prvMQTTProcessIncomingPublish( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                           AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    uint32_t index;

    configASSERT( pxPublishInfo != NULL );
    AzureIoTHubClientReceiveContext_t * context;

    for( index = 0; index < ( sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext ) / sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext[ 0 ] ) ); index++ )
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ index ];

        if( ( context->_internal.process_function != NULL ) &&
            ( context->_internal.process_function( context,
                                                   xAzureIoTHubClientHandle,
                                                   ( void * ) pxPublishInfo ) == AZURE_IOT_HUB_CLIENT_SUCCESS ) )
        {
            break;
        }
    }
}

static void prvMQTTProcessResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                    AzureIoTMQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    uint32_t index;
    AzureIoTHubClientReceiveContext_t * context;

    if( ( pxIncomingPacket->type & 0xF0U ) == AZURE_IOT_MQTT_PACKET_TYPE_SUBACK )
    {
        for( index = 0; index < ( sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext ) / sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext[ 0 ] ) ); index++ )
        {
            context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ index ];

            if( context->_internal.mqttSubPacketId == usPacketId )
            {
                /* TODO: inspect packet to see is ack was successful*/
                context->_internal.state = TOPIC_SUBSCRIBE_STATE_SUBACK;
                break;
            }
        }
    }
}

static void prvEventCallback( AzureIoTMqttHandle_t pxMQTTContext,
                              AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                              AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTHubClientHandle_t xAzureIoTHubClientHandle = ( AzureIoTHubClientHandle_t ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( xAzureIoTHubClientHandle, pxDeserializedInfo->pPublishInfo );
    }
    else
    {
        prvMQTTProcessResponse( xAzureIoTHubClientHandle, pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
}

static uint32_t azure_iot_hub_client_cloud_message_process( AzureIoTHubClientReceiveContext_t * context,
                                                            AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            void * pxPublishInfo )
{
    AzureIoTHubClientC2DRequest_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_c2d_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );

    if( az_result_failed( az_iot_hub_client_c2d_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                      topic_span, &out_request ) ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "C2D message QoS : %d\n", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "C2D message topic: %.*s  with payload : %.*s",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.c2dCallback )
    {
        message.messagePayload = mqttPublishInfo->pPayload;
        message.payloadLength = mqttPublishInfo->payloadLength;
        message.properties._internal.properties = out_request.properties;
        context->_internal.callbacks.c2dCallback( &message,
                                                  context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_direct_method_process( AzureIoTHubClientReceiveContext_t * context,
                                                            AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            void * pxPublishInfo )
{
    AzureIoTHubClientMethodRequest_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_method_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );

    if( az_result_failed( az_iot_hub_client_methods_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                          topic_span, &out_request ) ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Direct method QoS : %d\n", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Direct method topic: %.*s  with method payload : %.*s",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.methodCallback )
    {
        message.messagePayload = mqttPublishInfo->pPayload;
        message.payloadLength = mqttPublishInfo->payloadLength;
        message.methodName = az_span_ptr( out_request.name );
        message.methodNameLength = az_span_size( out_request.name );
        message.requestId = az_span_ptr( out_request.request_id );
        message.requestIdLength = az_span_size( out_request.request_id );
        context->_internal.callbacks.methodCallback( &message, context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_device_twin_process( AzureIoTHubClientReceiveContext_t * context,
                                                          AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          void * pxPublishInfo )
{
    AzureIoTHubClientTwinResponse_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_twin_response out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );
    uint32_t request_id = 0;

    if( az_result_failed( az_iot_hub_client_twin_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                       topic_span, &out_request ) ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Twin Incoming QoS : %d\n", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Twin topic: %.*s. with payload : %.*s",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.twinCallback )
    {
        if( az_span_size( out_request.request_id ) == 0 )
        {
            message.messageType = AZURE_IOT_HUB_TWIN_DESIRED_PROPERTY_MESSAGE;
        }
        else
        {
            if( az_result_failed( az_span_atou32( out_request.request_id, &request_id ) ) )
            {
                return AZURE_IOT_HUB_CLIENT_FAILED;
            }

            if( request_id & 0x01 )
            {
                message.messageType = AZURE_IOT_HUB_TWIN_REPORTED_RESPONSE_MESSAGE;
            }
            else
            {
                message.messageType = AZURE_IOT_HUB_TWIN_GET_MESSAGE;
            }
        }

        message.messagePayload = mqttPublishInfo->pPayload;
        message.payloadLength = mqttPublishInfo->payloadLength;
        message.messageStatus = out_request.status;
        message.version = az_span_ptr( out_request.version );
        message.versionLength = az_span_size( out_request.version );
        message.requestId = request_id;
        context->_internal.callbacks.twinCallback( &message,
                                                   context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_token_get( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                                uint64_t expiryTimeSecs,
                                                const uint8_t * pKey,
                                                uint32_t keyLen,
                                                uint8_t * pSASBuffer,
                                                uint32_t sasBufferLen,
                                                uint32_t * pSaSLength )
{
    uint8_t buffer[ 512 ];
    az_span span = az_span_create( pSASBuffer, sasBufferLen );
    uint8_t * pOutput;
    uint32_t outputLen;
    az_result core_result;
    uint32_t bytesUsed;
    size_t length;

    core_result = az_iot_hub_client_sas_get_signature( &( xAzureIoTHubClientHandle->_internal.iot_hub_client_core ),
                                                       expiryTimeSecs, span, &span );

    if( az_result_failed( core_result ) )
    {
        AZLogError( ( "IoTHub failed failed to get signature with error status: %d", core_result ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    bytesUsed = ( uint32_t ) az_span_size( span );

    if( AzureIoTBase64HMACCalculate( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function,
                                     pKey, keyLen, pSASBuffer, bytesUsed,
                                     buffer, sizeof( buffer ), &pOutput, &outputLen ) )
    {
        AZLogError( ( "IoTHub failed to encoded hash" ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    span = az_span_create( pOutput, ( int32_t ) outputLen );
    core_result = az_iot_hub_client_sas_get_password( &( xAzureIoTHubClientHandle->_internal.iot_hub_client_core ),
                                                      expiryTimeSecs, span, AZ_SPAN_EMPTY,
                                                      ( char * ) pSASBuffer, sasBufferLen, &length );

    if( az_result_failed( core_result ) )
    {
        AZLogError( ( "IoTHub failed to generate token with error status: %d", core_result ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    *pSaSLength = (uint32_t)length;

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    return ulTimeMs;
}

static int prv_request_id_get( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                               az_span in_span,
                               bool odd_seq,
                               uint32_t * pRequestId,
                               az_span * out_span )
{
    if( odd_seq == ( bool ) ( xAzureIoTHubClientHandle->_internal.currentRequestId & 0x01 ) )
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId += 2;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId += 1;
    }

    if( xAzureIoTHubClientHandle->_internal.currentRequestId == 0 )
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId = 2;
    }

    az_span remainder;
    az_result res = az_span_u32toa( in_span, xAzureIoTHubClientHandle->_internal.currentRequestId, &remainder );

    if( az_result_failed( res ) )
    {
        AZLogError( ( "Couldn't convert request id to span" ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    *out_span = az_span_slice( *out_span, 0, az_span_size( *out_span ) - az_span_size( remainder ) );

    if( pRequestId )
    {
        *pRequestId = xAzureIoTHubClientHandle->_internal.currentRequestId;
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static AzureIoTHubClientError_t waitforSubAck( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                               AzureIoTHubClientReceiveContext_t * context,
                                               uint32_t timeout )
{
    AzureIoTHubClientError_t ret = AZURE_IOT_HUB_CLIENT_SUBACK_WAIT_TIMEOUT;
    uint32_t waitTime;

    do
    {
        if( context->_internal.state == TOPIC_SUBSCRIBE_STATE_SUBACK )
        {
            ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            break;
        }

        if( timeout > azureIoTHubClientSUBACK_WAIT_INTERVAL_MS )
        {
            timeout -= azureIoTHubClientSUBACK_WAIT_INTERVAL_MS;
            waitTime = azureIoTHubClientSUBACK_WAIT_INTERVAL_MS;
        }
        else
        {
            waitTime = timeout;
            timeout = 0;
        }

        if( AzureIoTMQTT_ProcessLoop( &xAzureIoTHubClientHandle->_internal.xMQTTContext, waitTime ) != AzureIoTMQTTSuccess )
        {
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
            break;
        }
    } while( timeout );

    if( context->_internal.state == TOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                 const uint8_t * pHostname,
                                                 uint32_t hostnameLength,
                                                 const uint8_t * pDeviceId,
                                                 uint32_t deviceIdLength,
                                                 AzureIoTHubClientOptions_t * pHubClientOptions,
                                                 uint8_t * pBuffer,
                                                 uint32_t bufferLength,
                                                 AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                 const TransportInterface_t * pTransportInterface )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    az_iot_hub_client_options options;

    memset( ( void * ) xAzureIoTHubClientHandle, 0, sizeof( AzureIoTHubClient_t ) );

    /* Initialize Azure IoT Hub Client */
    az_span hostname_span = az_span_create( ( uint8_t * ) pHostname, hostnameLength );
    az_span device_id_span = az_span_create( ( uint8_t * ) pDeviceId, deviceIdLength );

    options = az_iot_hub_client_options_default();
    options.module_id = az_span_create( ( uint8_t * ) pHubClientOptions->pModuleId, pHubClientOptions->moduleIdLength );
    options.model_id = az_span_create( ( uint8_t * ) pHubClientOptions->pModelId, pHubClientOptions->modelIdLength );
    options.user_agent = az_span_create( ( uint8_t * ) pHubClientOptions->pUserAgent, pHubClientOptions->userAgentLength );

    if( az_result_failed( az_iot_hub_client_init( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                  hostname_span, device_id_span, &options ) ) )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    /* Initialize AzureIoTMQTT library. */
    else if( ( xResult = AzureIoTMQTT_Init( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ), pTransportInterface,
                                            prvGetTimeMs, prvEventCallback,
                                            pBuffer,  bufferLength ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.deviceId = pDeviceId;
        xAzureIoTHubClientHandle->_internal.deviceIdLength = deviceIdLength;
        xAzureIoTHubClientHandle->_internal.hostname = pHostname;
        xAzureIoTHubClientHandle->_internal.hostnameLength = hostnameLength;
        xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_time_function = getTimeFunction;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}


void AzureIoTHubClient_Deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    ( void ) xAzureIoTHubClientHandle;
}


AzureIoTHubClientError_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    bool cleanSession,
                                                    TickType_t xTimeoutTicks )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo;
    AzureIoTHubClientError_t ret;
    AzureIoTMQTTStatus_t xResult;
    bool xSessionPresent;
    uint32_t bytesCopied;

    ( void ) memset( ( void * ) &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    xConnectInfo.cleanSession = cleanSession;

    size_t mqtt_user_name_length;

    if( az_result_failed( az_iot_hub_client_get_user_name( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                           mqtt_user_name, sizeof( mqtt_user_name ),
                                                           &mqtt_user_name_length ) ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    xConnectInfo.pClientIdentifier = ( const char * ) xAzureIoTHubClientHandle->_internal.deviceId;
    xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTHubClientHandle->_internal.deviceIdLength;
    xConnectInfo.pUserName = mqtt_user_name;
    xConnectInfo.userNameLength = ( uint16_t ) mqtt_user_name_length;
    xConnectInfo.keepAliveSeconds = azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS;
    xConnectInfo.passwordLength = 0;
    xConnectInfo.pPassword = mqtt_password;

    if( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh )
    {
        if( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh( xAzureIoTHubClientHandle,
                                                                                    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_time_function() + azureIoTHUBDEFAULTTOKENTIMEOUTINSEC,
                                                                                    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key,
                                                                                    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length,
                                                                                    ( uint8_t * ) mqtt_password, sizeof( mqtt_password ),
                                                                                    &bytesCopied ) )
        {
            AZLogError( ( "Failed to generate auth token \r\n" ) );
            return AZURE_IOT_HUB_CLIENT_FAILED;
        }

        xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;
    }

    /* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
     * is passed as NULL. */
    if( ( xResult = AzureIoTMQTT_Connect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                          &xConnectInfo,
                                          NULL,
                                          xTimeoutTicks,
                                          &xSessionPresent ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d \r\n",
                      xAzureIoTHubClientHandle->_internal.hostnameLength, xAzureIoTHubClientHandle->_internal.hostname,
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        /* Successfully established and MQTT connection with the broker. */
        AZLogInfo( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTHubClientHandle->_internal.hostnameLength,
                     ( const char * ) xAzureIoTHubClientHandle->_internal.hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;

    if( ( xResult = AzureIoTMQTT_Disconnect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        AZLogInfo( ( "Disconnecting the MQTT connection with %.*s.\r\n", xAzureIoTHubClientHandle->_internal.hostnameLength,
                     ( const char * ) xAzureIoTHubClientHandle->_internal.hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          const uint8_t * pTelemetryData,
                                                          uint32_t telemetryDataLength,
                                                          AzureIoTMessageProperties_t * properties )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    size_t telemetry_topic_length;

    if( az_result_failed( az_iot_hub_client_telemetry_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                         ( properties != NULL ) ? &properties->_internal.properties : NULL,
                                                                         telemetry_topic, sizeof( telemetry_topic ), &telemetry_topic_length ) ) )
    {
        /* Some fields are not used by this demo so start with everything at 0. */
        ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );
    }

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = AzureIoTMQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = telemetry_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) telemetry_topic_length;
    xMQTTPublishInfo.pPayload = ( const void * ) pTelemetryData;
    xMQTTPublishInfo.payloadLength = telemetryDataLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                          &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMethodRequest_t * message,
                                                               uint32_t status,
                                                               const uint8_t * pMethodPayload,
                                                               uint32_t methodPayloadLength )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    az_span requestIdSpan = az_span_create( ( uint8_t * ) message->requestId, message->requestIdLength );

    size_t method_topic_length;
    az_result res = az_iot_hub_client_methods_response_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                          requestIdSpan, ( uint16_t ) status, method_topic,
                                                                          sizeof( method_topic ), &method_topic_length );

    if( az_result_failed( res ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = method_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) method_topic_length;
    xMQTTPublishInfo.pPayload = ( const void * ) pMethodPayload;
    xMQTTPublishInfo.payloadLength = methodPayloadLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                          &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                        uint32_t timeoutMs )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;

    /* Process incoming UNSUBACK packet from the broker. */
    if( ( xResult = AzureIoTMQTT_ProcessLoop( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ), timeoutMs ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "MQTT_ProcessLoop failed : ProcessLoopDuration=%u, Error=%d\r\n", timeoutMs,
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientC2DCallback_t callback,
                                                               void * callback_context,
                                                               uint32_t timeout )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription;
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_C2D ];

    mqttSubscription.qos = AzureIoTMQTTQoS1;
    mqttSubscription.pTopicFilter = AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
    mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                            &mqttSubscription, 1, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%d",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        context->_internal.state = TOPIC_SUBSCRIBE_STATE_SUB;
        context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
        context->_internal.process_function = azure_iot_hub_client_cloud_message_process;
        context->_internal.callbacks.c2dCallback = callback;
        context->_internal.callback_context = callback_context;

        ret = waitforSubAck( xAzureIoTHubClientHandle, context, timeout );
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMethodCallback_t callback,
                                                               void * callback_context,
                                                               uint32_t timeout )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription;
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_METHODS ];

    mqttSubscription.qos = AzureIoTMQTTQoS0;
    mqttSubscription.pTopicFilter = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
    mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                            &mqttSubscription, 1, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%d",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        context->_internal.state = TOPIC_SUBSCRIBE_STATE_SUB;
        context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
        context->_internal.process_function = azure_iot_hub_client_direct_method_process;
        context->_internal.callbacks.methodCallback = callback;
        context->_internal.callback_context = callback_context;

        ret = waitforSubAck( xAzureIoTHubClientHandle, context, timeout );
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             AzureIoTHubClientTwinCallback_t callback,
                                                             void * callback_context,
                                                             uint32_t timeout )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription[ 2 ];
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_TWIN ];

    mqttSubscription[ 0 ].qos = AzureIoTMQTTQoS0;
    mqttSubscription[ 0 ].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    mqttSubscription[ 0 ].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
    mqttSubscription[ 1 ].qos = AzureIoTMQTTQoS0;
    mqttSubscription[ 1 ].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    mqttSubscription[ 1 ].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to subscribe to the MQTT  %s and %s topics.\r\n",
                 AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                            mqttSubscription, 2, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%d \r\n",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        context->_internal.state = TOPIC_SUBSCRIBE_STATE_SUB;
        context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
        context->_internal.process_function = azure_iot_hub_client_device_twin_process;
        context->_internal.callbacks.twinCallback = callback;
        context->_internal.callback_context = callback_context;

        ret = waitforSubAck( xAzureIoTHubClientHandle, context, timeout );
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_SymmetricKeySet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            const uint8_t * pSymmetricKey,
                                                            uint32_t pSymmetricKeyLength,
                                                            AzureIoTGetHMACFunc_t hmacFunction )
{
    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( pSymmetricKey == NULL ) || ( pSymmetricKeyLength == 0 ) ||
        ( hmacFunction == NULL ) )
    {
        AZLogError( ( "IoTHub client symmetric key fail: Invalid argument" ) );
        return AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }

    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key = pSymmetricKey;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length = pSymmetricKeyLength;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh = azure_iot_hub_client_token_get;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function = hmacFunction;

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinReportedSend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   const uint8_t * pReportedPayload,
                                                                   uint32_t reportedPayloadLength,
                                                                   uint32_t * pRequestId )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    char request_id[ 10 ];
    size_t device_twin_reported_topic_length;
    az_result res;
    az_span request_id_span = az_span_create( ( uint8_t * ) request_id, sizeof( request_id ) );

    if( xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_TWIN ]._internal.state != TOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    prv_request_id_get( xAzureIoTHubClientHandle, request_id_span, true, pRequestId, &request_id_span );

    res = az_iot_hub_client_twin_patch_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                          request_id_span,
                                                          device_twin_reported_topic, sizeof( device_twin_reported_topic ),
                                                          &device_twin_reported_topic_length );

    if( az_result_failed( res ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = device_twin_reported_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_reported_topic_length;
    xMQTTPublishInfo.pPayload = ( const void * ) pReportedPayload;
    xMQTTPublishInfo.payloadLength = reportedPayloadLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                          &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    char request_id[ 10 ];
    az_span request_id_span = az_span_create( ( uint8_t * ) request_id, sizeof( request_id ) );
    size_t device_twin_get_topic_length;
    az_result res;

    if( xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_TWIN ]._internal.state != TOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    prv_request_id_get( xAzureIoTHubClientHandle, request_id_span, false, NULL, &request_id_span );

    res = az_iot_hub_client_twin_document_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                             request_id_span,
                                                             device_twin_get_topic, sizeof( device_twin_get_topic ),
                                                             &device_twin_get_topic_length );

    if( az_result_failed( res ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = device_twin_get_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_get_topic_length;
    xMQTTPublishInfo.pPayload = NULL;
    xMQTTPublishInfo.payloadLength = 0;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                          &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

/*-----------------------------------------------------------*/

AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription;
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_C2D ];

    mqttSubscription.qos = AzureIoTMQTTQoS1;
    mqttSubscription.pTopicFilter = AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
    mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to unsubscribe from the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Unsubscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                              &mqttSubscription, 1, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to UNSUBSCRIBE from MQTT topic. Error=%d",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        memset( context, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

/*-----------------------------------------------------------*/

AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription;
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_METHODS ];

    mqttSubscription.qos = AzureIoTMQTTQoS0;
    mqttSubscription.pTopicFilter = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
    mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to unsubscribe from the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Unsubscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                              &mqttSubscription, 1, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to UNSUBSCRIBE from MQTT topic. Error=%d",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        memset( context, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

/*-----------------------------------------------------------*/

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription[ 2 ];
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ RECEIVE_CONTEXT_INDEX_TWIN ];

    mqttSubscription[ 0 ].qos = AzureIoTMQTTQoS0;
    mqttSubscription[ 0 ].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    mqttSubscription[ 0 ].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
    mqttSubscription[ 1 ].qos = AzureIoTMQTTQoS0;
    mqttSubscription[ 1 ].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    mqttSubscription[ 1 ].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
    usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    AZLogInfo( ( "Attempt to unsubscribe from the MQTT  %s and %s topics.\r\n",
                 AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

    if( ( xResult = AzureIoTMQTT_Unsubscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                              mqttSubscription, 2, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to UNSUBSCRIBE from MQTT topic. Error=%d \r\n",
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        memset( context, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

/*-----------------------------------------------------------*/
