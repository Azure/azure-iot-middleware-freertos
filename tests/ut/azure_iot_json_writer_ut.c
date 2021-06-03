/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_json_writer.h"
#include "azure_iot_mqtt.h"
#include "azure_iot_hub_client.h"
/*-----------------------------------------------------------*/

/*
 * {
 * "property_one": 42
 * }
 */
#define azureiotjsonwriterutTEST_JSON_INT32 \
    "{\"property\":42}"

/*
 * {
 * "property_one": 42.42
 * }
 */
#define azureiotjsonwriterutTEST_JSON_DOUBLE \
    "{\"property\":42.42}"

/*
 * {
 * "property_one": true
 * }
 */
#define azureiotjsonwriterutTEST_JSON_BOOL \
    "{\"property\":true}"

/*
 * {
 * "property_one": "value"
 * }
 */
#define azureiotjsonwriterutTEST_JSON_STRING \
    "{\"property\":\"value\"}"

/*
 * {
 * "property_one": null
 * }
 */
#define azureiotjsonwriterutTEST_JSON_NULL \
    "{\"property\":null}"

/*
 * {
 * "property_one": "value"
 * }
 */
#define azureiotjsonwriterutTEST_JSON_COMPOUND \
    "{\"property\":{\"property\":42}}"

/*
 * {
 * "property": ["value_one", "value_two"],
 * }
 */
#define azureiotjsonwriterutTEST_JSON_ARRAY \
    "{\"property\":[\"value_one\",\"value_two\"]}"

static uint8_t ucJSONWriterBuffer[ 128 ];

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter );
uint32_t ulGetAllTests();

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter )
{
    memset( ucJSONWriterBuffer, 0, sizeof( ucJSONWriterBuffer ) );
    assert_int_equal( AzureIoTJSONWriter_Init( pxWriter, ucJSONWriterBuffer, sizeof( ucJSONWriterBuffer ) ), eAzureIoTSuccess );
}

static void testAzureIoTJSONWriter_Init_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;
    uint8_t pucTestJSON[ 32 ];

    /* Fail init if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_Init( NULL,
                                               pucTestJSON,
                                               sizeof( pucTestJSON ) ), eAzureIoTErrorInvalidArgument );

    /* Fail init if JSON text is NULL */
    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter,
                                               NULL,
                                               sizeof( pucTestJSON ) ), eAzureIoTErrorInvalidArgument );

    /* Fail init if JSON text length is 0 */
    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter,
                                               pucTestJSON,
                                               0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_Init_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    uint8_t pucTestJSON[ 32 ];

    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter,
                                               pucTestJSON,
                                               sizeof( pucTestJSON ) ), eAzureIoTSuccess );
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
                                                                       lValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with int32 if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       NULL,
                                                                       strlen( pucPropertyName ),
                                                                       lValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with int32 if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       pucPropertyName,
                                                                       0,
                                                                       lValue ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithInt32_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       "property",
                                                                       strlen( "property" ),
                                                                       42 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_INT32 ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_INT32 );
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
                                                                        usFractionalDigits ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with double if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        NULL,
                                                                        strlen( pucPropertyName ),
                                                                        xValue,
                                                                        usFractionalDigits ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with double if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        xValue,
                                                                        usFractionalDigits ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithDouble_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        "property",
                                                                        strlen( "property" ),
                                                                        42.42, 2 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_DOUBLE ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_DOUBLE );
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
                                                                      usValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with bool if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      NULL,
                                                                      strlen( pucPropertyName ),
                                                                      usValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with bool if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      pucPropertyName,
                                                                      0,
                                                                      usValue ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithBool_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      "property",
                                                                      strlen( "property" ),
                                                                      true ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_BOOL ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_BOOL );
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
                                                                        strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        NULL,
                                                                        strlen( pucPropertyName ),
                                                                        pucValue,
                                                                        strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        pucValue,
                                                                        strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        0,
                                                                        NULL,
                                                                        strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        pucPropertyName,
                                                                        strlen( pucPropertyName ),
                                                                        pucValue,
                                                                        0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithString_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        "property",
                                                                        strlen( "property" ),
                                                                        "value",
                                                                        strlen( "value" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_STRING ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_STRING );
}

