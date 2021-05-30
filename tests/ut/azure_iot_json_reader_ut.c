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
                                               strlen( pucTestJSON ) ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text is NULL */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               NULL,
                                               strlen( pucTestJSON ) ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if JSON text length is 0 */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               pucTestJSON,
                                               0 ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_NextToken_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_NextToken( NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_SkipChildren_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_SkipChildren( NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenBool_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    bool xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenBool( NULL,
                                                       &xValue ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenBool( &xReader,
                                                       NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenInt32_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    int32_t lValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( NULL,
                                                        &lValue ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xReader,
                                                        NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenDouble_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    double xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( NULL,
                                                         &xValue ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xReader,
                                                         NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenString_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    char ucValue[ 16 ];
    uint32_t usBytesCopied;

    /* Fail get token string if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( NULL,
                                                         ucValue,
                                                         sizeof( ucValue ),
                                                         &usBytesCopied ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail get token string if char pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         NULL,
                                                         sizeof( ucValue ),
                                                         &usBytesCopied ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail get token string if char buffer size is 0 */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         ucValue,
                                                         0,
                                                         &usBytesCopied ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail get token string if out size is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         ucValue,
                                                         sizeof( ucValue ),
                                                         NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_TokenIsTextEqual_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    char * pucValue = "value";

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenIsTextEqual( NULL,
                                                           pucValue,
                                                           strlen( pucValue ) ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if char pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                           NULL,
                                                           strlen( pucValue ) ),
                      eAzureIoTHubClientInvalidArgument );

    /* Fail init if char pointer length is 0 */
    assert_int_equal( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                           pucValue,
                                                           0 ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_TokenType_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenType( NULL,
                                                    &xValue ), eAzureIoTHubClientInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader,
                                                    NULL ), eAzureIoTHubClientInvalidArgument );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONReader_Init_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_NextToken_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_SkipChildren_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenBool_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenInt32_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenDouble_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenString_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenIsTextEqual_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenType_Failure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_reader_ut", tests, NULL, NULL );
}
