/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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
 *
 * {
 * "component": {
 *  "__t": "c",
 *  "property": "value"
 * }
 * }
 *
 */
static const uint8_t ucTestJSONComponent[] =
    "{\"component\":{\"__t\":\"c\",\"property\":\"value\"}}";

/*
 *
 * {
 *   "property": {
 *     "ac": 200,
 *     "av": 1,
 *     "ad": "success",
 *     "value": "val"
 *   }
 * }
 *
 */
static const uint8_t ucTestJSONResponse[] =
    "{\"property\":{\"ac\":200,\"av\":1,\"ad\":\"success\",\"value\":\"val\"}}";

/*
 *
 * {
 *   "one_component": {
 *     "thing_one": 1,
 *     "thing_two": "string",
 *     "thing_three": true,
 *     "thing_four": 40.2
 *   },
 *   "two_component": {
 *     "prop_one": 45,
 *     "prop_two": "foo"
 *   },
 *   "not_component": 42,
 *   "$version": 5
 * }
 *
 */
static const uint8_t ucTestJSONVersion[] =
    "{\"one_component\":{\"thing_one\":1,\"thing_two\":\"string\",\"thing_three\":true,\"thing_four\":40.2},\"two_component\":{\"prop_one\":" \
    "45,\"prop_two\":\"foo\"},\"not_component\":42,\"$version\":5}";

/*
 *
 * {
 *   "desired":{
 *    "targetTemperature":40,
 *    "$version":2
 *   },
 *   "reported":{
 *     "PropertyIterationForCurrentConnection":"3",
 *     "$version":4
 *   }
 * }
 *
 */
static const uint8_t ucTestJSONGetPayload[] =
    "{\"desired\":{\"targetTemperature\":40,\"$version\":2},\"reported\":{\"PropertyIterationForCurrentConnection\":\"3\",\"$version\":4}}";

/*
 *
 * {
 *   "targetTemperature": 40,
 *   "$version": 6
 * }
 *
 */
static const uint8_t ucTestJSONWriteableUpdate[] =
    "{\"targetTemperature\":40,\"$version\":6}";

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";

static uint8_t ucBuffer[ 512 ];
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};

static uint8_t ucJSONWriterBuffer[ 128 ];

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter );
TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

void prvInitJSONWriter( AzureIoTJSONWriter_t * pxWriter )
{
    memset( ucJSONWriterBuffer, 0, sizeof( ucJSONWriterBuffer ) );
    assert_int_equal( AzureIoTJSONWriter_Init( pxWriter, ucJSONWriterBuffer, sizeof( ucJSONWriterBuffer ) ), eAzureIoTSuccess );
}


