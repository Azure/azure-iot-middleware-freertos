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
 * @file azure_iot_hub_client.c
 * @brief -------.
 */

#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "azure_iot_hub_client.h"

#include "azure/az_iot.h"

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND                             ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK                               ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/*
 * Indexes of the receive context buffer for each feature
 */
#define RECEIVE_CONTEXT_INDEX_C2D       0
#define RECEIVE_CONTEXT_INDEX_METHODS   1
#define RECEIVE_CONTEXT_INDEX_TWIN      2

static char mqtt_user_name[128];
static char mqtt_password[256];
static char telemetry_topic[128];
static char method_topic[128];
static char device_twin_reported_topic[128];
static char device_twin_get_topic[128];

static void prvMQTTProcessIncomingPublish( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                           MQTTPublishInfo_t * pxPublishInfo )
{
    uint32_t index;
    configASSERT( pxPublishInfo != NULL );

    for( index = 0; index < ( sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext ) / sizeof( xAzureIoTHubClientHandle->_internal.xReceiveContext[0] ) ); index++ )
    {
        if ( xAzureIoTHubClientHandle->_internal.xReceiveContext[index].process_function != NULL &&
             ( xAzureIoTHubClientHandle->_internal.xReceiveContext[index].process_function( xAzureIoTHubClientHandle, pxPublishInfo ) == AZURE_IOT_HUB_CLIENT_SUCCESS ) )
        {
            break;
        }
    }
}

