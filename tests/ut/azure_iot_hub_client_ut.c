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

#define testCLOUD_CALLBACK_ID                 ( 1 )
#define testCOMMAND_CALLBACK_ID               ( 2 )
#define testPROPERTY_CALLBACK_ID              ( 3 )
#define testEMPTY_JSON                        "{}"
#define testCLOUD_MESSAGE_TOPIC               "devices/unittest/messages/devicebound/test=1"
#define testCLOUD_MESSAGE                     "Hello"
#define testCOMMAND_MESSAGE_TOPIC             "$iothub/methods/POST/echo/?$rid=1"
#define testCOMMAND_MESSAGE                   testEMPTY_JSON
#define testPROPERTY_GET_MESSAGE_TOPIC        "$iothub/twin/res/200/?$rid=2"
#define testPROPERTY_MESSAGE                  "{\"desired\":{\"telemetrySendFrequency\":\"5m\"},\"reported\":{\"telemetrySendFrequency\":\"5m\"}}"
#define testPROPERTY_DESIRED_MESSAGE_TOPIC    "$iothub/twin/PATCH/properties/desired/?$version=1"
#define testPROPERTY_DESIRED_MESSAGE          "{\"telemetrySendFrequency\":\"5m\"}"
/*-----------------------------------------------------------*/

typedef struct ReceiveTestData
{
    const uint8_t * pucTopic;
    uint32_t ulTopicLength;
    const uint8_t * pucPayload;
    uint32_t ulPayloadLength;
    uint32_t ulCallbackFunctionId;
} ReceiveTestData_t;
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
static const uint8_t ucTestSymmetricKey[] = "dEI++++bZ1DZ6667LMlBNv88888IVnrQEWh999994FcdGuvXZE7Yr1BBS+sctwjuLTTTc7/3AuwUYsxUubZXg==";
static const uint8_t ucTestTelemetryPayload[] = "Unit Test Payload";
static const uint8_t ucTestCommandResponsePayload[] = "{\"Command\":\"Unit Test CommandResponse\"}";
static const uint8_t ucTestPropertyReportedPayload[] = "{\"Property\":\"Unit Test Payload\"}";
static uint8_t ucBuffer[ 512 ];
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};
static uint32_t ulReceivedCallbackFunctionId;
static const ReceiveTestData_t xTestReceiveData[] =
{
    {
        .pucTopic = ( const uint8_t * ) testCLOUD_MESSAGE_TOPIC,
        .ulTopicLength = sizeof( testCLOUD_MESSAGE_TOPIC ) - 1,
        .pucPayload = ( const uint8_t * ) testCLOUD_MESSAGE,
        .ulPayloadLength = sizeof( testCLOUD_MESSAGE ) - 1,
        .ulCallbackFunctionId = testCLOUD_CALLBACK_ID
    },
    {
        .pucTopic = ( const uint8_t * ) testCOMMAND_MESSAGE_TOPIC,
        .ulTopicLength = sizeof( testCOMMAND_MESSAGE_TOPIC ) - 1,
        .pucPayload = ( const uint8_t * ) testCOMMAND_MESSAGE,
        .ulPayloadLength = sizeof( testCOMMAND_MESSAGE ) - 1,
        .ulCallbackFunctionId = testCOMMAND_CALLBACK_ID
    },
    {
        .pucTopic = ( const uint8_t * ) testPROPERTY_GET_MESSAGE_TOPIC,
        .ulTopicLength = sizeof( testPROPERTY_GET_MESSAGE_TOPIC ) - 1,
        .pucPayload = ( const uint8_t * ) testPROPERTY_MESSAGE,
        .ulPayloadLength = sizeof( testPROPERTY_MESSAGE ) - 1,
        .ulCallbackFunctionId = testPROPERTY_CALLBACK_ID
    },
    {
        .pucTopic = ( const uint8_t * ) testPROPERTY_DESIRED_MESSAGE_TOPIC,
        .ulTopicLength = sizeof( testPROPERTY_DESIRED_MESSAGE_TOPIC ) - 1,
        .pucPayload = ( const uint8_t * ) testPROPERTY_DESIRED_MESSAGE,
        .ulPayloadLength = sizeof( testPROPERTY_DESIRED_MESSAGE ) - 1,
        .ulCallbackFunctionId = testPROPERTY_CALLBACK_ID
    }
};
static const ReceiveTestData_t xTestRandomReceiveData[] =
{
    {
        .pucTopic = ( const uint8_t * ) "a/b",
        .ulTopicLength = sizeof( "a/b" ) - 1,
        .pucPayload = ( const uint8_t * ) testCLOUD_MESSAGE,
        .ulPayloadLength = sizeof( testCLOUD_MESSAGE ) - 1,
        .ulCallbackFunctionId = 0
    },
    {
        .pucTopic = ( const uint8_t * ) "c",
        .ulTopicLength = sizeof( "c" ) - 1,
        .pucPayload = ( const uint8_t * ) testCLOUD_MESSAGE,
        .ulPayloadLength = sizeof( testCLOUD_MESSAGE ) - 1,
        .ulCallbackFunctionId = 0
    },
    {
        .pucTopic = ( const uint8_t * ) "randomdata",
        .ulTopicLength = sizeof( "randomdata" ) - 1,
        .pucPayload = ( const uint8_t * ) testCLOUD_MESSAGE,
        .ulPayloadLength = sizeof( testCLOUD_MESSAGE ) - 1,
        .ulCallbackFunctionId = 0
    },
    {
        .pucTopic = ( const uint8_t * ) "/",
        .ulTopicLength = sizeof( "/" ) - 1,
        .pucPayload = ( const uint8_t * ) testCLOUD_MESSAGE,
        .ulPayloadLength = sizeof( testCLOUD_MESSAGE ) - 1,
        .ulCallbackFunctionId = 0
    }
};
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