TickType_t xTaskGetTickCount( void )
{
    return 1;
}
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    return 0xFFFFFFFFFFFFFFFF;
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClientProperties_BuilderBeginComponent_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;
    const char * pucComponentName = "component_one";

    /* Fail begin component when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( NULL,
                                                                         &xJSONWriter,
                                                                         pucComponentName,
                                                                         strlen( pucComponentName ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin component when JSON writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         NULL,
                                                                         pucComponentName,
                                                                         strlen( pucComponentName ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin component when component name is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         &xJSONWriter,
                                                                         NULL,
                                                                         strlen( pucComponentName ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin component when component name length is zero */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         &xJSONWriter,
                                                                         pucComponentName,
                                                                         0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderEndComponent_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;

    /* Fail end component when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndComponent( NULL,
                                                                       &xJSONWriter ), eAzureIoTErrorInvalidArgument );

    /* Fail end component when JSON writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndComponent( &xTestIoTHubClient,
                                                                       NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderComponent_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;
    const char * pucComponentName = "component";

    prvInitJSONWriter( &xJSONWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xJSONWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         &xJSONWriter,
                                                                         pucComponentName,
                                                                         strlen( pucComponentName ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendPropertyName( &xJSONWriter,
                                                             "property",
                                                             strlen( "property" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xJSONWriter,
                                                       "value",
                                                       strlen( "value" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndComponent( &xTestIoTHubClient,
                                                                       &xJSONWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xJSONWriter ), eAzureIoTSuccess );

    assert_string_equal( ucTestJSONComponent, ucJSONWriterBuffer );
}

static void testAzureIoTHubClientProperties_BuilderBeginResponseStatus_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;
    const char * pucPropertyName = "property_one";
    int32_t lAckCode = 200;
    int32_t lAckVersion = 1;
    const char * pucAckDescription = "Property Accepted";

    /* Fail begin response when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( NULL,
                                                                              &xJSONWriter,
                                                                              pucPropertyName,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin response when JSON writer NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              NULL,
                                                                              pucPropertyName,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin response when property name is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              &xJSONWriter,
                                                                              NULL,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTErrorInvalidArgument );

    /* Fail begin response when property name length is 0 */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              &xJSONWriter,
                                                                              pucPropertyName,
                                                                              0,
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderEndResponseStatus_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;

    /* Fail end response when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndResponseStatus( NULL,
                                                                            &xJSONWriter ), eAzureIoTErrorInvalidArgument );

    /* Fail end response when JSON Writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndResponseStatus( &xTestIoTHubClient,
                                                                            NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderResponse_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;
    const char * pucPropertyName = "property";
    int32_t lAckCode = 200;
    int32_t lAckVersion = 1;
    const char * pucAckDescription = "success";

    prvInitJSONWriter( &xJSONWriter );

    assert_int_equal( AzureIoTJSONWriter_AppendBeginObject( &xJSONWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              &xJSONWriter,
                                                                              pucPropertyName,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendString( &xJSONWriter,
                                                       "val",
                                                       strlen( "val" ) ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndResponseStatus( &xTestIoTHubClient,
                                                                            &xJSONWriter ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONWriter_AppendEndObject( &xJSONWriter ), eAzureIoTSuccess );

    assert_string_equal( ucTestJSONResponse, ucJSONWriterBuffer );
}

static void testAzureIoTHubClientProperties_GetPropertiesVersion_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesRequestedMessage;
    uint32_t ulVersion;

    /* Fail get properties version when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( NULL,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTErrorInvalidArgument );

    /* Fail get properties version when JSON reader is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        NULL,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTErrorInvalidArgument );

    /* Fail get properties version when response type is not alllowed */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        eAzureIoTHubPropertiesReportedResponseMessage,
                                                                        &ulVersion ), eAzureIoTErrorInvalidArgument );

    /* Fail get properties verion when version pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_GetPropertiesVersion_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesWritablePropertyMessage;
    uint32_t ulVersion;

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONVersion,
                          strlen( ucTestJSONVersion ) ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTSuccess );
    assert_int_equal( ulVersion, 5 );
}

static void testAzureIoTHubClientProperties_GetPropertiesVersionGetDocument_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesRequestedMessage;
    uint32_t ulVersion;

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONGetPayload,
                          strlen( ucTestJSONGetPayload ) ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTSuccess );
    assert_int_equal( ulVersion, 2 );
}

static void testAzureIoTHubClientProperties_GetPropertiesVersionWriteableUpdate_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesWritablePropertyMessage;
    uint32_t ulVersion;

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONWriteableUpdate,
                          strlen( ucTestJSONWriteableUpdate ) ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTSuccess );
    assert_int_equal( ulVersion, 6 );
}


static void testAzureIoTHubClientProperties_GetNextComponentProperty_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesRequestedMessage;
    AzureIoTHubClientPropertyType_t xPropertyType = eAzureIoTHubClientPropertyWritable;
    const uint8_t * pucComponentName = NULL;
    uint32_t usComponentNameLength = 0;

    /* Fail get next component property when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( NULL,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTErrorInvalidArgument );

    /* Fail get next component property when JSON reader is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            NULL,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTErrorInvalidArgument );

    /* Fail get next component property when response type is not allowed */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            eAzureIoTHubPropertiesReportedResponseMessage,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTErrorInvalidArgument );

    /* Fail get next component property when component name pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            NULL,
                                                                            &usComponentNameLength ), eAzureIoTErrorInvalidArgument );

    /* Fail get next component property when component name length pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTHubClientProperties_GetNextComponentProperty_Success( void ** ppvState )
{
    AzureIoTResult_t xResult;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTJSONTokenType_t xTokenType;
    double xTokenDouble;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesWritablePropertyMessage;
    AzureIoTHubClientPropertyType_t xPropertyType = eAzureIoTHubClientPropertyWritable;
    uint32_t ulVersion;
    const uint8_t * ucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    int32_t lValue = 0;

    AzureIoTHubClientOptions_t xOptions;
    AzureIoTHubClientComponent_t pxComponentNameList[] =
    {
        azureiothubCREATE_COMPONENT( "one_component" ), azureiothubCREATE_COMPONENT( "two_component" )
    };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    AzureIoTHubClient_OptionsInit( &xOptions );
    xOptions.pxComponentList = pxComponentNameList;
    xOptions.ulComponentListLength = 2;

    AzureIoTHubClient_Init( &xTestIoTHubClient,
                            ucHostname,
                            strlen( ucHostname ),
                            ucDeviceId,
                            strlen( ucDeviceId ),
                            &xOptions,
                            ucBuffer, sizeof( ucBuffer ),
                            prvGetUnixTime,
                            &xTransportInterface );

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONVersion,
                          strlen( ucTestJSONVersion ) ),
                      eAzureIoTSuccess );

    while( ( xResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                             &xJSONReader,
                                                                             xResponseType,
                                                                             xPropertyType,
                                                                             &ucComponentName,
                                                                             &ulComponentNameLength ) ) == eAzureIoTSuccess )
    {
        /* Make sure we're on a property name */
        assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
        assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );

        if( ucComponentName == NULL )
        {
            assert_int_equal( ulComponentNameLength, 0 );
            assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "not_component", strlen( "not_component" ) ) );
            /*Advance to property value */
            assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
            assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
            assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xJSONReader, &lValue ), eAzureIoTSuccess );
            assert_int_equal( lValue, 42 );
            assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
        }
        else if( strncmp( "one_component", ucComponentName, strlen( "one_component" ) ) == 0 )
        {
            assert_int_equal( strlen( "one_component" ), ulComponentNameLength );

            if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "thing_one", strlen( "thing_one" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
                assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xJSONReader, &lValue ), eAzureIoTSuccess );
                assert_int_equal( lValue, 1 );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "thing_two", strlen( "thing_two" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
                assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "string", strlen( "string" ) ) );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "thing_three", strlen( "thing_three" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenTRUE );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "thing_four", strlen( "thing_four" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
                assert_int_equal( AzureIoTJSONReader_GetTokenDouble( &xJSONReader, &xTokenDouble ), eAzureIoTSuccess );
                assert_in_range( xTokenDouble, 40.19, 40.21 );
                assert_true( xTokenDouble );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else
            {
                /* A property was encountered that we don't recognize. Should not reach here. */
                assert_true( false );
            }
        }
        else if( strncmp( "two_component", ucComponentName, strlen( "two_component" ) ) == 0 )
        {
            assert_int_equal( strlen( "two_component" ), ulComponentNameLength );

            if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "prop_one", strlen( "prop_one" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
                assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xJSONReader, &lValue ), eAzureIoTSuccess );
                assert_int_equal( lValue, 45 );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "prop_two", strlen( "prop_two" ) ) )
            {
                /*Advance to property value */
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
                assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
                assert_int_equal( xTokenType, eAzureIoTJSONTokenSTRING );
                assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "foo", strlen( "foo" ) ) );
                assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
            }
            else
            {
                /* A property was encountered that we don't recognize. Should not reach here. */
                assert_true( false );
            }
        }
        else
        {
            /* A property was encountered that we don't recognize. Should not reach here. */
            assert_true( false );
        }
    }

    /* Make sure we ended on "end of properties" and not an error. */
    assert_int_equal( xResult, eAzureIoTErrorEndOfProperties );
}

