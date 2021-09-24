/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot.h"
#include "azure_iot_message.h"
#include "azure_iot_private.h"
#include <azure/core/internal/az_log_internal.h>

/*-----------------------------------------------------------*/

extern uint64_t ullLogLineCount;
static const uint8_t ucTestKey[] = "Hello";
static const uint8_t ucTestUnknownKey[] = "UnknownKey";
static const uint8_t ucTestValue[] = "World";
static const uint8_t ucURLEncodedHMACSHA256Key[] = "De1TOYsqBULq0nSzjVWvjCYUnQ3pklTuUdmoLsleyaw=";
static const uint8_t ucURLEncodedHMACSHA256Message[] = "Hello Unit Test";
static const uint8_t ucURLEncodedHMACSHA256Base64[] = "AAECAwQFBgcICRAREhMUFRYXGBkgISIjJCUmJygpMDE=";
static uint8_t ucBuffer[ 1024 ];
static const uint8_t ucFixedHMACSHA256[ 32 ] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x30, 0x31
};

/*-----------------------------------------------------------*/

uint32_t ulGetAllTests();
/*-----------------------------------------------------------*/

uint32_t ulFixedHMAC( const uint8_t * pucKey,
                      uint32_t ulKeyLength,
                      const uint8_t * pucData,
                      uint32_t ulDataLength,
                      uint8_t * pucOutput,
                      uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied )
{
    memcpy( pucOutput, ucFixedHMACSHA256, sizeof( ucFixedHMACSHA256 ) );
    *pulBytesCopied = sizeof( ucFixedHMACSHA256 );

    return 0;
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesInit_Failure( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    /* Fail init when null Message control block */
    assert_int_equal( AzureIoTMessage_PropertiesInit( NULL, ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTErrorInvalidArgument );

    /* Fail init when null Buffer */
    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      NULL, 0, 0 ),
                      eAzureIoTErrorInvalidArgument );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesInit_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesAppend_Failure( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    /* Setup property bag */
    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucTestKey ) ),
                      eAzureIoTSuccess );

    /* Failed for NULL key passed */
    assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
                                                        NULL, 0, ucTestValue,
                                                        sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for NULL value passed */
    assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
                                                        NULL, 0, ucTestValue,
                                                        sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for bigger data append - buffer was initialized to be size sizeof( ucTestKey ) and isn't big enough for request */
    assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
                                                        ucTestKey, sizeof( ucTestKey ) - 1,
                                                        ucTestValue, sizeof( ucTestValue ) - 1 ),
                      eAzureIoTErrorFailed );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesAppend_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;

    ( void ) ppvState;

    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
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
    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
                                                        ucTestKey, sizeof( ucTestKey ) - 1,
                                                        ucTestValue, sizeof( ucTestValue ) - 1 ),
                      eAzureIoTSuccess );

    /* Failed for NULL key */
    assert_int_equal( AzureIoTMessage_PropertiesFind( &xTestMessageProperties,
                                                      NULL, 0, &pucOutValue, &ulOutValueLength ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for NULL outvalue */
    assert_int_equal( AzureIoTMessage_PropertiesFind( &xTestMessageProperties,
                                                      ucTestKey, sizeof( ucTestKey ) - 1,
                                                      NULL, 0 ),
                      eAzureIoTErrorInvalidArgument );

    /* Failed for not found key */
    assert_int_equal( AzureIoTMessage_PropertiesFind( &xTestMessageProperties,
                                                      ucTestUnknownKey, sizeof( ucTestUnknownKey ) - 1,
                                                      &pucOutValue, &ulOutValueLength ),
                      eAzureIoTErrorItemNotFound );
}
/*-----------------------------------------------------------*/

static void testAzureIoTMessagePropertiesFind_Success( void ** ppvState )
{
    AzureIoTMessageProperties_t xTestMessageProperties;
    const uint8_t * pucOutValue;
    uint32_t ulOutValueLength;

    ( void ) ppvState;

    /* Setup property bag and add key-value to it */
    assert_int_equal( AzureIoTMessage_PropertiesInit( &xTestMessageProperties,
                                                      ucBuffer, 0, sizeof( ucBuffer ) ),
                      eAzureIoTSuccess );

    for( uint32_t ulIndex = 0; ulIndex < sizeof( ucTestKey ) - 1; ulIndex++ )
    {
        assert_int_equal( AzureIoTMessage_PropertiesAppend( &xTestMessageProperties,
                                                            &( ucTestKey[ ulIndex ] ), ( uint32_t ) ( sizeof( ucTestKey ) - 1 - ulIndex ),
                                                            ucTestValue, sizeof( ucTestValue ) - 1 ),
                          eAzureIoTSuccess );
    }

    /* Find all keys in reverse order */
    for( uint32_t ulPos = sizeof( ucTestKey ) - 1; ulPos > 0; ulPos-- )
    {
        assert_int_equal( AzureIoTMessage_PropertiesFind( &xTestMessageProperties,
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

static void testAzureIoTInit_LogSuccess( void ** ppvState )
{
    az_span xReceivedTestTopic = AZ_SPAN_LITERAL_FROM_STR( "unittest/fake/topic" );

    ( void ) ppvState;

    /* Should always succeed  */
    assert_int_equal( AzureIoT_Init(),
                      eAzureIoTSuccess );

    _az_log_write( AZ_LOG_MQTT_RECEIVED_TOPIC, xReceivedTestTopic );
    assert_int_not_equal( ullLogLineCount, 0 );

    AzureIoT_Deinit();
}
/*-----------------------------------------------------------*/

static void testAzureIoT_Base64HMACCalculateSuccess()
{
    uint8_t ucOutBuffer[ 512 ];
    uint32_t ulOutBufferLength;

    assert_int_equal( AzureIoT_Base64HMACCalculate( ulFixedHMAC,
                                                    ucURLEncodedHMACSHA256Key,
                                                    sizeof( ucURLEncodedHMACSHA256Key ) - 1,
                                                    ucURLEncodedHMACSHA256Message,
                                                    sizeof( ucURLEncodedHMACSHA256Message ) - 1,
                                                    ucBuffer, sizeof( ucBuffer ), ucOutBuffer,
                                                    sizeof( ucOutBuffer ), &ulOutBufferLength ),
                      eAzureIoTSuccess );
    assert_int_equal( sizeof( ucURLEncodedHMACSHA256Base64 ) - 1, ulOutBufferLength );
    assert_memory_equal( ucURLEncodedHMACSHA256Base64, ucOutBuffer, ulOutBufferLength );
}
/*-----------------------------------------------------------*/

/*
 * Private test functions
 */

static void testAzureIoT_TranslateCoreError( void ** state )
{
    /* Translated results */
    assert_int_equal( eAzureIoTSuccess, AzureIoT_TranslateCoreError( AZ_OK ) );

    assert_int_equal( eAzureIoTErrorTopicNoMatch, AzureIoT_TranslateCoreError( AZ_ERROR_IOT_TOPIC_NO_MATCH ) );

    assert_int_equal( eAzureIoTErrorEndOfProperties, AzureIoT_TranslateCoreError( AZ_ERROR_IOT_END_OF_PROPERTIES ) );

    assert_int_equal( eAzureIoTErrorInvalidArgument, AzureIoT_TranslateCoreError( AZ_ERROR_ARG ) );

    assert_int_equal( eAzureIoTErrorItemNotFound, AzureIoT_TranslateCoreError( AZ_ERROR_ITEM_NOT_FOUND ) );

    assert_int_equal( eAzureIoTErrorUnexpectedChar, AzureIoT_TranslateCoreError( AZ_ERROR_UNEXPECTED_CHAR ) );

    assert_int_equal( eAzureIoTErrorJSONInvalidState, AzureIoT_TranslateCoreError( AZ_ERROR_JSON_INVALID_STATE ) );

    assert_int_equal( eAzureIoTErrorJSONNestingOverflow, AzureIoT_TranslateCoreError( AZ_ERROR_JSON_NESTING_OVERFLOW ) );

    assert_int_equal( eAzureIoTErrorJSONReaderDone, AzureIoT_TranslateCoreError( AZ_ERROR_JSON_READER_DONE ) );

    /* Test non-translated result */
    assert_int_equal( eAzureIoTErrorFailed, AzureIoT_TranslateCoreError( AZ_ERROR_HTTP_INVALID_STATE ) );
}

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
        cmocka_unit_test( testAzureIoTInit_Success ),
        cmocka_unit_test( testAzureIoTInit_LogSuccess ),
        cmocka_unit_test( testAzureIoT_Base64HMACCalculateSuccess ),
        cmocka_unit_test( testAzureIoT_TranslateCoreError )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