static uint32_t prvHmacFunction( const uint8_t * pucKey,
                                 uint32_t ulKeyLength,
                                 const uint8_t * pucData,
                                 uint32_t ulDataLength,
                                 uint8_t * pucOutput,
                                 uint32_t ulOutputLength,
                                 uint32_t * pucBytesCopied )
{
    ( void ) pucKey;
    ( void ) ulKeyLength;
    ( void ) pucData;
    ( void ) ulDataLength;
    ( void ) pucOutput;
    ( void ) ulOutputLength;
    ( void ) pucBytesCopied;

    return( ( uint32_t ) mock() );
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

static void prvTestCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                 void * pvContext )
{
    assert_true( pxMessage != NULL );
    assert_true( pvContext == NULL );
    assert_int_equal( pxMessage->ulPayloadLength, sizeof( testCLOUD_MESSAGE ) - 1 );
    assert_memory_equal( pxMessage->pvMessagePayload, testCLOUD_MESSAGE, sizeof( testCLOUD_MESSAGE ) - 1 );

    ulReceivedCallbackFunctionId = testCLOUD_CALLBACK_ID;
}
/*-----------------------------------------------------------*/

static void prvTestCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                            void * pvContext )
{
    assert_true( pxMessage != NULL );
    assert_true( pvContext == NULL );
    assert_int_equal( pxMessage->ulPayloadLength, sizeof( testCOMMAND_MESSAGE ) - 1 );
    assert_memory_equal( pxMessage->pvMessagePayload, testCOMMAND_MESSAGE, sizeof( testCOMMAND_MESSAGE ) - 1 );

    ulReceivedCallbackFunctionId = testCOMMAND_CALLBACK_ID;
}
/*-----------------------------------------------------------*/