static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    ( void )pxIncomingPacket;
    ( void )usPacketId;
}

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTHubClientHandle_t  xAzureIoTHubClientHandle = ( AzureIoTHubClientHandle_t ) pxMQTTContext;

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
    AzureIoTHubClientMessage_t message;

    az_iot_hub_client_c2d_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength );
    if ( az_result_failed( az_iot_hub_client_c2d_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core, topic_span, &out_request ) ) )
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

    if ( xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].callback )
    {
        message.message_type = AZURE_IOT_HUB_C2D_MESSAGE;
        message.pxPublishInfo = pxPublishInfo;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].callback( &message,
                                                               xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_direct_method_process( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            MQTTPublishInfo_t * pxPublishInfo )
{
    AzureIoTHubClientMessage_t message;

    az_iot_hub_client_method_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength );
    if ( az_result_failed( az_iot_hub_client_methods_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                           topic_span, &out_request ) ) )
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

    if ( xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].callback )
    {
        message.message_type = AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE;
        message.pxPublishInfo = pxPublishInfo;
        message.parsed_message.method_request = out_request;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].callback( &message,
                                                               xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_device_twin_process( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          MQTTPublishInfo_t * pxPublishInfo )
{
    AzureIoTHubClientMessage_t message;

    az_iot_hub_client_twin_response out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength );
    if ( az_result_failed( az_iot_hub_client_twin_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                        topic_span, &out_request ) ) )
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

    if ( xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].callback )
    {
        uint32_t request_id;
        if ( az_span_size(out_request.request_id) == 0 )
        {
            message.message_type = AZURE_IOT_HUB_TWIN_DESIRED_PROPERTY_MESSAGE;
        }
        else
        {
            if( az_result_failed(az_span_atou32(out_request.request_id, &request_id)) )
            {
                return AZURE_IOT_HUB_CLIENT_FAILED;
            }

            if( request_id & 0x01 )
            {
                message.message_type = AZURE_IOT_HUB_TWIN_REPORTED_RESPONSE_MESSAGE;
            }
            else
            {
                message.message_type = AZURE_IOT_HUB_TWIN_GET_MESSAGE;
            }
        }

        message.pxPublishInfo = pxPublishInfo;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].callback( &message,
                                                               xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

static uint32_t azure_iot_hub_client_token_get( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                                uint64_t expiryTimeSecs, const uint8_t * pKey, uint32_t keyLen,
                                                uint8_t * pSASBuffer, uint32_t sasBufferLen, uint32_t * pSaSLength )
{
    uint8_t buffer[512];
    az_span span = az_span_create( pSASBuffer, sasBufferLen );
    uint8_t * pOutput;
    uint32_t outputLen;
    az_result core_result;
    uint32_t bytesUsed;

    core_result = az_iot_hub_client_sas_get_signature( &( xAzureIoTHubClientHandle->_internal.iot_hub_client_core ),
                                                       expiryTimeSecs, span, &span );
    if ( az_result_failed( core_result ) )
    {
        LogError( ( "IoTHub failed failed to get signature with error status: %d", core_result ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    bytesUsed = ( uint32_t ) az_span_size( span );
    if ( AzureIoTBase64HMACCalculate( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function,
                                      pKey, keyLen, pSASBuffer, bytesUsed,
                                      buffer, sizeof( buffer ), &pOutput, &outputLen ) )
    {
        LogError( ( "IoTHub failed to encoded hash" ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    span = az_span_create( pOutput, ( int32_t )outputLen );
    core_result= az_iot_hub_client_sas_get_password( &( xAzureIoTHubClientHandle->_internal.iot_hub_client_core ),
                                                     expiryTimeSecs, span, AZ_SPAN_EMPTY,
                                                     ( char * ) pSASBuffer, sasBufferLen, pSaSLength );
    if ( az_result_failed( core_result ) )
    {
        LogError( ( "IoTHub failed to generate token with error status: %d", core_result ) );
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

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
                               az_span in_span, bool odd_seq, az_span* out_span)
{
    az_span span;

    if ( odd_seq == (xAzureIoTHubClientHandle->_internal.currentRequestId & 0x01 == 1) )
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId += 2;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId += 1;
    }
    
    if ( xAzureIoTHubClientHandle->_internal.currentRequestId == 0)
    {
        xAzureIoTHubClientHandle->_internal.currentRequestId = 2;
    }

    az_span remainder;
    az_result res = az_span_u32toa(in_span, xAzureIoTHubClientHandle->_internal.currentRequestId, &remainder);

    if (az_result_failed(res))
    {
        LogError(("Couldn't convert request id to span"));
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    *out_span = az_span_slice(*out_span, 0, az_span_size(*out_span) - az_span_size(remainder));

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

AzureIoTHubClientError_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                 const uint8_t * pHostname, uint32_t hostnameLength,
                                                 const uint8_t * pDeviceId, uint32_t deviceIdLength,
                                                 const uint8_t * pModuleId, uint32_t moduleIdLength,
                                                 uint8_t * pBuffer, uint32_t bufferLength,
                                                 AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                 const TransportInterface_t * pTransportInterface )
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    MQTTFixedBuffer_t xBuffer = { pBuffer, bufferLength };
    az_iot_hub_client_options options;

    memset( ( void * )xAzureIoTHubClientHandle, 0, sizeof( AzureIoTHubClient_t ) );

    /* Initialize Azure IoT Hub Client */
    az_span hostname_span = az_span_create( ( uint8_t * ) pHostname, hostnameLength );
    az_span device_id_span = az_span_create( ( uint8_t * ) pDeviceId, deviceIdLength );
    options = az_iot_hub_client_options_default();
    options.module_id = az_span_create( ( uint8_t * ) pModuleId, moduleIdLength );

    if ( az_result_failed( az_iot_hub_client_init( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                   hostname_span, device_id_span, &options ) ) )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    /* Initialize MQTT library. */
    else if ( ( xResult = MQTT_Init( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ), pTransportInterface,
                                     prvGetTimeMs, prvEventCallback,
                                     &xBuffer ) ) != MQTTSuccess )
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
    ( void )xAzureIoTHubClientHandle;
}


AzureIoTHubClientError_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    bool cleanSession, TickType_t xTimeoutTicks )
{
    MQTTConnectInfo_t xConnectInfo;
    AzureIoTHubClientError_t ret;
    MQTTStatus_t xResult;
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
        return AZURE_IOT_HUB_CLIENT_INIT_FAILED;
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

    if ( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh )
    {
        if ( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh( xAzureIoTHubClientHandle,
                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_time_function() + azureIoTHUBDEFAULTTOKENTIMEOUTINSEC,
                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key,
                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length,
                                                                           ( uint8_t * ) mqtt_password, sizeof( mqtt_password ),
                                                                           &bytesCopied ) )
        {
            LogError( ( "Failed to generate auth token \r\n" ) );
            return AZURE_IOT_HUB_CLIENT_FAILED;
        }

        xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;
    }
    
    /* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
     * is passed as NULL. */
    if ( ( xResult = MQTT_Connect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                   &xConnectInfo,
                                   NULL,
                                   xTimeoutTicks,
                                   &xSessionPresent ) ) != MQTTSuccess )
    {
        LogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%s \r\n",
                    xAzureIoTHubClientHandle->_internal.hostnameLength, xAzureIoTHubClientHandle->_internal.hostname,
                    MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        /* Successfully established and MQTT connection with the broker. */
        LogInfo( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTHubClientHandle->_internal.hostnameLength,
                   ( const char * ) xAzureIoTHubClientHandle->_internal.hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;

    if ( ( xResult = MQTT_Disconnect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) ) ) != MQTTSuccess )
    {
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        LogInfo( ( "Disconnecting the MQTT connection with %.*s.\r\n", xAzureIoTHubClientHandle->_internal.hostnameLength,
                   ( const char * ) xAzureIoTHubClientHandle->_internal.hostname ) );
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          const char * pTelemetryData, uint32_t telemetryDataLength )
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    size_t telemetry_topic_length;
    if( az_result_failed( az_iot_hub_client_telemetry_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                         NULL, telemetry_topic,
                                                                         sizeof( telemetry_topic ), &telemetry_topic_length ) ) )

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
    usPublishPacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if ( ( xResult = MQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
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

AzureIoTHubClientError_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMessage_t* message, uint32_t status,
                                                               const char * pMethodPayload, uint32_t methodPayloadLength)
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    size_t method_topic_length;
    az_result res = az_iot_hub_client_methods_response_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                          message->parsed_message.method_request.request_id,
                                                                          ( uint16_t )status, method_topic,
                                                                          sizeof( method_topic ), &method_topic_length );
    if( az_result_failed( res ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = MQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = method_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) method_topic_length;
    xMQTTPublishInfo.pPayload = pMethodPayload;
    xMQTTPublishInfo.payloadLength = methodPayloadLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if ( ( xResult = MQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
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

AzureIoTHubClientError_t AzureIoTHubClient_DoWork( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                   uint32_t timeoutMs )
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;

    /* Process incoming UNSUBACK packet from the broker. */
    if ( ( xResult = MQTT_ProcessLoop( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ), timeoutMs ) ) != MQTTSuccess )
    {
        LogError( ( "Failed to receive UNSUBACK packet from broker: ProcessLoopDuration=%u, Error=%s\r\n", timeoutMs,
                    MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context )
{
    MQTTSubscribeInfo_t * pMQTTSubscription = &xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].xMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription->qos = MQTTQoS1;
    pMQTTSubscription->pTopicFilter = AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
    pMQTTSubscription->topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );
    
    LogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                     pMQTTSubscription, 1,  usSubscribePacketIdentifier ) ) != MQTTSuccess )
    {
        LogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s",
                    MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].process_function = azure_iot_hub_client_cloud_message_process;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].callback = callback;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_C2D].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context )
{   
    MQTTSubscribeInfo_t * pMQTTSubscription = &xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].xMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription->qos = MQTTQoS0;
    pMQTTSubscription->pTopicFilter = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
    pMQTTSubscription->topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );
    
    LogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                     pMQTTSubscription, 1,  usSubscribePacketIdentifier ) ) != MQTTSuccess )
    {
        LogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s",
                    MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].process_function = azure_iot_hub_client_direct_method_process;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].callback = callback;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_METHODS].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                             void * callback_context )
{
    MQTTSubscribeInfo_t * pMQTTSubscription;
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    pMQTTSubscription = &xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].xMQTTSubscription;
    pMQTTSubscription[0].qos = MQTTQoS0;
    pMQTTSubscription[0].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    pMQTTSubscription[0].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
    pMQTTSubscription[1].qos = MQTTQoS0;
    pMQTTSubscription[1].pTopicFilter = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    pMQTTSubscription[1].topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
    
    usSubscribePacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );
    
    LogInfo( ( "Attempt to subscribe to the MQTT  %s and %s topics.\r\n",
                AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

    if ( ( xResult = MQTT_Subscribe( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                     pMQTTSubscription, 2,  usSubscribePacketIdentifier ) ) != MQTTSuccess )
    {
        LogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s \r\n",
                    MQTT_Status_strerror( xResult ) ) );
        ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].process_function = azure_iot_hub_client_device_twin_process;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].callback = callback;
        xAzureIoTHubClientHandle->_internal.xReceiveContext[RECEIVE_CONTEXT_INDEX_TWIN].callback_context = callback_context;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}

