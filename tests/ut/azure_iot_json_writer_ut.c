/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_hub_client_properties.h"
#include "azure_iot_mqtt.h"
#include "azure_iot_hub_client.h"
/*-----------------------------------------------------------*/

/*
 * {
 * "property_one": "value_one",
 * "property_two": "value_two"
 * }
 */
#define azureiotjsonwriterutTEST_JSON \
    "{\"property_one\":\"value_one\",\"property_two\",\"value_two\"}"

uint32_t ulGetAllTests();

static void testAzureIoTJSONWriter_Init_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;
    uint8_t pucTestJSON[32];

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONWriter_Init( NULL,
                                               pucTestJSON,
                                               sizeof( pucTestJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text is NULL */
    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter,
                                               NULL,
                                               sizeof( pucTestJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text length is 0 */
    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter,
                                               pucTestJSON,
                                               0 ), eAzureIoTHubClientInvalidArgument );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONWriter_Init_Failure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_writer_ut", tests, NULL, NULL );
}