static void prvTestProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                               void * pvContext )
{
    assert_true( pxMessage != NULL );
    assert_true( pvContext == NULL );

    if( pxMessage->xMessageType == eAzureIoTHubPropertiesRequestedMessage )
    {
        assert_int_equal( pxMessage->ulPayloadLength, sizeof( testPROPERTY_MESSAGE ) - 1 );
        assert_memory_equal( pxMessage->pvMessagePayload, testPROPERTY_MESSAGE, sizeof( testPROPERTY_MESSAGE ) - 1 );
    }
    else if( pxMessage->xMessageType == eAzureIoTHubPropertiesWritablePropertyMessage )
    {
        assert_int_equal( pxMessage->ulPayloadLength, sizeof( testPROPERTY_DESIRED_MESSAGE ) - 1 );
        assert_memory_equal( pxMessage->pvMessagePayload, testPROPERTY_DESIRED_MESSAGE, sizeof( testPROPERTY_DESIRED_MESSAGE ) - 1 );
    }
    else
    {
        assert_int_equal( pxMessage->xMessageType, eAzureIoTHubPropertiesReportedResponseMessage );
    }

    ulReceivedCallbackFunctionId = testPROPERTY_CALLBACK_ID;
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Init_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    ( void ) ppvState;

    /* Fail init when client is NULL */
    assert_int_equal( AzureIoTHubClient_Init( NULL,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null hostname passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              NULL, 0,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null deviceId passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              NULL, 0,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null buffer passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              NULL, 0,
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null AzureIoTGetCurrentTimeFunc_t passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer, sizeof( ucBuffer ),
                                              NULL, &xTransportInterface ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null Transport passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer, sizeof( ucBuffer ),
                                              prvGetUnixTime, NULL ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail when smaller buffer is passed */
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer, azureiotconfigUSERNAME_MAX,
                                              prvGetUnixTime, &xTransportInterface ),
                      eAzureIoTErrorOutOfMemory );

    /* Fail init when AzureIoTMQTT_Init fails */
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTNoMemory );
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer, sizeof( ucBuffer ),
                                              prvGetUnixTime, &xTransportInterface ),
                      eAzureIoTErrorInitFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Init_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    ( void ) ppvState;

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
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

