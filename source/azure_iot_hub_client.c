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

#include "azure_iot_version.h"
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
    #define azureiothubUSER_AGENT    "DeviceClientType=c%2F" azureiotVERSION_STRING "%28FreeRTOS%29"
#endif /* azureiothubUSER_AGENT */

/*
 * Topic subscribe state
 */
#define azureiothubTOPIC_SUBSCRIBE_STATE_NONE       ( 0x0 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUB        ( 0x1 )
#define azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK     ( 0x2 )

/*
 * Indexes of the receive context buffer for each feature
 */
#define azureiothubRECEIVE_CONTEXT_INDEX_C2D        ( 0 )
#define azureiothubRECEIVE_CONTEXT_INDEX_METHODS    ( 1 )
#define azureiothubRECEIVE_CONTEXT_INDEX_TWIN       ( 2 )

#define azureiothubMETHOD_EMPTY_RESPONSE            "{}"

#define azureiothubMAX_SIZE_FOR_UINT32              ( 10 )
#define azureiothubHMACBufferLength                 ( 48 )
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

    /* If reached the end of the list and haven't found a context, log none found */
    if( ulIndex == azureiothubSUBSCRIBE_FEATURE_COUNT )
    {
        AZLogInfo( ( "No receive context found for incoming publish on topic: %.*s",
                     pxPublishInfo->usTopicNameLength, pxPublishInfo->pcTopicName ) );
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

    ( void ) pxIncomingPacket;

    configASSERT( pxIncomingPacket != NULL );
    configASSERT( ( azureiotmqttGET_PACKET_TYPE( pxIncomingPacket->ucType ) ) == azureiotmqttPACKET_TYPE_SUBACK );

    for( ulIndex = 0; ulIndex < azureiothubSUBSCRIBE_FEATURE_COUNT; ulIndex++ )
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ ulIndex ];

        if( pxContext->_internal.usMqttSubPacketID == usPacketId )
        {
            /* We assume success since IoT Hub would disconnect if there was a problem subscribing. */
            pxContext->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK;
            AZLogInfo( ( "Suback receive context found: 0x%08x", ulIndex ) );
            break;
        }
    }

    /* If reached the end of the list and haven't found a context, log none found */
    if( ulIndex == azureiothubSUBSCRIBE_FEATURE_COUNT )
    {
        AZLogInfo( ( "No receive context found for incoming suback" ) );
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

    if( ( azureiotmqttGET_PACKET_TYPE( pxPacketInfo->ucType ) ) == azureiotmqttPACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxAzureIoTHubClient, pxDeserializedInfo->pxPublishInfo );
    }
    else if( ( azureiotmqttGET_PACKET_TYPE( pxPacketInfo->ucType ) ) == azureiotmqttPACKET_TYPE_SUBACK )
    {
        prvMQTTProcessSuback( pxAzureIoTHubClient, pxPacketInfo, pxDeserializedInfo->usPacketIdentifier );
    }
    else
    {
        AZLogDebug( ( "AzureIoTHubClient received packet of type: 0x%08x", pxPacketInfo->ucType ) );
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
                                                void * pvPublishInfo )
{
    AzureIoTHubClientResult_t xResult;
    AzureIoTHubClientCloudToDeviceMessageRequest_t xCloudToDeviceMessage = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pvPublishInfo;
    az_result xCoreResult;
    az_iot_hub_client_c2d_request xOutEmbeddedRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );

    /* Failed means no topic match. This means the message is not for cloud to device messaging. */
    xCoreResult = az_iot_hub_client_c2d_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                              xTopicSpan, &xOutEmbeddedRequest );

    if( az_result_failed( xCoreResult ) )
    {
        xResult = eAzureIoTHubClientTopicNoMatch;
    }
    else
    {
        AZLogDebug( ( "Cloud xCloudToDeviceMessage topic: %.*s  with payload : %.*s",
                      xMQTTPublishInfo->usTopicNameLength,
                      xMQTTPublishInfo->pcTopicName,
                      xMQTTPublishInfo->xPayloadLength,
                      xMQTTPublishInfo->pvPayload ) );

        if( pxContext->_internal.callbacks.xCloudToDeviceMessageCallback )
        {
            xCloudToDeviceMessage.pvMessagePayload = xMQTTPublishInfo->pvPayload;
            xCloudToDeviceMessage.ulPayloadLength = ( uint32_t ) xMQTTPublishInfo->xPayloadLength;
            xCloudToDeviceMessage.xProperties._internal.xProperties = xOutEmbeddedRequest.properties;

            AZLogDebug( ( "Invoking Cloud to Device callback" ) );
            pxContext->_internal.callbacks.xCloudToDeviceMessageCallback( &xCloudToDeviceMessage,
                                                                          pxContext->_internal.pvCallbackContext );
            AZLogDebug( ( "Returned from Cloud to Device callback" ) );

            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 *
 * Check/Process messages for incoming direct method messages.
 *
 * */
static uint32_t prvAzureIoTHubClientDirectMethodProcess( AzureIoTHubClientReceiveContext_t * pxContext,
                                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         void * pvPublishInfo )
{
    AzureIoTHubClientResult_t xResult;
    AzureIoTHubClientMethodRequest_t xMethodRequest = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pvPublishInfo;
    az_result xCoreResult;
    az_iot_hub_client_method_request xOutEmbeddedRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );

    /* Failed means no topic match. This means the message is not for direct method. */
    xCoreResult = az_iot_hub_client_methods_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                  xTopicSpan, &xOutEmbeddedRequest );

    if( az_result_failed( xCoreResult ) )
    {
        xResult = eAzureIoTHubClientTopicNoMatch;
    }
    else
    {
        AZLogDebug( ( "Direct method topic: %.*s  with method payload : %.*s",
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

            AZLogDebug( ( "Invoking method callback" ) );
            pxContext->_internal.callbacks.xMethodCallback( &xMethodRequest, pxContext->_internal.pvCallbackContext );
            AZLogDebug( ( "Returned from method callback" ) );

            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 *
 * Check/Process messages for incoming device twin messages.
 *
 * */
static uint32_t prvAzureIoTHubClientDeviceTwinProcess( AzureIoTHubClientReceiveContext_t * pxContext,
                                                       AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                       void * pvPublishInfo )
{
    AzureIoTHubClientResult_t xResult;
    AzureIoTHubClientTwinResponse_t xTwinResponse = { 0 };
    AzureIoTMQTTPublishInfo_t * xMQTTPublishInfo = ( AzureIoTMQTTPublishInfo_t * ) pvPublishInfo;
    az_result xCoreResult;
    az_iot_hub_client_twin_response xOutRequest;
    az_span xTopicSpan = az_span_create( ( uint8_t * ) xMQTTPublishInfo->pcTopicName, xMQTTPublishInfo->usTopicNameLength );
    uint32_t ulRequestID = 0;

    /* Failed means no topic match. This means the message is not for twin messaging. */
    xCoreResult = az_iot_hub_client_twin_parse_received_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                               xTopicSpan, &xOutRequest );

    if( az_result_failed( xCoreResult ) )
    {
        xResult = eAzureIoTHubClientTopicNoMatch;
    }
    else
    {
        AZLogDebug( ( "Twin topic: %.*s. with payload : %.*s",
                      xMQTTPublishInfo->usTopicNameLength,
                      xMQTTPublishInfo->pcTopicName,
                      xMQTTPublishInfo->xPayloadLength,
                      xMQTTPublishInfo->pvPayload ) );

        xResult = eAzureIoTHubClientSuccess;

        if( pxContext->_internal.callbacks.xTwinCallback )
        {
            if( az_span_size( xOutRequest.request_id ) == 0 )
            {
                xTwinResponse.xMessageType = eAzureIoTHubTwinDesiredPropertyMessage;
            }
            else
            {
                if( az_result_succeeded( xCoreResult = az_span_atou32( xOutRequest.request_id, &ulRequestID ) ) )
                {
                    if( ulRequestID & 0x01 )
                    {
                        xTwinResponse.xMessageType = eAzureIoTHubTwinReportedResponseMessage;
                    }
                    else
                    {
                        xTwinResponse.xMessageType = eAzureIoTHubTwinGetMessage;
                    }
                }
                else
                {
                    /* Failed to parse the message */
                    AZLogError( ( "Request ID parsing failed: core error=0x%08x", xCoreResult ) );
                    xResult = eAzureIoTHubClientFailed;
                }
            }

            if( xResult == eAzureIoTHubClientSuccess )
            {
                xTwinResponse.pvMessagePayload = xMQTTPublishInfo->pvPayload;
                xTwinResponse.ulPayloadLength = ( uint32_t ) xMQTTPublishInfo->xPayloadLength;
                xTwinResponse.xMessageStatus = xOutRequest.status;
                xTwinResponse.pucVersion = az_span_ptr( xOutRequest.version );
                xTwinResponse.usVersionLength = ( uint16_t ) az_span_size( xOutRequest.version );
                xTwinResponse.ulRequestID = ulRequestID;

                AZLogDebug( ( "Invoking twin callback" ) );
                pxContext->_internal.callbacks.xTwinCallback( &xTwinResponse,
                                                              pxContext->_internal.pvCallbackContext );
                AZLogDebug( ( "Returning from twin callback" ) );
            }
        }
    }

    return xResult;
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

/**
 * Get the next request Id available. Currently we are using
 * odd for TwinReported property and even for TwinGet.
 *
 * */
static AzureIoTHubClientResult_t prvGetTwinRequestId( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                      az_span xSpan,
                                                      bool xOddSeq,
                                                      uint32_t * pulRequestId,
                                                      az_span * pxSpan )
{
    AzureIoTHubClientResult_t xResult;
    az_span xRemainder;
    az_result xCoreResult;

    /* Check the last request Id used, and do increment of 2 or 1 based on xOddSeq */
    if( xOddSeq == ( bool ) ( pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID & 0x01 ) )
    {
        pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID += 2;
    }
    else
    {
        pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID += 1;
    }

    if( pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID == 0 )
    {
        pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID = 2;
    }

    xCoreResult = az_span_u32toa( xSpan, pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID, &xRemainder );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "Couldn't convert request id to span" ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        *pxSpan = az_span_slice( *pxSpan, 0, az_span_size( *pxSpan ) - az_span_size( xRemainder ) );

        if( pulRequestId )
        {
            *pulRequestId = pxAzureIoTHubClient->_internal.ulCurrentTwinRequestID;
        }

        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 * Do blocking wait for sub-ack of particular receive context.
 *
 **/
static AzureIoTHubClientResult_t prvWaitForSubAck( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   AzureIoTHubClientReceiveContext_t * pxContext,
                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTHubClientResult_t xResult = eAzureIoTHubClientSubackWaitTimeout;
    uint32_t ulWaitTime;

    AZLogDebug( ( "Waiting for sub ack id: %d", pxContext->_internal.usMqttSubPacketID ) );

    do
    {
        if( pxContext->_internal.usState == azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
        {
            xResult = eAzureIoTHubClientSuccess;
            break;
        }

        if( ulTimeoutMilliseconds > azureiothubSUBACK_WAIT_INTERVAL_MS )
        {
            ulTimeoutMilliseconds -= azureiothubSUBACK_WAIT_INTERVAL_MS;
            ulWaitTime = azureiothubSUBACK_WAIT_INTERVAL_MS;
        }
        else
        {
            ulWaitTime = ulTimeoutMilliseconds;
            ulTimeoutMilliseconds = 0;
        }

        if( AzureIoTMQTT_ProcessLoop( &pxAzureIoTHubClient->_internal.xMQTTContext, ulWaitTime ) != eAzureIoTMQTTSuccess )
        {
            xResult = eAzureIoTHubClientFailed;
            break;
        }
    } while( ulTimeoutMilliseconds );

    if( pxContext->_internal.usState == azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    AZLogDebug( ( "Done waiting for sub ack id: %d, result: 0x%08x",
                  pxContext->_internal.usMqttSubPacketID, xResult ) );

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 * Generate the SAS token based on :
 *   https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security#use-a-shared-access-policy
 **/
static uint32_t prvIoTHubClientGetToken( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                         uint64_t ullExpiryTimeSecs,
                                         const uint8_t * ucKey,
                                         uint32_t ulKeyLen,
                                         uint8_t * pucSASBuffer,
                                         uint32_t ulSasBufferLen,
                                         uint32_t * pulSaSLength )
{
    uint8_t * pucHMACBuffer;
    az_span xSpan = az_span_create( pucSASBuffer, ( int32_t ) ulSasBufferLen );
    az_result xCoreResult;
    uint32_t ulSignatureLength;
    uint32_t ulBytesUsed;
    uint32_t ulBufferLeft;
    size_t xLength;

    xCoreResult = az_iot_hub_client_sas_get_signature( &( pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore ),
                                                       ullExpiryTimeSecs, xSpan, &xSpan );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "AzureIoTHubClient failed to get signature: core error=0x%08x", xCoreResult ) );
        return eAzureIoTHubClientFailed;
    }

    ulBytesUsed = ( uint32_t ) az_span_size( xSpan );
    ulBufferLeft = ulSasBufferLen - ulBytesUsed;

    if( ulBufferLeft < azureiothubHMACBufferLength )
    {
        AZLogError( ( "AzureIoTHubClient token generation failed with insufficient buffer size" ) );
        return eAzureIoTHubClientOutOfMemory;
    }

    /* Calculate HMAC at the end of buffer, so we do less data movement when returning back to caller. */
    ulBufferLeft -= azureiothubHMACBufferLength;
    pucHMACBuffer = pucSASBuffer + ulSasBufferLen - azureiothubHMACBufferLength;

    if( AzureIoT_Base64HMACCalculate( pxAzureIoTHubClient->_internal.xHMACFunction,
                                      ucKey, ulKeyLen, pucSASBuffer, ulBytesUsed, pucSASBuffer + ulBytesUsed, ulBufferLeft,
                                      pucHMACBuffer, azureiothubHMACBufferLength,
                                      &ulSignatureLength ) != eAzureIoTSuccess )
    {
        AZLogError( ( "AzureIoTHubClient failed to encode HMAC hash" ) );
        return eAzureIoTHubClientFailed;
    }

    xSpan = az_span_create( pucHMACBuffer, ( int32_t ) ulSignatureLength );
    xCoreResult = az_iot_hub_client_sas_get_password( &( pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore ),
                                                      ullExpiryTimeSecs, xSpan, AZ_SPAN_EMPTY, ( char * ) pucSASBuffer,
                                                      ulSasBufferLen - azureiothubHMACBufferLength,
                                                      &xLength );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "AzureIoTHubClient failed to generate token: core error=0x%08x", xCoreResult ) );
        return eAzureIoTHubClientFailed;
    }

    *pulSaSLength = ( uint32_t ) xLength;

    return eAzureIoTHubClientSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions )
{
    AzureIoTHubClientResult_t xResult;

    if( pxHubClientOptions == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_OptionsInit failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        memset( pxHubClientOptions, 0, sizeof( AzureIoTHubClientOptions_t ) );
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
    az_result xCoreResult;
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
        AZLogError( ( "AzureIoTHubClient_Init failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( ( ulBufferLength < azureiotTOPIC_MAX ) ||
             ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "AzureIoTHubClient_Init failed: not enough memory passed" ) );
        xResult = eAzureIoTHubClientOutOfMemory;
    }
    else
    {
        memset( ( void * ) pxAzureIoTHubClient, 0, sizeof( AzureIoTHubClient_t ) );

        /* Setup working buffer to be used by middleware */
        pxAzureIoTHubClient->_internal.ulWorkingBufferLength =
            ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) > azureiotTOPIC_MAX ? ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) : azureiotTOPIC_MAX;
        pxAzureIoTHubClient->_internal.pucWorkingBuffer = pucBuffer;
        pucNetworkBuffer = pucBuffer + pxAzureIoTHubClient->_internal.ulWorkingBufferLength;
        ulNetworkBufferLength = ulBufferLength - pxAzureIoTHubClient->_internal.ulWorkingBufferLength;

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

        if( az_result_failed( xCoreResult = az_iot_hub_client_init( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                    xHostnameSpan, xDeviceIDSpan, &xHubOptions ) ) )
        {
            AZLogError( ( "Failed to initialize az_iot_hub_client_init: core error=0x%08x", xCoreResult ) );
            xResult = eAzureIoTHubClientInitFailed;
        }
        /* Initialize AzureIoTMQTT library. */
        else if( ( xMQTTResult = AzureIoTMQTT_Init( &( pxAzureIoTHubClient->_internal.xMQTTContext ), pxTransportInterface,
                                                    prvGetTimeMs, prvEventCallback,
                                                    pucNetworkBuffer, ulNetworkBufferLength ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to initialize AzureIoTMQTT_Init: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientInitFailed;
        }
        else
        {
            pxAzureIoTHubClient->_internal.pucDeviceID = pucDeviceId;
            pxAzureIoTHubClient->_internal.ulDeviceIDLength = ulDeviceIdLength;
            pxAzureIoTHubClient->_internal.pucHostname = pucHostname;
            pxAzureIoTHubClient->_internal.ulHostnameLength = ulHostnameLength;
            pxAzureIoTHubClient->_internal.xTimeFunction = xGetTimeFunction;
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

AzureIoTHubClientResult_t AzureIoTHubClient_SetSymmetricKey( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                             const uint8_t * pucSymmetricKey,
                                                             uint32_t ulSymmetricKeyLength,
                                                             AzureIoTGetHMACFunc_t xHMACFunction )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( pucSymmetricKey == NULL ) || ( ulSymmetricKeyLength == 0 ) ||
        ( xHMACFunction == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_SetSymmetricKey failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxAzureIoTHubClient->_internal.pucSymmetricKey = pucSymmetricKey;
        pxAzureIoTHubClient->_internal.ulSymmetricKeyLength = ulSymmetricKeyLength;
        pxAzureIoTHubClient->_internal.pxTokenRefresh = prvIoTHubClientGetToken;
        pxAzureIoTHubClient->_internal.xHMACFunction = xHMACFunction;
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_Connect( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                     bool xCleanSession,
                                                     bool * pxOutSessionPresent,
                                                     uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTResult_t xMQTTResult;
    uint32_t ulPasswordLength = 0;
    size_t xMQTTUserNameLength;
    az_result xCoreResult;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxOutSessionPresent == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_Connect failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        /* Use working buffer for username/password */
        xConnectInfo.pcUserName = pxAzureIoTHubClient->_internal.pucWorkingBuffer;
        xConnectInfo.pcPassword = xConnectInfo.pcUserName + azureiotUSERNAME_MAX;

        if( az_result_failed( xCoreResult = az_iot_hub_client_get_user_name( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                             ( char * ) xConnectInfo.pcUserName, azureiotUSERNAME_MAX,
                                                                             &xMQTTUserNameLength ) ) )
        {
            AZLogError( ( "Failed to get username: core error=0x%08x", xCoreResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        /* Check if token refresh is set, then generate password */
        else if( ( pxAzureIoTHubClient->_internal.pxTokenRefresh ) &&
                 ( pxAzureIoTHubClient->_internal.pxTokenRefresh( pxAzureIoTHubClient,
                                                                  pxAzureIoTHubClient->_internal.xTimeFunction() +
                                                                  azureiothubDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                                  pxAzureIoTHubClient->_internal.pucSymmetricKey,
                                                                  pxAzureIoTHubClient->_internal.ulSymmetricKeyLength,
                                                                  ( uint8_t * ) xConnectInfo.pcPassword, azureiotPASSWORD_MAX,
                                                                  &ulPasswordLength ) ) )
        {
            AZLogError( ( "Failed to generate SAS token" ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xConnectInfo.xCleanSession = xCleanSession;
            xConnectInfo.pcClientIdentifier = pxAzureIoTHubClient->_internal.pucDeviceID;
            xConnectInfo.usClientIdentifierLength = ( uint16_t ) pxAzureIoTHubClient->_internal.ulDeviceIDLength;
            xConnectInfo.usUserNameLength = ( uint16_t ) xMQTTUserNameLength;
            xConnectInfo.usKeepAliveSeconds = azureiothubKEEP_ALIVE_TIMEOUT_SECONDS;
            xConnectInfo.usPasswordLength = ( uint16_t ) ulPasswordLength;

            /* Send MQTT CONNECT packet to broker. Last Will and Testament is not used. */
            if( ( xMQTTResult = AzureIoTMQTT_Connect( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xConnectInfo,
                                                      NULL,
                                                      ulTimeoutMilliseconds,
                                                      pxOutSessionPresent ) ) != eAzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTT error=0x%08x",
                              pxAzureIoTHubClient->_internal.ulHostnameLength, ( const char * ) pxAzureIoTHubClient->_internal.pucHostname,
                              xMQTTResult ) );
                xResult = eAzureIoTHubClientFailed;
            }
            else
            {
                /* Successfully established a MQTT connection with the broker. */
                AZLogInfo( ( "An MQTT connection is established with %.*s", pxAzureIoTHubClient->_internal.ulHostnameLength,
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
        AZLogError( ( "AzureIoTHubClient_Disconnect failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( ( xMQTTResult = AzureIoTMQTT_Disconnect( &( pxAzureIoTHubClient->_internal.xMQTTContext ) ) ) != eAzureIoTMQTTSuccess )
    {
        AZLogError( ( "AzureIoTHubClient_Disconnect failed to disconnect: MQTT error=0x%08x", xMQTTResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        AZLogInfo( ( "Disconnecting the MQTT connection with %.*s", pxAzureIoTHubClient->_internal.ulHostnameLength,
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
        AZLogError( ( "AzureIoTHubClient_SendTelemetry failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( az_result_failed( xCoreResult = az_iot_hub_client_telemetry_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                            ( pxProperties != NULL ) ? &pxProperties->_internal.xProperties : NULL,
                                                                                            ( char * ) pxAzureIoTHubClient->_internal.pucWorkingBuffer,
                                                                                            pxAzureIoTHubClient->_internal.ulWorkingBufferLength,
                                                                                            &xTelemetryTopicLength ) ) )
    {
        AZLogError( ( "Failed to get telemetry topic: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS1;
        xMQTTPublishInfo.pcTopicName = pxAzureIoTHubClient->_internal.pucWorkingBuffer;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xTelemetryTopicLength;
        xMQTTPublishInfo.pvPayload = ( const void * ) pucTelemetryData;
        xMQTTPublishInfo.xPayloadLength = ulTelemetryDataLength;

        /* Get a unique packet id. */
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        /* Send PUBLISH packet. */
        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to publish telemetry: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientPublishFailed;
        }
        else
        {
            AZLogInfo( ( "Successfully sent telemetry message" ) );
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_ProcessLoop failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( ( xMQTTResult = AzureIoTMQTT_ProcessLoop( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                       ulTimeoutMilliseconds ) ) != eAzureIoTMQTTSuccess )
    {
        AZLogError( ( "AzureIoTMQTT_ProcessLoop failed: ProcessLoopDuration=%u, MQTT error=0x%08x",
                      ulTimeoutMilliseconds, xMQTTResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeCloudToDeviceMessage( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                           AzureIoTHubClientCloudToDeviceMessageCallback_t xCallback,
                                                                           void * prvCallbackContext,
                                                                           uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( xCallback == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_SubscribeCloudToDeviceMessage failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_C2D ];
        xMqttSubscription.xQoS = eAzureIoTMQTTQoS1;
        xMqttSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
        xMqttSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to subscribe to the MQTT topic: %s", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Subscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                    &xMqttSubscription, 1, usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Cloud to device subscribe failed: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientSubscribeFailed;
        }
        else
        {
            pxContext->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            pxContext->_internal.usMqttSubPacketID = usSubscribePacketIdentifier;
            pxContext->_internal.pxProcessFunction = prvAzureIoTHubClientC2DProcess;
            pxContext->_internal.callbacks.xCloudToDeviceMessageCallback = xCallback;
            pxContext->_internal.pvCallbackContext = prvCallbackContext;

            if( ( xResult = prvWaitForSubAck( pxAzureIoTHubClient, pxContext,
                                              ulTimeoutMilliseconds ) ) != eAzureIoTHubClientSuccess )
            {
                AZLogError( ( "Wait for cloud to device sub ack failed : error=0x%08x", xResult ) );
                memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_UnsubscribeCloudToDeviceMessage failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_C2D ];
        xMqttSubscription.xQoS = eAzureIoTMQTTQoS1;
        xMqttSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC;
        xMqttSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to unsubscribe from the MQTT topic: %s", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Unsubscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xMqttSubscription, 1,
                                                      usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Cloud to device unsubscribe failed: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientUnsubscribeFailed;
        }
        else
        {
            memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeDirectMethod( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                   AzureIoTHubClientMethodCallback_t xCallback,
                                                                   void * prvCallbackContext,
                                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( xCallback == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_SubscribeDirectMethod failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_METHODS ];
        xMqttSubscription.xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
        xMqttSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to subscribe to the MQTT topic: %s", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Subscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                    &xMqttSubscription, 1,
                                                    usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Direct method subscribe failed: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientSubscribeFailed;
        }
        else
        {
            pxContext->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            pxContext->_internal.usMqttSubPacketID = usSubscribePacketIdentifier;
            pxContext->_internal.pxProcessFunction = prvAzureIoTHubClientDirectMethodProcess;
            pxContext->_internal.callbacks.xMethodCallback = xCallback;
            pxContext->_internal.pvCallbackContext = prvCallbackContext;

            if( ( xResult = prvWaitForSubAck( pxAzureIoTHubClient, pxContext,
                                              ulTimeoutMilliseconds ) ) != eAzureIoTHubClientSuccess )
            {
                AZLogError( ( "Wait for direct method sub ack failed: error=0x%08x", xResult ) );
                memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_UnsubscribeDirectMethod( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_UnsubscribeDirectMethod failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_METHODS ];
        xMqttSubscription.xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
        xMqttSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to unsubscribe from the MQTT topic: %s", AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Unsubscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xMqttSubscription, 1,
                                                      usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Direct method unsubscribe failed: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientUnsubscribeFailed;
        }
        else
        {
            memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                const AzureIoTHubClientMethodRequest_t * pxMessage,
                                                                uint32_t ulStatus,
                                                                const uint8_t * pucMethodPayload,
                                                                uint32_t ulMethodPayloadLength )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    az_span xRequestID;
    size_t xTopicLength;
    az_result xCoreResult;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( pxMessage == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_SendMethodResponse failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        xRequestID = az_span_create( ( uint8_t * ) pxMessage->pucRequestID, ( int32_t ) pxMessage->usRequestIDLength );

        if( az_result_failed( xCoreResult =
                                  az_iot_hub_client_methods_response_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                        xRequestID, ( uint16_t ) ulStatus,
                                                                                        ( char * ) pxAzureIoTHubClient->_internal.pucWorkingBuffer,
                                                                                        pxAzureIoTHubClient->_internal.ulWorkingBufferLength,
                                                                                        &xTopicLength ) ) )
        {
            AZLogError( ( "Failed to get method response topic: core error=0x%08x", xCoreResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
            xMQTTPublishInfo.pcTopicName = pxAzureIoTHubClient->_internal.pucWorkingBuffer;
            xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xTopicLength;

            if( ( pucMethodPayload == NULL ) || ( ulMethodPayloadLength == 0 ) )
            {
                xMQTTPublishInfo.pvPayload = ( const void * ) azureiothubMETHOD_EMPTY_RESPONSE;
                xMQTTPublishInfo.xPayloadLength = sizeof( azureiothubMETHOD_EMPTY_RESPONSE ) - 1;
            }
            else
            {
                xMQTTPublishInfo.pvPayload = ( const void * ) pucMethodPayload;
                xMQTTPublishInfo.xPayloadLength = ulMethodPayloadLength;
            }

            if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xMQTTPublishInfo, 0 ) ) != eAzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to publish response: MQTT error=0x%08x", xMQTTResult ) );
                xResult = eAzureIoTHubClientPublishFailed;
            }
            else
            {
                xResult = eAzureIoTHubClientSuccess;
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeDeviceTwin( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                 AzureIoTHubClientTwinCallback_t xCallback,
                                                                 void * prvCallbackContext,
                                                                 uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription[ 2 ] = { { 0 }, { 0 } };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( xCallback == NULL ) )
    {
        AZLogError( ( "AzureIoTHubClient_SubscribeDeviceTwin failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ];
        xMqttSubscription[ 0 ].xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription[ 0 ].pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
        xMqttSubscription[ 0 ].usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
        xMqttSubscription[ 1 ].xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription[ 1 ].pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
        xMqttSubscription[ 1 ].usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to subscribe to the MQTT topics: %s and %s",
                      AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Subscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                    xMqttSubscription, 2,
                                                    usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Twin subscribe failed: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientSubscribeFailed;
        }
        else
        {
            pxContext->_internal.usState = azureiothubTOPIC_SUBSCRIBE_STATE_SUB;
            pxContext->_internal.usMqttSubPacketID = usSubscribePacketIdentifier;
            pxContext->_internal.pxProcessFunction = prvAzureIoTHubClientDeviceTwinProcess;
            pxContext->_internal.callbacks.xTwinCallback = xCallback;
            pxContext->_internal.pvCallbackContext = prvCallbackContext;

            if( ( xResult = prvWaitForSubAck( pxAzureIoTHubClient, pxContext,
                                              ulTimeoutMilliseconds ) ) != eAzureIoTHubClientSuccess )
            {
                AZLogError( ( "Wait for twin sub ack failed: error=0x%08x", xResult ) );
                memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_UnsubscribeDeviceTwin( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTSubscribeInfo_t xMqttSubscription[ 2 ] = { { 0 }, { 0 } };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;
    AzureIoTHubClientReceiveContext_t * pxContext;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_UnsubscribeDeviceTwin failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        pxContext = &pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ];
        xMqttSubscription[ 0 ].xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription[ 0 ].pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
        xMqttSubscription[ 0 ].usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) - 1;
        xMqttSubscription[ 1 ].xQoS = eAzureIoTMQTTQoS0;
        xMqttSubscription[ 1 ].pcTopicFilter = ( const uint8_t * ) AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
        xMqttSubscription[ 1 ].usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) - 1;
        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        AZLogDebug( ( "Attempting to unsubscribe from MQTT topics: %s and %s",
                      AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Unsubscribe( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      xMqttSubscription, 2,
                                                      usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to unsubscribe: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTHubClientUnsubscribeFailed;
        }
        else
        {
            memset( pxContext, 0, sizeof( AzureIoTHubClientReceiveContext_t ) );
            xResult = eAzureIoTHubClientSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_SendDeviceTwinReported( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                    const uint8_t * pucReportedPayload,
                                                                    uint32_t ulReportedPayloadLength,
                                                                    uint32_t * pulRequestId )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint8_t ucRequestID[ azureiothubMAX_SIZE_FOR_UINT32 ];
    size_t xTopicLength;
    az_result xCoreResult;
    az_span xRequestID = az_span_create( ucRequestID, sizeof( ucRequestID ) );

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( pucReportedPayload == NULL ) || ( ulReportedPayloadLength == 0 ) )
    {
        AZLogError( ( "AzureIoTHubClient_SendDeviceTwinReported failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ]._internal.usState !=
             azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        AZLogError( ( "AzureIoTHubClient_SendDeviceTwinReported failed: twin topic not subscribed" ) );
        xResult = eAzureIoTHubClientTopicNotSubscribed;
    }
    else
    {
        if( ( xResult = prvGetTwinRequestId( pxAzureIoTHubClient, xRequestID,
                                             true, pulRequestId, &xRequestID ) ) != eAzureIoTHubClientSuccess )
        {
            AZLogError( ( "Failed to get request id: error=0x%08x", xResult ) );
        }
        else if( az_result_failed( xCoreResult =
                                       az_iot_hub_client_twin_patch_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                       xRequestID,
                                                                                       ( char * ) pxAzureIoTHubClient->_internal.pucWorkingBuffer,
                                                                                       pxAzureIoTHubClient->_internal.ulWorkingBufferLength,
                                                                                       &xTopicLength ) ) )
        {
            AZLogError( ( "Failed to get twin patch topic: core error=0x%08x", xCoreResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
            xMQTTPublishInfo.pcTopicName = pxAzureIoTHubClient->_internal.pucWorkingBuffer;
            xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xTopicLength;
            xMQTTPublishInfo.pvPayload = ( const void * ) pucReportedPayload;
            xMQTTPublishInfo.xPayloadLength = ulReportedPayloadLength;

            if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xMQTTPublishInfo, 0 ) ) != eAzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to Publish twin reported message: MQTT error=0x%08x", xMQTTResult ) );
                xResult = eAzureIoTHubClientPublishFailed;
            }
            else
            {
                xResult = eAzureIoTHubClientSuccess;
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTHubClientResult_t AzureIoTHubClient_GetDeviceTwin( AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTHubClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint8_t ucRequestID[ 10 ];
    az_span xRequestID = az_span_create( ( uint8_t * ) ucRequestID, sizeof( ucRequestID ) );
    size_t xTopicLength;
    az_result xCoreResult;

    if( pxAzureIoTHubClient == NULL )
    {
        AZLogError( ( "AzureIoTHubClient_GetDeviceTwin failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else if( pxAzureIoTHubClient->_internal.xReceiveContext[ azureiothubRECEIVE_CONTEXT_INDEX_TWIN ]._internal.usState !=
             azureiothubTOPIC_SUBSCRIBE_STATE_SUBACK )
    {
        AZLogError( ( "AzureIoTHubClient_GetDeviceTwin failed: twin topic not subscribed" ) );
        xResult = eAzureIoTHubClientTopicNotSubscribed;
    }
    else
    {
        if( ( xResult = prvGetTwinRequestId( pxAzureIoTHubClient, xRequestID,
                                             false, NULL, &xRequestID ) ) != eAzureIoTHubClientSuccess )
        {
            AZLogError( ( "Failed to get request id: error=0x%08x", xResult ) );
        }
        else if( az_result_failed( xCoreResult =
                                       az_iot_hub_client_twin_document_get_publish_topic( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                          xRequestID,
                                                                                          ( char * ) pxAzureIoTHubClient->_internal.pucWorkingBuffer,
                                                                                          pxAzureIoTHubClient->_internal.ulWorkingBufferLength,
                                                                                          &xTopicLength ) ) )
        {
            AZLogError( ( "Failed to get twin document topic: core error=0x%08x", xCoreResult ) );
            xResult = eAzureIoTHubClientFailed;
        }
        else
        {
            xMQTTPublishInfo.pcTopicName = pxAzureIoTHubClient->_internal.pucWorkingBuffer;
            xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
            xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xTopicLength;

            if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                      &xMQTTPublishInfo, 0 ) ) != eAzureIoTMQTTSuccess )
            {
                AZLogError( ( "Failed to Publish get twin message: MQTT error=0x%08x", xMQTTResult ) );
                xResult = eAzureIoTHubClientPublishFailed;
            }
            else
            {
                xResult = eAzureIoTHubClientSuccess;
            }
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/
