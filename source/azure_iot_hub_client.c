/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client.c
 * @brief Implementation of the Azure IoT Hub Client.
 */

#include "azure_iot_hub_client.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "azure_iot_mqtt.h"
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
#define azureiothubTOPIC_SUBSCRIBE_STATE_NONE      ( 0x0 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUB       ( 0x1 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK    ( 0x2 )

/*-----------------------------------------------------------*/

/**
 *
 * Handle any incoming publish messages.
 *
 * */
static void prvMQTTProcessIncomingPublish( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                           AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    uint32_t ulIndex;
    AzureIoTHubClientReceiveContext_t * pxContext;

    configASSERT( pxPublishInfo != NULL );

    for( ulIndex = 0; ulIndex < azureiothubSUBSCRIBE_FEATURE_COUNT; ulIndex++ )
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ ulIndex ];

        if( ( pxContext->_internal.pxProcessFunction != NULL ) &&
            ( pxContext->_internal.pxProcessFunction( pxContext,
                                                      pxAzureIoTHubClient,
                                                      ( void * ) pxPublishInfo ) == eAzureIoTHubClientSuccess ) )
        {
            break;
        }
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Handle any incoming suback messages.
 *
 * */
static void prvMQTTProcessSuback( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                  AzureIoTMQTTPacketInfo_t * pxIncomingPacket,
                                  uint16_t usPacketId )
{
    uint32_t ulIndex;
    AzureIoTHubClientReceiveContext_t * pxContext;

    configASSERT( pxIncomingPacket != NULL );

    for( ulIndex = 0; ulIndex < azureiothubSUBSCRIBE_FEATURE_COUNT; ulIndex++ )
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ ulIndex ];

        if( pxContext->_internal.usMqttSubPacketID == usPacketId )
        {
            /* TODO: inspect packet to see is ack was successful*/
            pxContext->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK;
            break;
        }
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Process incoming MQTT events.
 *
 * */
