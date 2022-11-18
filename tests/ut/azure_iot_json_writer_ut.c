/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_json_writer.h"
/*-----------------------------------------------------------*/

int32_t lInt32Value = 42;
double xDoubleValue = 42.42;
bool xBoolValue = true;
int16_t usFractionalDigits = 2;
uint8_t ucProperty[] = "property";
uint8_t ucPropertyName[] = "property_name";
uint8_t ucValue[] = "value";
uint8_t ucValueOne[] = "value_one";

/*
 * {
 * "property_one": 42
 * }
 */
static uint8_t ucTestJSONInt32[] =
    "{\"property\":42}";

/*
 * {
 * "property_one": 42.42
 * }
 */
static uint8_t ucTestJSONDouble[] =
    "{\"property\":42.42}";

/*
 * {
 * "property_one": true
 * }
 */
static uint8_t ucTestJSONBool[] =
    "{\"property\":true}";

/*
 * {
 * "property_one": "value"
 * }
 */
static uint8_t ucTestJSONString[] =
    "{\"property\":\"value\"}";

/*
 * {
 * "property_one": null
 * }
 */
static uint8_t ucTestJSONNull[] =
    "{\"property\":null}";

/*
 * {
 * "property_one": "value"
 * }
 */
static uint8_t ucTestJSONCompound[] =
    "{\"property\":{\"property\":42}}";

/*
 * {
 * "property": ["value_one", "value_two"],
 * }
 */
static uint8_t ucTestJSONArray[] =
    "{\"property\":[\"value_one\",\"value_two\"]}";

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

    /* Fail append property with int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( NULL,
                                                                       ucPropertyName,
                                                                       strlen( ucPropertyName ),
                                                                       lInt32Value ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with int32 if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       NULL,
                                                                       strlen( ucPropertyName ),
                                                                       lInt32Value ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with int32 if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       ucPropertyName,
                                                                       0,
                                                                       lInt32Value ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendPropertyWithInt32_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       "property",
                                                                       strlen( "property" ),
                                                                       lInt32Value ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONInt32 ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONInt32 );
}

static void testAzureIoTJSONWriter_AppendPropertyWithDouble_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append property with double if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( NULL,
                                                                        ucPropertyName,
                                                                        strlen( ucPropertyName ),
                                                                        xDoubleValue,
                                                                        usFractionalDigits ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with double if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        NULL,
                                                                        strlen( ucPropertyName ),
                                                                        xDoubleValue,
                                                                        usFractionalDigits ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with double if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        ucPropertyName,
                                                                        0,
                                                                        xDoubleValue,
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
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONDouble ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONDouble );
}

static void testAzureIoTJSONWriter_AppendPropertyWithBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append property with bool if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( NULL,
                                                                      ucPropertyName,
                                                                      strlen( ucPropertyName ),
                                                                      xBoolValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with bool if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      NULL,
                                                                      strlen( ucPropertyName ),
                                                                      xBoolValue ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with bool if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      ucPropertyName,
                                                                      0,
                                                                      xBoolValue ), eAzureIoTErrorInvalidArgument );
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
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONBool ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONBool );
}

static void testAzureIoTJSONWriter_AppendPropertyWithString_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append property with string if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( NULL,
                                                                        ucPropertyName,
                                                                        strlen( ucPropertyName ),
                                                                        ucValue,
                                                                        strlen( ucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if property name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        NULL,
                                                                        strlen( ucPropertyName ),
                                                                        ucValue,
                                                                        strlen( ucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if property name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        ucPropertyName,
                                                                        0,
                                                                        ucValue,
                                                                        strlen( ucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        ucPropertyName,
                                                                        0,
                                                                        NULL,
                                                                        strlen( ucValue ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property with string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        ucPropertyName,
                                                                        strlen( ucPropertyName ),
                                                                        ucValue,
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
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONString ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONString );
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
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONString ) );
}

static void testAzureIoTJSONWriter_AppendString_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append string if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendString( NULL,
                                                       ucValueOne,
                                                       strlen( ucValueOne ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append string if value name is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       NULL,
                                                       strlen( ucValueOne ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append string if value name length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       ucValueOne,
                                                       0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendString_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter,
                                                       "value",
                                                       strlen( "value" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONString ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONString );
}


static void testAzureIoTJSONWriter_AppendJSON_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail JSON text if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( NULL,
                                                         ucTestJSONInt32,
                                                         strlen( ucTestJSONInt32 ) ), eAzureIoTErrorInvalidArgument );

    /* Fail JSON text if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         NULL,
                                                         strlen( ucTestJSONInt32 ) ), eAzureIoTErrorInvalidArgument );

    /* Fail JSON text if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         ucTestJSONInt32,
                                                         0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendJSON_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         ucTestJSONInt32,
                                                         strlen( ucTestJSONInt32 ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONCompound ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONCompound );
}

static void testAzureIoTJSONWriter_AppendPropertyName_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append property name if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( NULL,
                                                             ucPropertyName,
                                                             strlen( ucPropertyName ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property name if JSON is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             NULL,
                                                             strlen( ucPropertyName ) ), eAzureIoTErrorInvalidArgument );

    /* Fail append property name if JSON length is 0 */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucPropertyName,
                                                             0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append bool if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendBool( NULL,
                                                     xBoolValue ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendBool_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendBool( &xWriter,
                                                     true ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONBool ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONBool );
}

