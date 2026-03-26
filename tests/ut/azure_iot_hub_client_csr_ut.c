/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_hub_client.h"
/*-----------------------------------------------------------*/

#define testCSR_CALLBACK_ID                         ( 4 )
#define testCSR_ACCEPTED_MESSAGE_TOPIC              "$iothub/credentials/res/202/?$rid=1"
#define testCSR_COMPLETED_MESSAGE_TOPIC             "$iothub/credentials/res/200/?$rid=1"
#define testCSR_ERROR_MESSAGE_TOPIC                 "$iothub/credentials/res/400/?$rid=1"
#define testCSR_ACCEPTED_PAYLOAD                    "{\"correlationId\":\"test-corr-id\",\"operationExpires\":\"2025-06-09T17:31:31.426Z\"}"
#define testCSR_COMPLETED_PAYLOAD                   "test-certificate-data"
#define testCSR_ERROR_PAYLOAD                       "{\"errorCode\":400040,\"message\":\"Credential management operation failed\"}"
#define testCSR_DATA                                "MIICYTCCAUkCAQAwHDEaMBgGA1wRZGAw"
#define testCSR_REQUEST_ID                          "1"
#define testEMPTY_JSON                              "{}"
/*-----------------------------------------------------------*/

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern const uint8_t * pucPublishPayload;
extern uint16_t usSentQOS;
extern uint32_t ulDelayReceivePacket;

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static uint8_t ucBuffer[ 512 ];
static uint8_t ucPayloadBuffer[ 512 ];
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};
static uint32_t ulReceivedCallbackFunctionId;
static AzureIoTHubClientCertificateSigningResponseType_t xReceivedResponseType;
static AzureIoTHubMessageStatus_t xReceivedMessageStatus;
static const void * pvReceivedPayload;
static uint32_t ulReceivedPayloadLength;
static const uint8_t * pucReceivedRequestID;
static uint16_t usReceivedRequestIDLength;
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

TickType_t xTaskGetTickCount( void )
{
    return 1;
}
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    return 0xFFFFFFFFFFFFFFFF;
}
/*-----------------------------------------------------------*/

