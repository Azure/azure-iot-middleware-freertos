/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_json_reader.h"
/*-----------------------------------------------------------*/

static uint8_t ucProperty[] = "property";
static uint8_t ucPropertyTwo[] = "property_two";
static uint8_t ucValueOne[] = "value_one";

/*
 * {
 *   "property_one": "value_one",
 *   "property_two": 42,
 *   "property_three": 42.42,
 *   "property_four": true,
 *   "property_five": {
 *     "dummy": "dummy"
 *   },
 *   "property_six": ["test"]
 * }
 */
static uint8_t ucTestJSON[] =
    "{\"property_one\":\"value_one\","
    "\"property_two\":42,"
    "\"property_three\":42.42,"
    "\"property_four\":true,"
    "\"property_five\":{\"dummy\":\"dummy\"},"
    "\"property_six\":[\"test\"]}";

/*
 * {
 * "property_one": "value_one"
 * }
 */
static uint8_t ucTestJSONString[] =
    "{\"property_one\":\"value_one\"}";

/*
 * {
 * "property_one": "value_one"
 * }
 */
static uint8_t ucTestJSONMalformed[] =
    "{\"property_one\":}";

/*
 * {
 * "property_one": {
 *     "child_one":"value_one"
 * },
 * "property_two": "value_two"
 * }
 */
static uint8_t ucTestJSONChildren[] =
    "{\"property_one\":{\"child_one\":\"value_one\"},\"property_two\":\"value_two\"}";

/*
 * {
 * "property_one": true
 * }
 */
static uint8_t ucTestJSONBool[] =
    "{\"property\":true}";

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
 * "property_one": ["value_one", "value_two"],
 * }
 */
static uint8_t ucTestJSONArray[] =
    "{\"property_one\":[\"value_one\",\"value_two\"]}";


/*
 * {}
 */
static uint8_t ucTestEmptyJSONArray[] =
    "{}";

uint32_t ulGetAllTests();

static void testAzureIoTJSONReader_Init_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_Init( NULL,
                                               ucTestJSONString,
                                               strlen( ucTestJSONString ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init if JSON text is NULL */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               NULL,
                                               strlen( ucTestJSONString ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init if JSON text length is 0 */
    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               ucTestJSONString,
                                               0 ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_Init_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               ucTestJSON,
                                               strlen( ucTestJSON ) ),
                      eAzureIoTSuccess );
}

static void testAzureIoTJSONReader_NextToken_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_NextToken( NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_NextTokenBadJSON_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONMalformed, strlen( ucTestJSONMalformed ) ),
                      eAzureIoTSuccess );

    /* Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /* Property Name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTErrorUnexpectedChar );
}

static void testAzureIoTJSONReader_NextToken_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSON, strlen( ucTestJSON ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property int */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property double */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property bool (true) */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenTRUE );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );
    assert_int_equal( AzureIoTJSONReader_SkipChildren( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /* Property array begin */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_ARRAY );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /* Property array end */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_ARRAY );

    /*End object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );
}

static void testAzureIoTJSONReader_SkipChildren_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_SkipChildren( NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_SkipChildren_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONChildren, strlen( ucTestJSONChildren ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Begin child object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*End child object */
    assert_int_equal( AzureIoTJSONReader_SkipChildren( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );
    assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xReader, ucPropertyTwo, strlen( ucPropertyTwo ) ) );
}

static void testAzureIoTJSONReader_GetTokenBool_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    bool xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenBool( NULL,
                                                       &xValue ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenBool( &xReader,
                                                       NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenBool_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    bool xValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONBool, strlen( ucTestJSONBool ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Bool value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_GetTokenBool( &xReader, &xValue ), eAzureIoTSuccess );
    assert_true( xValue );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenTRUE );
}

static void testAzureIoTJSONReader_GetTokenInt32_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    int32_t lValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( NULL,
                                                        &lValue ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xReader,
                                                        NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenInt32_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    int32_t lValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONInt32, strlen( ucTestJSONInt32 ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*int32 value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xReader, &lValue ), eAzureIoTSuccess );
    assert_int_equal( lValue, 42 );
}

