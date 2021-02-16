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

#define azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS           ( 500U )

#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_INIT           ( 0x0 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT        ( 0x1 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE      ( 0x2 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST        ( 0x3 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE       ( 0x4 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING    ( 0x5 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING     ( 0x6 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING        ( 0x7 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE       ( 0x8 )

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND                               ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK                                 ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

static char mqtt_user_name[ 128 ];
static char mqtt_password[ 256 ];
static char provisioning_topic[ 128 ];

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
static void azure_iot_provisioning_client_update_state( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                        uint32_t action_result )
{
    uint32_t state = xAzureIoTProvisioningClientHandle->_internal.workflowState;

    xAzureIoTProvisioningClientHandle->_internal.lastOperationResult = action_result;

    switch( state )
    {
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING:
            xAzureIoTProvisioningClientHandle->_internal.workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST :
                AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE:
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
 * Implementation of connect action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT
 *
 * */
static void azure_iot_provisioning_client_connect( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTConnectInfo_t xConnectInfo = { 0 };
    AzureIoTProvisioningClientError_t ret;
    AzureIoTMQTTStatus_t xResult;
    bool xSessionPresent;
    uint32_t bytesCopied;
    size_t mqtt_user_name_length;
    az_result res;

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT )
    {
        AZLogWarn( ( "Connect action called in wrong state %d\r\n",
                     xAzureIoTProvisioningClientHandle->_internal.workflowState ) );
        return;
    }

    xConnectInfo.cleanSession = true;
    res = az_iot_provisioning_client_get_user_name( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                    mqtt_user_name, sizeof( mqtt_user_name ), &mqtt_user_name_length );
    if( az_result_failed( res ) )
    {
        AZLogError( ( "Failed to get username: %08x \r\n", res ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
    }
    else
    {
        xConnectInfo.pClientIdentifier = ( const char * ) xAzureIoTProvisioningClientHandle->_internal.pRegistrationId;
        xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTProvisioningClientHandle->_internal.registrationIdLength;
        xConnectInfo.pUserName = mqtt_user_name;
        xConnectInfo.userNameLength = ( uint16_t ) mqtt_user_name_length;
        xConnectInfo.keepAliveSeconds = azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS;
        xConnectInfo.pPassword = mqtt_password;

        if( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh )
        {
            if( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh( xAzureIoTProvisioningClientHandle,
                                                                                                          xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() + azureIoTPROVISIONINGDEFAULTTOKENTIMEOUTINSEC,
                                                                                                          xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key,
                                                                                                          xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key_length,
                                                                                                          ( uint8_t * ) mqtt_password, sizeof( mqtt_password ),
                                                                                                          &bytesCopied ) )
            {
                AZLogError( ( "Failed to generate auth token \r\n" ) );
                azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
                return;
            }

            xConnectInfo.passwordLength = ( uint16_t ) bytesCopied;
        }

        if( ( xResult = AzureIoTMQTT_Connect( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ),
                                              &xConnectInfo,
                                              NULL,
                                              azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS,
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

    azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
}
/*-----------------------------------------------------------*/

/**
 *
 * Implementation of subscribe action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE
 *
 * */
static void azure_iot_provisioning_client_subscribe( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTSubscribeInfo_t mqttSubscription = { 0 };
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE )
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

        azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
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
 * Implementation of request action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST
 *
 * */
static void azure_iot_provisioning_client_request( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientError_t ret;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    size_t mqtt_topic_length;
    uint16_t usPublishPacketIdentifier;
    az_result core_result;

    /* Check the state.  */
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST )
    {
        if( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length == 0 )
        {
            core_result = az_iot_provisioning_client_register_get_publish_topic( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                                 provisioning_topic, sizeof( provisioning_topic ), &mqtt_topic_length );
        }
        else
        {
            core_result = az_iot_provisioning_client_query_status_get_publish_topic( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                                     xAzureIoTProvisioningClientHandle->_internal.register_response.operation_id, provisioning_topic,
                                                                                     sizeof( provisioning_topic ), &mqtt_topic_length );
        }

        if( az_result_failed( core_result ) )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            return;
        }

        ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

        xMQTTPublishInfo.qos = AzureIoTMQTTQoS0;
        xMQTTPublishInfo.pTopicName = provisioning_topic;
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

        azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
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
 * Implementation of parsing response action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE
 *
 * */
static void azure_iot_provisioning_client_parse_response( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    az_result core_result;
    az_span payload = az_span_create( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload,
                                      ( int32_t ) xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length );
    az_span topic = az_span_create( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic,
                                    ( int32_t ) xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic_length );

    /* Check the state.  */
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE )
    {
        core_result =
            az_iot_provisioning_client_parse_received_topic_and_payload( &xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core,
                                                                         topic, payload, &xAzureIoTProvisioningClientHandle->_internal.register_response );

        if( az_result_failed( core_result ) )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            AZLogError( ( "IoTProvisioning client failed to parse packet, error status: %08x", core_result ) );
            return;
        }

        if( xAzureIoTProvisioningClientHandle->_internal.register_response.operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
        else if( xAzureIoTProvisioningClientHandle->_internal.register_response.retry_after_seconds == 0 )
        {
            /* Server responded with error with no retry.  */
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR );
        }
        else
        {
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout =
                xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() + xAzureIoTProvisioningClientHandle->_internal.register_response.retry_after_seconds;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING );
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
 * Implementation of wait check action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING
 *
 * */
static void azure_iot_provisioning_client_check_timeout( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING )
    {
        if( xAzureIoTProvisioningClientHandle->_internal.getTimeFunction() >
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout )
        {
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_req_timeout = 0;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Trigger state machine action base on the state.
 *
 **/
static void triggerAction( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    switch( xAzureIoTProvisioningClientHandle->_internal.workflowState )
    {
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT:
            azure_iot_provisioning_client_connect( xAzureIoTProvisioningClientHandle );
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE:
            azure_iot_provisioning_client_subscribe( xAzureIoTProvisioningClientHandle );
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST:
            azure_iot_provisioning_client_request( xAzureIoTProvisioningClientHandle );
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE:
            azure_iot_provisioning_client_parse_response( xAzureIoTProvisioningClientHandle );
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING:
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING:
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE:
            /* None action taken here, as these states are waiting for receive path. */
            break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING:
            azure_iot_provisioning_client_check_timeout( xAzureIoTProvisioningClientHandle );
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
static AzureIoTProvisioningClientError_t azure_iot_provisioning_client_run_workflow( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                                     uint32_t timeout )
{
    AzureIoTMQTTStatus_t xResult;
    AzureIoTProvisioningClientError_t ret;
    uint32_t waitTime;

    do
    {
        if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE )
        {
            break;
        }

        if( timeout > azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS )
        {
            timeout -= azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS;
            waitTime = azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS;
        }
        else
        {
            waitTime = timeout;
            timeout = 0;
        }

        triggerAction( xAzureIoTProvisioningClientHandle );

        if( ( xResult = AzureIoTMQTT_ProcessLoop( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ), waitTime ) ) != AzureIoTMQTTSuccess )
        {
            AZLogError( ( "Failed to process loop: ProcessLoopDuration=%u, Error=%d\r\n", timeout, xResult ) );
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            break;
        }
    } while( timeout );

    if( ( xAzureIoTProvisioningClientHandle->_internal.workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE ) )
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
    azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
}
/*-----------------------------------------------------------*/

/**
 * Process MQTT Response from Provisioning Service
 *
 */
static void prvMQTTProcessResponse( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                    AzureIoTMQTTPublishInfo_t * pPublishInfo )
{
    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING )
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
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload_length =  pPublishInfo->payloadLength;
            memcpy( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic,
                    pPublishInfo->pTopicName, pPublishInfo->topicNameLength );
            xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic_length = pPublishInfo->topicNameLength;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
        else
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
        }
    }
}
/*-----------------------------------------------------------*/

static void prvEventCallback( AzureIoTMQTTHandle_t pxMQTTContext,
                              AzureIoTMQTTPacketInfo_t * pxPacketInfo,
                              AzureIoTMQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
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

static uint32_t azure_iot_provisioning_client_token_get( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
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

    if( AzureIoTBase64HMACCalculate( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_hmac_function,
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
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    return ulTimeMs;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                   const uint8_t * pEndpoint,
                                                                   uint32_t endpointLength,
                                                                   const uint8_t * pIdScope,
                                                                   uint32_t idScopeLength,
                                                                   const uint8_t * pRegistrationId,
                                                                   uint32_t registrationIdLength,
                                                                   uint8_t * pBuffer,
                                                                   uint32_t bufferLength,
                                                                   AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                                   const TransportInterface_t * pTransportInterface )
{
    AzureIoTProvisioningClientError_t ret;
    az_span endpoint_span = az_span_create( ( uint8_t * ) pEndpoint, ( int32_t ) endpointLength );
    az_span id_scope_span = az_span_create( ( uint8_t * ) pIdScope, ( int32_t ) idScopeLength );
    az_span registration_id_span = az_span_create( ( uint8_t * ) pRegistrationId, ( int32_t ) registrationIdLength );
    az_result core_result;
    AzureIoTMQTTStatus_t xResult;

    if( ( xAzureIoTProvisioningClientHandle == NULL ) ||
        ( pEndpoint == NULL ) || ( endpointLength == 0 ) ||
        ( pIdScope == NULL ) || ( idScopeLength == 0 ) ||
        ( pRegistrationId == NULL ) || ( registrationIdLength == 0 ) ||
        ( pBuffer == NULL ) || ( bufferLength == 0 ) ||
        ( getTimeFunction == NULL ) || ( pTransportInterface == NULL ) )
    {
        AZLogError( ( "Provisioning initialization failed: Invalid argument" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    memset( xAzureIoTProvisioningClientHandle, 0, sizeof( AzureIoTProvisioningClient_t ) );

    xAzureIoTProvisioningClientHandle->_internal.pEndpoint = pEndpoint;
    xAzureIoTProvisioningClientHandle->_internal.endpointLength = endpointLength;
    xAzureIoTProvisioningClientHandle->_internal.pIdScope = pIdScope;
    xAzureIoTProvisioningClientHandle->_internal.idScopeLength = idScopeLength;
    xAzureIoTProvisioningClientHandle->_internal.pRegistrationId = pRegistrationId;
    xAzureIoTProvisioningClientHandle->_internal.registrationIdLength = registrationIdLength;
    xAzureIoTProvisioningClientHandle->_internal.getTimeFunction = getTimeFunction;

    core_result = az_iot_provisioning_client_init( &( xAzureIoTProvisioningClientHandle->_internal.iot_dps_client_core ),
                                                   endpoint_span, id_scope_span,
                                                   registration_id_span, NULL );
    if( az_result_failed( core_result ) )
    {
        AZLogError( ( "Provisioning initialization failed: with core error : %08x", core_result ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }
    else if( ( xResult = AzureIoTMQTT_Init( &( xAzureIoTProvisioningClientHandle->_internal.xMQTTContext ), pTransportInterface,
                                            prvGetTimeMs, prvEventCallback,
                                            pBuffer, bufferLength ) ) != AzureIoTMQTTSuccess )
    {
        AZLogError( ( "Provisioning initialization failed: with mqtt init error : %d", xResult ) );
        ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
    }

    return ret;
}
/*-----------------------------------------------------------*/

void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if ( xAzureIoTProvisioningClientHandle == NULL )
    {
        AZLogError( ( "AzureIoTProvisioningClient_Deinit failed: Invalid argument" ) );
        return;
    }

    vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload );
    vPortFree( xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic );
    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_payload = NULL;
    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_last_response_topic = NULL;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                       uint32_t timeout )
{
    if( xAzureIoTProvisioningClientHandle == NULL )
    {
        AZLogError( ( "Provisioning registration failed: Invalid argument" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_INIT )
    {
        xAzureIoTProvisioningClientHandle->_internal.workflowState = AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT;
    }

    return azure_iot_provisioning_client_run_workflow( xAzureIoTProvisioningClientHandle, timeout );
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                     uint8_t * pHubHostname,
                                                                     uint32_t * pHostnameLength,
                                                                     uint8_t * pDeviceId,
                                                                     uint32_t * pDeviceIdLength )
{
    uint32_t hostnameLength;
    uint32_t deviceIdLength;

    if( ( xAzureIoTProvisioningClientHandle == NULL ) || ( pHubHostname == NULL ) ||
        ( pHostnameLength == NULL ) || ( pDeviceId == NULL ) || ( pDeviceIdLength == NULL ) )
    {
        AZLogError( ( "Provisioning hub get failed: Invalid argument" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    if( xAzureIoTProvisioningClientHandle->_internal.workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE )
    {
        AZLogError( ( "Provisioning client state is not in complete state" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    if( xAzureIoTProvisioningClientHandle->_internal.lastOperationResult )
    {
        return xAzureIoTProvisioningClientHandle->_internal.lastOperationResult;
    }

    hostnameLength = ( uint32_t ) az_span_size( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.assigned_hub_hostname );
    deviceIdLength = ( uint32_t ) az_span_size( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.device_id );

    if( ( *pHostnameLength < hostnameLength ) || ( *pDeviceIdLength < deviceIdLength ) )
    {
        AZLogWarn( ( "Memory passed is not enough to store hub info \r\n" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    memcpy( pHubHostname, az_span_ptr( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.assigned_hub_hostname ), hostnameLength );
    memcpy( pDeviceId, az_span_ptr( xAzureIoTProvisioningClientHandle->_internal.register_response.registration_state.device_id ), deviceIdLength );
    *pHostnameLength = hostnameLength;
    *pDeviceIdLength = deviceIdLength;

    return AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
}
/*-----------------------------------------------------------*/

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_SymmetricKeySet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                              const uint8_t * pSymmetricKey,
                                                                              uint32_t pSymmetricKeyLength,
                                                                              AzureIoTGetHMACFunc_t hmacFunction )
{
    if( ( xAzureIoTProvisioningClientHandle == NULL ) ||
        ( pSymmetricKey == NULL ) || ( pSymmetricKeyLength == 0 ) )
    {
        AZLogError( ( "Provisioning client symmetric key fail: Invalid argument" ) );
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key = pSymmetricKey;
    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_symmetric_key_length = pSymmetricKeyLength;
    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_token_refresh = azure_iot_provisioning_client_token_get;
    xAzureIoTProvisioningClientHandle->_internal.azure_iot_provisioning_client_hmac_function = hmacFunction;

    return AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
}
/*-----------------------------------------------------------*/