static void testAzureIoTHubClient_Init_NULLOptions_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              NULL,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Deinit_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    AzureIoTHubClient_Deinit( &xTestIoTHubClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_OptionsInit_Fail( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail if options pointer is NULL */
    assert_int_equal( AzureIoTHubClient_OptionsInit( NULL ), eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_OptionsInit_Success( void ** ppvState )
{
    AzureIoTHubClientOptions_t xOptions;

    ( void ) ppvState;

    assert_int_equal( AzureIoTHubClient_OptionsInit( &xOptions ), eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Connect_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    bool xSessionPresent;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if hub client is NULL */
    assert_int_equal( AzureIoTHubClient_Connect( NULL,
                                                 false,
                                                 &xSessionPresent,
                                                 60 ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail if session present bool pointer is NULL */
    assert_int_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                 false,
                                                 NULL,
                                                 60 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Connect_MQTTConnectFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    bool xSessionPresent;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if the MQTT client couldn't connect due to an error. */
    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTServerRefused );
    assert_int_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                 false,
                                                 &xSessionPresent,
                                                 60 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Connect_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    bool xSessionPresent;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                 false,
                                                 &xSessionPresent,
                                                 60 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Disconnect_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail if hub client is NULL. */
    assert_int_equal( AzureIoTHubClient_Disconnect( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Disconnect_MQTTDisconnectFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if the MQTT client couldn't disconnect due to an error. */
    will_return( AzureIoTMQTT_Disconnect, eAzureIoTMQTTIllegalState );
    assert_int_equal( AzureIoTHubClient_Disconnect( &xTestIoTHubClient ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Disconnect_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Disconnect, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Disconnect( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendTelemetry_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if the hub client is NULL. */
    assert_int_equal( AzureIoTHubClient_SendTelemetry( NULL,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1,
                                                       NULL,
                                                       eAzureIoTHubMessageQoS0,
                                                       NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendTelemetry_BigTopicFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMessageProperties_t properties =
    {
        ._internal.xProperties._internal.current_property_index = 0,
        ._internal.xProperties._internal.properties_written     = sizeof( ucBuffer ) + 1
    };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if the topic is not able to fit in the working buffer. */
    assert_int_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1,
                                                       &properties,
                                                       eAzureIoTHubMessageQoS0,
                                                       NULL ),
                      eAzureIoTErrorOutOfMemory );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendTelemetry_SendFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail if the MQTT publish call returns an error. */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1,
                                                       NULL,
                                                       eAzureIoTHubMessageQoS0,
                                                       NULL ),
                      eAzureIoTErrorPublishFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendTelemetryQOS0_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    usSentQOS = eAzureIoTMQTTQoS0; /* Check to make sure the hub QOS translates correctly to MQTT layer */

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1,
                                                       NULL,
                                                       eAzureIoTHubMessageQoS0,
                                                       NULL ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/


static void testAzureIoTHubClient_SendTelemetryQOS1WithPacketID_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint16_t usPacketId = 0;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    usSentQOS = eAzureIoTMQTTQoS1; /* Check to make sure the hub QOS translates correctly to MQTT QOS */

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1,
                                                       NULL,
                                                       eAzureIoTHubMessageQoS1,
                                                       &usPacketId ),
                      eAzureIoTSuccess );
    assert_int_equal( usPacketId, 1 );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_ProcessLoop_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail ProcessLoop when client is NULL */
    assert_int_equal( AzureIoTHubClient_ProcessLoop( NULL,
                                                     1234 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_ProcessLoop_MQTTProcessFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTRecvFailed );
    assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient,
                                                     1234 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_ProcessLoop_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient,
                                                     1234 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    /* Fail SubscribeCloudToDevice when client is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( NULL,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SubscribeCloudToDevice when function callback is NULL  */
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       NULL, NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_SubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorSubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_ReceiveFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTRecvFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_DelayedSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return_always( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 5 * azureiotconfigSUBACK_WAIT_INTERVAL_MS;
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCloudMessage_MultipleSuccess( void ** ppvState )
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
        assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                           prvTestCloudMessage,
                                                                           NULL, ( uint32_t ) -1 ),
                          eAzureIoTSuccess );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    /* Fail SubscribeCommand when client is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( NULL,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SubscribeCommand when function callback is NULL  */
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          NULL, NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_SubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorSubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_ReceiveFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTRecvFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_DelayedSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return_always( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 5 * azureiotconfigSUBACK_WAIT_INTERVAL_MS;
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeCommand_MultipleSuccess( void ** ppvState )
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
        assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                              prvTestCommand,
                                                              NULL, ( uint32_t ) -1 ),
                          eAzureIoTSuccess );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    /* Fail SubscribeProperties when client is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( NULL,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SubscribeProperties when function callback is NULL */
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             NULL, NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_SubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorSubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_ReceiveFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTRecvFailed );
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_DelayedSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return_always( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 5 * azureiotconfigSUBACK_WAIT_INTERVAL_MS;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubscribeProperties_MultipleSuccess( void ** ppvState )
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
        assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                                 prvTestProperties,
                                                                 NULL, ( uint32_t ) -1 ),
                          eAzureIoTSuccess );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCloudMessage_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail UnsubscribeCloudToDevice when client is NULL */
    assert_int_equal( AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCloudMessage_UnsubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xTestIoTHubClient ),
                      eAzureIoTErrorUnsubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCloudMessage_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubUnsubCloudMessage_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCommand_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail UnsubscribeCommand when client is NULL */
    assert_int_equal( AzureIoTHubClient_UnsubscribeCommand( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCommand_UnsubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCommand( &xTestIoTHubClient ),
                      eAzureIoTErrorUnsubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeCommand_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCommand( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubUnsubCommand_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeCommand( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeProperties_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail UnsubscribeProperties when client is NULL */
    assert_int_equal( AzureIoTHubClient_UnsubscribeProperties( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeProperties_UnsubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_UnsubscribeProperties( &xTestIoTHubClient ),
                      eAzureIoTErrorUnsubscribeFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_UnsubscribeProperties_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeProperties( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SubUnsubProperties_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Unsubscribe, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_UnsubscribeProperties( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCommandResponse_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientCommandRequest_t req =
    {
        .pucRequestID      = &req_id,
        .usRequestIDLength = sizeof( req_id )
    };

    ( void ) ppvState;

    /* Fail SendCommandResponse when client is NULL */
    assert_int_equal( AzureIoTHubClient_SendCommandResponse( NULL,
                                                             &req, 200, ucTestCommandResponsePayload,
                                                             sizeof( ucTestCommandResponsePayload ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SendCommandResponse when request context is NULL */
    assert_int_equal( AzureIoTHubClient_SendCommandResponse( &xTestIoTHubClient,
                                                             NULL, 200, ucTestCommandResponsePayload,
                                                             sizeof( ucTestCommandResponsePayload ) ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCommandResponse_SendFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientCommandRequest_t req =
    {
        .pucRequestID      = &req_id,
        .usRequestIDLength = sizeof( req_id )
    };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTHubClient_SendCommandResponse( &xTestIoTHubClient,
                                                             &req, 200, ucTestCommandResponsePayload,
                                                             sizeof( ucTestCommandResponsePayload ) ),
                      eAzureIoTErrorPublishFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCommandResponse_EmptyResponseSuccess( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientCommandRequest_t req =
    {
        .pucRequestID      = &req_id,
        .usRequestIDLength = sizeof( req_id )
    };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    pucPublishPayload = ( const uint8_t * ) testEMPTY_JSON;
    assert_int_equal( AzureIoTHubClient_SendCommandResponse( &xTestIoTHubClient,
                                                             &req, 200, NULL, 0 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendCommandResponse_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientCommandRequest_t req =
    {
        .pucRequestID      = &req_id,
        .usRequestIDLength = sizeof( req_id )
    };

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    pucPublishPayload = ucTestCommandResponsePayload;
    assert_int_equal( AzureIoTHubClient_SendCommandResponse( &xTestIoTHubClient,
                                                             &req, 200, ucTestCommandResponsePayload,
                                                             sizeof( ucTestCommandResponsePayload ) - 1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendPropertiesReported_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail SendPropertiesReported when client is NULL */
    assert_int_equal( AzureIoTHubClient_SendPropertiesReported( NULL,
                                                                ucTestPropertyReportedPayload,
                                                                sizeof( ucTestPropertyReportedPayload ) - 1,
                                                                &requestId ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SendPropertiesReported when reported payload is NULL */
    assert_int_equal( AzureIoTHubClient_SendPropertiesReported( &xTestIoTHubClient,
                                                                NULL, 0,
                                                                &requestId ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendPropertiesReported_NotSubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTHubClient_SendPropertiesReported( &xTestIoTHubClient,
                                                                ucTestPropertyReportedPayload,
                                                                sizeof( ucTestPropertyReportedPayload ) - 1,
                                                                &requestId ),
                      eAzureIoTErrorTopicNotSubscribed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendPropertiesReported_SendFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    pucPublishPayload = ucTestPropertyReportedPayload;
    assert_int_equal( AzureIoTHubClient_SendPropertiesReported( &xTestIoTHubClient,
                                                                ucTestPropertyReportedPayload,
                                                                sizeof( ucTestPropertyReportedPayload ) - 1,
                                                                &requestId ),
                      eAzureIoTErrorPublishFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SendPropertiesReported_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    pucPublishPayload = ucTestPropertyReportedPayload;
    assert_int_equal( AzureIoTHubClient_SendPropertiesReported( &xTestIoTHubClient,
                                                                ucTestPropertyReportedPayload,
                                                                sizeof( ucTestPropertyReportedPayload ) - 1,
                                                                &requestId ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_RequestPropertiesAsync_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    /* Fail GetProperties when client is NULL */
    assert_int_equal( AzureIoTHubClient_RequestPropertiesAsync( NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_RequestPropertiesAsync_NotSubscribeFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTHubClient_RequestPropertiesAsync( &xTestIoTHubClient ),
                      eAzureIoTErrorTopicNotSubscribed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_RequestPropertiesAsync_SendFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    pucPublishPayload = NULL;
    assert_int_equal( AzureIoTHubClient_RequestPropertiesAsync( &xTestIoTHubClient ),
                      eAzureIoTErrorPublishFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_RequestPropertiesAsync_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    pucPublishPayload = NULL;
    assert_int_equal( AzureIoTHubClient_RequestPropertiesAsync( &xTestIoTHubClient ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_ReceiveMessages_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t publishInfo;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xTestIoTHubClient,
                                                                       prvTestCloudMessage,
                                                                       NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    xDeserializedInfo.usPacketIdentifier = ++usTestPacketId;
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SubscribeCommand( &xTestIoTHubClient,
                                                          prvTestCommand,
                                                          NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    xDeserializedInfo.usPacketIdentifier = ++usTestPacketId;
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );

    for( size_t index = 0; index < ( sizeof( xTestReceiveData ) / sizeof( ReceiveTestData_t ) ); index++ )
    {
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
        xDeserializedInfo.usPacketIdentifier = 1;
        publishInfo.pcTopicName = xTestReceiveData[ index ].pucTopic;
        publishInfo.usTopicNameLength = ( uint16_t ) xTestReceiveData[ index ].ulTopicLength;
        publishInfo.pvPayload = xTestReceiveData[ index ].pucPayload;
        publishInfo.xPayloadLength = xTestReceiveData[ index ].ulPayloadLength;
        xDeserializedInfo.pxPublishInfo = &publishInfo;
        ulReceivedCallbackFunctionId = 0;
        ulDelayReceivePacket = 0;

        assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                          eAzureIoTSuccess );

        assert_int_equal( ulReceivedCallbackFunctionId, xTestReceiveData[ index ].ulCallbackFunctionId );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_ReceiveRandomMessages_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t publishInfo;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    for( size_t index = 0; index < ( sizeof( xTestReceiveData ) / sizeof( ReceiveTestData_t ) ); index++ )
    {
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
        xDeserializedInfo.usPacketIdentifier = 1;
        publishInfo.pcTopicName = xTestReceiveData[ index ].pucTopic;
        publishInfo.usTopicNameLength = ( uint16_t ) xTestReceiveData[ index ].ulTopicLength;
        publishInfo.pvPayload = xTestReceiveData[ index ].pucPayload;
        publishInfo.xPayloadLength = xTestReceiveData[ index ].ulPayloadLength;
        xDeserializedInfo.pxPublishInfo = &publishInfo;
        ulReceivedCallbackFunctionId = 0;
        ulDelayReceivePacket = 0;

        assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                          eAzureIoTSuccess );

        assert_int_equal( ulReceivedCallbackFunctionId, 0 );
    }

    for( size_t index = 0; index < ( sizeof( xTestRandomReceiveData ) / sizeof( ReceiveTestData_t ) ); index++ )
    {
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
        xDeserializedInfo.usPacketIdentifier = 1;
        publishInfo.pcTopicName = xTestRandomReceiveData[ index ].pucTopic;
        publishInfo.usTopicNameLength = ( uint16_t ) xTestRandomReceiveData[ index ].ulTopicLength;
        publishInfo.pvPayload = xTestRandomReceiveData[ index ].pucPayload;
        publishInfo.xPayloadLength = xTestRandomReceiveData[ index ].ulPayloadLength;
        xDeserializedInfo.pxPublishInfo = &publishInfo;
        ulReceivedCallbackFunctionId = 0;
        ulDelayReceivePacket = 0;

        assert_int_equal( AzureIoTHubClient_ProcessLoop( &xTestIoTHubClient, 60 ),
                          eAzureIoTSuccess );

        assert_int_equal( ulReceivedCallbackFunctionId, 0 );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SetSymmetricKey_InvalidArgFailure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* Fail SetSymmetricKey when client is NULL */
    assert_int_equal( AzureIoTHubClient_SetSymmetricKey( NULL,
                                                         ucTestSymmetricKey,
                                                         sizeof( ucTestSymmetricKey ) - 1,
                                                         prvHmacFunction ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SetSymmetricKey when Symmetric key is NULL */
    assert_int_equal( AzureIoTHubClient_SetSymmetricKey( &xTestIoTHubClient,
                                                         NULL, 0,
                                                         prvHmacFunction ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail SetSymmetricKey when HMAC Callback is NULL */
    assert_int_equal( AzureIoTHubClient_SetSymmetricKey( &xTestIoTHubClient,
                                                         ucTestSymmetricKey,
                                                         sizeof( ucTestSymmetricKey ) - 1,
                                                         NULL ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_SetSymmetricKey_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) ppvState;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTHubClient_SetSymmetricKey( &xTestIoTHubClient,
                                                         ucTestSymmetricKey,
                                                         sizeof( ucTestSymmetricKey ) - 1,
                                                         prvHmacFunction ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTHubClient_Init_Failure ),
        cmocka_unit_test( testAzureIoTHubClient_Init_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Init_NULLOptions_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Deinit_Success ),
        cmocka_unit_test( testAzureIoTHubClient_OptionsInit_Fail ),
        cmocka_unit_test( testAzureIoTHubClient_OptionsInit_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Connect_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_Connect_MQTTConnectFailure ),
        cmocka_unit_test( testAzureIoTHubClient_Connect_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Disconnect_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_Disconnect_MQTTDisconnectFailure ),
        cmocka_unit_test( testAzureIoTHubClient_Disconnect_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetry_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetry_BigTopicFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetry_SendFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetryQOS0_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetryQOS1WithPacketID_Success ),
        cmocka_unit_test( testAzureIoTHubClient_ProcessLoop_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_ProcessLoop_MQTTProcessFailure ),
        cmocka_unit_test( testAzureIoTHubClient_ProcessLoop_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_SubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_ReceiveFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_DelayedSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCloudMessage_MultipleSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_SubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_ReceiveFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_DelayedSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeCommand_MultipleSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_SubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_ReceiveFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_DelayedSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SubscribeProperties_MultipleSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCloudMessage_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCloudMessage_UnsubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCloudMessage_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubUnsubCloudMessage_Success ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCommand_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCommand_UnsubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeCommand_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubUnsubCommand_Success ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeProperties_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeProperties_UnsubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_UnsubscribeProperties_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SubUnsubProperties_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendCommandResponse_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendCommandResponse_SendFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendCommandResponse_EmptyResponseSuccess ),
        cmocka_unit_test( testAzureIoTHubClient_SendCommandResponse_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SendPropertiesReported_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendPropertiesReported_NotSubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendPropertiesReported_SendFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SendPropertiesReported_Success ),
        cmocka_unit_test( testAzureIoTHubClient_RequestPropertiesAsync_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_RequestPropertiesAsync_NotSubscribeFailure ),
        cmocka_unit_test( testAzureIoTHubClient_RequestPropertiesAsync_SendFailure ),
        cmocka_unit_test( testAzureIoTHubClient_RequestPropertiesAsync_Success ),
        cmocka_unit_test( testAzureIoTHubClient_ReceiveMessages_Success ),
        cmocka_unit_test( testAzureIoTHubClient_ReceiveRandomMessages_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SetSymmetricKey_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTHubClient_SetSymmetricKey_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