static void testAzureIoTJSONReader_GetTokenDouble_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    double xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( NULL,
                                                         &xValue ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xReader,
                                                         NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenDouble_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    double xValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONDouble, strlen( ucTestJSONDouble ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*int32 value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xReader, &xValue ), eAzureIoTSuccess );
    assert_int_equal( xValue, 42.42 );
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
                      eAzureIoTErrorInvalidArgument );

    /* Fail get token string if char pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         NULL,
                                                         sizeof( ucValue ),
                                                         &usBytesCopied ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail get token string if char buffer size is 0 */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         ucValue,
                                                         0,
                                                         &usBytesCopied ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail get token string if out size is NULL */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader,
                                                         ucValue,
                                                         sizeof( ucValue ),
                                                         NULL ),
                      eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_GetTokenString_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    uint8_t ucValue[ 16 ] = { 0 };
    uint32_t ulBytesCopied;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONString, strlen( ucTestJSONString ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader, ucValue, sizeof( ucValue ), &ulBytesCopied ), eAzureIoTSuccess );
    assert_int_equal( ulBytesCopied, strlen( ucValueOne ) );
    assert_string_equal( ucValueOne, ucValue );
}

static void testAzureIoTJSONReader_TokenIsTextEqual_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;

    /* Fail init if JSON reader is NULL */
    assert_false( AzureIoTJSONReader_TokenIsTextEqual( NULL,
                                                       ucValueOne,
                                                       strlen( ucValueOne ) ) );

    /* Fail init if char pointer is NULL */
    assert_false( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                       NULL,
                                                       strlen( ucValueOne ) ) );

    /* Fail init if char pointer length is 0 */
    assert_false( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                       ucValueOne,
                                                       0 ) );
}

static void testAzureIoTJSONReader_TokenIsTextEqual_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONString, strlen( ucTestJSONString ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
    assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xReader, ucValueOne, strlen( ucValueOne ) ) );
}

static void testAzureIoTJSONReader_TokenType_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xValue;

    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenType( NULL,
                                                    &xValue ), eAzureIoTErrorInvalidArgument );

    /* Fail init if bool pointer is NULL */
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader,
                                                    NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTJSONReader_TokenType_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestJSONArray, strlen( ucTestJSONArray ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Begin array */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_ARRAY );

    /*String value one */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*String value two */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*Begin array */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_ARRAY );

    /*End object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );
}

static void testAzureIoTJSONReader_InvalidRead_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    uint8_t ucValue[ 16 ] = { 0 };
    uint32_t ulBytesCopied;
    bool xValue;
    int32_t lValue;
    double xDoubleValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucTestEmptyJSONArray, strlen( ucTestEmptyJSONArray ) ),
                      eAzureIoTSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /* Access invalid token api */
    assert_int_equal( AzureIoTJSONReader_GetTokenString( &xReader, ucValue, sizeof( ucValue ), &ulBytesCopied ),
                      eAzureIoTErrorJSONInvalidState );

    assert_int_equal( AzureIoTJSONReader_GetTokenBool( &xReader, &xValue ),
                      eAzureIoTErrorJSONInvalidState );

    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xReader, &lValue ),
                      eAzureIoTErrorJSONInvalidState );

    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xReader, &xDoubleValue ),
                      eAzureIoTErrorJSONInvalidState );

    /*End object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );

    /*Invalid next token */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTErrorJSONReaderDone );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONReader_Init_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_Init_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_NextToken_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_NextTokenBadJSON_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_NextToken_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_SkipChildren_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_SkipChildren_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenBool_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenBool_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenInt32_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenInt32_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenDouble_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenDouble_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenString_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_GetTokenString_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenIsTextEqual_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenIsTextEqual_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenType_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_TokenType_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_InvalidRead_Failure )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_reader_ut", tests, NULL, NULL );
}
