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

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";

static uint8_t ucBuffer[ 512 ];
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};

TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

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

static void prvSetupTestIoTHubClient( AzureIoTHubClient_t * pxTestIoTHubClient )
{
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( pxTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucBuffer,
                                              sizeof( ucBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTHubClientSuccess );
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
                                                                         strlen( pucComponentName ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin component when JSON writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         NULL,
                                                                         pucComponentName,
                                                                         strlen( pucComponentName ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin component when component name is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         &xJSONWriter,
                                                                         NULL,
                                                                         strlen( pucComponentName ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin component when component name length is zero */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginComponent( &xTestIoTHubClient,
                                                                         &xJSONWriter,
                                                                         pucComponentName,
                                                                         0 ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderEndComponent_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;

    /* Fail end component when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndComponent( NULL,
                                                                       &xJSONWriter ), eAzureIoTHubClientInvalidArgument );

    /* Fail end component when JSON writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndComponent( &xTestIoTHubClient,
                                                                       NULL ), eAzureIoTHubClientInvalidArgument );
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
                                                                              strlen( pucAckDescription ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin response when JSON writer NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              NULL,
                                                                              pucPropertyName,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin response when property name is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              &xJSONWriter,
                                                                              NULL,
                                                                              strlen( pucPropertyName ),
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTHubClientInvalidArgument );

    /* Fail begin response when property name length is 0 */
    assert_int_equal( AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xTestIoTHubClient,
                                                                              &xJSONWriter,
                                                                              pucPropertyName,
                                                                              0,
                                                                              lAckCode,
                                                                              lAckVersion,
                                                                              pucAckDescription,
                                                                              strlen( pucAckDescription ) ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTHubClientProperties_BuilderEndResponseStatus_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONWriter_t xJSONWriter;

    /* Fail end response when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndResponseStatus( NULL,
                                                                            &xJSONWriter ), eAzureIoTHubClientInvalidArgument );

    /* Fail end response when JSON Writer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_BuilderEndResponseStatus( &xTestIoTHubClient,
                                                                            NULL ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTHubClientProperties_GetPropertiesVersion_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesGetMessage;
    uint32_t ulVersion;

    /* Fail get properties version when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( NULL,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTHubClientInvalidArgument );

    /* Fail get properties version when JSON reader is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        NULL,
                                                                        xResponseType,
                                                                        &ulVersion ), eAzureIoTHubClientInvalidArgument );

    /* Fail get properties version when response type is not alllowed */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        eAzureIoTHubPropertiesReportedResponseMessage,
                                                                        &ulVersion ), eAzureIoTHubClientInvalidArgument );

    /* Fail get properties verion when version pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetPropertiesVersion( &xTestIoTHubClient,
                                                                        &xJSONReader,
                                                                        xResponseType,
                                                                        NULL ), eAzureIoTHubClientInvalidArgument );
}

static void testAzureIoTHubClientProperties_GetNextComponentProperty_Failure( void ** ppvState )
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTHubMessageType_t xResponseType = eAzureIoTHubPropertiesGetMessage;
    AzureIoTHubClientPropertyType_t xPropertyType = eAzureIoTHubClientPropertyWriteable;
    const uint8_t * pucComponentName;
    uint32_t usComponentNameLength;

    /* Fail get next component property when client is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( NULL,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTHubClientInvalidArgument );

    /* Fail get next component property when JSON reader is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            NULL,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTHubClientInvalidArgument );

    /* Fail get next component property when response type is not allowed */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            eAzureIoTHubPropertiesReportedResponseMessage,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            &usComponentNameLength ), eAzureIoTHubClientInvalidArgument );

    /* Fail get next component property when component name pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            NULL,
                                                                            &usComponentNameLength ), eAzureIoTHubClientInvalidArgument );

    /* Fail get next component property when component name length pointer is NULL */
    assert_int_equal( AzureIoTHubClientProperties_GetNextComponentProperty( &xTestIoTHubClient,
                                                                            &xJSONReader,
                                                                            xResponseType,
                                                                            xPropertyType,
                                                                            &pucComponentName,
                                                                            NULL ), eAzureIoTHubClientInvalidArgument );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderBeginComponent_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderEndComponent_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderBeginResponseStatus_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_BuilderEndResponseStatus_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetPropertiesVersion_Failure ),
        cmocka_unit_test( testAzureIoTHubClientProperties_GetNextComponentProperty_Failure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_properties_ut", tests, NULL, NULL );
}
