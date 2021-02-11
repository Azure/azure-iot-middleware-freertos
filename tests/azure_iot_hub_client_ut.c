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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_hub_client.h"

AzureIoTMQTTStatus_t AzureIoTMQTT_Init( AzureIoTMQTTHandle_t  pContext,
                                        const TransportInterface_t * pTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t getTimeFunction,
                                        AzureIoTMQTTEventCallback_t userCallback,
                                        uint8_t * pNetworkBuffer, uint32_t networkBufferLength )
{
  ( void )pContext;
  ( void )pTransportInterface;
  ( void )getTimeFunction;
  ( void )userCallback;
  ( void )pNetworkBuffer;
  ( void )networkBufferLength;

  return((AzureIoTMQTTStatus_t)mock());
}

TickType_t xTaskGetTickCount( void )
{
  return 0;
}

static void test_check(void** state)
{
  (void)state;

  struct AzureIoTHubClient hubClient;
  //AzureIoTHubClient_Init( &hubClient, NULL, 0,
  //                        NULL, 0, NULL, NULL, 0, NULL, NULL );
  AzureIoTHubClient_Deinit( &hubClient );
  
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test( test_check )
    };

    return cmocka_run_group_tests_name("azure_iot_hub_client_ut", tests, NULL, NULL);
}
