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
 * @file azure_iot_provisioning_client.c
 * @brief -------.
 */

#include <stdio.h>

#include "azure_iot_provisioning_client.h"

#include "azure/az_iot.h"

#define azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS             ( 500U )

#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT          ( 0x1 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE        ( 0x2 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST          ( 0x3 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE         ( 0x4 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING      ( 0x5 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING       ( 0x6 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING          ( 0x7 )
#define AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE         ( 0x8 )

static char mqtt_user_name[128];
static char provisioning_topic[128];

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
static void azure_iot_provisioning_client_update_state( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle, uint32_t action_result )
{
    uint32_t state = xAzureIoTProvisioningClientHandle->workflowState;

    xAzureIoTProvisioningClientHandle->lastOperationResult = action_result;

    switch ( state )
    {
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST:
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING :
        {
            xAzureIoTProvisioningClientHandle->workflowState =
                action_result == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS ? AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST :
                    AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE;
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE :
        {
            LogInfo( ( "Complete done\r\n" ) );
        }
        break;

        default :
        {
            LogError( ( "Unknown state: %u", state ) );
            configASSERT( false );
        }
        break;
    }
}

/**
 * 
 * Implementation of connect action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT
 *
 * */
static void azure_iot_provisioning_client_connect( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    MQTTConnectInfo_t xConnectInfo;
    AzureIoTProvisioningClientError_t ret;
    MQTTStatus_t xResult;
    bool xSessionPresent;

    if ( xAzureIoTProvisioningClientHandle->workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT )
    {
        return;
    }

    ( void ) memset( ( void * ) &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

    xConnectInfo.cleanSession = true;

    size_t mqtt_user_name_length;
    if( az_result_failed( az_iot_provisioning_client_get_user_name( &xAzureIoTProvisioningClientHandle->iot_dps_client_core,
                                                                    mqtt_user_name, sizeof( mqtt_user_name ), &mqtt_user_name_length ) ) )
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
    }
    else
    {
        xConnectInfo.pClientIdentifier = xAzureIoTProvisioningClientHandle->pRegistrationId;
        xConnectInfo.clientIdentifierLength = ( uint16_t ) xAzureIoTProvisioningClientHandle->registrationIdLength;
        xConnectInfo.pUserName = mqtt_user_name;
        xConnectInfo.userNameLength = ( uint16_t )mqtt_user_name_length;
        xConnectInfo.pPassword = NULL;
        xConnectInfo.passwordLength = 0;
        xConnectInfo.keepAliveSeconds = azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS;

        if ( ( xResult = MQTT_Connect( &( xAzureIoTProvisioningClientHandle->xMQTTContext ),
                                       &xConnectInfo,
                                       NULL,
                                       azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS,
                                       &xSessionPresent ) ) != MQTTSuccess )
        {
            LogError( ( "Failed to establish MQTT connection: Server=%.*s, MQTTStatus=%s \r\n",
                        xAzureIoTProvisioningClientHandle->endpointLength, xAzureIoTProvisioningClientHandle->pEndpoint,
                        MQTT_Status_strerror( xResult ) ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
        }
        else
        {
            /* Successfully established and MQTT connection with the broker. */
            LogInfo( ( "An MQTT connection is established with %.*s.\r\n", xAzureIoTProvisioningClientHandle->endpointLength,
                       xAzureIoTProvisioningClientHandle->pEndpoint ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }
    }

    azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
}

/**
 * 
 * Implementation of subscribe action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE
 *
 * */
static void azure_iot_provisioning_client_subscribe( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    MQTTSubscribeInfo_t mqttSubscription;
    MQTTStatus_t xResult;
    AzureIoTProvisioningClientError_t ret;
    uint16_t usSubscribePacketIdentifier;

    if ( xAzureIoTProvisioningClientHandle->workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE )
    {
        mqttSubscription.qos = MQTTQoS0;
        mqttSubscription.pTopicFilter = AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
        mqttSubscription.topicFilterLength = ( uint16_t ) sizeof( AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC ) - 1;

        usSubscribePacketIdentifier = MQTT_GetPacketId( & ( xAzureIoTProvisioningClientHandle->xMQTTContext ) );

        LogInfo( ( "Attempt to subscribe to the MQTT  %s topics.\r\n", AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC ) );

        if ( ( xResult = MQTT_Subscribe( &( xAzureIoTProvisioningClientHandle->xMQTTContext ),
                                         &mqttSubscription, 1,  usSubscribePacketIdentifier ) ) != MQTTSuccess )
        {
            LogError( ( "Failed to SUBSCRIBE to MQTT topic. Error=%s",
                        MQTT_Status_strerror( xResult ) ) );
            ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
        }
        else
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }

        azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
    }
}

/**
 * 
 * Implementation of request action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST
 *
 * */
static void azure_iot_provisioning_client_request( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    MQTTStatus_t xResult;
    AzureIoTProvisioningClientError_t ret;
    MQTTPublishInfo_t xMQTTPublishInfo;
    uint32_t mqtt_topic_length;
    uint16_t usPublishPacketIdentifier;
    az_result core_result;

    /* Check the state.  */
    if ( xAzureIoTProvisioningClientHandle->workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST )
    {
        if ( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload_length == 0 )
        {
            core_result = az_iot_provisioning_client_register_get_publish_topic( &xAzureIoTProvisioningClientHandle->iot_dps_client_core,
                                                                                 provisioning_topic, sizeof( provisioning_topic ), &mqtt_topic_length );
        }
        else
        {
            core_result = az_iot_provisioning_client_query_status_get_publish_topic( &xAzureIoTProvisioningClientHandle->iot_dps_client_core,
                                                                                     xAzureIoTProvisioningClientHandle->register_response.operation_id, provisioning_topic,
                                                                                     sizeof( provisioning_topic ), &mqtt_topic_length );
        }

        if ( az_result_failed( core_result ) )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle,  AZURE_IOT_PROVISIONING_CLIENT_FAILED);
            return;
        }

        ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

        xMQTTPublishInfo.qos = MQTTQoS0;
        xMQTTPublishInfo.retain = false;
        xMQTTPublishInfo.pTopicName = provisioning_topic;
        xMQTTPublishInfo.topicNameLength = ( uint16_t ) mqtt_topic_length;
        xMQTTPublishInfo.pPayload = NULL;
        xMQTTPublishInfo.payloadLength = 0;
        usPublishPacketIdentifier = MQTT_GetPacketId( &( xAzureIoTProvisioningClientHandle->xMQTTContext ) );

        if ( ( xResult = MQTT_Publish( &( xAzureIoTProvisioningClientHandle->xMQTTContext ),
                                       &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != MQTTSuccess )
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED; 
        }
        else
        {
            ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
        }

        azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, ret );
    }
}

/**
 * 
 * Implementation of parsing response action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE
 *
 * */
static void azure_iot_provisioning_client_parse_response( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    az_result core_result;
    az_span payload = az_span_create( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload,
                                      xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload_length );
    az_span topic = az_span_create( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic,
                                    xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic_length );

    /* Check the state.  */
    if ( xAzureIoTProvisioningClientHandle->workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE )
    {
        core_result =
            az_iot_provisioning_client_parse_received_topic_and_payload( &xAzureIoTProvisioningClientHandle -> iot_dps_client_core,
                                                                         topic, payload, &xAzureIoTProvisioningClientHandle->register_response );
        if ( az_result_failed( core_result ) )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
            LogError( ( "IoTProvisioning client failed to parse packet, error status: %d", core_result ) );
            return;
        }

        if ( xAzureIoTProvisioningClientHandle->register_response.operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED )
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
        else if ( xAzureIoTProvisioningClientHandle->register_response.retry_after_seconds == 0 )
        {

            /* Server responded with error with no retry.  */
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR );
        }
        else
        {
            xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_req_timeout =
                xAzureIoTProvisioningClientHandle->getTimeFunction() + xAzureIoTProvisioningClientHandle->register_response.retry_after_seconds * 1000;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING );
        }
    }
}

