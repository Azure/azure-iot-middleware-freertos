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
#define azureiotjsonreaderutTEST_JSON \
    "{\"property_one\":\"value_one\",\"property_two\",\"value_two\"}"

uint32_t ulGetAllTests();

static void testAzureIoTJSONReader_Init_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    uint8_t * pucTestJSON = azureiotjsonreaderutTEST_JSON;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_Init( NULL,
                                               pucTestJSON,
                                               strlen( pucTestJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text is NULL */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               NULL,
                                               strlen( pucTestJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text length is 0 */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               pucTestJSON,
                                               0 ), eAzureIoTHubClientInvalidArgument );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONReader_Init_Failure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_reader_ut", tests, NULL, NULL );
}