static void testAzureIoTJSONWriter_GetBytesUsed_Failure( void ** ppvState )
{
    /* Fail get bytes used if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( NULL ), -1 );
}

static void testAzureIoTJSONWriter_GetBytesUsed_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );
    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        "property",
                                                                        strlen( "property" ),
                                                                        "value",
                                                                        strlen( "value" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_STRING ) );
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
                                                       strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       NULL,
                                                       strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       pucValue,
                                                       0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendString_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       "value",
                                                       strlen( "value" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_STRING ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_STRING );
}


static void testAzureIoTJSONWriter_AppendJSON_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucJSON = azureiotjsonwriterutTEST_JSON_INT32;

    /* Fail JSON text if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( NULL,
                                                         pucJSON,
                                                         strlen( pucJSON ) ), eAzureIoTErrorInvalidArgument );

    /* Fail JSON text if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         NULL,
                                                         strlen( pucJSON ) ), eAzureIoTErrorInvalidArgument );

    /* Fail JSON text if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         pucJSON,
                                                         0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendJSON_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucJSON = azureiotjsonwriterutTEST_JSON_INT32;

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         azureiotjsonwriterutTEST_JSON_INT32,
                                                         strlen( azureiotjsonwriterutTEST_JSON_INT32 ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_COMPOUND ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_COMPOUND );
}

static void testAzureIoTJSONWriter_AppendPropertyName_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    uint8_t * pucValue = "property_name";

    /* Fail append property name if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( NULL,
                                                             pucValue,
                                                             strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property name if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             NULL,
                                                             strlen( pucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property name if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             pucValue,
                                                             0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    bool xValue = true;

    /* Fail append bool if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendBool( NULL,
                                                     xValue ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendBool( &xWriter,
                                                     true ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_BOOL ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_BOOL );
}

static void testAzureIoTJSONWriter_AppendInt32_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    int32_t lValue = 42;

    /* Fail append int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( NULL,
                                                      lValue ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendInt32_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( &xWriter,
                                                      42 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_INT32 ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_INT32 );
}

static void testAzureIoTJSONWriter_AppendDouble_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    double xValue = 42.42;
    int16_t usFractionalDigits = 2;

    /* Fail append int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendDouble( NULL,
                                                       xValue,
                                                       usFractionalDigits ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendDouble_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendDouble( &xWriter,
                                                       42.42, 2 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_DOUBLE ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_DOUBLE );
}

static void testAzureIoTJSONWriter_AppendNull_Failure( void ** ppvState )
{
    /* Fail append NULL if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendNull( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendNull_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendNull( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_NULL ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_NULL );
}

static void testAzureIoTJSONWriter_AppendBeginObject_Failure( void ** ppvState )
{
    /* Fail append begin object if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendEndObject_Failure( void ** ppvState )
{
    /* Fail append end object if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBeginArray_Failure( void ** ppvState )
{
    /* Fail append begin array if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendBeginArray( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendEndArray_Failure( void ** ppvState )
{
    /* Fail append end array if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendEndArray( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendArray_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendBeginArray( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter, "value_one", strlen( "value_one" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter, "value_two", strlen( "value_two" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndArray( &xWriter ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( azureiotjsonwriterutTEST_JSON_ARRAY ) );

    assert_string_equal( ucJSONWriterBuffer, azureiotjsonwriterutTEST_JSON_ARRAY );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONWriter_Init_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_Init_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithInt32_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithInt32_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithDouble_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithDouble_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithBool_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithBool_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithString_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyWithString_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_GetBytesUsed_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_GetBytesUsed_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendString_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendString_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendJSON_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendJSON_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendPropertyName_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendBool_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendBool_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendInt32_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendInt32_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendDouble_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendDouble_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendNull_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendNull_Success ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendBeginObject_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendEndObject_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendBeginArray_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendEndArray_Failure ),
        cmocka_unit_test( testAzureIoTJSONWriter_AppendArray_Success ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_writer_ut", tests, NULL, NULL );
}