static void testAzureIoTJSONWriter_AppendInt32_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( NULL,
                                                      lInt32Value ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendInt32_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( &xWriter,
                                                      42 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONInt32 ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONInt32 );
}

static void testAzureIoTJSONWriter_AppendDouble_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    /* Fail append int32 if JSON writer is NULL */
    assert_int_equal( AzureIoTJSONWriter_AppendDouble( NULL,
                                                       xDoubleValue,
                                                       usFractionalDigits ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONWriter_AppendDouble_Success( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;

    prvInitJSONWriter( &xWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendDouble( &xWriter,
                                                       42.42, 2 ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONDouble ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONDouble );
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
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendNull( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONNull ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONNull );
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
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendBeginArray( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter, "value_one", strlen( "value_one" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter, "value_two", strlen( "value_two" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndArray( &xWriter ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_GetBytesUsed( &xWriter ), strlen( ucTestJSONArray ) );

    assert_string_equal( ucJSONWriterBuffer, ucTestJSONArray );
}

static void testAzureIoTJSONWriter_InvalidWrite_Failure( void ** ppvState )
{
    AzureIoTJSONWriter_t xWriter;
    uint8_t ucBuffer[ 5 ];

    assert_int_equal( AzureIoTJSONWriter_Init( &xWriter, ucBuffer, sizeof( ucBuffer ) ), eAzureIoTSuccess );

    /*Begin Object */
    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTSuccess );

    /*Fail PropertyName value api's (not enough space in buffer) */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             ucProperty,
                                                             strlen( ucProperty ) ), eAzureIoTErrorOutOfMemory );

    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter,
                                                                       "property",
                                                                       strlen( "property" ),
                                                                       lInt32Value ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter,
                                                                        "property",
                                                                        strlen( "property" ),
                                                                        42.42, 2 ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithBoolValue( &xWriter,
                                                                      "property",
                                                                      strlen( "property" ),
                                                                      true ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                        "property",
                                                                        strlen( "property" ),
                                                                        "value",
                                                                        strlen( "value" ) ), eAzureIoTErrorOutOfMemory );
    /*Add PropertyName */
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xWriter,
                                                             "f",
                                                             strlen( "f" ) ), eAzureIoTSuccess );

    /* Fail value api's (not enough space in buffer) */
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xWriter, "value_two", strlen( "value_two" ) ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendJSONText( &xWriter,
                                                         ucTestJSONInt32,
                                                         strlen( ucTestJSONInt32 ) ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendBool( &xWriter,
                                                     true ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendInt32( &xWriter,
                                                      42 ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendDouble( &xWriter,
                                                       42.42, 2 ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendNull( &xWriter ), eAzureIoTErrorOutOfMemory );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xWriter ), eAzureIoTErrorOutOfMemory );
    assert_int_equal( AzureIoTJSONWriter_AppendBeginArray( &xWriter ), eAzureIoTErrorOutOfMemory );
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
        cmocka_unit_test( testAzureIoTJSONWriter_InvalidWrite_Failure )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_writer_ut", tests, NULL, NULL );
}