/**
 * 
 * Implementation of wait check action, this action is only allowed in AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING
 *
 * */
static void azure_iot_provisioning_client_check_timeout( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if (xAzureIoTProvisioningClientHandle->workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING)
    {
        if ( xAzureIoTProvisioningClientHandle->getTimeFunction() >
                xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_req_timeout )
        {
            xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_req_timeout = 0;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        }
    }
}

/**
 * Trigger state machine action base on the state.
 *  
 **/
static void triggerAction( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{

    switch ( xAzureIoTProvisioningClientHandle->workflowState )
    {
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT :
        {
            azure_iot_provisioning_client_connect( xAzureIoTProvisioningClientHandle );
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBE :
        {
            azure_iot_provisioning_client_subscribe( xAzureIoTProvisioningClientHandle );
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUEST :
        {
            azure_iot_provisioning_client_request( xAzureIoTProvisioningClientHandle );
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_RESPONSE :
        {
            azure_iot_provisioning_client_parse_response( xAzureIoTProvisioningClientHandle );
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_SUBSCRIBING :
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING :
        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE :
        {
            /* None action taken here, as these states are waiting for receive path. */
        }
        break;

        case AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_WAITING :
        {
            azure_iot_provisioning_client_check_timeout( xAzureIoTProvisioningClientHandle );
        }
        break;
        
        default :
        {
            configASSERT( false );
        }
    }
}

/**
 *  Run the workflow : trigger action on state and process receive path in MQTT loop
 * 
 */
static AzureIoTProvisioningClientError_t azure_iot_provisioning_client_run_workflow( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    MQTTStatus_t xResult;
    xAzureIoTProvisioningClientHandle->workflowState = AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_CONNECT;

    while ( ( xAzureIoTProvisioningClientHandle->workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE ))
    {
        triggerAction( xAzureIoTProvisioningClientHandle );

        if ( ( xResult = MQTT_ProcessLoop( &( xAzureIoTProvisioningClientHandle->xMQTTContext ),
                                           azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS ) ) != MQTTSuccess )
        {
            LogError( ( "Failed to process loop: ProcessLoopDuration=%u, Error=%s\r\n", azureIoTProvisioningPROCESS_LOOP_TIMEOUT_MS,
                        MQTT_Status_strerror( xResult ) ) );
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED );
        }
    }

    return xAzureIoTProvisioningClientHandle->lastOperationResult; 
}

static void prvMQTTProcessSubAck( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                  MQTTPublishInfo_t * pxPacketInfo )
{
    ( void )pxPacketInfo;
    // TODO check the state
    azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
}

/**
 * Process MQTT Response from Provisioning Service
 * 
 */
static void prvMQTTProcessResponse( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                    MQTTPublishInfo_t * pPublishInfo )
{
    if ( xAzureIoTProvisioningClientHandle->workflowState == AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_REQUESTING)
    {
        vPortFree( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload);
        vPortFree( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic );

        xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload =
            ( uint8_t * ) pvPortMalloc( pPublishInfo->payloadLength );
        xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic = 
            ( uint8_t * ) pvPortMalloc( pPublishInfo->topicNameLength );

        if ( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload &&
             xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic )
        {
            memcpy( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload,
                    pPublishInfo->pPayload, pPublishInfo->payloadLength );
            xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload_length = pPublishInfo->payloadLength;
            memcpy( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic,
                    pPublishInfo->pTopicName, pPublishInfo->topicNameLength );
            xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic_length = pPublishInfo->topicNameLength;
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
        }
        else
        {
            azure_iot_provisioning_client_update_state( xAzureIoTProvisioningClientHandle, AZURE_IOT_PROVISIONING_CLIENT_FAILED);
        }
    }
}

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* First element in AzureIoTHubClientHandle */
    AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle = ( AzureIoTProvisioningClientHandle_t )pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_SUBACK )
    {
        prvMQTTProcessSubAck( xAzureIoTProvisioningClientHandle, pxDeserializedInfo->pPublishInfo );
    }
    else if (( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH)
    {
        prvMQTTProcessResponse( xAzureIoTProvisioningClientHandle, pxDeserializedInfo->pPublishInfo );
    }
}

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                   const char * pEndpoint, uint32_t endpointLength,
                                                                   const char * pIdScope, uint32_t idScopeLength,
                                                                   const char * pRegistrationId, uint32_t registrationIdLength,
                                                                   uint8_t * pBuffer, uint32_t bufferLength,
                                                                   AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                                   const TransportInterface_t * pTransportInterface )
{
    AzureIoTProvisioningClientError_t ret;
    MQTTFixedBuffer_t xBuffer = { pBuffer, bufferLength };
    az_span endpoint_span = az_span_create( ( uint8_t * ) pEndpoint, endpointLength );
    az_span id_scope_span = az_span_create( ( uint8_t * ) pIdScope, idScopeLength );
    az_span registration_id_span = az_span_create( ( uint8_t * ) pRegistrationId, registrationIdLength );

    memset( xAzureIoTProvisioningClientHandle, 0, sizeof( AzureIoTProvisioningClient_t ) );
     
    xAzureIoTProvisioningClientHandle->pEndpoint = pEndpoint;
    xAzureIoTProvisioningClientHandle->endpointLength = endpointLength;
    xAzureIoTProvisioningClientHandle->pIdScope = pIdScope;
    xAzureIoTProvisioningClientHandle->idScopeLength = idScopeLength;
    xAzureIoTProvisioningClientHandle->pRegistrationId = pRegistrationId;
    xAzureIoTProvisioningClientHandle->registrationIdLength = registrationIdLength;
    xAzureIoTProvisioningClientHandle->getTimeFunction = getTimeFunction;

    if ( az_result_failed( az_iot_provisioning_client_init( &( xAzureIoTProvisioningClientHandle->iot_dps_client_core ),
                                                            endpoint_span, id_scope_span,
                                                            registration_id_span, NULL ) ) )
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }
    else if ( MQTT_Init( &( xAzureIoTProvisioningClientHandle -> xMQTTContext ), pTransportInterface,
                         getTimeFunction, prvEventCallback,
                         &xBuffer ) != MQTTSuccess )
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED;
    }
    else
    {
        ret = AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
    }

    return ret;
}

