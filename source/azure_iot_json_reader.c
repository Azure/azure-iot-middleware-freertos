/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_reader.c
 * @brief Implementation of the Azure IoT JSON reader.
 */

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_json_reader.h"

#include "azure_iot_result.h"

AzureIoTResult_t AzureIoTJSONReader_Init( AzureIoTJSONReader_t * pxReader,
                                          const uint8_t * pucBuffer,
                                          uint32_t usBufferLen )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;
    az_span xJSONSpan;


    if( ( pxReader == NULL ) || ( pucBuffer == NULL ) || ( usBufferLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONReader_Init failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xJSONSpan = az_span_create( ( uint8_t * ) pucBuffer, ( int32_t ) usBufferLen );

    if( az_result_failed( xCoreResult = az_json_reader_init( &pxReader->_internal.xCoreReader, xJSONSpan, NULL ) ) )
    {
        AZLogError( ( "Could not initialize the JSON reader: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


AzureIoTResult_t AzureIoTJSONReader_NextToken( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( pxReader == NULL )
    {
        AZLogError( ( "AzureIoTJSONReader_NextToken failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_reader_next_token( &pxReader->_internal.xCoreReader ) ) )
    {
        AZLogError( ( "Could not get next JSON token: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTJSONReader_SkipChildren( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( pxReader == NULL )
    {
        AZLogError( ( "AzureIoTJSONReader_SkipChildren failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_reader_skip_children( &pxReader->_internal.xCoreReader ) ) )
    {
        AZLogError( ( "Could not skip children in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


AzureIoTResult_t AzureIoTJSONReader_GetTokenBool( AzureIoTJSONReader_t * pxReader,
                                                  bool * pxValue )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pxValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenBool failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_boolean( &pxReader->_internal.xCoreReader.token, pxValue ) ) )
    {
        AZLogError( ( "Could not get boolean in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


AzureIoTResult_t AzureIoTJSONReader_GetTokenInt32( AzureIoTJSONReader_t * pxReader,
                                                   int32_t * pilValue )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pilValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenInt32 failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_int32( &pxReader->_internal.xCoreReader.token, pilValue ) ) )
    {
        AZLogError( ( "Could not get int32_t in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


AzureIoTResult_t AzureIoTJSONReader_GetTokenDouble( AzureIoTJSONReader_t * pxReader,
                                                    double * pxValue )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pxValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenDouble failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_double( &pxReader->_internal.xCoreReader.token, pxValue ) ) )
    {
        AZLogError( ( "Could not get double in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


AzureIoTResult_t AzureIoTJSONReader_GetTokenString( AzureIoTJSONReader_t * pxReader,
                                                    uint8_t * pucBuffer,
                                                    uint32_t usBufferSize,
                                                    uint32_t * pusBytesCopied )
{
    AzureIoTResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pucBuffer == NULL ) || ( usBufferSize == 0 ) || ( pusBytesCopied == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_string( &pxReader->_internal.xCoreReader.token,
                                                                  ( char * ) pucBuffer, ( int32_t ) usBufferSize, ( int32_t * ) pusBytesCopied ) ) )
    {
        AZLogError( ( "Could not get string in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}


bool AzureIoTJSONReader_TokenIsTextEqual( AzureIoTJSONReader_t * pxReader,
                                          const uint8_t * pucExpectedText,
                                          uint32_t ulExpectedTextLength )
{
    az_span xExpectedTextSpan;

    if( ( pxReader == NULL ) || ( pucExpectedText == NULL ) || ( ulExpectedTextLength == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        return false;
    }

    xExpectedTextSpan = az_span_create( ( uint8_t * ) pucExpectedText, ( int32_t ) ulExpectedTextLength );

    return az_json_token_is_text_equal( &pxReader->_internal.xCoreReader.token, xExpectedTextSpan );
}

AzureIoTResult_t AzureIoTJSONReader_TokenType( AzureIoTJSONReader_t * pxReader,
                                               AzureIoTJSONTokenType_t * pxTokenType )
{
    AzureIoTResult_t xResult;

    if( ( pxReader == NULL ) || ( pxTokenType == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    }
    else
    {
        *pxTokenType = ( AzureIoTJSONTokenType_t ) pxReader->_internal.xCoreReader.token.kind;
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}
