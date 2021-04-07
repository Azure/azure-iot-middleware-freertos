/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client.c
 * @brief -------.
 */

#include "azure_iot_hub_client.h"
#include "azure_iot_mqtt.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "azure/az_iot.h"
#include "azure/core/az_version.h"

#ifndef azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC
    #define azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC    azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC
#endif /* azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC */

#ifndef azureiothubKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureiothubKEEP_ALIVE_TIMEOUT_SECONDS    azureiotKEEP_ALIVE_TIMEOUT_SECONDS
#endif /* azureiothubKEEP_ALIVE_TIMEOUT_SECONDS */

#ifndef azureiothubSUBACK_WAIT_INTERVAL_MS
    #define azureiothubSUBACK_WAIT_INTERVAL_MS    azureiotSUBACK_WAIT_INTERVAL_MS
#endif /* azureiothubSUBACK_WAIT_INTERVAL_MS */

#ifndef azureiothubUSER_AGENT
    #define azureiothubUSER_AGENT    "DeviceClientType=c%2F" AZ_SDK_VERSION_STRING "%28FreeRTOS%29"
#endif /* azureiothubUSER_AGENT */

/*
 * Topic subscribe state
 */
#define azureiothubTOPIC_SUBSCRIBE_STATE_NONE       ( 0x0 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUB        ( 0x1 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK     ( 0x2 )

/*-----------------------------------------------------------*/

static void prvMQTTProcessIncomingPublish( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                           AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    uint32_t index;

    configASSERT( pxPublishInfo != NULL );
    AzureIoTHubClientReceiveContext_t * context;

    for( index = 0; index < azureiothubSUBSCRIBE_FEATURE_COUNT; index++ )
    {
        context = &pxAzureIoTHubClient->_internal.xReceiveContext[ index ];

        if( ( context->_internal.pxProcessFunction != NULL ) &&
            ( context->_internal.pxProcessFunction( context,
                                                   pxAzureIoTHubClient,
                                                   ( void * ) pxPublishInfo ) == eAzureIoTHubClientSuccess ) )
        {
            break;
        }
    }
}
/*-----------------------------------------------------------*/

static void prvMQTTProcessResponse( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                    AzureIoTMQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    uint32_t index;
    AzureIoTHubClientReceiveContext_t * context;

    if( ( pxIncomingPacket->type & 0xF0U ) == AZURE_IOT_MQTT_PACKET_TYPE_SUBACK )
    {
        for( index = 0; index < azureiothubSUBSCRIBE_FEATURE_COUNT; index++ )
        {
            context = &pxAzureIoTHubClient->_internal.xReceiveContext[ index ];

            if( context->_internal.usMqttSubPacketID == usPacketId )
            {
                /* TODO: inspect packet to see is ack was successful*/
                context->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK;
                break;
            }
        }
    }
}
/*-----------------------------------------------------------*/

