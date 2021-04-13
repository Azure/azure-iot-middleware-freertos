/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_hub_client.h"

#define testCLOUD_CALLBACK_ID     1
#define testMETHOD_CALLBACK_ID    2
#define testTWIN_CALLBACK_ID      3
#define testEMPTY_JSON            "{}"

typedef struct ReceviceTestData
{
    const uint8_t * pucTopic;
    uint32_t ulTopicLength;
    const uint8_t * pucPayload;
    uint32_t ulPayloadLength;
    uint32_t ulCallbackFunctionId;
} ReceviceTestData_t;

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern const uint8_t * pucPublishPayload;

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static const uint8_t ucTestTelemetryPayload[] = "Unit Test Payload";
static uint8_t ucBuffer[ 512 ];
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};

/*-----------------------------------------------------------*/
TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

TickType_t xTaskGetTickCount( void )
{
    return 1;
}

static uint64_t prvGetUnixTime( void )
{
    return 0xFFFFFFFFFFFFFFFF;
}

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
                      eAzureIoTHubClientSuccess );
}

/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Init_Failure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    ( void ) state;

    /* Fail init when null hostname passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  NULL, 0,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  ucBuffer,
                                                  sizeof( ucBuffer ),
                                                  prvGetUnixTime,
                                                  &xTransportInterface ),
                          eAzureIoTHubClientSuccess );

    /* Fail init when null deviceId passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  NULL, 0,
                                                  &xHubClientOptions,
                                                  ucBuffer,
                                                  sizeof( ucBuffer ),
                                                  prvGetUnixTime,
                                                  &xTransportInterface ),
                          eAzureIoTHubClientSuccess );

    /* Fail init when null buffer passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  NULL, 0,
                                                  prvGetUnixTime,
                                                  &xTransportInterface ),
                          eAzureIoTHubClientSuccess );

    /* Fail init when null AzureIoTGetCurrentTimeFunc_t passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  ucBuffer, sizeof( ucBuffer ),
                                                  NULL, &xTransportInterface ),
                          eAzureIoTHubClientSuccess );

    /* Fail init when null Transport passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  ucBuffer, sizeof( ucBuffer ),
                                                  prvGetUnixTime, NULL ),
                          eAzureIoTHubClientSuccess );

    /* Fail when smaller buffer is passed */
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  ucBuffer, azureiotUSERNAME_MAX,
                                                  prvGetUnixTime, &xTransportInterface ),
                          eAzureIoTHubClientSuccess );

    /* Fail init when AzureIoTMQTT_Init fails */
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTNoMemory );
    assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                  ucHostname, sizeof( ucHostname ) - 1,
                                                  ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                  &xHubClientOptions,
                                                  ucBuffer, sizeof( ucBuffer ),
                                                  prvGetUnixTime, &xTransportInterface ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Init_Success( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    ( void ) state;
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Init_NULLOptions( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              NULL,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Deinit_Success( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    AzureIoTHubClient_Deinit( &xTestIoTHubClient );
}

static void testAzureIoTHubClient_OptionsInit_Fail( void ** state )
{
    ( void ) state;

    assert_int_equal( AzureIoTHubClient_OptionsInit( NULL ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTHubClient_OptionsInit_Success( void ** state )
{
    AzureIoTHubClientOptions_t xOptions;

    ( void ) state;

    assert_int_equal( AzureIoTHubClient_OptionsInit( &xOptions ), eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Connect_InvalidArgFailure( void ** state )
{
    ( void ) state;

    bool xSessionPresent;

    assert_int_not_equal( AzureIoTHubClient_Connect( NULL,
                                                     false,
                                                     &xSessionPresent,
                                                     60 ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Connect_MQTTConnectFailure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    bool xSessionPresent;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTServerRefused );
    assert_int_not_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                     false,
                                                     &xSessionPresent,
                                                     60 ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Connect_Success( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    bool xSessionPresent;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                 false,
                                                 &xSessionPresent,
                                                 60 ),
                      eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Disconnect_InvalidArgFailure( void ** state )
{
    ( void ) state;

    assert_int_not_equal( AzureIoTHubClient_Disconnect( NULL ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Disconnect_MQTTDisconnectFailure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Disconnect, eAzureIoTMQTTIllegalState );
    assert_int_not_equal( AzureIoTHubClient_Disconnect( &xTestIoTHubClient ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_Disconnect_Success( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Disconnect, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Disconnect( &xTestIoTHubClient ),
                      eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_SendTelemetry_InvalidArgFailure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    /* NULL handler */
    assert_int_not_equal( AzureIoTHubClient_SendTelemetry( NULL,
                                                           ucTestTelemetryPayload,
                                                           sizeof( ucTestTelemetryPayload ) - 1, NULL ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_SendTelemetry_BigTopicFailure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMessageProperties_t properties =
    {
        ._internal.xProperties._internal.current_property_index = 0,
        ._internal.xProperties._internal.properties_written     = sizeof( ucBuffer ) + 1
    };

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_not_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                           ucTestTelemetryPayload,
                                                           sizeof( ucTestTelemetryPayload ) - 1, &properties ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_SendTelemetry_SendFailure( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    assert_int_not_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                           ucTestTelemetryPayload,
                                                           sizeof( ucTestTelemetryPayload ) - 1, NULL ),
                          eAzureIoTHubClientSuccess );
}

static void testAzureIoTHubClient_SendTelemetry_Success( void ** state )
{
    AzureIoTHubClient_t xTestIoTHubClient;

    ( void ) state;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_SendTelemetry( &xTestIoTHubClient,
                                                       ucTestTelemetryPayload,
                                                       sizeof( ucTestTelemetryPayload ) - 1, NULL ),
                      eAzureIoTHubClientSuccess );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTHubClient_Init_Failure ),
        cmocka_unit_test( testAzureIoTHubClient_Init_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Init_NULLOptions ),
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
        cmocka_unit_test( testAzureIoTHubClient_SendTelemetry_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_ut", tests, NULL, NULL );
}