static void prvEventCallback( AzureIoTMQTTHandle_t pxMQTTContext,
                              AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                              AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTHubClient_t * pxAzureIoTHubClient = ( AzureIoTHubClient_t * ) pxMQTTContext;

    if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxAzureIoTHubClient, pxDeserializedInfo->pxPublishInfo );
    }
    else if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_SUBACK )
    {
        prvMQTTProcessSuback( pxAzureIoTHubClient, pxPacketInfo, pxDeserializedInfo->usPacketIdentifier );
    }
    else
    {
        AZLogDebug( ( "AzureIoTHubClient received packet of type: %d", pxPacketInfo->ucType ) );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Check/Process messages for incoming Cloud to Device messages.
 *
 * */
static uint32_t prvAzureIoTHubClientC2DProcess( AzureIoTHubClientReceiveContext_t * pxContext,
                                                AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                void * pxPublishInfo )
{
    AzureIoTHubClientCloudToDeviceMessageRequest_t xCloudToDeviceMessage = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_c2d_request xOutEmbeddedRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );

    /* Failed means no topic match */
    if( az_result_failed( az_iot_hub_client_c2d_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                      xTopicSpan, &xOutEmbeddedRequest ) ) )
    {
        return eAzureIoTHubClientTopicNoMatch;
    }

    /* Verify the received publish is for the feature we have subscribed to. */
    AZLogDebug( ( "Cloud xCloudToDeviceMessage topic: %.*s  with payload : %.*s.",
                  xMQTTPublishInfo->usTopicNameLength,
                  xMQTTPublishInfo->pcTopicName,
                  xMQTTPublishInfo->xPayloadLength,
                  xMQTTPublishInfo->pvPayload ) );

    if( pxContext->_internal.callbacks.xCloudToDeviceMessageCallback )
    {
        xCloudToDeviceMessage.pvMessagePayload = xMQTTPublishInfo->pvPayload;
        xCloudToDeviceMessage.ulPayloadLength = ( uint32_t ) xMQTTPublishInfo->xPayloadLength;
        xCloudToDeviceMessage.xProperties._internal.xProperties = xOutEmbeddedRequest.properties;
        pxContext->_internal.callbacks.xCloudToDeviceMessageCallback( &xCloudToDeviceMessage,
                                                                      pxContext->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

/**
 *
 * Check/Process messages for incoming direct method messages.
 *
 * */
static uint32_t prvAzureIoTHubClientDirectMethodProcess( AzureIoTHubClientReceiveContext_t * pxContext,
                                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         void * pxPublishInfo )
{
    AzureIoTHubClientMethodRequest_t xMethodRequest = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_method_request xOutEmbeddedRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );

    /* Failed means no topic match */
    if( az_result_failed( az_iot_hub_client_methods_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                          xTopicSpan, &xOutEmbeddedRequest ) ) )
    {
        return eAzureIoTHubClientTopicNoMatch;
    }

    /* Verify the received publish is for the feature we have subscribed to. */
    AZLogDebug( ( "Direct method topic: %.*s  with method payload : %.*s.",
                  xMQTTPublishInfo->usTopicNameLength,
                  xMQTTPublishInfo->pcTopicName,
                  xMQTTPublishInfo->xPayloadLength,
                  xMQTTPublishInfo->pvPayload ) );

    if( pxContext->_internal.callbacks.xMethodCallback )
    {
        xMethodRequest.pvMessagePayload = xMQTTPublishInfo->pvPayload;
        xMethodRequest.ulPayloadLength = ( uint32_t ) xMQTTPublishInfo->xPayloadLength;
        xMethodRequest.pucMethodName = az_span_ptr( xOutEmbeddedRequest.name );
        xMethodRequest.usMethodNameLength = ( uint16_t ) az_span_size( xOutEmbeddedRequest.name );
        xMethodRequest.pucRequestID = az_span_ptr( xOutEmbeddedRequest.request_id );
        xMethodRequest.usRequestIDLength = ( uint16_t ) az_span_size( xOutEmbeddedRequest.request_id );
        pxContext->_internal.callbacks.xMethodCallback( &xMethodRequest, pxContext->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

/**
 *
 * Check/Process messages for incoming device twin messages.
 *
 * */
static uint32_t prvAzureIoTHubClientDeviceTwinProcess( AzureIoTHubClientReceiveContext_t * pxContext,
                                                       AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                       void * pxPublishInfo )
{
    AzureIoTHubClientTwinResponse_t xTwinResponse = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pxPublishInfo;
    az_iot_hub_client_twin_response xOutRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );
    uint32_t ulRequestID = 0;

    /* Failed means no topic match */
    if( az_result_failed( az_iot_hub_client_twin_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                       xTopicSpan, &xOutRequest ) ) )
    {
        return eAzureIoTHubClientTopicNoMatch;
    }

    /* Verify the received publish is for the feature we have subscribed to. */
    AZLogDebug( ( "Twin topic: %.*s. with payload : %.*s.",
                  xMQTTPublishInfo->usTopicNameLength,
                  xMQTTPublishInfo->pcTopicName,
                  xMQTTPublishInfo->xPayloadLength,
                  xMQTTPublishInfo->pvPayload ) );

    if( pxContext->_internal.callbacks.xTwinCallback )
    {
        if( az_span_size( xOutRequest.request_id ) == 0 )
        {
            xTwinResponse.messageType = eAzureIoTHubTwinDesiredPropertyMessage;
        }
        else
        {
            if( az_result_failed( az_span_atou32( xOutRequest.request_id, &ulRequestID ) ) )
            {
                return eAzureIoTHubClientFailed;
            }

            if( ulRequestID & 0x01 )
            {
                xTwinResponse.messageType = eAzureIoTHubTwinReportedResponseMessage;
            }
            else
            {
                xTwinResponse.messageType = eAzureIoTHubTwinGetMessage;
            }
        }

        xTwinResponse.pvMessagePayload = xMQTTPublishInfo->pvPayload;
        xTwinResponse.ulPayloadLength = ( uint32_t ) xMQTTPublishInfo->xPayloadLength;
        xTwinResponse.messageStatus = xOutRequest.status;
        xTwinResponse.pucVersion = az_span_ptr( xOutRequest.version );
        xTwinResponse.usVersionLength = ( uint16_t ) az_span_size( xOutRequest.version );
        xTwinResponse.ulRequestID = ulRequestID;
        pxContext->_internal.callbacks.xTwinCallback( &xTwinResponse,
                                                      pxContext->_internal.pvCallbackContext );
    }

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

/**
 *
 * Time callback for MQTT initialization.
 *
 * */
static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount;
    uint32_t ulTimeMs;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * azureiotMILLISECONDS_PER_TICK;

    return ulTimeMs;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions )
{
    AzureIoTHubClientResult_t xResult;

    if( pxHubClientOptions == NULL )
    {
        AZLogError( ( "Failed to initialize options: Invalid argument." ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxHubClientOptions->pucModelID = NULL;
        pxHubClientOptions->ulModelIDLength = 0;
        pxHubClientOptions->pucModuleID = NULL;
        pxHubClientOptions->ulModuleIDLength = 0;
        pxHubClientOptions->pucUserAgent = ( const uint8_t * ) azureiothubUSER_AGENT;
        pxHubClientOptions->ulUserAgentLength = sizeof( azureiothubUSER_AGENT ) - 1;
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
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
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTResult_t xMQTTResult;
    az_iot_hub_client_options xHubOptions;
    uint8_t * pucNetworkBuffer;
    uint32_t ulNetworkBufferLength;
    az_span xHostnameSpan;
    az_span xDeviceIDSpan;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( pucHostname == NULL ) || ( ulHostnameLength == 0 ) ||
        ( pucDeviceId == NULL ) || ( ulDeviceIdLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "Failed to initialize: Invalid argument." ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( ( ulBufferLength < azureiotTOPIC_MAX ) ||
             ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "Failed to initialize: Not enough memory passed." ) );
        xResult = eAzureIoTHubClientOutOfMemory;
    }
    else
    {
        memset( ( void * ) pxAzureIoTHubClient, 0, sizeof( AzureIoTHubClient_t ) );

        /* Setup working buffer to be used by middleware */
        pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength =
            ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) > azureiotTOPIC_MAX ? ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) : azureiotTOPIC_MAX;
        pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer = pucBuffer;
        pucNetworkBuffer = pucBuffer + pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength;
        ulNetworkBufferLength = ulBufferLength - pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength;

        /* Initialize Azure IoT Hub Client */
        xHostnameSpan = az_span_create( ( uint8_t * ) pucHostname, ( int32_t ) ulHostnameLength );
        xDeviceIDSpan = az_span_create( ( uint8_t * ) pucDeviceId, ( int32_t ) ulDeviceIdLength );

        xHubOptions = az_iot_hub_client_options_default();

        if( pxHubClientOptions )
        {
            xHubOptions.module_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pucModuleID, ( int32_t ) pxHubClientOptions->ulModuleIDLength );
            xHubOptions.model_id = az_span_create( ( uint8_t * ) pxHubClientOptions->pucModelID, ( int32_t ) pxHubClientOptions->ulModelIDLength );
            xHubOptions.user_agent = az_span_create( ( uint8_t * ) pxHubClientOptions->pucUserAgent, ( int32_t ) pxHubClientOptions->ulUserAgentLength );
        }
        else
        {
            xHubOptions.user_agent = az_span_create( ( uint8_t * ) azureiothubUSER_AGENT, sizeof( azureiothubUSER_AGENT ) - 1 );
        }

        if( az_result_failed( az_iot_hub_client_init( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                      xHostnameSpan, xDeviceIDSpan, &xHubOptions ) ) )
        {
            AZLogError( ( "Failed to initialize az_iot_hub_client_init." ) );
            xResult = eAzureIoTHubClientInitFailed;
        }
        /* Initialize AzureIoTMQTT library. */
        else if( ( xMQTTResult = AzureIoTMQTT_Init( &( pxAzureIoTHubClient->_internal.xMQTTContext ), pxTransportInterface,
                                                    prvGetTimeMs, prvEventCallback,
                                                    pucNetworkBuffer, ulNetworkBufferLength ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to initialize AzureIoTMQTT_Init: 0x%08x.", xMQTTResult ) );
            xResult = eAzureIoTHubClientInitFailed;
        }
        else
        {
            pxAzureIoTHubClient->_internal.pucDeviceID = pucDeviceId;
            pxAzureIoTHubClient->_internal.ulDeviceIDLength = ulDeviceIdLength;
            pxAzureIoTHubClient->_internal.pucHostname = pucHostname;
            pxAzureIoTHubClient->_internal.ulHostnameLength = ulHostnameLength;
            pxAzureIoTHubClient->_internal.xAzureIoTHubClientTimeFunction = xGetTimeFunction;
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

void AzureIoTHubClient_Deinit( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    ( void ) pxAzureIoTHubClient;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Connect( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                     bool xCleanSession,
                                                     uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTResult_t xMQTTResult;
    bool xSessionPresent;
    uint32_t ulPasswordLength = 0;
    size_t xMQTTUserNameLength;
    az_result xCoreResult;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to connect: Invalid argument." ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        /* Use working buffer for username/password */
        xConnectInfo.pcUserName = ( const char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer;
        xConnectInfo.pcPassword = xConnectInfo.pcUserName + azureiotUSERNAME_MAX;

        if( az_result_failed( xCoreResult = az_iot_hub_client_get_user_name( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                             ( char * ) xConnectInfo.pcUserName, azureiotUSERNAME_MAX,
                                                                             &xMQTTUserNameLength ) ) )
        {
            AZLogError( ( "Failed to get username: 0x%08x.", xCoreResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        /* Check if token refresh is set, then generate password */
        else if( ( pxAzureIoTHubClient->_internal.pxAzureIoTHubClientTokenRefresh ) &&
                 ( pxAzureIoTHubClient->_internal.pxAzureIoTHubClientTokenRefresh( pxAzureIoTHubClient,
                                                                                   pxAzureIoTHubClient->_internal.xAzureIoTHubClientTimeFunction() + azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                                                   pxAzureIoTHubClient->_internal.pucAzureIoTHubClientSymmetricKey,
                                                                                   pxAzureIoTHubClient->_internal.ulAzureIoTHubClientSymmetricKeyLength,
                                                                                   ( uint8_t * ) xConnectInfo.pcPassword, azureiotPASSWORD_MAX,
                                                                                   &ulPasswordLength ) ) )
        {
            AZLogError( ( "Failed to generate SAS token." ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xConnectInfo.ucCleanSession = xCleanSession;
            xConnectInfo.pcClientIdentifier = ( const char * ) pxAzureIoTHubClient->_internal.pucDeviceID;
            xConnectInfo.usClientIdentifierLength = ( uint16_t ) pxAzureIoTHubClient->_internal.ulDeviceIDLength;
            xConnectInfo.usUserNameLength = ( uint16_t ) xMQTTUserNameLength;
            xConnectInfo.usKeepAliveSeconds = azureiothubKEEP_ALIVE_TIMEOUT_SECONDS;
            xConnectInfo.usPasswordLength = ( uint16_t ) ulPasswordLength;

            /* Send MQTT CONNECT packet to broker. Last Will and Testament is not used. */
            if( ( xMQTTResult = AzureIoTMQTT_Connect( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xConnectInfo,
                                                      NULL,
                                                      ulTimeoutMilliseconds,
                                                      &xSessionPresent ) ) != eAzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d.",
                              pxAzureIoTHubClient->_internal.ulHostnameLength, pxAzureIoTHubClient->_internal.pucHostname,
                              xMQTTResult ) );
                xResult = eAzureIoTHubClientFailed;
            }
            else
            {
                /* Successfully established a MQTT connection with the broker. */
                AZLogInfo( ( "An MQTT connection is established with %.*s.", pxAzureIoTHubClient->_internal.ulHostnameLength,
                             ( const char * ) pxAzureIoTHubClient->_internal.pucHostname ) );
                xResult = eAzureIoTHubClientSuccess;
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Disconnect( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to disconnect: Invalid argument." ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( ( xMQTTResult = AzureIoTMQTT_Disconnect( &( pxAzureIoTHubClient->_internal.xMQTTContext ) ) ) != eAzureIoTMQTTSuccess )
    {
        AZLogError( ( "Failed to disconnect: %d.", xMQTTResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        AZLogInfo( ( "Disconnecting the MQTT connection with %.*s.", pxAzureIoTHubClient->_internal.ulHostnameLength,
                     ( const char * ) pxAzureIoTHubClient->_internal.pucHostname ) );
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SendTelemetry( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                           const uint8_t * pucTelemetryData,
                                                           uint32_t ulTelemetryDataLength,
                                                           AzureIoTMessageProperties_t * pxProperties )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier;
    size_t xTelemetryTopicLength;
    az_result xCoreResult;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "Failed to send telemetry: Invalid argument." ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( az_result_failed( xCoreResult = az_iot_hub_client_telemetry_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                            ( pxProperties != NULL ) ? &pxProperties->_internal.xProperties : NULL,
                                                                                            ( char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer,
                                                                                            pxAzureIoTHubClient->_internal.ulAzureIoTHubClientWorkingBufferLength,
                                                                                            &xTelemetryTopicLength ) ) )
    {
        AZLogError( ( "Failed to get telemetry topic : 0x%08x.", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS1;
        xMQTTPublishInfo.pcTopicName = ( const char * ) pxAzureIoTHubClient->_internal.pucAzureIoTHubClientWorkingBuffer;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xTelemetryTopicLength;
        xMQTTPublishInfo.pvPayload = ( const void * ) pucTelemetryData;
        xMQTTPublishInfo.xPayloadLength = ulTelemetryDataLength;

        /* Get a unique packet id. */
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        /* Send PUBLISH packet. */
        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to publish telementry : Error=%d.", xMQTTResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
