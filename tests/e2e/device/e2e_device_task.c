/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <setjmp.h>
#include <cmocka.h> /* macros: https://api.cmocka.org/group__cmocka__asserts.html */

/* Azure IoT library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* E2E test includes */
#include "e2e_device_process_commands.h"
/*-----------------------------------------------------------*/

#define e2etestE2E_STACKSIZE    ( 8 * 1024 )
#define e2etestE2E_PRIORITY     ( 2 )
/*-----------------------------------------------------------*/

extern int ulArgc;
extern char ** ppcArgv;

static AzureIoTHubClient_t xAzureIoTHubClient;
static uint8_t ucSharedBuffer[ 5 * 1024 ];
static uint16_t usReceivedPubacks[ 7 ] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF,0xFF};
static uint16_t usReceivedPubacksIndex;
/*-----------------------------------------------------------*/

/**
 * Telemetry PUBACK callback
 *
 **/
static void prvTelemetryPubackCallback( uint16_t usPacketID )
{
    AZLogInfo( ( "Puback received for packet id in callback: 0x%08x", usPacketID ) );
    usReceivedPubacks[ usReceivedPubacksIndex++ ] = usPacketID;
}
/*-----------------------------------------------------------*/

/**
 * Verify sent/received packet id PUBACK
 *
 **/
static AzureIoTHubClientResult_t prvVerifyPuback( uint16_t * pusSentPacketID )
{
    AZLogInfo( ( "Verifying match of sent and received packet id's" ) );

    for( int i = 0; i < 7; i++ )
    {
        AZLogInfo( ( "Check: %d = %d", usReceivedPubacks[ i ], pusSentPacketID[ i ] ) );
        assert_int_equal( usReceivedPubacks[ i ], pusSentPacketID[ i ] );
    }
}

/*
 * Entry point to E2E tests
 **/
static void vTestEntry( void ** ppvState )
{
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    bool xSessionPresent = false;
    uint16_t * pusArrayOfSentPacketID;

    if( ulArgc != 5 )
    {
        LogError( ( "Usage: %s <hostname> <device_id> <module_id> <symmetric key>\r\n", ppcArgv[ 0 ] ) );
        assert_int_equal( ulArgc, 5 );
    }

    xNetworkCredentials.disableSni = true;
    xNetworkCredentials.pRootCa = ( const unsigned char * ) azureiotROOT_CA_PEM;
    xNetworkCredentials.rootCaSize = sizeof( azureiotROOT_CA_PEM );
    xNetworkContext.pParams = &xTlsTransportParams;

    assert_int_equal( xConnectToServerWithBackoffRetries( ( const char * ) ppcArgv[ 1 ],
                                                          8883, &xNetworkCredentials,
                                                          &xNetworkContext ),
                      TLS_TRANSPORT_SUCCESS );

    assert_int_equal( AzureIoT_Init(), eAzureIoTSuccess );

    LogInfo( ( "Creating an MQTT connection to %s.\r\n", ppcArgv[ 1 ] ) );

    xTransport.pxNetworkContext = &xNetworkContext;
    xTransport.xSend = TLS_FreeRTOS_send;
    xTransport.xRecv = TLS_FreeRTOS_recv;

    assert_int_equal( AzureIoTHubClient_OptionsInit( &xHubOptions ),
                      eAzureIoTHubClientSuccess );

    xHubOptions.xTelemetryCallback = prvTelemetryPubackCallback;

    xHubOptions.pucModelID = ( const uint8_t * ) ppcArgv[ 3 ];
    xHubOptions.ulModuleIDLength = ( uint32_t ) strlen( ppcArgv[ 3 ] );

    assert_int_equal( AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                              ppcArgv[ 1 ], strlen( ppcArgv[ 1 ] ),
                                              ppcArgv[ 2 ], strlen( ppcArgv[ 2 ] ),
                                              &xHubOptions,
                                              ucSharedBuffer, sizeof( ucSharedBuffer ),
                                              ulGetUnixTime,
                                              &xTransport ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                         ( const uint8_t * ) ppcArgv[ 4 ],
                                                         strlen( ppcArgv[ 4 ] ),
                                                         ulCalculateHMAC ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                 false, &xSessionPresent,
                                                 e2etestCONNACK_RECV_TIMEOUT_MS ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient,
                                                                       vHandleCloudMessage,
                                                                       &xAzureIoTHubClient,
                                                                       ULONG_MAX ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTHubClient_SubscribeDirectMethod( &xAzureIoTHubClient,
                                                               vHandleDirectMethod,
                                                               &xAzureIoTHubClient,
                                                               ULONG_MAX ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTHubClient_SubscribeDeviceTwin( &xAzureIoTHubClient,
                                                             vHandleDeviceTwinMessage,
                                                             &xAzureIoTHubClient,
                                                             ULONG_MAX ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( ulE2EDeviceProcessCommands( &xAzureIoTHubClient, &pusArrayOfSentPacketID ),
                      eAzureIoTHubClientSuccess );

    prvVerifyPuback( pusArrayOfSentPacketID );

    AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
    AzureIoTHubClient_Deinit( &xAzureIoTHubClient );
    TLS_FreeRTOS_Disconnect( &xNetworkContext );
}
/*-----------------------------------------------------------*/

/*
 * Task to run E2E tests
 **/
static void prvE2ETask( void * pvParameters )
{
    const struct CMUnitTest xTests[] =
    {
        cmocka_unit_test( vTestEntry ),
    };

    setbuf( stdout, NULL );
    exit( cmocka_run_group_tests( xTests, NULL, NULL ) );
}
/*-----------------------------------------------------------*/

/*
 * Hook to start all the tasks required
 **/
void vInitD( void )
{
    xTaskCreate( prvE2ETask,
                 "prvE2ETask",
                 e2etestE2E_STACKSIZE,
                 NULL,
                 e2etestE2E_PRIORITY,
                 NULL );
}
/*-----------------------------------------------------------*/
