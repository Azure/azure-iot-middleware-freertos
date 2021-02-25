/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_hub_client.h"

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static const uint8_t ucSymmetricKey[] = "ABC12345";
static uint8_t ucBuffer[100];
static AzureIoTTransportInterface_t xTransportInterface = {
  .pNetworkContext = NULL,
  .send = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
  .recv = ( AzureIoTTransportRecv_t ) 0xACACACAC
};
static uint32_t uMallocAllocationCount = 0;

TickType_t xTaskGetTickCount(void)
{
  return 1;
}

void * pvPortMalloc( size_t xWantedSize )
{
  void * ret = ((void *)mock());

  if (ret)
  {
    uMallocAllocationCount++;
    ret = malloc( xWantedSize );
  }
  
  return ret;
}

void vPortFree( void * pv )
{
  uMallocAllocationCount--;
  free( pv );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Init( AzureIoTMQTTHandle_t  pContext,
                                        const AzureIoTTransportInterface_t * pTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t getTimeFunction,
                                        AzureIoTMQTTEventCallback_t userCallback,
                                        uint8_t * pNetworkBuffer, uint32_t networkBufferLength )
{
  return((AzureIoTMQTTStatus_t)mock());
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Connect( AzureIoTMQTTHandle_t  pContext,
                                           const AzureIoTMQTTConnectInfo_t * pConnectInfo,
                                           const AzureIoTMQTTPublishInfo_t * pWillInfo,
                                           uint32_t timeoutMs,
                                           bool * pSessionPresent )
{
  return((AzureIoTMQTTStatus_t)mock());
}

static uint64_t prvGetUnixTime( void )
{
  return 0xFFFFFFFFFFFFFFFF;
}

static uint32_t prvHmacFunction( const uint8_t * pucKey,
                                 uint32_t ulKeyLength,
                                 const uint8_t * pucData,
                                 uint32_t ulDataLength,
                                 uint8_t * pucOutput,
                                 uint32_t ulOutputLength,
                                 uint32_t * pucBytesCopied )
{
  return((uint32_t)mock());
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Init_Failure(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;
  AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

  (void)state;

  /* Fail init when null hostname passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                NULL, 0,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                &xHubClientOptions,
                                                ucBuffer,
                                                sizeof( ucBuffer ),
                                                prvGetUnixTime,
                                                &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );
  
  /* Fail init when null deviceId passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                NULL, 0,
                                                &xHubClientOptions,
                                                ucBuffer,
                                                sizeof( ucBuffer ),
                                                prvGetUnixTime,
                                                &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail init when null options passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                NULL,
                                                ucBuffer,
                                                sizeof( ucBuffer ),
                                                prvGetUnixTime,
                                                &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail init when null buffer passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                &xHubClientOptions,
                                                NULL, 0,
                                                prvGetUnixTime,
                                                &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail init when null AzureIoTGetCurrentTimeFunc_t passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof( ucBuffer ),
                                                NULL, &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail init when null Transport passed */
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof( ucBuffer ),
                                                prvGetUnixTime, NULL ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail init when AzureIoTMQTT_Init fails */
  will_return( AzureIoTMQTT_Init, AzureIoTMQTTNoMemory );
  assert_int_not_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                                ucHostname, sizeof( ucHostname ) - 1,
                                                ucDeviceId, sizeof( ucDeviceId ) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof( ucBuffer ),
                                                prvGetUnixTime, &xTransportInterface ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );
}

static void testAzureIoTHubClient_Init_Success(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;
  AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

  (void)state;
  will_return( AzureIoTMQTT_Init, AzureIoTMQTTSuccess );

  assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                            ucHostname, sizeof( ucHostname ) - 1,
                                            ucDeviceId, sizeof( ucDeviceId ) - 1,
                                            &xHubClientOptions,
                                            ucBuffer,
                                            sizeof( ucBuffer ),
                                            prvGetUnixTime,
                                            &xTransportInterface ),
                    AZURE_IOT_HUB_CLIENT_SUCCESS );
}

static void testAzureIoTHubClient_Deinit_Success(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;

  (void)state;

  AzureIoTHubClient_Deinit( &xTestIoTHubClient );
}

static void testAzureIoTHubClient_SymmetricKeySet_Failure(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;

  (void)state;

  /* Fail AzureIoTHubClient_SymmetricKeySet when null symmetric key is passed */
  assert_int_not_equal( AzureIoTHubClient_SymmetricKeySet( &xTestIoTHubClient,
                                                           NULL, 0,
                                                           prvHmacFunction ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );

  /* Fail AzureIoTHubClient_SymmetricKeySet when null hashing function is passed */
  assert_int_not_equal( AzureIoTHubClient_SymmetricKeySet( &xTestIoTHubClient,
                                                           ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                           NULL ),
                        AZURE_IOT_HUB_CLIENT_SUCCESS );
}

static void testAzureIoTHubClient_SymmetricKeySet_Success(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;

  (void)state;

  assert_int_equal( AzureIoTHubClient_SymmetricKeySet( &xTestIoTHubClient,
                                                       ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                       prvHmacFunction ),
                    AZURE_IOT_HUB_CLIENT_SUCCESS );
}

static void testAzureIoTHubClient_Connect_Success(void** state)
{
  AzureIoTHubClient_t xTestIoTHubClient;
  AzureIoTHubClientOptions_t xHubClientOptions = { 0 };
  uMallocAllocationCount = 0;

  (void)state;

  will_return( AzureIoTMQTT_Init, AzureIoTMQTTSuccess );
  will_return( prvHmacFunction, 0 );
  assert_int_equal( AzureIoTHubClient_Init( &xTestIoTHubClient,
                                            ucHostname, sizeof( ucHostname ) - 1,
                                            ucDeviceId, sizeof( ucDeviceId ) - 1,
                                            &xHubClientOptions,
                                            ucBuffer,
                                            sizeof( ucBuffer ),
                                            prvGetUnixTime,
                                            &xTransportInterface ),
                    AZURE_IOT_HUB_CLIENT_SUCCESS );
  assert_int_equal( AzureIoTHubClient_SymmetricKeySet( &xTestIoTHubClient,
                                                       ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                       prvHmacFunction ),
                    AZURE_IOT_HUB_CLIENT_SUCCESS );

    will_return( pvPortMalloc, !NULL );
    will_return( pvPortMalloc, !NULL );
    will_return( AzureIoTMQTT_Connect, AzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Connect( &xTestIoTHubClient,
                                                 false,
                                                 60 ),
                      AZURE_IOT_HUB_CLIENT_SUCCESS );

    assert_int_equal( uMallocAllocationCount, 0 );
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test( testAzureIoTHubClient_Init_Failure ),
        cmocka_unit_test( testAzureIoTHubClient_Init_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Deinit_Success ),
        cmocka_unit_test( testAzureIoTHubClient_SymmetricKeySet_Failure ),
        cmocka_unit_test( testAzureIoTHubClient_SymmetricKeySet_Success ),
        cmocka_unit_test( testAzureIoTHubClient_Connect_Success )
    };

    return cmocka_run_group_tests_name("azure_iot_hub_client_ut", tests, NULL, NULL);
}
