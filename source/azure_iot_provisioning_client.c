/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_provisioning_client.c
 * @brief -------.
 */

#include "azure_iot_provisioning_client.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "azure/az_iot.h"
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
    #define azureiotprovisioningUSER_AGENT             ""
#endif /* azureiotprovisioningUSER_AGENT */

#define azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS    ( 500U )

#define azureiotprovisioningWF_STATE_INIT              ( 0x0 )
#define azureiotprovisioningWF_STATE_CONNECT           ( 0x1 )
#define azureiotprovisioningWF_STATE_SUBSCRIBE         ( 0x2 )
#define azureiotprovisioningWF_STATE_REQUEST           ( 0x3 )
#define azureiotprovisioningWF_STATE_RESPONSE          ( 0x4 )
#define azureiotprovisioningWF_STATE_SUBSCRIBING       ( 0x5 )
#define azureiotprovisioningWF_STATE_REQUESTING        ( 0x6 )
#define azureiotprovisioningWF_STATE_WAITING           ( 0x7 )
#define azureiotprovisioningWF_STATE_COMPLETE          ( 0x8 )

/*-----------------------------------------------------------*/

/**
 *
 * State transitions :
 *          CONNECT -> { SUBSCRIBE | COMPLETE } -> { SUBSCRIBING | COMPLETE } -> { REQUEST | COMPLETE } ->
 *          REQUESTING -> { RESPONSE | COMPLETE } -> { WAITING | COMPLETE } -> { REQUEST | COMPLETE }
 *
 * Note : At COMPLETE state we should check the status of lastOperation to see if WorkFlow( WF )
 *        finshed without error.
 *
 **/
