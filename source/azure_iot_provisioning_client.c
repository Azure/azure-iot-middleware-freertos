/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_provisioning_client.c
 * @brief Implementation of Azure IoT Provisioning client.
 *
 */

#include "azure_iot_provisioning_client.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* azure iot includes. */
#include "azure_iot_mqtt.h"
#include "azure/az_iot.h"
#include "azure/core/az_json.h"
#include "azure/core/az_version.h"

#ifndef azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC
    #define azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC    azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC
#endif /* azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC */

#ifndef azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS      azureiotKEEP_ALIVE_TIMEOUT_SECONDS
#endif /* azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS */

#ifndef azureiotprovisioningCONNACK_RECV_TIMEOUT_MS
    #define azureiotprovisioningCONNACK_RECV_TIMEOUT_MS         azureiotCONNACK_RECV_TIMEOUT_MS
#endif /* azureiotprovisioningCONNACK_RECV_TIMEOUT_MS */

#ifndef azureiotprovisioningUSER_AGENT
    #define azureiotprovisioningUSER_AGENT                      ""
#endif /* azureiotprovisioningUSER_AGENT */

#ifndef azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS
    #define azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS         ( 500U )
#endif /* azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS */

/* Define various workflow (WF) states Provisioning can get into */
#define azureiotprovisioningWF_STATE_INIT              ( 0x0 )
#define azureiotprovisioningWF_STATE_CONNECT           ( 0x1 )
#define azureiotprovisioningWF_STATE_SUBSCRIBE         ( 0x2 )
#define azureiotprovisioningWF_STATE_REQUEST           ( 0x3 )
#define azureiotprovisioningWF_STATE_RESPONSE          ( 0x4 )
#define azureiotprovisioningWF_STATE_SUBSCRIBING       ( 0x5 )
#define azureiotprovisioningWF_STATE_REQUESTING        ( 0x6 )
#define azureiotprovisioningWF_STATE_WAITING           ( 0x7 )
#define azureiotprovisioningWF_STATE_COMPLETE          ( 0x8 )

#define azureiotPrvGetMaxInt( a, b )    ( ( a ) > ( b ) ? ( a ) : ( b ) )

#define azureiotprovisioningREQUEST_PAYLOAD_LABEL           "payload"
#define azureiotprovisioningREQUEST_REGISTRATION_ID_LABEL   "registrationId"

#define azureiotprovisioningHMACBufferLength                ( 48 )
/*-----------------------------------------------------------*/

/**
 *
 * State transitions :
 *          CONNECT -> { SUBSCRIBE | COMPLETE } -> { SUBSCRIBING | COMPLETE } -> { REQUEST | COMPLETE } ->
 *          REQUESTING -> { RESPONSE | COMPLETE } -> { WAITING | COMPLETE } -> { REQUEST | COMPLETE }
 *
 * Note : At COMPLETE state, we will check the status of lastOperation to see if WorkFlow( WF )
 *        finshed without error.
 *
 **/
static void prvProvClientUpdateState( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                      uint32_t ulActionResult )
{
    uint32_t ulState = xProvClientHandle->_internal.ulWorkflowState;

    xProvClientHandle->_internal.ulLastOperationResult = ulActionResult;

    switch( ulState )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_SUBSCRIBE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_SUBSCRIBING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_REQUEST :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_REQUESTING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUESTING:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_RESPONSE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningPending ? azureiotprovisioningWF_STATE_WAITING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            xProvClientHandle->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_REQUEST :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_COMPLETE:
            AZLogInfo( ( "AzureIoTProvisioning state 'Complete'" ) );
            break;

        default:
            AZLogError( ( "AzureIoTProvisioning unknown state: %u", ulState ) );
            configASSERT( false );
            break;
    }

    AZLogDebug( ( "AzureIoTProvisioning updated state from [%d] -> [%d]", ulState,
                  xProvClientHandle->_internal.ulWorkflowState ) );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of the connect action. This action is only allowed in azureiotprovisioningWF_STATE_CONNECT
 *
 * */
