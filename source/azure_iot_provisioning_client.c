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
    #define azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS    azureiotKEEP_ALIVE_TIMEOUT_SECONDS
#endif /* azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS */

#ifndef azureiotprovisioningCONNACK_RECV_TIMEOUT_MS
    #define azureiotprovisioningCONNACK_RECV_TIMEOUT_MS    azureiotCONNACK_RECV_TIMEOUT_MS
#endif /* azureiotprovisioningCONNACK_RECV_TIMEOUT_MS */

#ifndef azureiotprovisioningUSER_AGENT
    #define azureiotprovisioningUSER_AGENT    ""
#endif /* azureiotprovisioningUSER_AGENT */

#ifndef azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS
    #define azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS    ( 500U )
#endif /* azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS */

/* Define various workflow (WF) states Provisioning can get into */
#define azureiotprovisioningWF_STATE_INIT           ( 0x0 )
#define azureiotprovisioningWF_STATE_CONNECT        ( 0x1 )
#define azureiotprovisioningWF_STATE_SUBSCRIBE      ( 0x2 )
#define azureiotprovisioningWF_STATE_REQUEST        ( 0x3 )
#define azureiotprovisioningWF_STATE_RESPONSE       ( 0x4 )
#define azureiotprovisioningWF_STATE_SUBSCRIBING    ( 0x5 )
#define azureiotprovisioningWF_STATE_REQUESTING     ( 0x6 )
#define azureiotprovisioningWF_STATE_WAITING        ( 0x7 )
#define azureiotprovisioningWF_STATE_COMPLETE       ( 0x8 )

#define azureiotPrvGetMaxInt( a, b )    ( ( a ) > ( b ) ? ( a ) : ( b ) )

#define azureiotprovisioningREQUEST_PAYLOAD_LABEL            "payload"
#define azureiotprovisioningREQUEST_REGISTRATION_ID_LABEL    "registrationId"

#define azureiotprovisioningHMACBufferLength                 ( 48 )
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
static void prvProvClientUpdateState( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                      uint32_t ulActionResult )
{
    uint32_t ulState = pxAzureProvClient->_internal.ulWorkflowState;

    pxAzureProvClient->_internal.ulLastOperationResult = ulActionResult;

    switch( ulState )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_SUBSCRIBE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_SUBSCRIBING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_REQUEST :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_REQUESTING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUESTING:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningSuccess ? azureiotprovisioningWF_STATE_RESPONSE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            pxAzureProvClient->_internal.ulWorkflowState =
                ulActionResult == eAzureIoTProvisioningPending ? azureiotprovisioningWF_STATE_WAITING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            pxAzureProvClient->_internal.ulWorkflowState =
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
                  pxAzureProvClient->_internal.ulWorkflowState ) );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of the connect action. This action is only allowed in azureiotprovisioningWF_STATE_CONNECT
 *
 * */