static void prvSetupTestIoTHubClient( AzureIoTHubClient_t * pxTestIoTHubClient )
{
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( pxTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void prvTestCSRCallback( AzureIoTHubClientCertificateSigningResponse_t * pxResponse,
                                 void * pvContext )
{
    assert_true( pxResponse != NULL );
    assert_true( pvContext == NULL );

    xReceivedResponseType = pxResponse->xResponseType;
    xReceivedMessageStatus = pxResponse->xMessageStatus;
    pvReceivedPayload = pxResponse->pvMessagePayload;
    ulReceivedPayloadLength = pxResponse->ulPayloadLength;
    pucReceivedRequestID = pxResponse->pucRequestID;
    usReceivedRequestIDLength = pxResponse->usRequestIDLength;
    ulReceivedCallbackFunctionId = testCSR_CALLBACK_ID;
}
/*-----------------------------------------------------------*/

static void prvSubscribeToCSR( AzureIoTHubClient_t * pxTestIoTHubClient )
{
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( pxTestIoTHubClient,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail subscribe when client is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( NULL,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail subscribe when callback is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              NULL,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_SubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorSubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_ReceiveFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTRecvFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_DelayedSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return_always( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 5 * azureiotconfigSUBACK_WAIT_INTERVAL_MS;
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              prvTestCSRCallback,
                                                                              NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCSR_MultipleSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    for( uint32_t ulRunCount = 0; ulRunCount < 4; ulRunCount++ )
    {
        will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
        xDeserializedInfo.usPacketIdentifier = usTestPacketId;
        ulDelayReceivePacket = 0;
        assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                                  prvTestCSRCallback,
                                                                                  NULL, ( uint32_t ) -1 ),
                          eAzureIoTSuccess );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCSR_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    assert_int_equal( AzureIoTHubClient_UnsubscribeCertificateSigningResponse( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCSR_UnsubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xTestIoTHubClient ),
                      eAzureIoTErrorUnsubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCSR_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubUnsubCSR_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Subscribe */
    prvSubscribeToCSR( &xTestIoTHubClient );

    /* Unsubscribe */
    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    /* Fail when client is NULL */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( NULL,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail when CSR is NULL */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        NULL, 0,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail when request ID is NULL */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        NULL, 0,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail when payload buffer is NULL */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        NULL, 0 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_NotSubscribedFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Send without subscribing first */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorTopicNotSubscribed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_PublishFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorPublishFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    usSentQOS = eAzureIoTMQTTQoS1;
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_CSR_ReceiveAccepted_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo = { 0 };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
    xDeserializedInfo.usPacketIdentifier = 1;
    xPublishInfo.pcTopicName = testCSR_ACCEPTED_MESSAGE_TOPIC;
    xPublishInfo.usTopicNameLength = sizeof( testCSR_ACCEPTED_MESSAGE_TOPIC ) - 1;
    xPublishInfo.pvPayload = ( const void * ) testCSR_ACCEPTED_PAYLOAD;
    xPublishInfo.xPayloadLength = sizeof( testCSR_ACCEPTED_PAYLOAD ) - 1;
    xDeserializedInfo.pxPublishInfo = &xPublishInfo;
    ulReceivedCallbackFunctionId = 0;
    xReceivedResponseType = 0;

    assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                      eAzureIoTSuccess );

    assert_int_equal( ulReceivedCallbackFunctionId, testCSR_CALLBACK_ID );
    assert_int_equal( xReceivedResponseType, eAzureIoTHubClientCertificateSigningResponseAccepted );
    assert_int_equal( xReceivedMessageStatus, eAzureIoTStatusAccepted );
    assert_non_null( pvReceivedPayload );
    assert_int_equal( ulReceivedPayloadLength, sizeof( testCSR_ACCEPTED_PAYLOAD ) - 1 );
    assert_non_null( pucReceivedRequestID );
    assert_int_equal( usReceivedRequestIDLength, sizeof( testCSR_REQUEST_ID ) - 1 );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_CSR_ReceiveCompleted_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo = { 0 };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
    xDeserializedInfo.usPacketIdentifier = 1;
    xPublishInfo.pcTopicName = testCSR_COMPLETED_MESSAGE_TOPIC;
    xPublishInfo.usTopicNameLength = sizeof( testCSR_COMPLETED_MESSAGE_TOPIC ) - 1;
    xPublishInfo.pvPayload = ( const void * ) testCSR_COMPLETED_PAYLOAD;
    xPublishInfo.xPayloadLength = sizeof( testCSR_COMPLETED_PAYLOAD ) - 1;
    xDeserializedInfo.pxPublishInfo = &xPublishInfo;
    ulReceivedCallbackFunctionId = 0;
    xReceivedResponseType = 0;

    assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                      eAzureIoTSuccess );

    assert_int_equal( ulReceivedCallbackFunctionId, testCSR_CALLBACK_ID );
    assert_int_equal( xReceivedResponseType, eAzureIoTHubClientCertificateSigningResponseCompleted );
    assert_int_equal( xReceivedMessageStatus, eAzureIoTStatusOk );
    assert_non_null( pvReceivedPayload );
    assert_int_equal( ulReceivedPayloadLength, sizeof( testCSR_COMPLETED_PAYLOAD ) - 1 );
    assert_non_null( pucReceivedRequestID );
    assert_int_equal( usReceivedRequestIDLength, sizeof( testCSR_REQUEST_ID ) - 1 );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_CSR_ReceiveError_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo = { 0 };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
    xDeserializedInfo.usPacketIdentifier = 1;
    xPublishInfo.pcTopicName = testCSR_ERROR_MESSAGE_TOPIC;
    xPublishInfo.usTopicNameLength = sizeof( testCSR_ERROR_MESSAGE_TOPIC ) - 1;
    xPublishInfo.pvPayload = ( const void * ) testCSR_ERROR_PAYLOAD;
    xPublishInfo.xPayloadLength = sizeof( testCSR_ERROR_PAYLOAD ) - 1;
    xDeserializedInfo.pxPublishInfo = &xPublishInfo;
    ulReceivedCallbackFunctionId = 0;
    xReceivedResponseType = 0;

    assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                      eAzureIoTSuccess );

    assert_int_equal( ulReceivedCallbackFunctionId, testCSR_CALLBACK_ID );
    assert_int_equal( xReceivedResponseType, eAzureIoTHubClientCertificateSigningResponseError );
    assert_int_equal( xReceivedMessageStatus, eAzureIoTStatusBadRequest );
    assert_non_null( pvReceivedPayload );
    assert_int_equal( ulReceivedPayloadLength, sizeof( testCSR_ERROR_PAYLOAD ) - 1 );
    assert_non_null( pucReceivedRequestID );
    assert_int_equal( usReceivedRequestIDLength, sizeof( testCSR_REQUEST_ID ) - 1 );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_WithReplaceOption_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientCertificateSigningRequestOptions_t xOptions =
    {
        .pucReplace    = ( const uint8_t * ) "*",
        .usReplaceLength = sizeof( "*" ) - 1
    };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    usSentQOS = eAzureIoTMQTTQoS1;
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        &xOptions,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCSR_AfterUnsubscribe_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );
    prvSubscribeToCSR( &xTestIoTHubClient );

    /* Unsubscribe */
    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xTestIoTHubClient ),
                      eAzureIoTSuccess );

    /* After unsubscribing, send must fail with topic-not-subscribed */
    assert_int_equal( AzureIoTHubClient_SendCertificateSigningRequest( &xTestIoTHubClient,
                                                                        ( const uint8_t * ) testCSR_DATA,
                                                                        sizeof( testCSR_DATA ) - 1,
                                                                        ( const uint8_t * ) testCSR_REQUEST_ID,
                                                                        sizeof( testCSR_REQUEST_ID ) - 1,
                                                                        NULL,
                                                                        ucPayloadBuffer, sizeof( ucPayloadBuffer ) ),
                      eAzureIoTErrorTopicNotSubscribed );
}
/*-----------------------------------------------------------*/

static uint32_t ulTestContextValue = 0xDEADBEEF;

static void prvTestCSRCallbackWithContext( AzureIoTHubClientCertificateSigningResponse_t * pxResponse,
                                           void * pvContext )
{
    assert_true( pxResponse != NULL );
    assert_true( pvContext == &ulTestContextValue );

    xReceivedResponseType = pxResponse->xResponseType;
    ulReceivedCallbackFunctionId = testCSR_CALLBACK_ID;
}

static void testAzureIoTHubClient_SubscribeCSR_WithContext_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeCertificateSigningResponse( &xTestIoTHubClient,
                                                                              prvTestCSRCallbackWithContext,
                                                                              &ulTestContextValue, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_SubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_ReceiveFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_DelayedSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_MultipleSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCSR_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCSR_UnsubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCSR_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubUnsubCSR_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_NotSubscribedFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_PublishFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_Success ),
        cmocka_unit_test( testAzureIoTHubClient_CSR_ReceiveAccepted_Success ),
        cmocka_unit_test( testAzureIoTHubClient_CSR_ReceiveCompleted_Success ),
        cmocka_unit_test( testAzureIoTHubClient_CSR_ReceiveError_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_WithReplaceOption_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendCSR_AfterUnsubscribe_Failure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCSR_WithContext_Success ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_csr_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
