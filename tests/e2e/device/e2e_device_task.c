/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <setjmp.h>
#include <cmocka.h> /* macros: https://api.cmocka.org/group__cmocka__asserts.html */

/* Azure Provisioning/IoTHub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"
#include "e2e_device_process_commands.h"

#define e2etestE2E_STACKSIZE    ( 8 * 1024 )
#define e2etestE2E_PRIORITY     ( 2 )

extern int ulArgc;
extern char ** ppcArgv;

static AzureIoTHubClient_t xAzureIoTHubClient;
static uint8_t ucSharedBuffer[ 5 * 1024 ];

static void test_entry( void ** state )
{
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientOptions_t xHubOptions = { 0 };

    if( ulArgc != 5 )
    {
        LogError( ( "Usage: %s <hostname> <device_id> <module_id> <symmetric key>\r\n", ppcArgv[ 0 ] ) );
        assert_int_equal( ulArgc, 5 );
    }

    xNetworkCredentials.disableSni = true;
    /* Set the credentials for establishing a TLS connection. */
    xNetworkCredentials.pRootCa = ( const unsigned char * ) azureiotROOT_CA_PEM;
    xNetworkCredentials.rootCaSize = sizeof( azureiotROOT_CA_PEM );
    xNetworkContext.pParams = &xTlsTransportParams;

    assert_int_equal( xConnectToServerWithBackoffRetries( ( const char * ) ppcArgv[ 1 ],
                                                          8883, &xNetworkCredentials,
                                                          &xNetworkContext ),
                      TLS_TRANSPORT_SUCCESS );

    assert_int_equal( AzureIoT_Init(), AZURE_IOT_SUCCESS);

    /* Sends an MQTT Connect packet over the already established TLS connection,
     * and waits for connection acknowledgment (CONNACK) packet. */
    LogInfo( ( "Creating an MQTT connection to %s.\r\n", ppcArgv[ 1 ] ) );

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pNetworkContext = &xNetworkContext;
    xTransport.send = TLS_FreeRTOS_send;
    xTransport.recv = TLS_FreeRTOS_recv;

    xHubOptions.pModuleId = ( const uint8_t * ) ppcArgv[ 3 ];
    xHubOptions.moduleIdLength = strlen( ppcArgv[ 3 ] );

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
                                                 false,
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

    assert_int_equal( ulE2EDeviceProcessCommands( &xAzureIoTHubClient ),
                      eAzureIoTHubClientSuccess );

    AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
    AzureIoTHubClient_Deinit( &xAzureIoTHubClient );
    TLS_FreeRTOS_Disconnect( &xNetworkContext );
}

static void prvE2ETask( void * pvParameters )
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( test_entry ),
    };

    setbuf( stdout, NULL );
    exit( cmocka_run_group_tests( tests, NULL, NULL ) );
}

void vDemoEntry( void )
{
    xTaskCreate( prvE2ETask,           /* Function that implements the task. */
                 "prvE2ETask",         /* Text name for the task - only used for debugging. */
                 e2etestE2E_STACKSIZE, /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                 /* Task parameter - not used in this case. */
                 e2etestE2E_PRIORITY,  /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );               /* Used to pass out a handle to the created task - not used in this case. */
}
