/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_reader.c
 * @brief Implementation of the Azure IoT JSON reader.
 */

#include "azure_iot_json_reader.h"

/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_reader.h
 *
 * @brief The JSON reader used by the middleware for PnP properties.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_json_reader.h"

AzureIoTHubClientResult_t AzureIoTJSONReader_Init( AzureIoTJSONReader_t * pxReader,
                                                   const uint8_t * pucBuffer,
                                                   uint32_t usBufferLen )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_span xJSONSpan;


    if( ( pxReader == NULL ) || ( pucBuffer == NULL ) || ( usBufferLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONReader_Init failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    xJSONSpan = az_span_create( ( uint8_t * ) pucBuffer, ( int32_t ) usBufferLen );

    if( az_result_failed( xCoreResult = az_json_reader_init( &pxReader->_internal.xCoreReader, xJSONSpan, NULL ) ) )
    {
        AZLogError( ( "Could not initialize the JSON reader: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_NextToken( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_NextToken failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_reader_next_token( &pxReader->_internal.xCoreReader ) ) )
    {
        AZLogError( ( "Could not get next JSON token: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONReader_SkipChildren( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_SkipChildren failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_reader_skip_children( &pxReader->_internal.xCoreReader ) ) )
    {
        AZLogError( ( "Could not skip children in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenBool( AzureIoTJSONReader_t * pxReader,
                                                           bool * pxValue )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pxValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenBool failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_boolean( &pxReader->_internal.xCoreReader.token, pxValue ) ) )
    {
        AZLogError( ( "Could not get boolean in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenInt32( AzureIoTJSONReader_t * pxReader,
                                                            int32_t * pilValue )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pilValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenInt32 failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_int32( &pxReader->_internal.xCoreReader.token, pilValue ) ) )
    {
        AZLogError( ( "Could not get int32_t in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenDouble( AzureIoTJSONReader_t * pxReader,
                                                             double * pxValue )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pxValue == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_GetTokenDouble failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_double( &pxReader->_internal.xCoreReader.token, pxValue ) ) )
    {
        AZLogError( ( "Could not get double in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenString( AzureIoTJSONReader_t * pxReader,
                                                             uint8_t * pucBuffer,
                                                             uint32_t usBufferSize,
                                                             uint32_t * pusBytesCopied )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxReader == NULL ) || ( pucBuffer == NULL ) || ( usBufferSize == 0 ) || ( pusBytesCopied == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed( xCoreResult = az_json_token_get_string( &pxReader->_internal.xCoreReader.token,
                                                                  ( char * ) pucBuffer, ( int32_t ) usBufferSize, ( int32_t * ) pusBytesCopied ) ) )
    {
        AZLogError( ( "Could not get string in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_TokenIsTextEqual( AzureIoTJSONReader_t * pxReader,
                                                               const uint8_t * pucExpectedText,
                                                               uint32_t ulExpectedTextLength )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_span xExpectedTextSpan;

    if( ( pxReader == NULL ) || ( pucExpectedText == NULL ) || ( ulExpectedTextLength == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    xExpectedTextSpan = az_span_create( ( uint8_t * ) pucExpectedText, ( int32_t ) ulExpectedTextLength );

    if( az_result_failed( xCoreResult = az_json_token_is_text_equal( &pxReader->_internal.xCoreReader.token, xExpectedTextSpan ) ) )
    {
        AZLogError( ( "Could not compare text in JSON: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONReader_TokenType( AzureIoTJSONReader_t * pxReader,
                                                        AzureIoTJSONTokenType_t * pxTokenType )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxReader == NULL ) || ( pxTokenType == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONReader_TokenType failed: invalid argument" ) );
        xResult = eAzureIoTHubClientInvalidArgument;
    }
    else
    {
        *pxTokenType = pxReader->_internal.xCoreReader.token.kind;
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