static void prvEventCallback( AzureIoTMQTTHandle_t pxMQTTContext,
                              AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                              AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTHubClient_t * pxAzureIoTHubClient = ( AzureIoTHubClient_t * ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == azureiotmqttPACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxAzureIoTHubClient, pxDeserializedInfo->pPublishInfo );
    }
    else if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_SUBACK )
    {
        prvMQTTProcessResponse( pxAzureIoTHubClient, pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
    else
    {
        AZLogDebug( ( "AzureIoTHubClient received packet of type: %d", pxPacketInfo->ucType ) );
    }
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientC2DProcess( AzureIoTHubClientReceiveContext_t * context,
                                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         void * pxPublishInfo )
{
    AzureIoTHubClientCloudToDeviceMessageRequest_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_c2d_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );

    if( az_result_failed( az_iot_hub_client_c2d_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                      topic_span, &out_request ) ) )
    {
        return eAzureIoTHubClientFailed;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Cloud message QoS : %d.", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Cloud message topic: %.*s  with payload : %.*s.",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.xCloudToDeviceMessageCallback )
    {
        message.pvMessagePayload = mqttPublishInfo->pPayload;
        message.ulPayloadLength = (uint32_t)mqttPublishInfo->payloadLength;
        message.xProperties._internal.properties = out_request.properties;
        context->_internal.callbacks.xCloudToDeviceMessageCallback( &message,
                                                           context->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientDirectMethodProcess( AzureIoTHubClientReceiveContext_t * context,
                                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         void * pxPublishInfo )
{
    AzureIoTHubClientMethodRequest_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_method_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );

    if( az_result_failed( az_iot_hub_client_methods_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                          topic_span, &out_request ) ) )
    {
        return eAzureIoTHubClientFailed;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Direct method QoS : %d.", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Direct method topic: %.*s  with method payload : %.*s.",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.xMethodCallback )
    {
        message.pvMessagePayload = mqttPublishInfo->pPayload;
        message.ulPayloadLength = (uint32_t)mqttPublishInfo->payloadLength;
        message.pucMethodName = az_span_ptr( out_request.name );
        message.usMethodNameLength = ( uint16_t ) az_span_size( out_request.name );
        message.pucRequestID = az_span_ptr( out_request.request_id );
        message.usRequestIDLength = ( uint16_t ) az_span_size( out_request.request_id );
        context->_internal.callbacks.xMethodCallback( &message, context->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientDeviceTwinProcess( AzureIoTHubClientReceiveContext_t * context,
                                                       AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                       void * pxPublishInfo )
{
    AzureIoTHubClientTwinResponse_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_twin_response out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );
    uint32_t request_id = 0;

    if( az_result_failed( az_iot_hub_client_twin_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                       topic_span, &out_request ) ) )
    {
        return eAzureIoTHubClientFailed;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Twin Incoming QoS : %d.", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Twin topic: %.*s. with payload : %.*s.",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.xTwinCallback )
    {
        if( az_span_size( out_request.request_id ) == 0 )
        {
            message.messageType = eAzureIoTHubTwinDesiredPropertyMessage;
        }
        else
        {
            if( az_result_failed( az_span_atou32( out_request.request_id, &request_id ) ) )
            {
                return eAzureIoTHubClientFailed;
            }

            if( request_id & 0x01 )
            {
                message.messageType = eAzureIoTHubTwinReportedResponseMessage;
            }
            else
            {
                message.messageType = eAzureIoTHubTwinGetMessage;
            }
        }

        message.pvMessagePayload = mqttPublishInfo->pPayload;
        message.ulPayloadLength = (uint32_t)mqttPublishInfo->payloadLength;
        message.messageStatus = out_request.status;
        message.pucVersion = az_span_ptr( out_request.version );
        message.usVersionLength = ( uint16_t ) az_span_size( out_request.version );
        message.ulRequestID = request_id;
        context->_internal.callbacks.xTwinCallback( &message,
                                                   context->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * azureiotMILLISECONDS_PER_TICK;

    return ulTimeMs;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions )
{
    AzureIoTHubClientResult_t ret;

    if( pxHubClientOptions == NULL )
    {
        AZLogError( ( "Failed to initialize options: Invalid argument." ) );
        ret = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxHubClientOptions->pucModelID = NULL;
        pxHubClientOptions->ulModelIDLength = 0;
        pxHubClientOptions->pucModuleID = NULL;
        pxHubClientOptions->ulModuleIDLength = 0;
        pxHubClientOptions->pucUserAgent = ( const uint8_t * ) azureiothubUSER_AGENT;
        pxHubClientOptions->ulUserAgentLength = sizeof( azureiothubUSER_AGENT ) - 1;
        ret = eAzureIoTHubClientSuccess;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Init( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                  const uint8_t * pucHostname,
                                                  uint16_t ulHostnameLength,
                                                  const uint8_t * pucDeviceId,
                                                  uint16_t ulDeviceIdLength,
                                                  AzureIoTHubClientOptions_t * pxHubClientOptions,
                                                  uint8_t * pucBuffer,
                                                  uint32_t ulBufferLength,
                                                  AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                  const AzureIoTTransportInterface_t * pxTransportInterface )
{
    AzureIoTHubClientResult_t ret;
    az_iot_hub_client_options options;
    uint8_t * pucNetworkBuffer;
    uint32_t ulNetworkBufferLength;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( pucHostname == NULL ) || ( ulHostnameLength == 0 ) ||
        ( pucDeviceId == NULL ) || ( ulDeviceIdLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "Failed to initialize: Invalid argument." ) );
        ret = eAzureIoTHubClientInvalidArgument;
    }
    else if ( ( ulBufferLength < azureiotTOPIC_MAX ) ||
              ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "Failed to initialize: Not enough memory passed." ) );
        ret = eAzureIoTHubClientOutOfMemory;
    }
    else
    {
        memset( ( void * ) pxAzureIoTHubClient, 0, sizeof( AzureIoTHubClient_t ) );

        /* Setup scratch buffer to be used by middleware */
        pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength =
            ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) > azureiotTOPIC_MAX ? ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) : azureiotTOPIC_MAX;
        pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer = pucBuffer;
        pucNetworkBuffer = pucBuffer + pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength;
        ulNetworkBufferLength = ulBufferLength - pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength;

        /* Initialize Azure IoT Hub Client */
        az_span hostname_span = az_span_create( ( uint8_t * ) pucHostname, ( int32_t ) ulHostnameLength );
        az_span device_id_span = az_span_create( ( uint8_t * ) pucDeviceId, ( int32_t ) ulDeviceIdLength );

        options = az_iot_hub_client_options_default();

        if( pxHubClientOptions )
        {
            options.module_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pucModuleID, ( int32_t ) pxHubClientOptions->ulModuleIDLength );
            options.model_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pucModelID, ( int32_t ) pxHubClientOptions->ulModelIDLength );
            options.user_agent = az_span_create( ( uint8_t * ) pxHubClientOptions->pucUserAgent, ( int32_t ) pxHubClientOptions->ulUserAgentLength );
        }
        else
        {
            options.user_agent = az_span_create( ( uint8_t * ) azureiothubUSER_AGENT, sizeof( azureiothubUSER_AGENT ) - 1 );
        }

        if( az_result_failed( az_iot_hub_client_init( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                      hostname_span, device_id_span, &options ) ) )
        {
            AZLogError( ( "Failed to initialize az_iot_hub_client_init." ) );
            ret = eAzureIoTHubClientInitFailed;
        }
        /* Initialize AzureIoTMQTT library. */
        else if( ( AzureIoTMQTT_Init( &( pxAzureIoTHubClient->_internal.xMQTTContext ), pxTransportInterface,
                                                prvGetTimeMs, prvEventCallback,
                                                pucNetworkBuffer, ulNetworkBufferLength ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to initialize AzureIoTMQTT_Init." ) );
            ret = eAzureIoTHubClientInitFailed;
        }
        else
        {
            pxAzureIoTHubClient->_internal.pucDeviceID = pucDeviceId;
            pxAzureIoTHubClient->_internal.ulDeviceIDLength = ulDeviceIdLength;
            pxAzureIoTHubClient->_internal.pucHostname = pucHostname;
            pxAzureIoTHubClient->_internal.ulHostnameLength = ulHostnameLength;
            pxAzureIoTHubClient->_internal.xAzureIoTHubClientTimeFunction = xGetTimeFunction;
            ret = eAzureIoTHubClientSuccess;
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

void AzureIoTHubClient_Deinit( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    ( void ) pxAzureIoTHubClient;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Connect( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                     bool cleanSession,
                                                     uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTHubClientResult_t ret;
    AzureIoTMQTTStatus_t xResult;
    bool xSessionPresent;
    uint32_t bytesCopied = 0;
    size_t mqtt_user_name_length;
    az_result res;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to connect: Invalid argument." ) );
        ret = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        /* Use scratch buffer for username/password */
        xConnectInfo.pUserName = ( const char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer;
        xConnectInfo.pPassword =  xConnectInfo.pUserName + azureiotUSERNAME_MAX;

        if( az_result_failed( res = az_iot_hub_client_get_user_name( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                     ( char * ) xConnectInfo.pUserName, azureiotUSERNAME_MAX,
                                                                     &mqtt_user_name_length ) ) )
        {
            AZLogError( ( "Failed to get username: %08x.", res ) );
            ret = eAzureIoTHubClientFailed;
        }
        /* Check if token refresh is set, then generate password */
        else if( ( pxAzureIoTHubClient->_internal.pxAzureIoTHubClientTokenRefresh ) &&
                 ( pxAzureIoTHubClient->_internal.pxAzureIoTHubClientTokenRefresh( pxAzureIoTHubClient,
                                                                                           pxAzureIoTHubClient->_internal.xAzureIoTHubClientTimeFunction() + azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                                                           pxAzureIoTHubClient->_internal.pucAzureIoTHubClientSymmetricKey,
                                                                                           pxAzureIoTHubClient->_internal.ulAzureIoTHubClientSymmetricKeyLength,
                                                                                           ( uint8_t * ) xConnectInfo.pPassword, azureiotPASSWORD_MAX,
                                                                                           &bytesCopied ) ) )
        {
            AZLogError( ( "Failed to generate auth token." ) );
            ret = eAzureIoTHubClientFailed;
        }
        else
        {
            xConnectInfo.cleanSession = cleanSession;
            xConnectInfo.pClientIdentifier = ( const char * ) pxAzureIoTHubClient->_internal.pucDeviceID;
            xConnectInfo.clientIdentifierLength = ( uint16_t ) pxAzureIoTHubClient->_internal.ulDeviceIDLength;
            xConnectInfo.userNameLength = ( uint16_t ) mqtt_user_name_length;
            xConnectInfo.keepAliveSeconds = azureiothubKEEP_ALIVE_TIMEOUT_SECONDS;
            xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;

            /* Send MQTT CONNECT packet to broker. Last Will and Testament is not used. */
            if( ( xResult = AzureIoTMQTT_Connect( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xConnectInfo,
                                                  NULL,
                                                  ulTimeoutMilliseconds,
                                                  &xSessionPresent ) ) != AzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d.",
                              pxAzureIoTHubClient->_internal.ulHostnameLength, pxAzureIoTHubClient->_internal.pucHostname,
                              xResult ) );
                ret = eAzureIoTHubClientFailed;
            }
            else
            {
                /* Successfully established a MQTT connection with the broker. */
                AZLogInfo( ( "An MQTT connection is established with %.*s.", pxAzureIoTHubClient->_internal.ulHostnameLength,
                             ( const char * ) pxAzureIoTHubClient->_internal.pucHostname ) );
                ret = eAzureIoTHubClientSuccess;
            }
        }
    }
    

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Disconnect( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to disconnect: Invalid argument." ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( ( xResult = AzureIoTMQTT_Disconnect( &( pxAzureIoTHubClient->_internal.xMQTTContext ) ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to disconnect: %d.", xResult ) );
        ret = eAzureIoTHubClientFailed;
    }
    else
    {
        AZLogInfo( ( "Disconnecting the MQTT connection with %.*s.", pxAzureIoTHubClient->_internal.ulHostnameLength,
                     ( const char * ) pxAzureIoTHubClient->_internal.pucHostname ) );
        ret = eAzureIoTHubClientSuccess;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SendTelemetry( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                           const uint8_t * pucTelemetryData,
                                                           uint32_t ulTelemetryDataLength,
                                                           AzureIoTMessageProperties_t * pxProperties )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    size_t telemetry_topic_length;
    az_result res;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to send telemetry: Invalid argument." ) );
        ret = eAzureIoTHubClientInvalidArgument;
    }
    else if( az_result_failed( res = az_iot_hub_client_telemetry_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                    ( pxProperties != NULL ) ? &pxProperties->_internal.properties : NULL,
                                                                                    ( char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer,
                                                                                    pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength,
                                                                                    &telemetry_topic_length ) ) )
    {
        AZLogError( ( "Failed to get telemetry topic : %08x.", res ) );
        ret = eAzureIoTHubClientFailed;
    }
    else
    {
        xMQTTPublishInfo.qos = AzureIoTMQTTQoS1;
        xMQTTPublishInfo.pTopicName = ( const char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer;
        xMQTTPublishInfo.topicNameLength = ( uint16_t ) telemetry_topic_length;
        xMQTTPublishInfo.pPayload = ( const void * ) pucTelemetryData;
        xMQTTPublishInfo.payloadLength = ulTelemetryDataLength;

        /* Get a unique packet id. */
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
        if( ( xResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                              &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to publish telementry : Error=%d.", xResult ) );
            ret = eAzureIoTHubClientFailed;
        }
        else
        {
            ret = eAzureIoTHubClientSuccess;
        }
    }

    return ret;
}