static void prvProvClientConnect( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTProvisioningClientResult_t xResult;
    AzureIoTMQTTResult_t xMQTTResult;
    uint8_t ucSessionPresent;
    uint32_t ulPasswordLength = 0;
    size_t xMQTTUsernameLength;
    az_result xCoreResult;

    if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_CONNECT )
    {
        AZLogError( ( "AzureIoTProvisioning connect action called in wrong state (%d)",
                      xProvClientHandle->_internal.ulWorkflowState ) );
        return;
    }

    xConnectInfo.pcUserName = xProvClientHandle->_internal.pucScratchBuffer;
    xConnectInfo.pcPassword = xConnectInfo.pcUserName + azureiotUSERNAME_MAX;

    if( az_result_failed( xCoreResult = az_iot_provisioning_client_get_user_name( &xProvClientHandle->_internal.xProvisioningClientCore,
                                                                                  ( char * ) xConnectInfo.pcUserName,
                                                                                  azureiotUSERNAME_MAX, &xMQTTUsernameLength ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to get username (%08x)", xCoreResult ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    /* Check if token refresh is set, then generate password */
    else if( ( xProvClientHandle->_internal.pxTokenRefresh ) &&
             ( xProvClientHandle->_internal.pxTokenRefresh( xProvClientHandle,
                                                            xProvClientHandle->_internal.xGetTimeFunction() +
                                                            azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                            xProvClientHandle->_internal.pucSymmetricKey,
                                                            xProvClientHandle->_internal.ulSymmetricKeyLength,
                                                            ( uint8_t * ) xConnectInfo.pcPassword,
                                                            azureiotPASSWORD_MAX,
                                                            &ulPasswordLength ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to generate auth token" ) );
        xResult = eAzureIoTProvisioningTokenGenerationFailed;
    }
    else
    {
        xConnectInfo.ucCleanSession = true;
        xConnectInfo.pcClientIdentifier = xProvClientHandle->_internal.pucRegistrationID;
        xConnectInfo.usClientIdentifierLength = ( uint16_t ) xProvClientHandle->_internal.ulRegistrationIDLength;
        xConnectInfo.usUserNameLength = ( uint16_t ) xMQTTUsernameLength;
        xConnectInfo.usKeepAliveSeconds = azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS;
        xConnectInfo.usPasswordLength = ( uint16_t ) ulPasswordLength;

        if( ( xMQTTResult = AzureIoTMQTT_Connect( &( xProvClientHandle->_internal.xMQTTContext ),
                                                  &xConnectInfo, NULL, azureiotprovisioningCONNACK_RECV_TIMEOUT_MS,
                                                  &ucSessionPresent ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d ",
                          xProvClientHandle->_internal.ulEndpointLength,
                          xProvClientHandle->_internal.pucEndpoint,
                          xMQTTResult ) );
            xResult = eAzureIoTProvisioningServerError;
        }
        else
        {
            /* Successfully established and MQTT connection with the broker. */
            AZLogInfo( ( "AzureIoTProvisioning an MQTT connection is established with %.*s.",
                         xProvClientHandle->_internal.ulEndpointLength,
                         xProvClientHandle->_internal.pucEndpoint ) );
            xResult = eAzureIoTProvisioningSuccess;
        }
    }

    prvProvClientUpdateState( xProvClientHandle, xResult );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of subscribe action, this action is only allowed in azureiotprovisioningWF_STATE_SUBSCRIBE
 *
 * */
static void prvProvClientSubscribe( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t xMQTTSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTProvisioningClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;

    if ( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_SUBSCRIBE )
    {
        AZLogWarn( ( "AzureIoTProvisioning subscribe action called in wrong state %d",
                     xProvClientHandle->_internal.ulWorkflowState ) );
    }
    else
    {
        xMQTTSubscription.xQoS = eAzureIoTMQTTQoS0;
        xMQTTSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
        xMQTTSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC ) - 1;

        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xProvClientHandle->_internal.xMQTTContext ) );

        AZLogInfo( ( "AzureIoTProvisioning attempting to subscribe to the MQTT %s topic.", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Subscribe( &( xProvClientHandle->_internal.xMQTTContext ),
                                                    &xMQTTSubscription, 1, usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to SUBSCRIBE to MQTT topic. Error=%d", xMQTTResult ) );
            xResult = eAzureIoTProvisioningSubscribeFailed;
        }
        else
        {
            xResult = eAzureIoTProvisioningSuccess;
        }

        prvProvClientUpdateState( xProvClientHandle, xResult );
    }
}
/*-----------------------------------------------------------*/

static AzureIoTProvisioningClientResult_t
    prvProvClientCreateRequestPayload( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                       uint8_t * pucPayload,
                                       uint32_t ulPayloadLength,
                                       uint32_t * pulBytesUsed )
{
    AzureIoTProvisioningClientResult_t xResult;
    az_json_writer xJsonWriter;
    az_span xBuffer = az_span_create( pucPayload, ( int32_t ) ulPayloadLength );
    az_span xRegistrationIdLabel = AZ_SPAN_LITERAL_FROM_STR( azureiotprovisioningREQUEST_REGISTRATION_ID_LABEL );
    az_span xRegistrationId = az_span_create( ( uint8_t * ) xProvClientHandle->_internal.pucRegistrationID,
                                              ( int32_t ) xProvClientHandle->_internal.ulRegistrationIDLength );
    az_span xPayloadLabel = AZ_SPAN_LITERAL_FROM_STR( azureiotprovisioningREQUEST_PAYLOAD_LABEL );
    az_span xPayload = az_span_create( ( uint8_t * ) xProvClientHandle->_internal.pucRegistrationPayload,
                                       ( int32_t ) xProvClientHandle->_internal.ulRegistrationPayloadLength );

    if( az_result_failed( az_json_writer_init( &xJsonWriter, xBuffer, NULL ) ) ||
        az_result_failed( az_json_writer_append_begin_object( &xJsonWriter ) ) ||
        az_result_failed( az_json_writer_append_property_name( &xJsonWriter, xRegistrationIdLabel ) ) ||
        az_result_failed( az_json_writer_append_string( &xJsonWriter, xRegistrationId ) ) ||
        ( ( xProvClientHandle->_internal.pucRegistrationPayload != NULL ) &&
          ( az_result_failed( az_json_writer_append_property_name( &xJsonWriter, xPayloadLabel ) ) ||
            az_result_failed( az_json_writer_append_json_text( &xJsonWriter, xPayload ) ) ) ) ||
        az_result_failed( az_json_writer_append_end_object( &xJsonWriter ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to create Request JSON payload." ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    else
    {
        xResult = eAzureIoTProvisioningSuccess;
        *pulBytesUsed = ( uint32_t ) az_span_size( az_json_writer_get_bytes_used_in_destination( &xJsonWriter ) );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of request action. This action is only allowed in azureiotprovisioningWF_STATE_REQUEST
 *
 * */
static void prvProvClientRequest( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTProvisioningClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    size_t xMQTTTopicLength;
    uint32_t xMQTTPayloadLength = 0;
    uint16_t usPublishPacketIdentifier;
    az_result xCoreResult;

    /* Check the state.  */
    if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_REQUEST )
    {
        AZLogWarn( ( "AzureIoTProvisioning request action called in wrong state %d",
                     xProvClientHandle->_internal.ulWorkflowState ) );
    }
    else
    {
        /* Check if previous this is the 1st request or subsequent query request */
        if( xProvClientHandle->_internal.xLastResponsePayloadLength == 0 )
        {
            xCoreResult =
                az_iot_provisioning_client_register_get_publish_topic( &xProvClientHandle->_internal.xProvisioningClientCore,
                                                                       ( char * ) xProvClientHandle->_internal.pucScratchBuffer,
                                                                       azureiotTOPIC_MAX, &xMQTTTopicLength );
        }
        else
        {
            xCoreResult =
                az_iot_provisioning_client_query_status_get_publish_topic( &xProvClientHandle->_internal.xProvisioningClientCore,
                                                                           xProvClientHandle->_internal.xRegisterResponse.operation_id,
                                                                           ( char * ) xProvClientHandle->_internal.pucScratchBuffer,
                                                                           azureiotTOPIC_MAX, &xMQTTTopicLength );
        }

        if( az_result_failed( xCoreResult ) )
        {
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningFailed );
            return;
        }

        if( ( xResult =
                  prvProvClientCreateRequestPayload( xProvClientHandle,
                                                     xProvClientHandle->_internal.pucScratchBuffer +
                                                     xMQTTTopicLength,
                                                     xProvClientHandle->_internal.ulScratchBufferLength -
                                                     ( uint32_t ) xMQTTTopicLength,
                                                     &xMQTTPayloadLength ) ) != eAzureIoTProvisioningSuccess )
        {
            prvProvClientUpdateState( xProvClientHandle, xResult );
            return;
        }

        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
        xMQTTPublishInfo.pcTopicName = xProvClientHandle->_internal.pucScratchBuffer;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xMQTTTopicLength;
        xMQTTPublishInfo.pvPayload = xProvClientHandle->_internal.pucScratchBuffer + xMQTTTopicLength;
        xMQTTPublishInfo.xPayloadLength = xMQTTPayloadLength;
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xProvClientHandle->_internal.xMQTTContext ) );

        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( xProvClientHandle->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to publish prov request, error=%d", xMQTTResult ) );
            xResult = eAzureIoTProvisioningPublishFailed;
        }
        else
        {
            xResult = eAzureIoTProvisioningSuccess;
        }

        prvProvClientUpdateState( xProvClientHandle, xResult );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of parsing response action, this action is only allowed in azureiotprovisioningWF_STATE_RESPONSE
 *
 * */
static void prvProvClientParseResponse( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    az_result xCoreResult;
    az_span xTopic = az_span_create( xProvClientHandle->_internal.ucProvisioningLastResponse,
                                     ( int32_t ) xProvClientHandle->_internal.usLastResponseTopicLength );
    az_span xPayload = az_span_create( xProvClientHandle->_internal.ucProvisioningLastResponse +
                                            xProvClientHandle->_internal.usLastResponseTopicLength,
                                       ( int32_t ) xProvClientHandle->_internal.xLastResponsePayloadLength );

    /* Check the state.  */
    if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_RESPONSE )
    {
        AZLogWarn( ( "AzureIoTProvisioning parse response action called in wrong state %d",
                     xProvClientHandle->_internal.ulWorkflowState ) );
    }
    else
    {
        xCoreResult =
            az_iot_provisioning_client_parse_received_topic_and_payload( &xProvClientHandle->_internal.xProvisioningClientCore,
                                                                         xTopic, xPayload, &xProvClientHandle->_internal.xRegisterResponse );

        if( az_result_failed( xCoreResult ) )
        {
            AZLogError( ( "AzureIoTProvisioning client failed to parse packet, error status: %08x", xCoreResult ) );
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningFailed );
            return;
        }

        if( xProvClientHandle->_internal.xRegisterResponse.operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED )
        {
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningSuccess );
        }
        else if( xProvClientHandle->_internal.xRegisterResponse.retry_after_seconds == 0 )
        {
            AZLogError( ( "AzureIoTProvisioning client failed with error %d, extended error status: %u and no server retry time duration",
                          xProvClientHandle->_internal.xRegisterResponse.registration_state.error_code,
                          xProvClientHandle->_internal.xRegisterResponse.registration_state.extended_error_code ) );
            /* Server responded with error with no retry.  */
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningServerError );
        }
        else
        {
            xProvClientHandle->_internal.ullRetryAfter =
                xProvClientHandle->_internal.xGetTimeFunction() +
                    xProvClientHandle->_internal.xRegisterResponse.retry_after_seconds;
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningPending );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of wait check action. this action is only allowed in azureiotprovisioningWF_STATE_WAITING
 *
 * */
static void prvProvClientCheckTimeout( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    if( xProvClientHandle->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_WAITING )
    {
        if( xProvClientHandle->_internal.xGetTimeFunction() >
            xProvClientHandle->_internal.ullRetryAfter )
        {
            xProvClientHandle->_internal.ullRetryAfter = 0;
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningSuccess );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Trigger state machine action base on the state.
 *
 **/
static void prvProvClientTriggerAction( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    switch( xProvClientHandle->_internal.ulWorkflowState )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            prvProvClientConnect( xProvClientHandle );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            prvProvClientSubscribe( xProvClientHandle );
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            prvProvClientRequest( xProvClientHandle );
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            prvProvClientParseResponse( xProvClientHandle );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
        case azureiotprovisioningWF_STATE_REQUESTING:
        case azureiotprovisioningWF_STATE_COMPLETE:
            /* None action taken here, as these states are waiting for receive path. */
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            prvProvClientCheckTimeout( xProvClientHandle );
            break;

        default:
            AZLogError( ( "AzureIoTProvisioning state not handled %d",
                          xProvClientHandle->_internal.ulWorkflowState ) );
            configASSERT( false );
    }
}
/*-----------------------------------------------------------*/

/**
 *  Run the workflow : trigger action on state and process receive path in MQTT loop
 *
 */
static AzureIoTProvisioningClientResult_t prvProvClientRunWorkflow( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                    uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTResult_t xResult;
    AzureIoTProvisioningClientResult_t xReturn;
    uint32_t ulWaitTime;

    do
    {
        if( xProvClientHandle->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_COMPLETE )
        {
            AZLogDebug( ( "AzureIoTProvisioning state is already in complete state" ) );
            break;
        }

        if( ulTimeoutMilliseconds > azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS )
        {
            ulTimeoutMilliseconds -= azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS;
            ulWaitTime = azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS;
        }
        else
        {
            ulWaitTime = ulTimeoutMilliseconds;
            ulTimeoutMilliseconds = 0;
        }

        prvProvClientTriggerAction( xProvClientHandle );

        if( ( xProvClientHandle->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_COMPLETE ) )
        {
            AZLogDebug( ( "AzureIoTProvisioning is in complete state, with status :%d",
                          xProvClientHandle->_internal.ulLastOperationResult ) );
            break;
        }
        else if ( ( xResult =
                        AzureIoTMQTT_ProcessLoop( &( xProvClientHandle->_internal.xMQTTContext ),
                                                  ulWaitTime ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning  failed to process loop: ProcessLoopDuration=%u, Error=%d",
                          ulTimeoutMilliseconds, xResult ) );
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningFailed );
            break;
        }
    } while( ulTimeoutMilliseconds );

    if( ( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE ) )
    {
        xReturn = eAzureIoTProvisioningPending;
    }
    else
    {
        xReturn = xProvClientHandle->_internal.ulLastOperationResult;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/


/**
 *  Process MQTT Subscribe Ack from Provisioning Service
 *
 */
static void prvProvClientMQTTProcessSubAck( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                            AzureIoTMQTTPublishInfo_t * pxPacketInfo )
{
    ( void ) pxPacketInfo;

    prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningSuccess );
}
/*-----------------------------------------------------------*/

/**
 * Process MQTT Response from Provisioning Service
 *
 */
static void prvProvClientMQTTProcessResponse( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                              AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    if( xProvClientHandle->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_REQUESTING )
    {
        if( ( pxPublishInfo->xPayloadLength + pxPublishInfo->usTopicNameLength ) <=
            sizeof( xProvClientHandle->_internal.ucProvisioningLastResponse ) )
        {
            /* Copy topic + payload into the response buffer.
             * ucProvisioningLastResponse = [ topic payload ] */
            xProvClientHandle->_internal.usLastResponseTopicLength =
                pxPublishInfo->usTopicNameLength;
            memcpy( xProvClientHandle->_internal.ucProvisioningLastResponse,
                    pxPublishInfo->pcTopicName, pxPublishInfo->usTopicNameLength );

            xProvClientHandle->_internal.xLastResponsePayloadLength =
                pxPublishInfo->xPayloadLength;
            memcpy( xProvClientHandle->_internal.ucProvisioningLastResponse +
                        xProvClientHandle->_internal.usLastResponseTopicLength,
                    pxPublishInfo->pvPayload, pxPublishInfo->xPayloadLength );

            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningSuccess );
        }
        else
        {
            AZLogError( ( "AzureIoTProvisioning process response failed with buffer too small required size is %d bytes",
                          pxPublishInfo->xPayloadLength + pxPublishInfo->usTopicNameLength ) );
            prvProvClientUpdateState( xProvClientHandle, eAzureIoTProvisioningOutOfMemory );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Callback that receives all the Event from MQTT
 *
 */
static void prvProvClientEventCallback( AzureIoTMQTTHandle_t pxMQTTContext,
                                        AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                                        AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    AzureIoTProvisioningClientHandle_t xProvClientHandle = ( AzureIoTProvisioningClientHandle_t ) pxMQTTContext;

    if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_SUBACK )
    {
        prvProvClientMQTTProcessSubAck( xProvClientHandle,
                                          pxDeserializedInfo->pxPublishInfo );
    }
    else if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_PUBLISH )
    {
        prvProvClientMQTTProcessResponse( xProvClientHandle,
                                            pxDeserializedInfo->pxPublishInfo );
    }
    else
    {
        AZLogDebug( ( "AzureIoTProvisioning received packet of type: %d", pxPacketInfo->ucType ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * Generate the SAS token based on :
 *   https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security#use-a-shared-access-policy
 *
 */
static uint32_t prvProvClientGetToken( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                       uint64_t ullExpiryTimeSecs, const uint8_t * ucKey,
                                       uint32_t ulKeyLen, uint8_t * pucSASBuffer,
                                       uint32_t ulSasBufferLen, uint32_t * pulSaSLength )
{
    uint8_t * pucHMACBuffer;
    uint32_t ulHMACBufferLength = azureiotprovisioningHMACBufferLength;
    az_span xSpan = az_span_create( pucSASBuffer, ( int32_t ) ulSasBufferLen );
    az_result xCoreResult;
    uint32_t ulBytesUsed;
    uint32_t ulBufferLeft;
    size_t xLength;

    xCoreResult = az_iot_provisioning_client_sas_get_signature( &( xProvClientHandle->_internal.xProvisioningClientCore ),
                                                                ullExpiryTimeSecs, xSpan, &xSpan );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to get signature with error status: %08x", xCoreResult ) );
        return eAzureIoTProvisioningFailed;
    }

    ulBytesUsed = ( uint32_t ) az_span_size( xSpan );
    ulBufferLeft = ulSasBufferLen - ulBytesUsed;

    if( ulBufferLeft < ulHMACBufferLength )
    {
        AZLogError( ( "AzureIoTProvisioning token generation failed with insufficient buffer size." ) );
        return eAzureIoTProvisioningOutOfMemory;
    }

    ulBufferLeft -= ulHMACBufferLength;
    pucHMACBuffer = pucSASBuffer + ulSasBufferLen - ulHMACBufferLength;

    if( AzureIoT_Base64HMACCalculate( xProvClientHandle->_internal.xHMACFunction,
                                      ucKey, ulKeyLen, pucSASBuffer, ulBytesUsed, pucSASBuffer + ulBytesUsed, ulBufferLeft,
                                      pucHMACBuffer, ulHMACBufferLength, &ulBytesUsed ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to encoded HMAC hash" ) );
        return eAzureIoTProvisioningFailed;
    }

    xSpan = az_span_create( pucHMACBuffer, ( int32_t ) ulBytesUsed );
    xCoreResult = az_iot_provisioning_client_sas_get_password( &( xProvClientHandle->_internal.xProvisioningClientCore ),
                                                               xSpan, ullExpiryTimeSecs, AZ_SPAN_EMPTY, ( char * ) pucSASBuffer,
                                                               ulSasBufferLen - ulHMACBufferLength, &xLength );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to generate token with error status: %08x", xCoreResult ) );
        return eAzureIoTProvisioningFailed;
    }

    *pulSaSLength = ( uint32_t ) xLength;

    return eAzureIoTProvisioningSuccess;
}
/*-----------------------------------------------------------*/

/**
 * Get number of millseconds passed to MQTT stack
 *
 */
static uint32_t prvProvClientGetTimeMillseconds( void )
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

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_OptionsInit( AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions )
{
    AzureIoTProvisioningClientResult_t xReturn;

    if( pxProvisioningClientOptions == NULL )
    {
        AZLogError( ( "AzureIoTProvisioning failed to initialize options: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        pxProvisioningClientOptions->pucUserAgent = ( const uint8_t * ) azureiotprovisioningUSER_AGENT;
        pxProvisioningClientOptions->ulUserAgentLength = sizeof( azureiotprovisioningUSER_AGENT ) - 1;
        xReturn = eAzureIoTProvisioningSuccess;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                    const uint8_t * pucEndpoint,
                                                                    uint32_t ulEndpointLength,
                                                                    const uint8_t * pucIDScope,
                                                                    uint32_t ulIDScopeLength,
                                                                    const uint8_t * pucRegistrationID,
                                                                    uint32_t ulRegistrationIDLength,
                                                                    AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions,
                                                                    uint8_t * pucBuffer,
                                                                    uint32_t ulBufferLength,
                                                                    AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                                    const AzureIoTTransportInterface_t * pxTransportInterface )
{
    AzureIoTProvisioningClientResult_t xReturn;
    az_span xEndpoint = az_span_create( ( uint8_t * ) pucEndpoint, ( int32_t ) ulEndpointLength );
    az_span xIDScope = az_span_create( ( uint8_t * ) pucIDScope, ( int32_t ) ulIDScopeLength );
    az_span xRegistrationID = az_span_create( ( uint8_t * ) pucRegistrationID, ( int32_t ) ulRegistrationIDLength );
    az_result xCoreResult;
    AzureIoTMQTTResult_t xResult;
    az_iot_provisioning_client_options xOptions = az_iot_provisioning_client_options_default();
    uint8_t * pucNetworkBuffer;
    uint32_t ulNetworkBufferLength;

    if( ( xProvClientHandle == NULL ) ||
        ( pucEndpoint == NULL ) || ( ulEndpointLength == 0 ) ||
        ( pucIDScope == NULL ) || ( ulIDScopeLength == 0 ) ||
        ( pucRegistrationID == NULL ) || ( ulRegistrationIDLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning initialization failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else if( ( ulBufferLength < ( azureiotTOPIC_MAX + azureiotPROVISIONING_REQUEST_PAYLOAD_MAX ) ) ||
             ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning initialize failed: Insufficient buffer size" ) );
        xReturn = eAzureIoTProvisioningOutOfMemory;
    }
    else
    {
        memset( xProvClientHandle, 0, sizeof( AzureIoTProvisioningClient_t ) );
        /* Setup scratch buffer to be used by middleware */
        xProvClientHandle->_internal.ulScratchBufferLength =
            azureiotPrvGetMaxInt( ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ),
                                  ( azureiotTOPIC_MAX + azureiotPROVISIONING_REQUEST_PAYLOAD_MAX ) );
        xProvClientHandle->_internal.pucScratchBuffer = pucBuffer;
        pucNetworkBuffer = pucBuffer + xProvClientHandle->_internal.ulScratchBufferLength;
        ulNetworkBufferLength = ulBufferLength - xProvClientHandle->_internal.ulScratchBufferLength;

        xProvClientHandle->_internal.pucEndpoint = pucEndpoint;
        xProvClientHandle->_internal.ulEndpointLength = ulEndpointLength;
        xProvClientHandle->_internal.pucIDScope = pucIDScope;
        xProvClientHandle->_internal.ulIDScopeLength = ulIDScopeLength;
        xProvClientHandle->_internal.pucRegistrationID = pucRegistrationID;
        xProvClientHandle->_internal.ulRegistrationIDLength = ulRegistrationIDLength;
        xProvClientHandle->_internal.xGetTimeFunction = xGetTimeFunction;

        if( pxProvisioningClientOptions )
        {
            xOptions.user_agent = az_span_create( ( uint8_t * ) pxProvisioningClientOptions->pucUserAgent,
                                                  ( int32_t ) pxProvisioningClientOptions->ulUserAgentLength );
        }
        else
        {
            xOptions.user_agent = az_span_create( ( uint8_t * ) azureiotprovisioningUSER_AGENT,
                                                  sizeof( azureiotprovisioningUSER_AGENT ) - 1 );
        }

        xCoreResult = az_iot_provisioning_client_init( &( xProvClientHandle->_internal.xProvisioningClientCore ),
                                                       xEndpoint, xIDScope, xRegistrationID, &xOptions );

        if( az_result_failed( xCoreResult ) )
        {
            AZLogError( ( "AzureIoTProvisioning initialization failed: with core error : %08x", xCoreResult ) );
            xReturn = eAzureIoTProvisioningFailed;
        }
        else if( ( xResult = AzureIoTMQTT_Init( &( xProvClientHandle->_internal.xMQTTContext ),
                                                pxTransportInterface, prvProvClientGetTimeMillseconds,
                                                prvProvClientEventCallback, pucNetworkBuffer,
                                                ulNetworkBufferLength ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning initialization failed with mqtt error : %d", xResult ) );
            xReturn = eAzureIoTProvisioningInitFailed;
        }
        else
        {
            xReturn = eAzureIoTProvisioningSuccess;
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xProvClientHandle )
{
    if( xProvClientHandle == NULL )
    {
        AZLogError( ( "AzureIoTProvisioningClient Deinit failed: Invalid argument" ) );
    }
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SetSymmetricKey( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                               const uint8_t * pucSymmetricKey,
                                                                               uint32_t ulSymmetricKeyLength,
                                                                               AzureIoTGetHMACFunc_t xHmacFunction )
{
    AzureIoTProvisioningClientResult_t xReturn;

    if( ( xProvClientHandle == NULL ) ||
        ( pucSymmetricKey == NULL ) || ( ulSymmetricKeyLength == 0 ) ||
        ( xHmacFunction == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning client symmetric key failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        xProvClientHandle->_internal.pucSymmetricKey = pucSymmetricKey;
        xProvClientHandle->_internal.ulSymmetricKeyLength = ulSymmetricKeyLength;
        xProvClientHandle->_internal.pxTokenRefresh = prvProvClientGetToken;
        xProvClientHandle->_internal.xHMACFunction = xHmacFunction;
        xReturn = eAzureIoTProvisioningSuccess;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                        uint32_t ulTimeoutMilliseconds )
{
    AzureIoTProvisioningClientResult_t xReturn;

    if( xProvClientHandle == NULL )
    {
        AZLogError( ( "AzureIoTProvisioning registration failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        if( xProvClientHandle->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_INIT )
        {
            xProvClientHandle->_internal.ulWorkflowState = azureiotprovisioningWF_STATE_CONNECT;
        }

        xReturn = prvProvClientRunWorkflow( xProvClientHandle, ulTimeoutMilliseconds );
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_GetDeviceAndHub( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                               uint8_t * pucHubHostname,
                                                                               uint32_t * pulHostnameLength,
                                                                               uint8_t * pucDeviceID,
                                                                               uint32_t * pulDeviceIDLength )
{
    uint32_t ulHostnameLength;
    uint32_t ulDeviceIDLength;
    AzureIoTProvisioningClientResult_t xReturn;
    az_span * pxHostname;
    az_span * pxDeviceID;

    if( ( xProvClientHandle == NULL ) || ( pucHubHostname == NULL ) ||
        ( pulHostnameLength == NULL ) || ( pucDeviceID == NULL ) || ( pulDeviceIDLength == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning get hub failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in complete state" ) );
        xReturn = eAzureIoTProvisioningFailed;
    }
    else if( xProvClientHandle->_internal.ulLastOperationResult )
    {
        xReturn = xProvClientHandle->_internal.ulLastOperationResult;
    }
    else
    {
        pxHostname = &xProvClientHandle->_internal.xRegisterResponse.registration_state.assigned_hub_hostname;
        pxDeviceID = &xProvClientHandle->_internal.xRegisterResponse.registration_state.device_id;
        ulHostnameLength = ( uint32_t ) az_span_size( *pxHostname );
        ulDeviceIDLength = ( uint32_t ) az_span_size( *pxDeviceID );

        if( ( *pulHostnameLength < ulHostnameLength ) || ( *pulDeviceIDLength < ulDeviceIDLength ) )
        {
            AZLogWarn( ( "AzureIoTProvisioning Memory buffer passed is not enough to store hub info" ) );
            xReturn = eAzureIoTProvisioningFailed;
        }
        else
        {
            memcpy( pucHubHostname, az_span_ptr( *pxHostname ), ulHostnameLength );
            memcpy( pucDeviceID, az_span_ptr( *pxDeviceID ), ulDeviceIDLength );
            *pulHostnameLength = ulHostnameLength;
            *pulDeviceIDLength = ulDeviceIDLength;
            xReturn = eAzureIoTProvisioningSuccess;
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_GetExtendedCode( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                               uint32_t * pulExtendedErrorCode )
{
    AzureIoTProvisioningClientResult_t xReturn;

    if( ( xProvClientHandle == NULL ) ||
        ( pulExtendedErrorCode == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning client get extended code failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in complete state" ) );
        xReturn = eAzureIoTProvisioningFailed;
    }
    else
    {
        *pulExtendedErrorCode =
            xProvClientHandle->_internal.xRegisterResponse.registration_state.extended_error_code;
        xReturn = eAzureIoTProvisioningSuccess;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SetRegistrationPayload( AzureIoTProvisioningClientHandle_t xProvClientHandle,
                                                                                      const uint8_t * pucPayload,
                                                                                      uint32_t ulPayloadLength )
{
    AzureIoTProvisioningClientResult_t xReturn;

    if( ( xProvClientHandle == NULL ) ||
        ( pucPayload == NULL ) || ( ulPayloadLength == 0 ) )
    {
        AZLogError( ( "AzureIoTProvisioning setting client registration payload failed: Invalid argument" ) );
        xReturn = eAzureIoTProvisioningInvalidArgument;
    }
    else if( xProvClientHandle->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_INIT )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in init" ) );
        xReturn = eAzureIoTProvisioningFailed;
    }
    else
    {
        xProvClientHandle->_internal.pucRegistrationPayload = pucPayload;
        xProvClientHandle->_internal.ulRegistrationPayloadLength = ulPayloadLength;
        xReturn = eAzureIoTProvisioningSuccess;
    }

    return xReturn;
}
/*-----------------------------------------------------------*/
