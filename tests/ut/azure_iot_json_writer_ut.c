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

static uint8_t ucJSONWriterBuffer[ 128 ];

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter );
uint32_t ulGetAllTests();

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTJSONWriter_Init( pxWriter, ucJSONWriterBuffer, sizeof( ucJSONWriterBuffer ) );
}

static void testAzureIoTJSONWriter_Init_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t pucTestJSON[ 32 ];

    /* Fail init if JSON writer is NULL */
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

static void testAzureIoTJSONWriter_AppendPropertyWithInt32_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucPropertyName = "property_name";
    int32_t lValue = 42;

    /* Fail append property with int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( NULL,
                                                                       pucPropertyName,
                                                                       strlen( pucPropertyName ),
                                                                       lValue ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with int32 if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       NULL,
                                                                       strlen( pucPropertyName ),
                                                                       lValue ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with int32 if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       pucPropertyName,
                                                                       0,
                                                                       lValue ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithDouble_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucPropertyName = "property_name";
    double xValue = 42.42;
    int16_t usFractionalDigits = 2;

    /* Fail append property with double if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( NULL,
                                                                        pucPropertyName,
                                                                        strlen( pucPropertyName ),
                                                                        xValue,
                                                                        usFractionalDigits ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with double if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        NULL,
                                                                        strlen( pucPropertyName ),
                                                                        xValue,
                                                                        usFractionalDigits ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with double if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        xValue,
                                                                        usFractionalDigits ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucPropertyName = "property_name";
    bool usValue = true;

    /* Fail append property with bool if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( NULL,
                                                                      pucPropertyName,
                                                                      strlen( pucPropertyName ),
                                                                      usValue ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with bool if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      NULL,
                                                                      strlen( pucPropertyName ),
                                                                      usValue ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with bool if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      pucPropertyName,
                                                                      0,
                                                                      usValue ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithString_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucPropertyName = "property_name";
    uint8_t * pucValue = "value_one";

    /* Fail append property with string if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( NULL,
                                                                        pucPropertyName,
                                                                        strlen( pucPropertyName ),
                                                                        pucValue,
                                                                        strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with string if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        NULL,
                                                                        strlen( pucPropertyName ),
                                                                        pucValue,
                                                                        strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with string if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        pucValue,
                                                                        strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        NULL,
                                                                        strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property with string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        strlen( pucPropertyName ),
                                                                        pucValue,
                                                                        0 ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_GetBytesUsed_Failure( void ** ppvState )
{
    /* Fail get bytes used if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( NULL ), -1 );
}

static void testAzureIoTJSONWriter_AppendString_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucPropertyName = "property_name";
    uint8_t * pucValue = "value_one";

    /* Fail append string if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendString( NULL,
                                                       pucValue,
                                                       strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       NULL,
                                                       strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       pucValue,
                                                       0 ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendJSON_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucJSON = azureiotjsonwriterutTEST_JSON;

    /* Fail JSON text if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( NULL,
                                                         pucJSON,
                                                         strlen( pucJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail JSON text if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         NULL,
                                                         strlen( pucJSON ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail JSON text if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         pucJSON,
                                                         0 ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyName_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucValue = "property_name";

    /* Fail append property name if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( NULL,
                                                             pucValue,
                                                             strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property name if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             NULL,
                                                             strlen( pucValue ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail append property name if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             pucValue,
                                                             0 ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    bool xValue = true;

    /* Fail append bool if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendBool( NULL,
                                                     xValue ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    int32_t lValue = 42;

    /* Fail append int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( NULL,
                                                     lValue ), eAzureIoTHubClientInvalidArgument );
}

AzureIoTJSONWriter_AppendInt32( AzureIoTJSONWriter_t * pxWriter,
                                                          int32_t lValue )


uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONWriter_Init_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithInt32_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithDouble_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithBool_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithString_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_GetBytesUsed_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendString_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendJSON_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyName_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendBool_Failure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_writer_ut", tests, NULL, NULL );
}