static void testAzureIoTHubClientProperties_GetNextComponentPropertyGetDocument_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTJSONTokenType_t xTokenType;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesRequestedMessage;
    AzureIoTHubClientPropertyType_t xPropertyType = eAzureIoTHubClientPropertyWritable;
    uint32_t ulVersion;
    const uint8_t * ucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    int32_t lValue = 0;
    AzureIoTHubClientOptions_t xOptions;

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    AzureIoTHubClient_OptionsInit( &xOptions );

    AzureIoTHubClient_Init( &xTestIoTHubClient,
                            ucHostname,
                            strlen( ucHostname ),
                            ucDeviceId,
                            strlen( ucDeviceId ),
                            &xOptions,
                            ucBuffer, sizeof( ucBuffer ),
                            prvGetUnixTime,
                            &xTransportInterface );

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONGetPayload,
                          strlen( ucTestJSONGetPayload ) ),
                      eAzureIoTSuccess );

    /* First component | first property */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &ucComponentName,
                                                                            &ulComponentNameLength ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );
    assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "targetTemperature", strlen( "targetTemperature" ) ) );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
    /*Advance to property value */
    assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xJSONReader, &lValue ), eAzureIoTSuccess );
    assert_int_equal( lValue, 40 );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );

    /* End of properties */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &ucComponentName,
                                                                            &ulComponentNameLength ), eAzureIoTErrorEndOfProperties );
}

