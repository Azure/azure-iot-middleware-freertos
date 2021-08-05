/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "internal/azure_iot_internal.h"
/*-----------------------------------------------------------*/

static void test_AzureIoT_TranslateCoreError( void ** state )
{
    /* Translated results */
    assert_int_equal( eAzureIoTSuccess, _AzureIoT_TranslateCoreError( AZ_OK ) );

    assert_int_equal( eAzureIoTErrorTopicNoMatch, _AzureIoT_TranslateCoreError( AZ_ERROR_IOT_TOPIC_NO_MATCH ) );

    assert_int_equal( eAzureIoTErrorEndOfProperties, _AzureIoT_TranslateCoreError( AZ_ERROR_IOT_END_OF_PROPERTIES ) );

    assert_int_equal( eAzureIoTErrorInvalidArgument, _AzureIoT_TranslateCoreError( AZ_ERROR_ARG ) );

    assert_int_equal( eAzureIoTErrorItemNotFound, _AzureIoT_TranslateCoreError( AZ_ERROR_ITEM_NOT_FOUND ) );

    assert_int_equal( eAzureIoTErrorUnexpectedChar, _AzureIoT_TranslateCoreError( AZ_ERROR_UNEXPECTED_CHAR ) );

    assert_int_equal( eAzureIoTErrorJSONInvalidState, _AzureIoT_TranslateCoreError( AZ_ERROR_JSON_INVALID_STATE ) );

    assert_int_equal( eAzureIoTErrorJSONNestingOverflow, _AzureIoT_TranslateCoreError( AZ_ERROR_JSON_NESTING_OVERFLOW ) );

    assert_int_equal( eAzureIoTErrorJSONReaderDone, _AzureIoT_TranslateCoreError( AZ_ERROR_JSON_READER_DONE ) );

    /* Test non-translated result */
    assert_int_equal( eAzureIoTErrorFailed, _AzureIoT_TranslateCoreError( AZ_ERROR_HTTP_INVALID_STATE ) );
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( test_AzureIoT_TranslateCoreError )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_internal_ut", tests, NULL, NULL );
}