static void prvProvClientConnect( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTProvisioningClientResult_t xResult;
    AzureIoTMQTTResult_t xMQTTResult;
    bool xSessionPresent;
    uint32_t ulPasswordLength = 0;
    size_t xMQTTUsernameLength;
    az_result xCoreResult;

    if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_CONNECT )
    {
        AZLogError( ( "AzureIoTProvisioning connect action called in wrong state (%d)",
                      pxAzureProvClient->_internal.ulWorkflowState ) );
        return;
    }

    xConnectInfo.pcUserName = pxAzureProvClient->_internal.pucScratchBuffer;
    xConnectInfo.pcPassword = xConnectInfo.pcUserName + azureiotUSERNAME_MAX;

    if( az_result_failed( xCoreResult = az_iot_provisioning_client_get_user_name( &pxAzureProvClient->_internal.xProvisioningClientCore,
                                                                                  ( char * ) xConnectInfo.pcUserName,
                                                                                  azureiotUSERNAME_MAX, &xMQTTUsernameLength ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to get username (%08x)", xCoreResult ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    /* Check if token refresh is set, then generate password */
    else if( ( pxAzureProvClient->_internal.pxTokenRefresh ) &&
             ( pxAzureProvClient->_internal.pxTokenRefresh( pxAzureProvClient,
                                                            pxAzureProvClient->_internal.xGetTimeFunction() +
                                                            azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                            pxAzureProvClient->_internal.pucSymmetricKey,
                                                            pxAzureProvClient->_internal.ulSymmetricKeyLength,
                                                            ( uint8_t * ) xConnectInfo.pcPassword,
                                                            azureiotPASSWORD_MAX,
                                                            &ulPasswordLength ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to generate auth token" ) );
        xResult = eAzureIoTProvisioningTokenGenerationFailed;
    }
    else
    {
        xConnectInfo.xCleanSession = true;
        xConnectInfo.pcClientIdentifier = pxAzureProvClient->_internal.pucRegistrationID;
        xConnectInfo.usClientIdentifierLength = ( uint16_t ) pxAzureProvClient->_internal.ulRegistrationIDLength;
        xConnectInfo.usUserNameLength = ( uint16_t ) xMQTTUsernameLength;
        xConnectInfo.usKeepAliveSeconds = azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS;
        xConnectInfo.usPasswordLength = ( uint16_t ) ulPasswordLength;

        if( ( xMQTTResult = AzureIoTMQTT_Connect( &( pxAzureProvClient->_internal.xMQTTContext ),
                                                  &xConnectInfo, NULL, azureiotprovisioningCONNACK_RECV_TIMEOUT_MS,
                                                  &xSessionPresent ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d ",
                          pxAzureProvClient->_internal.ulEndpointLength,
                          pxAzureProvClient->_internal.pucEndpoint,
                          xMQTTResult ) );
            xResult = eAzureIoTProvisioningServerError;
        }
        else
        {
            /* Successfully established and MQTT connection with the broker. */
            AZLogInfo( ( "AzureIoTProvisioning an MQTT connection is established with %.*s.",
                         pxAzureProvClient->_internal.ulEndpointLength,
                         pxAzureProvClient->_internal.pucEndpoint ) );
            xResult = eAzureIoTProvisioningSuccess;
        }
    }

    prvProvClientUpdateState( pxAzureProvClient, xResult );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of subscribe action, this action is only allowed in azureiotprovisioningWF_STATE_SUBSCRIBE
 *
 * */
static void prvProvClientSubscribe( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    AzureIoTMQTTSubscribeInfo_t xMQTTSubscription = { 0 };
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTProvisioningClientResult_t xResult;
    uint16_t usSubscribePacketIdentifier;

    if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_SUBSCRIBE )
    {
        AZLogWarn( ( "AzureIoTProvisioning subscribe action called in wrong state %d",
                     pxAzureProvClient->_internal.ulWorkflowState ) );
    }
    else
    {
        xMQTTSubscription.xQoS = eAzureIoTMQTTQoS0;
        xMQTTSubscription.pcTopicFilter = ( const uint8_t * ) AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
        xMQTTSubscription.usTopicFilterLength = ( uint16_t ) sizeof( AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC ) - 1;

        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureProvClient->_internal.xMQTTContext ) );

        AZLogInfo( ( "AzureIoTProvisioning attempting to subscribe to the MQTT %s topic.", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if( ( xMQTTResult = AzureIoTMQTT_Subscribe( &( pxAzureProvClient->_internal.xMQTTContext ),
                                                    &xMQTTSubscription, 1, usSubscribePacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to SUBSCRIBE to MQTT topic. Error=%d", xMQTTResult ) );
            xResult = eAzureIoTProvisioningSubscribeFailed;
        }
        else
        {
            xResult = eAzureIoTProvisioningSuccess;
        }

        prvProvClientUpdateState( pxAzureProvClient, xResult );
    }
}
/*-----------------------------------------------------------*/

static AzureIoTProvisioningClientResult_t prvProvClientCreateRequestPayload( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                             uint8_t * pucPayload,
                                                                             uint32_t ulPayloadLength,
                                                                             uint32_t * pulBytesUsed )
{
    AzureIoTProvisioningClientResult_t xResult;
    az_json_writer xJsonWriter;
    az_span xBuffer = az_span_create( pucPayload, ( int32_t ) ulPayloadLength );
    az_span xRegistrationIdLabel = AZ_SPAN_LITERAL_FROM_STR( azureiotprovisioningREQUEST_REGISTRATION_ID_LABEL );
    az_span xRegistrationId = az_span_create( ( uint8_t * ) pxAzureProvClient->_internal.pucRegistrationID,
                                              ( int32_t ) pxAzureProvClient->_internal.ulRegistrationIDLength );
    az_span xPayloadLabel = AZ_SPAN_LITERAL_FROM_STR( azureiotprovisioningREQUEST_PAYLOAD_LABEL );
    az_span xPayload = az_span_create( ( uint8_t * ) pxAzureProvClient->_internal.pucRegistrationPayload,
                                       ( int32_t ) pxAzureProvClient->_internal.ulRegistrationPayloadLength );

    if( az_result_failed( az_json_writer_init( &xJsonWriter, xBuffer, NULL ) ) ||
        az_result_failed( az_json_writer_append_begin_object( &xJsonWriter ) ) ||
        az_result_failed( az_json_writer_append_property_name( &xJsonWriter, xRegistrationIdLabel ) ) ||
        az_result_failed( az_json_writer_append_string( &xJsonWriter, xRegistrationId ) ) ||
        ( ( pxAzureProvClient->_internal.pucRegistrationPayload != NULL ) &&
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
static void prvProvClientRequest( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTProvisioningClientResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    size_t xMQTTTopicLength;
    uint32_t xMQTTPayloadLength = 0;
    uint16_t usPublishPacketIdentifier;
    az_result xCoreResult;

    /* Check the state.  */
    if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_REQUEST )
    {
        AZLogWarn( ( "AzureIoTProvisioning request action called in wrong state %d",
                     pxAzureProvClient->_internal.ulWorkflowState ) );
    }
    else
    {
        /* Check if previous this is the 1st request or subsequent query request */
        if( pxAzureProvClient->_internal.xLastResponsePayloadLength == 0 )
        {
            xCoreResult =
                az_iot_provisioning_client_register_get_publish_topic( &pxAzureProvClient->_internal.xProvisioningClientCore,
                                                                       ( char * ) pxAzureProvClient->_internal.pucScratchBuffer,
                                                                       azureiotTOPIC_MAX, &xMQTTTopicLength );
        }
        else
        {
            xCoreResult =
                az_iot_provisioning_client_query_status_get_publish_topic( &pxAzureProvClient->_internal.xProvisioningClientCore,
                                                                           pxAzureProvClient->_internal.xRegisterResponse.operation_id,
                                                                           ( char * ) pxAzureProvClient->_internal.pucScratchBuffer,
                                                                           azureiotTOPIC_MAX, &xMQTTTopicLength );
        }

        if( az_result_failed( xCoreResult ) )
        {
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningFailed );
            return;
        }

        if( ( xResult =
                  prvProvClientCreateRequestPayload( pxAzureProvClient,
                                                     pxAzureProvClient->_internal.pucScratchBuffer +
                                                     xMQTTTopicLength,
                                                     pxAzureProvClient->_internal.ulScratchBufferLength -
                                                     ( uint32_t ) xMQTTTopicLength,
                                                     &xMQTTPayloadLength ) ) != eAzureIoTProvisioningSuccess )
        {
            prvProvClientUpdateState( pxAzureProvClient, xResult );
            return;
        }

        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
        xMQTTPublishInfo.pcTopicName = pxAzureProvClient->_internal.pucScratchBuffer;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) xMQTTTopicLength;
        xMQTTPublishInfo.pvPayload = pxAzureProvClient->_internal.pucScratchBuffer + xMQTTTopicLength;
        xMQTTPublishInfo.xPayloadLength = xMQTTPayloadLength;
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureProvClient->_internal.xMQTTContext ) );

        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureProvClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning failed to publish prov request, error=%d", xMQTTResult ) );
            xResult = eAzureIoTProvisioningPublishFailed;
        }
        else
        {
            xResult = eAzureIoTProvisioningSuccess;
        }

        prvProvClientUpdateState( pxAzureProvClient, xResult );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of parsing response action, this action is only allowed in azureiotprovisioningWF_STATE_RESPONSE
 *
 * */
static void prvProvClientParseResponse( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    az_result xCoreResult;
    az_span xTopic = az_span_create( pxAzureProvClient->_internal.ucProvisioningLastResponse,
                                     ( int32_t ) pxAzureProvClient->_internal.usLastResponseTopicLength );
    az_span xPayload = az_span_create( pxAzureProvClient->_internal.ucProvisioningLastResponse +
                                       pxAzureProvClient->_internal.usLastResponseTopicLength,
                                       ( int32_t ) pxAzureProvClient->_internal.xLastResponsePayloadLength );

    /* Check the state.  */
    if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_RESPONSE )
    {
        AZLogWarn( ( "AzureIoTProvisioning parse response action called in wrong state %d",
                     pxAzureProvClient->_internal.ulWorkflowState ) );
    }
    else
    {
        xCoreResult =
            az_iot_provisioning_client_parse_received_topic_and_payload( &pxAzureProvClient->_internal.xProvisioningClientCore,
                                                                         xTopic, xPayload, &pxAzureProvClient->_internal.xRegisterResponse );

        if( az_result_failed( xCoreResult ) )
        {
            AZLogError( ( "AzureIoTProvisioning client failed to parse packet, error status: %08x", xCoreResult ) );
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningFailed );
            return;
        }

        if( pxAzureProvClient->_internal.xRegisterResponse.operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED )
        {
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningSuccess );
        }
        else if( pxAzureProvClient->_internal.xRegisterResponse.retry_after_seconds == 0 )
        {
            AZLogError( ( "AzureIoTProvisioning client failed with error %d, extended error status: %u and no server retry time duration",
                          pxAzureProvClient->_internal.xRegisterResponse.registration_state.error_code,
                          pxAzureProvClient->_internal.xRegisterResponse.registration_state.extended_error_code ) );
            /* Server responded with error with no retry.  */
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningServerError );
        }
        else
        {
            pxAzureProvClient->_internal.ullRetryAfter =
                pxAzureProvClient->_internal.xGetTimeFunction() +
                pxAzureProvClient->_internal.xRegisterResponse.retry_after_seconds;
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningPending );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of wait check action. this action is only allowed in azureiotprovisioningWF_STATE_WAITING
 *
 * */
static void prvProvClientCheckTimeout( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    if( pxAzureProvClient->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_WAITING )
    {
        if( pxAzureProvClient->_internal.xGetTimeFunction() >
            pxAzureProvClient->_internal.ullRetryAfter )
        {
            pxAzureProvClient->_internal.ullRetryAfter = 0;
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningSuccess );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Trigger state machine action base on the state.
 *
 **/
static void prvProvClientTriggerAction( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    switch( pxAzureProvClient->_internal.ulWorkflowState )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            prvProvClientConnect( pxAzureProvClient );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            prvProvClientSubscribe( pxAzureProvClient );
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            prvProvClientRequest( pxAzureProvClient );
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            prvProvClientParseResponse( pxAzureProvClient );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
        case azureiotprovisioningWF_STATE_REQUESTING:
        case azureiotprovisioningWF_STATE_COMPLETE:
            /* None action taken here, as these states are waiting for receive path. */
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            prvProvClientCheckTimeout( pxAzureProvClient );
            break;

        default:
            AZLogError( ( "AzureIoTProvisioning state not handled %d",
                          pxAzureProvClient->_internal.ulWorkflowState ) );
            configASSERT( false );
    }
}
/*-----------------------------------------------------------*/

/**
 *  Run the workflow : trigger action on state and process receive path in MQTT loop
 *
 */
static AzureIoTProvisioningClientResult_t prvProvClientRunWorkflow( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                    uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTProvisioningClientResult_t xResult;
    uint32_t ulWaitTime;

    do
    {
        if( pxAzureProvClient->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_COMPLETE )
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

        prvProvClientTriggerAction( pxAzureProvClient );

        if( ( pxAzureProvClient->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_COMPLETE ) )
        {
            AZLogDebug( ( "AzureIoTProvisioning is in complete state, with status :%d",
                          pxAzureProvClient->_internal.ulLastOperationResult ) );
            break;
        }
        else if( ( xMQTTResult =
                       AzureIoTMQTT_ProcessLoop( &( pxAzureProvClient->_internal.xMQTTContext ),
                                                 ulWaitTime ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning  failed to process loop: ProcessLoopDuration=%u, Error=%d",
                          ulTimeoutMilliseconds, xMQTTResult ) );
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningFailed );
            break;
        }
    } while( ulTimeoutMilliseconds );

    if( ( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE ) )
    {
        xResult = eAzureIoTProvisioningPending;
    }
    else
    {
        xResult = pxAzureProvClient->_internal.ulLastOperationResult;
    }

    return xResult;
}
/*-----------------------------------------------------------*/


/**
 *  Process MQTT Subscribe Ack from Provisioning Service
 *
 */
static void prvProvClientMQTTProcessSubAck( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                            AzureIoTMQTTPublishInfo_t * pxPacketInfo )
{
    ( void ) pxPacketInfo;

    /* We assume success since IoT Provisioning would disconnect if there was a problem subscribing. */
    prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningSuccess );
}
/*-----------------------------------------------------------*/

/**
 * Process MQTT Response from Provisioning Service
 *
 */
static void prvProvClientMQTTProcessResponse( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                              AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    if( pxAzureProvClient->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_REQUESTING )
    {
        if( ( pxPublishInfo->xPayloadLength + pxPublishInfo->usTopicNameLength ) <=
            sizeof( pxAzureProvClient->_internal.ucProvisioningLastResponse ) )
        {
            /* Copy topic + payload into the response buffer.
             * ucProvisioningLastResponse = [ topic payload ] */
            pxAzureProvClient->_internal.usLastResponseTopicLength =
                pxPublishInfo->usTopicNameLength;
            memcpy( pxAzureProvClient->_internal.ucProvisioningLastResponse,
                    pxPublishInfo->pcTopicName, pxPublishInfo->usTopicNameLength );

            pxAzureProvClient->_internal.xLastResponsePayloadLength =
                pxPublishInfo->xPayloadLength;
            memcpy( pxAzureProvClient->_internal.ucProvisioningLastResponse +
                    pxAzureProvClient->_internal.usLastResponseTopicLength,
                    pxPublishInfo->pvPayload, pxPublishInfo->xPayloadLength );

            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningSuccess );
        }
        else
        {
            AZLogError( ( "AzureIoTProvisioning process response failed with buffer too small required size is %d bytes",
                          pxPublishInfo->xPayloadLength + pxPublishInfo->usTopicNameLength ) );
            prvProvClientUpdateState( pxAzureProvClient, eAzureIoTProvisioningOutOfMemory );
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
    AzureIoTProvisioningClient_t * pxAzureProvClient = ( AzureIoTProvisioningClient_t * ) pxMQTTContext;

    if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_SUBACK )
    {
        prvProvClientMQTTProcessSubAck( pxAzureProvClient,
                                        pxDeserializedInfo->pxPublishInfo );
    }
    else if( ( pxPacketInfo->ucType & 0xF0U ) == azureiotmqttPACKET_TYPE_PUBLISH )
    {
        prvProvClientMQTTProcessResponse( pxAzureProvClient,
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
static uint32_t prvProvClientGetToken( AzureIoTProvisioningClient_t * pxAzureProvClient,
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
    uint32_t ulBytesUsed;
    uint32_t ulSignatureLength;
    uint32_t ulBufferLeft;
    size_t xLength;

    xCoreResult = az_iot_provisioning_client_sas_get_signature( &( pxAzureProvClient->_internal.xProvisioningClientCore ),
                                                                ullExpiryTimeSecs, xSpan, &xSpan );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to get signature with error status: %08x", xCoreResult ) );
        return eAzureIoTProvisioningFailed;
    }

    ulBytesUsed = ( uint32_t ) az_span_size( xSpan );
    ulBufferLeft = ulSasBufferLen - ulBytesUsed;

    if( ulBufferLeft < azureiotprovisioningHMACBufferLength )
    {
        AZLogError( ( "AzureIoTProvisioning token generation failed with insufficient buffer size." ) );
        return eAzureIoTProvisioningOutOfMemory;
    }

    /* Calculate HMAC at the end of buffer, so we do less data movement when returning back to caller. */
    ulBufferLeft -= azureiotprovisioningHMACBufferLength;
    pucHMACBuffer = pucSASBuffer + ulSasBufferLen - azureiotprovisioningHMACBufferLength;

    if( AzureIoT_Base64HMACCalculate( pxAzureProvClient->_internal.xHMACFunction,
                                      ucKey, ulKeyLen, pucSASBuffer, ulBytesUsed, pucSASBuffer + ulBytesUsed, ulBufferLeft,
                                      pucHMACBuffer, azureiotprovisioningHMACBufferLength,
                                      &ulSignatureLength ) )
    {
        AZLogError( ( "AzureIoTProvisioning failed to encoded HMAC hash" ) );
        return eAzureIoTProvisioningFailed;
    }

    xSpan = az_span_create( pucHMACBuffer, ( int32_t ) ulSignatureLength );
    xCoreResult = az_iot_provisioning_client_sas_get_password( &( pxAzureProvClient->_internal.xProvisioningClientCore ),
                                                               xSpan, ullExpiryTimeSecs, AZ_SPAN_EMPTY, ( char * ) pucSASBuffer,
                                                               ulSasBufferLen - azureiotprovisioningHMACBufferLength,
                                                               &xLength );

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
    AzureIoTProvisioningClientResult_t xResult;

    if( pxProvisioningClientOptions == NULL )
    {
        AZLogError( ( "AzureIoTProvisioning failed to initialize options: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        pxProvisioningClientOptions->pucUserAgent = ( const uint8_t * ) azureiotprovisioningUSER_AGENT;
        pxProvisioningClientOptions->ulUserAgentLength = sizeof( azureiotprovisioningUSER_AGENT ) - 1;
        xResult = eAzureIoTProvisioningSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClient_t * pxAzureProvClient,
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
    AzureIoTProvisioningClientResult_t xResult;
    az_span xEndpoint = az_span_create( ( uint8_t * ) pucEndpoint, ( int32_t ) ulEndpointLength );
    az_span xIDScope = az_span_create( ( uint8_t * ) pucIDScope, ( int32_t ) ulIDScopeLength );
    az_span xRegistrationID = az_span_create( ( uint8_t * ) pucRegistrationID, ( int32_t ) ulRegistrationIDLength );
    az_result xCoreResult;
    AzureIoTMQTTResult_t xMQTTResult;
    az_iot_provisioning_client_options xOptions = az_iot_provisioning_client_options_default();
    uint8_t * pucNetworkBuffer;
    uint32_t ulNetworkBufferLength;

    if( ( pxAzureProvClient == NULL ) ||
        ( pucEndpoint == NULL ) || ( ulEndpointLength == 0 ) ||
        ( pucIDScope == NULL ) || ( ulIDScopeLength == 0 ) ||
        ( pucRegistrationID == NULL ) || ( ulRegistrationIDLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning initialization failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else if( ( ulBufferLength < ( azureiotTOPIC_MAX + azureiotPROVISIONING_REQUEST_PAYLOAD_MAX ) ) ||
             ( ulBufferLength < ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ) ) )
    {
        AZLogError( ( "AzureIoTProvisioning initialize failed: Insufficient buffer size" ) );
        xResult = eAzureIoTProvisioningOutOfMemory;
    }
    else
    {
        memset( pxAzureProvClient, 0, sizeof( AzureIoTProvisioningClient_t ) );
        /* Setup scratch buffer to be used by middleware */
        pxAzureProvClient->_internal.ulScratchBufferLength =
            azureiotPrvGetMaxInt( ( azureiotUSERNAME_MAX + azureiotPASSWORD_MAX ),
                                  ( azureiotTOPIC_MAX + azureiotPROVISIONING_REQUEST_PAYLOAD_MAX ) );
        pxAzureProvClient->_internal.pucScratchBuffer = pucBuffer;
        pucNetworkBuffer = pucBuffer + pxAzureProvClient->_internal.ulScratchBufferLength;
        ulNetworkBufferLength = ulBufferLength - pxAzureProvClient->_internal.ulScratchBufferLength;

        pxAzureProvClient->_internal.pucEndpoint = pucEndpoint;
        pxAzureProvClient->_internal.ulEndpointLength = ulEndpointLength;
        pxAzureProvClient->_internal.pucIDScope = pucIDScope;
        pxAzureProvClient->_internal.ulIDScopeLength = ulIDScopeLength;
        pxAzureProvClient->_internal.pucRegistrationID = pucRegistrationID;
        pxAzureProvClient->_internal.ulRegistrationIDLength = ulRegistrationIDLength;
        pxAzureProvClient->_internal.xGetTimeFunction = xGetTimeFunction;

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

        xCoreResult = az_iot_provisioning_client_init( &( pxAzureProvClient->_internal.xProvisioningClientCore ),
                                                       xEndpoint, xIDScope, xRegistrationID, &xOptions );

        if( az_result_failed( xCoreResult ) )
        {
            AZLogError( ( "AzureIoTProvisioning initialization failed: with core error : %08x", xCoreResult ) );
            xResult = eAzureIoTProvisioningFailed;
        }
        else if( ( xMQTTResult = AzureIoTMQTT_Init( &( pxAzureProvClient->_internal.xMQTTContext ),
                                                    pxTransportInterface, prvProvClientGetTimeMillseconds,
                                                    prvProvClientEventCallback, pucNetworkBuffer,
                                                    ulNetworkBufferLength ) ) != eAzureIoTMQTTSuccess )
        {
            AZLogError( ( "AzureIoTProvisioning initialization failed with mqtt error : %d", xMQTTResult ) );
            xResult = eAzureIoTProvisioningInitFailed;
        }
        else
        {
            xResult = eAzureIoTProvisioningSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClient_t * pxAzureProvClient )
{
    if( pxAzureProvClient == NULL )
    {
        AZLogError( ( "AzureIoTProvisioningClient Deinit failed: Invalid argument" ) );
    }
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SetSymmetricKey( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                               const uint8_t * pucSymmetricKey,
                                                                               uint32_t ulSymmetricKeyLength,
                                                                               AzureIoTGetHMACFunc_t xHmacFunction )
{
    AzureIoTProvisioningClientResult_t xResult;

    if( ( pxAzureProvClient == NULL ) ||
        ( pucSymmetricKey == NULL ) || ( ulSymmetricKeyLength == 0 ) ||
        ( xHmacFunction == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning client symmetric key failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        pxAzureProvClient->_internal.pucSymmetricKey = pucSymmetricKey;
        pxAzureProvClient->_internal.ulSymmetricKeyLength = ulSymmetricKeyLength;
        pxAzureProvClient->_internal.pxTokenRefresh = prvProvClientGetToken;
        pxAzureProvClient->_internal.xHMACFunction = xHmacFunction;
        xResult = eAzureIoTProvisioningSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                        uint32_t ulTimeoutMilliseconds )
{
    AzureIoTProvisioningClientResult_t xResult;

    if( pxAzureProvClient == NULL )
    {
        AZLogError( ( "AzureIoTProvisioning registration failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else
    {
        if( pxAzureProvClient->_internal.ulWorkflowState == azureiotprovisioningWF_STATE_INIT )
        {
            pxAzureProvClient->_internal.ulWorkflowState = azureiotprovisioningWF_STATE_CONNECT;
        }

        xResult = prvProvClientRunWorkflow( pxAzureProvClient, ulTimeoutMilliseconds );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_GetDeviceAndHub( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                               uint8_t * pucHubHostname,
                                                                               uint32_t * pulHostnameLength,
                                                                               uint8_t * pucDeviceID,
                                                                               uint32_t * pulDeviceIDLength )
{
    uint32_t ulHostnameLength;
    uint32_t ulDeviceIDLength;
    AzureIoTProvisioningClientResult_t xResult;
    az_span * pxHostname;
    az_span * pxDeviceID;

    if( ( pxAzureProvClient == NULL ) || ( pucHubHostname == NULL ) ||
        ( pulHostnameLength == NULL ) || ( pucDeviceID == NULL ) || ( pulDeviceIDLength == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning get hub failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in complete state" ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    else if( pxAzureProvClient->_internal.ulLastOperationResult )
    {
        xResult = pxAzureProvClient->_internal.ulLastOperationResult;
    }
    else
    {
        pxHostname = &pxAzureProvClient->_internal.xRegisterResponse.registration_state.assigned_hub_hostname;
        pxDeviceID = &pxAzureProvClient->_internal.xRegisterResponse.registration_state.device_id;
        ulHostnameLength = ( uint32_t ) az_span_size( *pxHostname );
        ulDeviceIDLength = ( uint32_t ) az_span_size( *pxDeviceID );

        if( ( *pulHostnameLength < ulHostnameLength ) || ( *pulDeviceIDLength < ulDeviceIDLength ) )
        {
            AZLogWarn( ( "AzureIoTProvisioning Memory buffer passed is not enough to store hub info" ) );
            xResult = eAzureIoTProvisioningFailed;
        }
        else
        {
            memcpy( pucHubHostname, az_span_ptr( *pxHostname ), ulHostnameLength );
            memcpy( pucDeviceID, az_span_ptr( *pxDeviceID ), ulDeviceIDLength );
            *pulHostnameLength = ulHostnameLength;
            *pulDeviceIDLength = ulDeviceIDLength;
            xResult = eAzureIoTProvisioningSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_GetExtendedCode( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                               uint32_t * pulExtendedErrorCode )
{
    AzureIoTProvisioningClientResult_t xResult;

    if( ( pxAzureProvClient == NULL ) ||
        ( pulExtendedErrorCode == NULL ) )
    {
        AZLogError( ( "AzureIoTProvisioning client get extended code failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in complete state" ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    else
    {
        *pulExtendedErrorCode =
            pxAzureProvClient->_internal.xRegisterResponse.registration_state.extended_error_code;
        xResult = eAzureIoTProvisioningSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SetRegistrationPayload( AzureIoTProvisioningClient_t * pxAzureProvClient,
                                                                                      const uint8_t * pucPayload,
                                                                                      uint32_t ulPayloadLength )
{
    AzureIoTProvisioningClientResult_t xResult;

    if( ( pxAzureProvClient == NULL ) ||
        ( pucPayload == NULL ) || ( ulPayloadLength == 0 ) )
    {
        AZLogError( ( "AzureIoTProvisioning setting client registration payload failed: Invalid argument" ) );
        xResult = eAzureIoTProvisioningInvalidArgument;
    }
    else if( pxAzureProvClient->_internal.ulWorkflowState != azureiotprovisioningWF_STATE_INIT )
    {
        AZLogError( ( "AzureIoTProvisioning client state is not in init" ) );
        xResult = eAzureIoTProvisioningFailed;
    }
    else
    {
        pxAzureProvClient->_internal.pucRegistrationPayload = pucPayload;
        pxAzureProvClient->_internal.ulRegistrationPayloadLength = ulPayloadLength;
        xResult = eAzureIoTProvisioningSuccess;
    }

    return xResult;
}
/*-----------------------------------------------------------*/
