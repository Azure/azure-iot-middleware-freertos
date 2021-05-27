/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot.h"
/*-----------------------------------------------------------*/

static const uint8_t ucTestKey[] = "Hello";
static const uint8_t ucTestValue[] = "World";

static uint8_t ucBuffer[ 1024 ];

/*-----------------------------------------------------------*/

uint32_t ulGetAllTests();
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesInit_Failure( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    /* Fail init when null Message control block */
    assert_int_equal( AzureIoT_MessagePropertiesInit( NULL, ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null Buffer */
    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      NULL, 0, 0 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesInit_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesAppend_Failure( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    /* Setup property bag */
    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucTestKey ) ),
                      eAzureIoTSuccess );

    /* Failed for NULL key passed */
    assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                        NULL, 0, ucTestValue,
                                                        sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for NULL value passed */
    assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                        NULL, 0, ucTestValue,
                                                        sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for bigger data append - buffer was initialized to be size sizeof( ucTestKey ) and isn't big enough for request */
    assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                        ucTestKey, sizeof( ucTestKey ) - 1,
                                                        ucTestValue, sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesAppend_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                        ucTestKey, sizeof( ucTestKey ) - 1,
                                                        ucTestValue, sizeof( ucTestValue ) - 1 ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesFind_Failure( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;
    const uint8_t * pucOutValue;
    uint32_t ulOutValueLength;

    ( void ) ppvState;

    /* Setup property bag and add test data */
    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                        ucTestKey, sizeof( ucTestKey ) - 1,
                                                        ucTestValue, sizeof( ucTestValue ) - 1 ),
                      eAzureIoTSuccess );

    /* Failed for NULL key */
    assert_int_equal( AzureIoT_MessagePropertiesFind( &xTestMessageProperties,
                                                      NULL, 0, &pucOutValue, &ulOutValueLength ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for NULL outvalue */
    assert_int_equal( AzureIoT_MessagePropertiesFind( &xTestMessageProperties,
                                                      ucTestKey, sizeof( ucTestKey ) - 1,
                                                      NULL, 0 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesFind_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;
    const uint8_t * pucOutValue;
    uint32_t ulOutValueLength;

    ( void ) ppvState;

    /* Setup property bag and add key-value to it */
    assert_int_equal( AzureIoT_MessagePropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    for( uint32_t ulIndex = 0; ulIndex < sizeof( ucTestKey ) - 1; ulIndex++ )
    {
        assert_int_equal( AzureIoT_MessagePropertiesAppend( &xTestMessageProperties,
                                                            &( ucTestKey[ ulIndex ] ), ( uint32_t ) ( sizeof( ucTestKey ) - 1 - ulIndex ),
                                                            ucTestValue, sizeof( ucTestValue ) - 1 ),
                          eAzureIoTSuccess );
    }

    /* Find all keys in reverse order */
    for( uint32_t ulPos = sizeof( ucTestKey ) - 1; ulPos > 0; ulPos-- )
    {
        assert_int_equal( AzureIoT_MessagePropertiesFind( &xTestMessageProperties,
                                                          &( ucTestKey[ ulPos - 1 ] ), ( uint32_t ) ( sizeof( ucTestKey ) - ulPos ),
                                                          &pucOutValue, &ulOutValueLength ),
                          eAzureIoTSuccess );

        assert_int_equal( ulOutValueLength, sizeof( ucTestValue ) - 1 );
        assert_memory_equal( pucOutValue, ucTestValue, ulOutValueLength );
    }
}
/*-----------------------------------------------------------*/

static void testAzureIoTInit_Success( void ** ppvState )
{
    ( void ) ppvState;

    /* Should always succeed  */
    assert_int_equal( AzureIoT_Init(),
                      eAzureIoTSuccess );

    AzureIoT_Deinit();
}
/*-----------------------------------------------------------*/

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTMessagePropertiesInit_Failure ),
        cmocka_unit_test( testAzureIoTMessagePropertiesInit_Success ),
        cmocka_unit_test( testAzureIoTMessagePropertiesAppend_Failure ),
        cmocka_unit_test( testAzureIoTMessagePropertiesAppend_Success ),
        cmocka_unit_test( testAzureIoTMessagePropertiesFind_Failure ),
        cmocka_unit_test( testAzureIoTMessagePropertiesFind_Success ),
        cmocka_unit_test( testAzureIoTInit_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