static void prvAzureIoTProvisioningClientUpdateState( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                      uint32_t action_result )
{
    uint32_t state = xAzureIoTProvisioningClientHandle->_internal.workflowState;

    xAzureIoTProvisioningClientHandle->_internal.lastOperationResult = action_result;

    switch( state )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_SUBSCRIBE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_SUBSCRIBING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_REQUEST :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_REQUESTING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_REQUESTING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_RESPONSE :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_PENDING ? azureiotprovisioningWF_STATE_WAITING :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? azureiotprovisioningWF_STATE_REQUEST :
                azureiotprovisioningWF_STATE_COMPLETE;
            break;

        case azureiotprovisioningWF_STATE_COMPLETE:
            AZLogInfo( ( "Complete done\r\n" ) );
            break;

        default:
            AZLogError( ( "Unknown state: %u", state ) );
            configASSERT( false );
            break;
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of connect action, this action is only allowed in azureiotprovisioningWF_STATE_CONNECT
 *
 * */
static void prvAzureIoTProvisioningClientConnect( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTProvisioningClientResult_t ret;
    AzureIoTMQTTStatus_t xResult;
    bool xSessionPresent;
    uint32_t bytesCopied = 0;
    size_t mqtt_user_name_length;
    az_result res;

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState != azureiotprovisioningWF_STATE_CONNECT )
    {
        AZLogWarn( ( "Connect action called in wrong state %d\r\n",
                     xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
        return;
    }

    if( ( xConnectInfo.pUserName = ( char * ) pvPortMalloc( azureiotUSERNAME_MAX ) ) == NULL )
    {
        AZLogError( ( "Failed to allocate memory\r\n" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_OUT_OF_MEMORY;
    }
    else if( az_result_failed( res = az_iot_provisioning_client_get_user_name( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                               ( char * ) xConnectInfo.pUserName,
                                                                               azureiotUSERNAME_MAX, &mqtt_user_name_length ) ) )
    {
        AZLogError( ( "Failed to get username: %08x \r\n", res ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
    }
    /* Check if token refersh is set, then generate password */
    else if( ( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh ) &&
             ( ( ( xConnectInfo.pPassword = ( char * ) pvPortMalloc( azureiotPASSWORD_MAX ) ) == NULL ) ||
               ( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh( xAzureIoTProvisioningClientHandle,
                                                                                                           xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() + azureiotprovisioningDEFAULT_TOKEN_TIMEOUT_IN_SEC,
                                                                                                           xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key,
                                                                                                           xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key_length,
                                                                                                           ( uint8_t * ) xConnectInfo.pPassword,
                                                                                                           azureiotPASSWORD_MAX,
                                                                                                           &bytesCopied ) ) ) )
    {
        AZLogError( ( "Failed to generate auth token \r\n" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }
    else
    {
        xConnectInfo.cleanSession = true;
        xConnectInfo.pClientIdentifier = ( const char * ) xAzureIoTProvisioningClientHandle->_internal.pRegistrationId;
        xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTProvisioningClientHandle->_internal.registrationIdLength;
        xConnectInfo.userNameLength = ( uint16_t ) mqtt_user_name_length;
        xConnectInfo.keepAliveSeconds = azureiotprovisioningKEEP_ALIVE_TIMEOUT_SECONDS;
        xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;

        if( ( xResult = AzureIoTMQTT_Connect( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ),
                                              &xConnectInfo,
                                              NULL,
                                              azureiotprovisioningCONNACK_RECV_TIMEOUT_MS,
                                              &xSessionPresent ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%d \r\n",
                          xAzureIoTProvisioningClientHandle->_internal.endpointLength, xAzureIoTProvisioningClientHandle->_internal.pEndpoint,
                          xResult ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
        }
        else
        {
            /* Successfully established and MQTT connection with the broker. */
            AZLogInfo( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTProvisioningClientHandle->_internal.endpointLength,
                         xAzureIoTProvisioningClientHandle->_internal.pEndpoint ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }
    }

    if( xConnectInfo.pUserName != NULL )
    {
        vPortFree( ( void * ) xConnectInfo.pUserName );
    }

    if( xConnectInfo.pPassword != NULL )
    {
        vPortFree( ( void * ) xConnectInfo.pPassword );
    }

    prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, ret );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of subscribe action, this action is only allowed in azureiotprovisioningWF_STATE_SUBSCRIBE
 *
 * */
static void prvAzureIoTProvisioningClientSubscribe( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientResult_t ret;
    uint16_t usSubscribePacketIdentifier;

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_SUBSCRIBE )
    {
        mqttSubscription.qos = AzureIoTMQTTQoS0;
        mqttSubscription.pTopicFilter = AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
        mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC ) - 1;

        usSubscribePacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ) );

        AZLogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if( ( xResult = AzureIoTMQTT_Subscribe( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ),
                                                &mqttSubscription, 1, usSubscribePacketIdentifier ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%d", xResult ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
        }
        else
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }

        prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, ret );
    }
    else
    {
        AZLogWarn( ( "Subscribe action called in wrong state %d\r\n",
                     xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of request action, this action is only allowed in azureiotprovisioningWF_STATE_REQUEST
 *
 * */
static void prvAzureIoTProvisioningClientRequest( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientResult_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    size_t mqtt_topic_length;
    uint16_t usPublishPacketIdentifier;
    az_result core_result;

    /* Check the state.  */
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_REQUEST )
    {
        if( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length == 0 )
        {
            core_result = az_iot_provisioning_client_register_get_publish_topic( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                                 xAzureIoTProvisioningClientHandle->_internal.provisioning_topic_buffer,
                                                                                 sizeof( xAzureIoTProvisioningClientHandle->_internal.provisioning_topic_buffer ),
                                                                                 &mqtt_topic_length );
        }
        else
        {
            core_result = az_iot_provisioning_client_query_status_get_publish_topic( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                                     xAzureIoTProvisioningClientHandle->_internal.register_response.operation_id,
                                                                                     xAzureIoTProvisioningClientHandle->_internal.provisioning_topic_buffer,
                                                                                     sizeof( xAzureIoTProvisioningClientHandle->_internal.provisioning_topic_buffer ),
                                                                                     &mqtt_topic_length );
        }

        if( az_result_failed( core_result ) )
        {
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            return;
        }

        ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

        xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
        xMQTTPublishInfo.pTopicName = xAzureIoTProvisioningClientHandle->_internal.provisioning_topic_buffer;
        xMQTTPublishInfo.topicNameLength = ( uint16_t ) mqtt_topic_length;
        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ) );

        if( ( xResult = AzureIoTMQTT_Publish( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ),
                                              &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to publish. Error=%d", xResult ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
        }
        else
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }

        prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, ret );
    }
    else
    {
        AZLogWarn( ( "Request action called in wrong state %d\r\n",
                     xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of parsing response action, this action is only allowed in azureiotprovisioningWF_STATE_RESPONSE
 *
 * */
static void prvAzureIoTProvisioningClientParseResponse( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    az_result core_result;
    az_span payload = az_span_create( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload,
                                      ( int32_t ) xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length );
    az_span topic = az_span_create( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic,
                                    ( int32_t ) xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic_length );

    /* Check the state.  */
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_RESPONSE )
    {
        core_result =
            az_iot_provisioning_client_parse_received_topic_and_payload( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                         topic, payload, &xAzureIoTProvisioningClientHandle->_internal.register_response );

        if( az_result_failed( core_result ) )
        {
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            AZLogError( ( "IoTProvisioning client failed to parse packet, error status: %08x", core_result ) );
            return;
        }

        if( xAzureIoTProvisioningClientHandle->_internal.register_response.operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED )
        {
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
        else if( xAzureIoTProvisioningClientHandle->_internal.register_response.retry_after_seconds == 0 )
        {
            AZLogError( ( "IoTProvisioning client failed with error %d, extended error status: %u\r\n",
                          xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.error_code,
                          xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.extended_error_code ) );
            /* Server responded with error with no retry.  */
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR );
        }
        else
        {
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout =
                xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() + xAzureIoTProvisioningClientHandle->_internal.register_response.retry_after_seconds;
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_PENDING );
        }
    }
    else
    {
        AZLogWarn( ( "Parse response action called in wrong state %d\r\n",
                     xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
    }
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of wait check action, this action is only allowed in azureiotprovisioningWF_STATE_WAITING
 *
 * */
static void prvAzureIoTProvisioningClientCheckTimeout( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_WAITING )
    {
        if( xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() >
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout )
        {
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout = 0;
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Trigger state machine action base on the state.
 *
 **/
static void prvTriggerAction( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    switch( xAzureIoTProvisioningClientHandle->_internal.workflowState )
    {
        case azureiotprovisioningWF_STATE_CONNECT:
            prvAzureIoTProvisioningClientConnect( xAzureIoTProvisioningClientHandle );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBE:
            prvAzureIoTProvisioningClientSubscribe( xAzureIoTProvisioningClientHandle );
            break;

        case azureiotprovisioningWF_STATE_REQUEST:
            prvAzureIoTProvisioningClientRequest( xAzureIoTProvisioningClientHandle );
            break;

        case azureiotprovisioningWF_STATE_RESPONSE:
            prvAzureIoTProvisioningClientParseResponse( xAzureIoTProvisioningClientHandle );
            break;

        case azureiotprovisioningWF_STATE_SUBSCRIBING:
        case azureiotprovisioningWF_STATE_REQUESTING:
        case azureiotprovisioningWF_STATE_COMPLETE:
            /* None action taken here, as these states are waiting for receive path. */
            break;

        case azureiotprovisioningWF_STATE_WAITING:
            prvAzureIoTProvisioningClientCheckTimeout( xAzureIoTProvisioningClientHandle );
            break;

        default:
            AZLogError( ( "State not handled %d\r\n",
                          xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
            configASSERT( false );
    }
}
/*-----------------------------------------------------------*/

/**
 *  Run the workflow : trigger action on state and process receive path in MQTT loop
 *
 */
static AzureIoTProvisioningClientResult_t prvAzureIoTProvisioningClientRunWorkflow( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                                    uint32_t ulTimeoutMilliseconds )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientResult_t ret;
    uint32_t waitTime;

    do
    {
        if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_COMPLETE )
        {
            break;
        }

        if( ulTimeoutMilliseconds > azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS )
        {
            ulTimeoutMilliseconds -= azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS;
            waitTime = azureiotprovisioningPROCESS_LOOP_TIMEOUT_MS;
        }
        else
        {
            waitTime = ulTimeoutMilliseconds;
            ulTimeoutMilliseconds = 0;
        }

        prvTriggerAction( xAzureIoTProvisioningClientHandle );

        if( ( xResult = AzureIoTMQTT_ProcessLoop( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ), waitTime ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to process loop: ProcessLoopDuration=%u, Error=%d\r\n", ulTimeoutMilliseconds, xResult ) );
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            break;
        }
    } while( ulTimeoutMilliseconds );

    if( ( xAzureIoTProvisioningClientHandle->_internal.workflowState != azureiotprovisioningWF_STATE_COMPLETE ) )
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_PENDING;
    }
    else
    {
        ret = xAzureIoTProvisioningClientHandle->_internal.lastOperationResult;
    }

    return ret;
}
/*-----------------------------------------------------------*/

static void prvMQTTProcessSubAck( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                  AzureIoTMQTTPublishInfo_t * pxPacketInfo )
{
    ( void ) pxPacketInfo;
    /* TODO check the state */
    prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
}
/*-----------------------------------------------------------*/

/**
 * Process MQTT Response from Provisioning Service
 *
 */
static void prvMQTTProcessResponse( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                    AzureIoTMQTTPublishInfo_t * pPublishInfo )
{
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_REQUESTING )
    {
        vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload );
        vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic );

        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload =
            ( uint8_t * ) pvPortMalloc( pPublishInfo->payloadLength );
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic =
            ( uint8_t * ) pvPortMalloc( pPublishInfo->topicNameLength );

        if( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload &&
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic )
        {
            memcpy( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload,
                    pPublishInfo->pPayload, pPublishInfo->payloadLength );
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length = pPublishInfo->payloadLength;
            memcpy( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic,
                    pPublishInfo->pTopicName, pPublishInfo->topicNameLength );
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic_length = pPublishInfo->topicNameLength;
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
        else
        {
            prvAzureIoTProvisioningClientUpdateState( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
        }
    }
}
/*-----------------------------------------------------------*/

static void prvEventCallback( AzureIoTMQTTHandle_t pxMQTTContext,
                              AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                              AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle = ( AzureIoTProvisioningClientHandle_t ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == AZURE_IOT_MQTT_PACKET_TYPE_SUBACK )
    {
        prvMQTTProcessSubAck( xAzureIoTProvisioningClientHandle, pxDeserializedInfo->pPublishInfo );
    }
    else if( ( pxPacketInfo->type & 0xF0U ) == AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessResponse( xAzureIoTProvisioningClientHandle, pxDeserializedInfo->pPublishInfo );
    }
}
/*-----------------------------------------------------------*/

static uint32_t prvAzureIoTProvisioningClientTokenGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
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

    core_result = az_iot_provisioning_client_sas_get_signature( &( xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core ),
                                                                expiryTimeSecs, span, &span );

    if( az_result_failed( core_result ) )
    {
        AZLogError( ( "IoTProvisioning failed failed to get signature with error status: %08x", core_result ) );
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    bytesUsed = ( uint32_t ) az_span_size( span );

    if( AzureIoT_Base64HMACCalculate( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_hmac_function,
                                      pKey, keyLen, pSASBuffer, bytesUsed,
                                      buffer, sizeof( buffer ), &pOutput, &outputLen ) )
    {
        AZLogError( ( "IoTProvisioning failed to encoded hash" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    span = az_span_create( pOutput, ( int32_t ) outputLen );
    core_result = az_iot_provisioning_client_sas_get_password( &( xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core ),
                                                               span, expiryTimeSecs, AZ_SPAN_EMPTY,
                                                               ( char * ) pSASBuffer, sasBufferLen, &length );

    if( az_result_failed( core_result ) )
    {
        AZLogError( ( "IoTProvisioning failed to generate token with error status: %08x", core_result ) );
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    *pSaSLength = ( uint32_t ) length;

    return AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
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

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_OptionsInit( AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions )
{
    AzureIoTProvisioningClientResult_t ret;

    if( pxProvisioningClientOptions == NULL )
    {
        AZLogError( ( "Provisioning failed to initialize options: Invalid argument \r\n" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        pxProvisioningClientOptions->pUserAgent = ( const uint8_t * ) azureiotprovisioningUSER_AGENT;
        pxProvisioningClientOptions->userAgentLength = sizeof( azureiotprovisioningUSER_AGENT ) - 1;
        ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                    const uint8_t * pucEndpoint,
                                                                    uint32_t ulEndpointLength,
                                                                    const uint8_t * pucIdScope,
                                                                    uint32_t ulIdScopeLength,
                                                                    const uint8_t * pucRegistrationId,
                                                                    uint32_t ulRegistrationIdLength,
                                                                    AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions,
                                                                    uint8_t * pucBuffer,
                                                                    uint32_t ulBufferLength,
                                                                    AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                                    const AzureIoTTransportInterface_t * pxTransportInterface )
{
    AzureIoTProvisioningClientResult_t ret;
    az_span endpoint_span = az_span_create( ( uint8_t * ) pucEndpoint, ( int32_t ) ulEndpointLength );
    az_span id_scope_span = az_span_create( ( uint8_t * ) pucIdScope, ( int32_t ) ulIdScopeLength );
    az_span registration_id_span = az_span_create( ( uint8_t * ) pucRegistrationId, ( int32_t ) ulRegistrationIdLength );
    az_result core_result;
    AzureIoTMQTTStatus_t xResult;
    az_iot_provisioning_client_options options = az_iot_provisioning_client_options_default();

    if( ( xAzureIoTProvisioningClientHandle == NULL ) ||
        ( pucEndpoint == NULL ) || ( ulEndpointLength == 0 ) ||
        ( pucIdScope == NULL ) || ( ulIdScopeLength == 0 ) ||
        ( pucRegistrationId == NULL ) || ( ulRegistrationIdLength == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( xGetTimeFunction == NULL ) || ( pxTransportInterface == NULL ) )
    {
        AZLogError( ( "Provisioning initialization failed: Invalid argument" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        memset( xAzureIoTProvisioningClientHandle, 0, sizeof( AzureIoTProvisioningClient_t ) );

        xAzureIoTProvisioningClientHandle->_internal.pEndpoint = pucEndpoint;
        xAzureIoTProvisioningClientHandle->_internal.endpointLength = ulEndpointLength;
        xAzureIoTProvisioningClientHandle->_internal.pIdScope = pucIdScope;
        xAzureIoTProvisioningClientHandle->_internal.idScopeLength = ulIdScopeLength;
        xAzureIoTProvisioningClientHandle->_internal.pRegistrationId = pucRegistrationId;
        xAzureIoTProvisioningClientHandle->_internal.registrationIdLength = ulRegistrationIdLength;
        xAzureIoTProvisioningClientHandle->_internal.getTimeFunction = xGetTimeFunction;

        if( pxProvisioningClientOptions )
        {
            options.user_agent = az_span_create( ( uint8_t * ) pxProvisioningClientOptions->pUserAgent, ( int32_t ) pxProvisioningClientOptions->userAgentLength );
        }
        else
        {
            options.user_agent = az_span_create( ( uint8_t * ) azureiotprovisioningUSER_AGENT, sizeof( azureiotprovisioningUSER_AGENT ) - 1 );
        }

        core_result = az_iot_provisioning_client_init( &( xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core ),
                                                       endpoint_span, id_scope_span,
                                                       registration_id_span, &options );

        if( az_result_failed( core_result ) )
        {
            AZLogError( ( "Provisioning initialization failed: with core error : %08x", core_result ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
        }
        else if( ( xResult = AzureIoTMQTT_Init( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ), pxTransportInterface,
                                                prvGetTimeMs, prvEventCallback,
                                                pucBuffer, ulBufferLength ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Provisioning initialization failed: with mqtt init error : %d", xResult ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
        }
        else
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if( xAzureIoTProvisioningClientHandle == NULL )
    {
        AZLogError( ( "AzureIoTProvisioningClient_Deinit failed: Invalid argument" ) );
    }
    else
    {
        vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload );
        vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic );
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload = NULL;
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic = NULL;
    }
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                        uint32_t ulTimeoutMilliseconds )
{
    AzureIoTProvisioningClientResult_t ret;

    if( xAzureIoTProvisioningClientHandle == NULL )
    {
        AZLogError( ( "Provisioning registration failed: Invalid argument" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        if( xAzureIoTProvisioningClientHandle->_internal.workflowState == azureiotprovisioningWF_STATE_INIT )
        {
            xAzureIoTProvisioningClientHandle->_internal.workflowState = azureiotprovisioningWF_STATE_CONNECT;
        }

        ret = prvAzureIoTProvisioningClientRunWorkflow( xAzureIoTProvisioningClientHandle, ulTimeoutMilliseconds );
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                      uint8_t * pucHubHostname,
                                                                      uint32_t * pulHostnameLength,
                                                                      uint8_t * pucDeviceId,
                                                                      uint32_t * pulDeviceIdLength )
{
    uint32_t hostnameLength;
    uint32_t deviceIdLength;
    AzureIoTProvisioningClientResult_t ret;

    if( ( xAzureIoTProvisioningClientHandle == NULL ) || ( pucHubHostname == NULL ) ||
        ( pulHostnameLength == NULL ) || ( pucDeviceId == NULL ) || ( pulDeviceIdLength == NULL ) )
    {
        AZLogError( ( "Provisioning hub get failed: Invalid argument" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else if( xAzureIoTProvisioningClientHandle->_internal.workflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "Provisioning client state is not in complete state" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }
    else if( xAzureIoTProvisioningClientHandle->_internal.lastOperationResult )
    {
        ret = xAzureIoTProvisioningClientHandle->_internal.lastOperationResult;
    }
    else
    {
        hostnameLength = ( uint32_t ) az_span_size( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.assigned_hub_hostname );
        deviceIdLength = ( uint32_t ) az_span_size( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.device_id );

        if( ( *pulHostnameLength < hostnameLength ) || ( *pulDeviceIdLength < deviceIdLength ) )
        {
            AZLogWarn( ( "Memory passed is not enough to store hub info \r\n" ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
        }
        else
        {
            memcpy( pucHubHostname, az_span_ptr( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.assigned_hub_hostname ), hostnameLength );
            memcpy( pucDeviceId, az_span_ptr( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.device_id ), deviceIdLength );
            *pulHostnameLength = hostnameLength;
            *pulDeviceIdLength = deviceIdLength;
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SymmetricKeySet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                               const uint8_t * pucSymmetricKey,
                                                                               uint32_t ulSymmetricKeyLength,
                                                                               AzureIoTGetHMACFunc_t xHmacFunction )
{
    AzureIoTProvisioningClientResult_t ret;

    if( ( xAzureIoTProvisioningClientHandle == NULL ) ||
        ( pucSymmetricKey == NULL ) || ( ulSymmetricKeyLength == 0 ) ||
        ( xHmacFunction == NULL ) )
    {
        AZLogError( ( "Provisioning client symmetric key fail: Invalid argument" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else
    {
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key = pucSymmetricKey;
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key_length = ulSymmetricKeyLength;
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh = prvAzureIoTProvisioningClientTokenGet;
        xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_hmac_function = xHmacFunction;

        ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_ExtendedCodeGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                               uint32_t * pulExtendedErrorCode )
{
    AzureIoTProvisioningClientResult_t ret;

    if( ( xAzureIoTProvisioningClientHandle == NULL ) ||
        ( pulExtendedErrorCode == NULL ) )
    {
        AZLogError( ( "Provisioning client extended code get failed : Invalid argument" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }
    else if( xAzureIoTProvisioningClientHandle->_internal.workflowState != azureiotprovisioningWF_STATE_COMPLETE )
    {
        AZLogError( ( "Provisioning client state is not in complete state" ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }
    else
    {
        *pulExtendedErrorCode =
            xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.extended_error_code;
        ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/