AzureIoTHubClientError_t AzureIoTHubClient_SymmetricKeySet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            const uint8_t * pSymmetricKey, uint32_t pSymmetricKeyLength, 
                                                            AzureIoTGetHMACFunc_t hmacFunction )
{
    if ( ( xAzureIoTHubClientHandle == NULL ) ||
         ( pSymmetricKey == NULL ) || ( pSymmetricKeyLength == 0 ) ||
         ( hmacFunction == NULL ) )
    {
        LogError( ("IoTHub client symmetric key fail: Invalid argument" ) );
        return AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }

    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key = pSymmetricKey;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length = pSymmetricKeyLength;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh = azure_iot_hub_client_token_get;
    xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function = hmacFunction;

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinReportedSend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   const char* pReportedPayload, uint32_t reportedPayloadLength)
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    char request_id[10];
    az_span request_id_span = az_span_create( ( uint8_t* )request_id, sizeof(request_id) );
    prv_request_id_get(xAzureIoTHubClientHandle, request_id_span, true, &request_id_span);

    size_t device_twin_reported_topic_length;
    az_result res = az_iot_hub_client_twin_patch_get_publish_topic(&xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                      request_id_span,
                                                                      device_twin_reported_topic, sizeof(device_twin_reported_topic),
                                                                      &device_twin_reported_topic_length);
    if(az_result_failed(res))
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = MQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = device_twin_reported_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_reported_topic_length;
    xMQTTPublishInfo.pPayload = pReportedPayload;
    xMQTTPublishInfo.payloadLength = reportedPayloadLength;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if ( ( xResult = MQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
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

AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle)
{
    MQTTStatus_t xResult;
    AzureIoTHubClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint16_t usPublishPacketIdentifier;

    char request_id[10];
    az_span request_id_span = az_span_create( ( uint8_t* )request_id, sizeof(request_id) );
    prv_request_id_get(xAzureIoTHubClientHandle, request_id_span, false, &request_id_span);

    size_t device_twin_get_topic_length;
    az_result res = az_iot_hub_client_twin_document_get_publish_topic(&xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                      request_id_span,
                                                                      device_twin_get_topic, sizeof(device_twin_get_topic),
                                                                      &device_twin_get_topic_length);
    if(az_result_failed(res))
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = MQTTQoS1;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = device_twin_get_topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_get_topic_length;
    xMQTTPublishInfo.pPayload =     NULL;
    xMQTTPublishInfo.payloadLength = 0;

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

    /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
    if ( ( xResult = MQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
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

/*-----------------------------------------------------------*/
