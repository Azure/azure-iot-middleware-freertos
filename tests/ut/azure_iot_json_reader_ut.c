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
    "{\"property_one\":\"value_one\",\"property_two\":\"value_two\"}"

/*
 * {
 * "property_one": "value_one"
 * }
 */
#define azureiotjsonreaderutTEST_JSON_STRING \
    "{\"property_one\":\"value_one\"}"

/*
 * {
 * "property_one": {
 *     "child_one":"value_one"
 * },
 * "property_two": "value_two"
 * }
 */
#define azureiotjsonreaderutTEST_JSON_CHILDREN \
    "{\"property_one\":{\"child_one\":\"value_one\"},\"property_two\":\"value_two\"}"

/*
 * {
 * "property_one": true
 * }
 */
#define azureiotjsonreaderutTEST_JSON_BOOL \
    "{\"property\":true}"

/*
 * {
 * "property_one": 42
 * }
 */
#define azureiotjsonreaderutTEST_JSON_INT32 \
    "{\"property\":42}"

/*
 * {
 * "property_one": 42.42
 * }
 */
#define azureiotjsonreaderutTEST_JSON_DOUBLE \
    "{\"property\":42.42}"

/*
 * {
 * "property_one": ["value_one", "value_two"],
 * }
 */
#define azureiotjsonreaderutTEST_JSON_ARRAY \
    "{\"property_one\":[\"value_one\",\"value_two\"]}"



uint32_t ulGetAllTests();

static void testAzureIoTJSONReader_Init_Failure( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    uint8_t * pucTestJSON = azureiotjsonreaderutTEST_JSON_STRING;

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

static void testAzureIoTJSONReader_Init_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    uint8_t * pucTestJSON = azureiotjsonreaderutTEST_JSON;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader,
                                               pucTestJSON,
                                               strlen( pucTestJSON ) ),
                      eAzureIoTHubClientSuccess );
}

static void testAzureIoTJSONReader_NextToken_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_NextToken( NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_NextToken_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON, strlen( azureiotjsonreaderutTEST_JSON ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*End object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );
}

static void testAzureIoTJSONReader_SkipChildren_Failure( void ** ppvState )
{
    /* Fail init if JSON reader is NULL */
    assert_int_equal( AzureIoTJSONReader_SkipChildren( NULL ),
                      eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTJSONReader_SkipChildren_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_CHILDREN, strlen( azureiotjsonreaderutTEST_JSON_CHILDREN ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Begin child object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*End child object */
    assert_int_equal( AzureIoTJSONReader_SkipChildren( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );
    assert_int_equal( AzureIoTJSONReader_TokenIsTextEqual( &xReader, "property_two", strlen( "property_two" ) ), eAzureIoTHubClientSuccess );
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

static void testAzureIoTJSONReader_GetTokenBool_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    bool xValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_BOOL, strlen( azureiotjsonreaderutTEST_JSON_BOOL ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Bool value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenTRUE );
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

static void testAzureIoTJSONReader_GetTokenInt32_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    int32_t lValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_INT32, strlen( azureiotjsonreaderutTEST_JSON_INT32 ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*int32 value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xReader, &lValue ), eAzureIoTHubClientSuccess );
    assert_int_equal( lValue, 42 );
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

static void testAzureIoTJSONReader_GetTokenDouble_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    double xValue;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_DOUBLE, strlen( azureiotjsonreaderutTEST_JSON_DOUBLE ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*int32 value */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xReader, &xValue ), eAzureIoTHubClientSuccess );
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

static void testAzureIoTJSONReader_GetTokenString_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_STRING, strlen( azureiotjsonreaderutTEST_JSON_STRING ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
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

static void testAzureIoTJSONReader_TokenIsTextEqual_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_STRING, strlen( azureiotjsonreaderutTEST_JSON_STRING ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Property string */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
    assert_int_equal( AzureIoTJSONReader_TokenIsTextEqual( &xReader, "value_one", strlen( "value_one" ) ), eAzureIoTHubClientSuccess );
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

static void testAzureIoTJSONReader_TokenType_Success( void ** ppvState )
{
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, azureiotjsonreaderutTEST_JSON_ARRAY, strlen( azureiotjsonreaderutTEST_JSON_ARRAY ) ),
                      eAzureIoTHubClientSuccess );

    /*Begin object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );
    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_OBJECT );

    /*Property name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

    /*Begin array */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenBEGIN_ARRAY );

    /*String value one */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*String value two */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );

    /*Begin array */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_ARRAY );

    /*End object */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ),
                      eAzureIoTHubClientSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xReader, &xTokenType ), eAzureIoTHubClientSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenEND_OBJECT );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTJSONReader_Init_Failure ),
        cmocka_unit_test( testAzureIoTJSONReader_Init_Success ),
        cmocka_unit_test( testAzureIoTJSONReader_NextToken_Failure ),
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
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_json_reader_ut", tests, NULL, NULL );
}
