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
 * Indexes of the receive context buffer for each feature
 */
#define azureiothubRECEIVE_CONTEXT_INDEX_C2D        0
#define azureiothubRECEIVE_CONTEXT_INDEX_METHODS    1
#define azureiothubRECEIVE_CONTEXT_INDEX_TWIN       2

/*
 * Topic subscribe state
 */
#define azureiothubTOPIC_SUBSCRIBE_STATE_NONE       0
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUB        1
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK     2

#define azureiothubMETHOD_EMPTY_RESPONSE            "{}"

/*-----------------------------------------------------------*/

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
/*-----------------------------------------------------------*/

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
                context->_internal.state = azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK;
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
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientCloudMessageProcess( AzureIoTHubClientReceiveContext_t * context,
                                                         AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                         void * pxPublishInfo )
{
    AzureIoTHubClientCloudMessageRequest_t message;
    AzureIoTMQTTPublishInfo_t * mqttPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_c2d_request out_request;
    az_span topic_span = az_span_create( ( uint8_t * ) mqttPublishInfo->pTopicName, mqttPublishInfo->topicNameLength );

    if( az_result_failed( az_iot_hub_client_c2d_parse_received_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                      topic_span, &out_request ) ) )
    {
        return AZURE_IOT_HUB_CLIENT_FAILED;
    }

    /* Process incoming Publish. */
    AZLogInfo( ( "Cloud message QoS : %d\n", mqttPublishInfo->qos ) );

    /* Verify the received publish is for the we have subscribed to. */
    AZLogInfo( ( "Cloud message topic: %.*s  with payload : %.*s",
                 mqttPublishInfo->topicNameLength,
                 mqttPublishInfo->pTopicName,
                 mqttPublishInfo->payloadLength,
                 mqttPublishInfo->pPayload ) );

    if( context->_internal.callbacks.cloudMessageCallback )
    {
        message.messagePayload = mqttPublishInfo->pPayload;
        message.payloadLength = mqttPublishInfo->payloadLength;
        message.properties._internal.properties = out_request.properties;
        context->_internal.callbacks.cloudMessageCallback( &message,
                                                           context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientDirectMethodProcess( AzureIoTHubClientReceiveContext_t * context,
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
        message.methodNameLength = ( size_t ) az_span_size( out_request.name );
        message.requestId = az_span_ptr( out_request.request_id );
        message.requestIdLength = ( size_t ) az_span_size( out_request.request_id );
        context->_internal.callbacks.methodCallback( &message, context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientDeviceTwinProcess( AzureIoTHubClientReceiveContext_t * context,
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
        message.versionLength = ( size_t ) az_span_size( out_request.version );
        message.requestId = request_id;
        context->_internal.callbacks.twinCallback( &message,
                                                   context->_internal.callback_context );
    }

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTHubClientTokenGet( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                              uint64_t expiryTimeSecs,
                                              const uint8_t * pKey,
                                              uint32_t keyLen,
                                              uint8_t * pSASBuffer,
                                              uint32_t sasBufferLen,
                                              uint32_t * pSaSLength )
{
    uint8_t buffer[ 512 ];
    az_span span = az_span_create( pSASBuffer, ( int32_t ) sasBufferLen );
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

    if( AzureIoT_Base64HMACCalculate( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function,
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

    *pSaSLength = ( uint32_t ) length;

    return AZURE_IOT_HUB_CLIENT_SUCCESS;
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

static int prvRequestIdGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
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
/*-----------------------------------------------------------*/

static AzureIoTHubClientResult_t prvWaitForSubAck( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                   AzureIoTHubClientReceiveContext_t * context,
                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTHubClientResult_t ret = AZURE_IOT_HUB_CLIENT_SUBACK_WAIT_TIMEOUT;
    uint32_t waitTime;

    do
    {
        if( context->_internal.state == azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
        {
            ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            break;
        }

        if( ulTimeoutMilliseconds > azureiothubSUBACK_WAIT_INTERVAL_MS )
        {
            ulTimeoutMilliseconds -= azureiothubSUBACK_WAIT_INTERVAL_MS;
            waitTime = azureiothubSUBACK_WAIT_INTERVAL_MS;
        }
        else
        {
            waitTime = ulTimeoutMilliseconds;
            ulTimeoutMilliseconds = 0;
        }

        if( AzureIoTMQTT_ProcessLoop( &xAzureIoTHubClientHandle->_internal.xMQTTContext, waitTime ) != AzureIoTMQTTSuccess )
        {
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
            break;
        }
    } while( ulTimeoutMilliseconds );

    if( context->_internal.state == azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions )
{
    AzureIoTHubClientResult_t ret;

    if( pxHubClientOptions == NULL )
    {
        AZLogError( ( "Failed to initialize options: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        pxHubClientOptions->pModelId = NULL;
        pxHubClientOptions->modelIdLength = 0;
        pxHubClientOptions->pModuleId = NULL;
        pxHubClientOptions->moduleIdLength = 0;
        pxHubClientOptions->pUserAgent = ( const uint8_t * ) azureiothubUSER_AGENT;
        pxHubClientOptions->userAgentLength = sizeof( azureiothubUSER_AGENT ) - 1;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                  const uint8_t * pucHostname,
                                                  uint32_t ulHostnameLength,
                                                  const uint8_t * pucDeviceId,
                                                  uint32_t ulDeviceIdLength,
                                                  AzureIoTHubClientOptions_t * pxHubClientOptions,
                                                  uint8_t * pucBuffer,
                                                  uint32_t ulBufferLength,
                                                  AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                  const AzureIoTTransportInterface_t * pxTransportInterface )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    az_iot_hub_client_options options;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( pucHostname == NULL ) || ( ulHostnameLength == 0 ) ||
        ( pucDeviceId == NULL ) || ( ulDeviceIdLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "Failed to initialize: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else if ( ( ulBufferLength < azureiotTOPIC_MAX ) ||
              ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "Failed to initialize: Not enough memory passed \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_OUT_OF_MEMORY;
    }
    else
    {
        memset( ( void * ) xAzureIoTHubClientHandle, 0, sizeof( AzureIoTHubClient_t ) );

        /* Setup scratch buffer to be used by middleware */
        xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length =
            ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) > azureiotTOPIC_MAX ? ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) : azureiotTOPIC_MAX;
        xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer = pucBuffer;
        pucBuffer += xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length;
        ulBufferLength -= xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length;

        /* Initialize Azure IoT Hub Client */
        az_span hostname_span = az_span_create( ( uint8_t * ) pucHostname, ( int32_t ) ulHostnameLength );
        az_span device_id_span = az_span_create( ( uint8_t * ) pucDeviceId, ( int32_t ) ulDeviceIdLength );

        options = az_iot_hub_client_options_default();

        if( pxHubClientOptions )
        {
            options.module_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pModuleId, ( int32_t ) pxHubClientOptions->moduleIdLength );
            options.model_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pModelId, ( int32_t ) pxHubClientOptions->modelIdLength );
            options.user_agent = az_span_create( ( uint8_t * ) pxHubClientOptions->pUserAgent, ( int32_t ) pxHubClientOptions->userAgentLength );
        }
        else
        {
            options.user_agent = az_span_create( ( uint8_t * ) azureiothubUSER_AGENT, sizeof( azureiothubUSER_AGENT ) - 1 );
        }

        if( az_result_failed( az_iot_hub_client_init( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                      hostname_span, device_id_span, &options ) ) )
        {
            AZLogError( ( "Failed to initialize az_iot_hub_client_init \r\n" ) );
            ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
        }
        /* Initialize AzureIoTMQTT library. */
        else if( ( xResult = AzureIoTMQTT_Init( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ), pxTransportInterface,
                                                prvGetTimeMs, prvEventCallback,
                                                pucBuffer, ulBufferLength ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to initialize AzureIoTMQTT_Init \r\n" ) );
            ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
        }
        else
        {
            xAzureIoTHubClientHandle->_internal.deviceId = pucDeviceId;
            xAzureIoTHubClientHandle->_internal.deviceIdLength = ulDeviceIdLength;
            xAzureIoTHubClientHandle->_internal.hostname = pucHostname;
            xAzureIoTHubClientHandle->_internal.hostnameLength = ulHostnameLength;
            xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_time_function = xGetTimeFunction;
            ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

void AzureIoTHubClient_Deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    ( void ) xAzureIoTHubClientHandle;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
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

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to connect: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        /* Use scratch buffer for username/password */
        xConnectInfo.pUserName = ( const char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer;
        xConnectInfo.pPassword =  xConnectInfo.pUserName + azureiotUSERNAME_MAX;

        if( az_result_failed( res = az_iot_hub_client_get_user_name( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                     ( char * ) xConnectInfo.pUserName, azureiotUSERNAME_MAX,
                                                                     &mqtt_user_name_length ) ) )
        {
            AZLogError( ( "Failed to get username: %08x\r\n", res ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        /* Check if token refresh is set, then generate password */
        else if( ( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh ) &&
                 ( xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh( xAzureIoTHubClientHandle,
                                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_time_function() + azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key,
                                                                                           xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length,
                                                                                           ( uint8_t * ) xConnectInfo.pPassword, azureiotPASSWORD_MAX,
                                                                                           &bytesCopied ) ) )
        {
            AZLogError( ( "Failed to generate auth token \r\n" ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        else
        {
            xConnectInfo.cleanSession = cleanSession;
            xConnectInfo.pClientIdentifier = ( const char * ) xAzureIoTHubClientHandle->_internal.deviceId;
            xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTHubClientHandle->_internal.deviceIdLength;
            xConnectInfo.userNameLength = ( uint16_t ) mqtt_user_name_length;
            xConnectInfo.keepAliveSeconds = azureiothubKEEP_ALIVE_TIMEOUT_SECONDS;
            xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;

            /* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
             * is passed as NULL. */
            if( ( xResult = AzureIoTMQTT_Connect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                                  &xConnectInfo,
                                                  NULL,
                                                  ulTimeoutMilliseconds,
                                                  &xSessionPresent ) ) != AzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d \r\n",
                              xAzureIoTHubClientHandle->_internal.hostnameLength, xAzureIoTHubClientHandle->_internal.hostname,
                              xResult ) );
                ret = AZURE_IOT_HUB_CLIENT_FAILED;
            }
            else
            {
                /* Successfully established a MQTT connection with the broker. */
                AZLogInfo( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTHubClientHandle->_internal.hostnameLength,
                             ( const char * ) xAzureIoTHubClientHandle->_internal.hostname ) );
                ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            }
        }
    }
    

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to disconnect: Invalid argument \r\n" ) );
        return AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }

    if( ( xResult = AzureIoTMQTT_Disconnect( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to disconnect: %d\r\n", xResult ) );
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
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
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

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to send telemetry: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else if( az_result_failed( res = az_iot_hub_client_telemetry_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                                    ( pxProperties != NULL ) ? &pxProperties->_internal.properties : NULL,
                                                                                    ( char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer,
                                                                                    xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length,
                                                                                    &telemetry_topic_length ) ) )
    {
        AZLogError( ( "Failed to get telemetry topic : %08x \r\n", res ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        xMQTTPublishInfo.qos = AzureIoTMQTTQoS1;
        xMQTTPublishInfo.pTopicName = ( const char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer;
        xMQTTPublishInfo.topicNameLength = ( uint16_t ) telemetry_topic_length;
        xMQTTPublishInfo.pPayload = ( const void * ) pucTelemetryData;
        xMQTTPublishInfo.payloadLength = ulTelemetryDataLength;

        /* Get a unique packet id. */
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

        /* Send PUBLISH packet. Packet ID is not used for a QoS1 publish. */
        if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                              &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to publish telementry : Error=%d \r\n", xResult ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        else
        {
            ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                const AzureIoTHubClientMethodRequest_t * pxMessage,
                                                                uint32_t ulStatus,
                                                                const uint8_t * pucMethodPayload,
                                                                uint32_t ulMethodPayloadLength )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    az_span requestIdSpan;
    size_t method_topic_length;
    az_result res;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( pxMessage == NULL ) )
    {
        AZLogError( ( "Failed to send method response: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        requestIdSpan = az_span_create( ( uint8_t * ) pxMessage->requestId, ( int32_t ) pxMessage->requestIdLength );

        if( az_result_failed( res = az_iot_hub_client_methods_response_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                                          requestIdSpan, ( uint16_t ) ulStatus,
                                                                                          ( char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer,
                                                                                          xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length,
                                                                                          &method_topic_length ) ) )
        {
            AZLogError( ( "Failed to get method response topic : %08x \r\n", res ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        else
        {
            xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
            xMQTTPublishInfo.pTopicName = ( const char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer;
            xMQTTPublishInfo.topicNameLength = ( uint16_t ) method_topic_length;

            if( ( pucMethodPayload == NULL ) || ( ulMethodPayloadLength == 0 ) )
            {
                xMQTTPublishInfo.pPayload = ( const void * ) azureiothubMETHOD_EMPTY_RESPONSE;
                xMQTTPublishInfo.payloadLength = sizeof( azureiothubMETHOD_EMPTY_RESPONSE ) - 1;
            }
            else
            {
                xMQTTPublishInfo.pPayload = ( const void * ) pucMethodPayload;
                xMQTTPublishInfo.payloadLength = ulMethodPayloadLength;
            }

            /* Get a unique packet id. */
            usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

            if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to publish response : Error=%d \r\n", xResult ) );
                ret = AZURE_IOT_HUB_CLIENT_FAILED;
            }
            else
            {
                ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            }
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                         uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to process loop: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    /* Process incoming UNSUBACK packet from the broker. */
    else if( ( xResult = AzureIoTMQTT_ProcessLoop( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                                   ulTimeoutMilliseconds ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "MQTT_ProcessLoop failed : ProcessLoopDuration=%u, Error=%d\r\n", ulTimeoutMilliseconds,
                      xResult ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_CloudMessageSubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   AzureIoTHubClientCloudMessageCallback_t xCloudMessageCallback,
                                                                   void * prvCallbackContext,
                                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( xCloudMessageCallback == NULL ) )
    {
        AZLogError( ( "Failed to enable cloud message : Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_C2D ];
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
            context->_internal.state = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
            context->_internal.process_function = prvAzureIoTHubClientCloudMessageProcess;
            context->_internal.callbacks.cloudMessageCallback = xCloudMessageCallback;
            context->_internal.callback_context = prvCallbackContext;

            ret = prvWaitForSubAck( xAzureIoTHubClientHandle, context, ulTimeoutMilliseconds );
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DirectMethodSubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   AzureIoTHubClientMethodCallback_t xMethodCallback,
                                                                   void * prvCallbackContext,
                                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( xMethodCallback == NULL ) )
    {
        AZLogError( ( "Failed to enable direct method: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_METHODS ];
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
            context->_internal.state = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
            context->_internal.process_function = prvAzureIoTHubClientDirectMethodProcess;
            context->_internal.callbacks.methodCallback = xMethodCallback;
            context->_internal.callback_context = prvCallbackContext;

            ret = prvWaitForSubAck( xAzureIoTHubClientHandle, context, ulTimeoutMilliseconds );
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinSubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                 AzureIoTHubClientTwinCallback_t xTwinCallback,
                                                                 void * prvCallbackContext,
                                                                 uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription[ 2 ];
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( xTwinCallback == NULL ) )
    {
        AZLogError( ( "Failed to enable device twin: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ];
        memset( mqttSubscription, 0, sizeof( mqttSubscription ) );
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
            context->_internal.state = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            context->_internal.mqttSubPacketId = usSubscribePacketIdentifier;
            context->_internal.process_function = prvAzureIoTHubClientDeviceTwinProcess;
            context->_internal.callbacks.twinCallback = xTwinCallback;
            context->_internal.callback_context = prvCallbackContext;

            ret = prvWaitForSubAck( xAzureIoTHubClientHandle, context, ulTimeoutMilliseconds );
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SymmetricKeySet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             const uint8_t * pucSymmetricKey,
                                                             uint32_t ulSymmetricKeyLength,
                                                             AzureIoTGetHMACFunc_t xHmacFunction )
{
    AzureIoTHubClientResult_t ret;

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( pucSymmetricKey == NULL ) || ( ulSymmetricKeyLength == 0 ) ||
        ( xHmacFunction == NULL ) )
    {
        AZLogError( ( "IoTHub client symmetric key fail: Invalid argument" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key = pucSymmetricKey;
        xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_symmetric_key_length = ulSymmetricKeyLength;
        xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_token_refresh = prvAzureIoTHubClientTokenGet;
        xAzureIoTHubClientHandle->_internal.azure_iot_hub_client_hmac_function = xHmacFunction;
        ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinReportedSend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                    const uint8_t * pucReportedPayload,
                                                                    uint32_t ulReportedPayloadLength,
                                                                    uint32_t * pulRequestId )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    char request_id[ 10 ];
    size_t device_twin_reported_topic_length;
    az_result res;
    az_span request_id_span = az_span_create( ( uint8_t * ) request_id, sizeof( request_id ) );

    if( ( xAzureIoTHubClientHandle == NULL ) ||
        ( pucReportedPayload == NULL ) || ( ulReportedPayloadLength == 0 ) )
    {
        AZLogError( ( "Failed to reported property: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else if( xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ]._internal.state != azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        AZLogError( ( "Failed to reported property: twin topic not subscribed \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        prvRequestIdGet( xAzureIoTHubClientHandle, request_id_span, true, pulRequestId, &request_id_span );

        if( az_result_failed( res = az_iot_hub_client_twin_patch_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                                    request_id_span,
                                                                                    ( char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer,
                                                                                    xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length,
                                                                                    &device_twin_reported_topic_length ) ) )
        {
            AZLogError( ( "Failed to get twin patch topic: %08x\r\n", res ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        else
        {
            xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
            xMQTTPublishInfo.pTopicName = ( const char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer;
            xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_reported_topic_length;
            xMQTTPublishInfo.pPayload = ( const void * ) pucReportedPayload;
            xMQTTPublishInfo.payloadLength = ulReportedPayloadLength;

            /* Get a unique packet id. */
            usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

            if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to Publish twin reported message. Error=%d \r\n",
                              xResult ) );
                ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
            }
            else
            {
                ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            }
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    char request_id[ 10 ];
    az_span request_id_span = az_span_create( ( uint8_t * ) request_id, sizeof( request_id ) );
    size_t device_twin_get_topic_length;
    az_result res;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to get twin: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else if( xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ]._internal.state != azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        AZLogError( ( "Failed to get twin: twin topic not subscribed \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_FAILED;
    }
    else
    {
        prvRequestIdGet( xAzureIoTHubClientHandle, request_id_span, false, NULL, &request_id_span );

        if( az_result_failed( res = az_iot_hub_client_twin_document_get_publish_topic( &xAzureIoTHubClientHandle->_internal.iot_hub_client_core,
                                                                                       request_id_span,
                                                                                       ( char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer,
                                                                                       xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer_length,
                                                                                       &device_twin_get_topic_length ) ) )
        {
            AZLogError( ( "Failed to get twin document topic: %08x\r\n", res ) );
            ret = AZURE_IOT_HUB_CLIENT_FAILED;
        }
        else
        {
            xMQTTPublishInfo.pTopicName = ( const char * ) xAzureIoTHubClientHandle->_internal.iot_hub_client_scratch_buffer;
            xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
            xMQTTPublishInfo.topicNameLength = ( uint16_t ) device_twin_get_topic_length;

            /* Get a unique packet id. */
            usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ) );

            if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTHubClientHandle->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to Publish get twin message. Error=%d \r\n",
                              xResult ) );
                ret = AZURE_IOT_HUB_CLIENT_INIT_FAILED;
            }
            else
            {
                ret = AZURE_IOT_HUB_CLIENT_SUCCESS;
            }
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_CloudMessageUnsubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to disable cloud message: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_C2D ];
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
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DirectMethodUnsubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to disable direct method: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_METHODS ];
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
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinUnsubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription[ 2 ];
    AzureIoTMQTTStatus_t xResult;
    AzureIoTHubClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * context;

    if( xAzureIoTHubClientHandle == NULL )
    {
        AZLogError( ( "Failed to disable device twin: Invalid argument \r\n" ) );
        ret = AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        context = &xAzureIoTHubClientHandle->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ];
        memset( mqttSubscription, 0, sizeof( mqttSubscription ) );
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
    }

    return ret;
}
/*-----------------------------------------------------------*/