static void testAzureIoTHubClientProperties_GetNextComponentPropertyWriteableUpdate_Success( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTJSONTokenType_t xTokenType;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesWritablePropertyMessage;
    AzureIoTHubClientPropertyType_t xPropertyType = eAzureIoTHubClientPropertyWritable;
    uint32_t ulVersion;
    const uint8_t * ucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    int32_t lValue = 0;

    AzureIoTHubClientOptions_t xOptions;

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    AzureIoTHubClient_OptionsInit( &xOptions );

    AzureIoTHubClient_Init( &xTestIoTHubClient,
                            ucHostname,
                            strlen( ucHostname ),
                            ucDeviceId,
                            strlen( ucDeviceId ),
                            &xOptions,
                            ucBuffer, sizeof( ucBuffer ),
                            prvGetUnixTime,
                            &xTransportInterface );

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONWriteableUpdate,
                          strlen( ucTestJSONWriteableUpdate ) ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init(
                          &xJSONReader,
                          ucTestJSONWriteableUpdate,
                          strlen( ucTestJSONWriteableUpdate ) ),
                      eAzureIoTSuccess );

    /* First component | first property */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &ucComponentName,
                                                                            &ulComponentNameLength ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenPROPERTY_NAME );
    assert_true( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "targetTemperature", strlen( "targetTemperature" ) ) );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );
    /*Advance to property value */
    assert_int_equal( AzureIoTJSONReader_TokenType( &xJSONReader, &xTokenType ), eAzureIoTSuccess );
    assert_int_equal( xTokenType, eAzureIoTJSONTokenNUMBER );
    assert_int_equal( AzureIoTJSONReader_GetTokenInt32( &xJSONReader, &lValue ), eAzureIoTSuccess );
    assert_int_equal( lValue, 40 );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xJSONReader ), eAzureIoTSuccess );

    /* End of properties */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &ucComponentName,
                                                                            &ulComponentNameLength ), eAzureIoTErrorEndOfProperties );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderBeginComponent_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderComponent_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderEndComponent_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderComponent_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderBeginResponseStatus_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderEndResponseStatus_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderResponse_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetPropertiesVersion_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetPropertiesVersion_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetPropertiesVersionGetDocument_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetPropertiesVersionWriteableUpdate_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetNextComponentProperty_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetNextComponentProperty_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetNextComponentPropertyGetDocument_Success ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetNextComponentPropertyWriteableUpdate_Success ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_properties_ut ", tests, NULL, NULL );
}