void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    vPortFree( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload );
    vPortFree( xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic );
    xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_payload = NULL;
    xAzureIoTProvisioningClientHandle->azure_iot_provisioning_client_last_response_topic = NULL;
}

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle )
{
    if ( xAzureIoTProvisioningClientHandle == NULL ||
         xAzureIoTProvisioningClientHandle->workflowState != 0 )
    {
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    return azure_iot_provisioning_client_run_workflow( xAzureIoTProvisioningClientHandle );
}

AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                     uint8_t * pHubHostname, uint32_t * pHostnameLength,
                                                                     uint8_t * pDeviceId, uint32_t * pDeviceIdLength )
{
    uint32_t hostnameLength;
    uint32_t deviceIdLength;

    if ( xAzureIoTProvisioningClientHandle == NULL || pHubHostname == NULL ||
         pHostnameLength == NULL || pDeviceId == NULL || pDeviceIdLength == NULL )
    {
        return AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT;
    }

    if ( xAzureIoTProvisioningClientHandle->workflowState != AZURE_IOT_PROVISIONING_CLIENT_WF_STATE_COMPLETE )
    {
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    if ( xAzureIoTProvisioningClientHandle->lastOperationResult )
    {
        return xAzureIoTProvisioningClientHandle->lastOperationResult;
    }

    hostnameLength = az_span_size( xAzureIoTProvisioningClientHandle->register_response.registration_state.assigned_hub_hostname );
    deviceIdLength = az_span_size( xAzureIoTProvisioningClientHandle->register_response.registration_state.device_id );

    if ( *pHostnameLength < hostnameLength || *pDeviceIdLength < deviceIdLength )
    {
        return AZURE_IOT_PROVISIONING_CLIENT_FAILED;
    }

    memcpy( pHubHostname, az_span_ptr( xAzureIoTProvisioningClientHandle->register_response.registration_state.assigned_hub_hostname), hostnameLength );
    memcpy( pDeviceId, az_span_ptr( xAzureIoTProvisioningClientHandle->register_response.registration_state.device_id), deviceIdLength );
    *pHostnameLength = hostnameLength;
    *pDeviceIdLength = deviceIdLength;

    return AZURE_IOT_PROVISIONING_CLIENT_SUCCESS;
}
