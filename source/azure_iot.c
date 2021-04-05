/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot.c
 *
 * @brief Common Azure IoT code for FreeRTOS middleware.
 *
 */

#include <stdio.h>

#include "azure_iot.h"

#define azureiotBASE64_HASH_BUFFER_SIZE 33

static const char _cAzureIoTBase64Array[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*-----------------------------------------------------------*/

/**
 *
 * Set the log listener in the embedded SDK.
 *
 * */
static void prvAzureIoTLogListener( az_log_classification xClassification,
                                    az_span xMessage )
{
    ( void ) xClassification;
    /* In case logs are stripped out, suppress unused parameter error. */
    ( void ) xMessage;

    AZLogInfo( ( "%.*s", az_span_size( xMessage ), az_span_ptr( xMessage ) ) );
}
/*-----------------------------------------------------------*/

/**
 *
 * Decode Base64 bytes.
 *
 * TODO: Remove in favor of embedded SDK implementation.
 *
 * */
static AzureIoTResult_t prvAzureIoTBase64Decode( char * pcBase64name,
                                                 uint32_t ulLength,
                                                 uint8_t * pucName,
                                                 uint32_t ulNameSize,
                                                 uint32_t * pulBytesCopied )
{
    uint32_t i, j;
    uint32_t ulValue1, ulValue2;
    uint32_t ulStep;
    uint32_t ulSourceLength = ulLength;

    /* Adjust the ulLength to represent the ASCII name.  */
    ulLength = ( ( ulLength * 6 ) / 8 );

    if( pcBase64name[ ulSourceLength - 1 ] == '=' )
    {
        if( pcBase64name[ ulSourceLength - 2 ] == '=' )
        {
            ulLength--;
        }

        ulLength--;
    }

    if( ulNameSize < ulLength )
    {
        return eAzureIoTOutOfMemory;
    }

    /* Setup index into the ASCII name.  */
    j = 0;

    /* Compute the ASCII name.  */
    ulStep = 0;
    i = 0;

    while( ( j < ulLength ) && ( pcBase64name[ i ] ) && ( pcBase64name[ i ] != '=' ) )
    {
        /* Derive values of the Base64 name.  */
        if( ( pcBase64name[ i ] >= 'A' ) && ( pcBase64name[ i ] <= 'Z' ) )
        {
            ulValue1 = ( uint32_t ) ( pcBase64name[ i ] - 'A' );
        }
        else if( ( pcBase64name[ i ] >= 'a' ) && ( pcBase64name[ i ] <= 'z' ) )
        {
            ulValue1 = ( uint32_t ) ( pcBase64name[ i ] - 'a' ) + 26;
        }
        else if( ( pcBase64name[ i ] >= '0' ) && ( pcBase64name[ i ] <= '9' ) )
        {
            ulValue1 = ( uint32_t ) ( pcBase64name[ i ] - '0' ) + 52;
        }
        else if( pcBase64name[ i ] == '+' )
        {
            ulValue1 = 62;
        }
        else if( pcBase64name[ i ] == '/' )
        {
            ulValue1 = 63;
        }
        else
        {
            ulValue1 = 0;
        }

        /* Derive value for the next character.  */
        if( ( pcBase64name[ i + 1 ] >= 'A' ) && ( pcBase64name[ i + 1 ] <= 'Z' ) )
        {
            ulValue2 = ( uint32_t ) ( pcBase64name[ i + 1 ] - 'A' );
        }
        else if( ( pcBase64name[ i + 1 ] >= 'a' ) && ( pcBase64name[ i + 1 ] <= 'z' ) )
        {
            ulValue2 = ( uint32_t ) ( pcBase64name[ i + 1 ] - 'a' ) + 26;
        }
        else if( ( pcBase64name[ i + 1 ] >= '0' ) && ( pcBase64name[ i + 1 ] <= '9' ) )
        {
            ulValue2 = ( uint32_t ) ( pcBase64name[ i + 1 ] - '0' ) + 52;
        }
        else if( pcBase64name[ i + 1 ] == '+' )
        {
            ulValue2 = 62;
        }
        else if( pcBase64name[ i + 1 ] == '/' )
        {
            ulValue2 = 63;
        }
        else
        {
            ulValue2 = 0;
        }

        /* Determine which step we are in.  */
        if( ulStep == 0 )
        {
            /* Use first value and first 2 bits of second value.  */
            pucName[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 0x3f ) << 2 ) | ( ( ulValue2 >> 4 ) & 3 ) );
            i++;
            ulStep++;
        }
        else if( ulStep == 1 )
        {
            /* Use last 4 bits of first value and first 4 bits of next value.  */
            pucName[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 0xF ) << 4 ) | ( ulValue2 >> 2 ) );
            i++;
            ulStep++;
        }
        else if( ulStep == 2 )
        {
            /* Use first 2 bits and following 6 bits of next value.  */
            pucName[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 3 ) << 6 ) | ( ulValue2 & 0x3f ) );
            i++;
            i++;
            ulStep = 0;
        }
    }

    /* Put a NULL character in.  */
    pucName[ j ] = 0;
    *pulBytesCopied = j;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

/**
 *
 * Encode Base64 bytes.
 *
 * TODO: Remove in favor of embedded SDK implementation.
 *
 * */
static AzureIoTResult_t prvAzureIoTBase64Encode( uint8_t * pucName,
                                                 uint32_t ulLength,
                                                 char * pcBase64name,
                                                 uint32_t base64name_size,
                                                 uint32_t * pulOutputLength )
{
    uint32_t ulPad;
    uint32_t i, j;
    uint32_t ulStep;

    /* Adjust the length to represent the base64 name.  */
    ulLength = ( ( ulLength * 8 ) / 6 );

    /* Default padding to none.  */
    ulPad = 0;

    /* Determine if an extra conversion is needed.  */
    if( ( ulLength * 6 ) % 24 )
    {
        /* Some padding is needed.  */

        /* Calculate the number of pad characters.  */
        ulPad = ( ulLength * 6 ) % 24;
        ulPad = ( 24 - ulPad ) / 6;
        ulPad = ulPad - 1;

        /* Adjust the ulLength to pickup the character fraction.  */
        ulLength++;
    }

    if( base64name_size <= ulLength )
    {
        return eAzureIoTOutOfMemory;
    }

    /* Setup index into the pcBase64name.  */
    j = 0;

    /* Compute the pcBase64name.  */
    ulStep = 0;
    i = 0;

    while( j < ulLength )
    {
        /* Determine which step we are in.  */
        if( ulStep == 0 )
        {
            /* Use first 6 bits of name character for index.  */
            pcBase64name[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( uint8_t ) pucName[ i ] ) >> 2 ];
            ulStep++;
        }
        else if( ulStep == 1 )
        {
            /* Use last 2 bits of name character and first 4 bits of next name character for index.  */
            pcBase64name[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( ( uint8_t ) pucName[ i ] ) & 0x3 ) << 4 ) |
                                                                  ( ( ( uint8_t ) pucName[ i + 1 ] ) >> 4 ) ];
            i++;
            ulStep++;
        }
        else if( ulStep == 2 )
        {
            /* Use last 4 bits of name character and first 2 bits of next name character for index.  */
            pcBase64name[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( ( uint8_t ) pucName[ i ] ) & 0xF ) << 2 ) |
                                                                  ( ( ( uint8_t ) pucName[ i + 1 ] ) >> 6 ) ];
            i++;
            ulStep++;
        }
        else /* Step 3 */
        {
            /* Use last 6 bits of name character for index.  */
            pcBase64name[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( uint8_t ) pucName[ i ] ) & 0x3F ) ];
            i++;
            ulStep = 0;
        }
    }

    /* Determine if the index needs to be advanced.  */
    if( ulStep != 3 )
    {
        i++;
    }

    /* Now add the PAD characters.  */
    while( ( ulPad-- ) && ( j < base64name_size ) )
    {
        /* Pad pcBase64name with '=' characters.  */
        pcBase64name[ j++ ] = '=';
    }

    /* Put a NULL character in.  */
    pcBase64name[ j ] = 0;
    *pulOutputLength = j;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_Init()
{
    #if ( LIBRARY_LOG_LEVEL >= LOG_INFO )
        az_log_set_message_callback( prvAzureIoTLogListener );
    #endif

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

void AzureIoT_Deinit()
{
    AZLogDebug( ( "AzureIoT_Deinit called" ) );
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_MessagePropertiesInit( AzureIoTMessageProperties_t * pxMessageProperties,
                                                 uint8_t * pucBuffer,
                                                 uint32_t ulAlreadyWrittenLength,
                                                 uint32_t ulBufferLength )
{
    az_span xPropertyBufferSpan = az_span_create( pucBuffer, ( int32_t ) ulBufferLength );
    az_result xResult;

    if( pxMessageProperties == NULL )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesInit failed: Invalid argument" ) );
        return eAzureIoTInvalidArgument;
    }

    xResult = az_iot_message_properties_init( &pxMessageProperties->_internal.xProperties,
                                              xPropertyBufferSpan, ( int32_t ) ulAlreadyWrittenLength );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTFailed;
    }

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_MessagePropertiesAppend( AzureIoTMessageProperties_t * pxMessageProperties,
                                                   const uint8_t * pucName,
                                                   uint32_t ulNameLength,
                                                   const uint8_t * pucValue,
                                                   uint32_t ulValueLength )
{
    az_span xNameSpan = az_span_create( ( uint8_t * ) pucName, ( int32_t ) ulNameLength );
    az_span xValueSpan = az_span_create( ( uint8_t * ) pucValue, ( int32_t ) ulValueLength );
    az_result xResult;

    if( pxMessageProperties == NULL )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesAppend failed: Invalid argument" ) );
        return eAzureIoTInvalidArgument;
    }

    xResult = az_iot_message_properties_append( &pxMessageProperties->_internal.xProperties,
                                                xNameSpan, xValueSpan );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTFailed;
    }

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_MessagePropertiesFind( AzureIoTMessageProperties_t * pxMessageProperties,
                                                 const uint8_t * pucName,
                                                 uint32_t ulNameLength,
                                                 const uint8_t ** ppucOutValue,
                                                 uint32_t * pulOutValueLength )
{
    az_span xNameSpan = az_span_create( ( uint8_t * ) pucName, ( int32_t ) ulNameLength );
    az_span xOutValueSpan;
    az_result xResult;

    if( ( pxMessageProperties == NULL ) ||
        ( ppucOutValue == NULL ) || ( pulOutValueLength == NULL ) )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesFind failed: Invalid argument" ) );
        return eAzureIoTInvalidArgument;
    }

    xResult = az_iot_message_properties_find( &pxMessageProperties->_internal.xProperties,
                                              xNameSpan, &xOutValueSpan );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTItemNotFound;
    }

    *ppucOutValue = az_span_ptr( xOutValueSpan );
    *pulOutValueLength = ( uint32_t ) az_span_size( xOutValueSpan );

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_Base64HMACCalculate( AzureIoTGetHMACFunc_t xAzureIoTHMACFunction,
                                               const uint8_t * pucKey,
                                               uint32_t ulKeySize,
                                               const uint8_t * pucMessage,
                                               uint32_t ulMessageSize,
                                               uint8_t * pucBuffer,
                                               uint32_t ulBufferLength,
                                               uint8_t * pucOutput,
                                               uint32_t ulOutputSize,
                                               uint32_t * pulOutputLength )
{
    AzureIoTResult_t xStatus;
    uint8_t * pucHashBuf;
    uint32_t ulHashBufSize = azureiotBASE64_HASH_BUFFER_SIZE;
    uint32_t ulBinaryKeyBufSize;

    if( ( xAzureIoTHMACFunction == NULL ) ||
        ( pucKey == NULL ) || ( ulKeySize == 0 ) ||
        ( pucMessage == NULL ) || ( ulMessageSize == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( pucOutput == NULL ) || ( pulOutputLength == NULL ) )
    {
        AZLogError( ( "AzureIoT_Base64HMACCalculate failed: Invalid argument" ) );
        return eAzureIoTInvalidArgument;
    }

    ulBinaryKeyBufSize = ulBufferLength;
    xStatus = prvAzureIoTBase64Decode( ( char * ) pucKey, ulKeySize,
                                       pucBuffer, ulBinaryKeyBufSize, &ulBinaryKeyBufSize );

    if( xStatus )
    {
        return xStatus;
    }

    ulBufferLength -= ulBinaryKeyBufSize;

    if( ulHashBufSize > ulBufferLength )
    {
        return eAzureIoTOutOfMemory;
    }

    pucHashBuf = pucBuffer + ulBinaryKeyBufSize;
    memset( pucHashBuf, 0, ulHashBufSize );

    if( xAzureIoTHMACFunction( pucBuffer, ulBinaryKeyBufSize,
                               pucMessage, ( uint32_t ) ulMessageSize,
                               pucHashBuf, ulHashBufSize, &ulHashBufSize ) )
    {
        return eAzureIoTFailed;
    }

    uint32_t ulBase64OutputLength;
    xStatus = prvAzureIoTBase64Encode( pucHashBuf, ulHashBufSize,
                                       ( char * ) pucOutput, ulOutputSize, &ulBase64OutputLength );

    if( xStatus )
    {
        return xStatus;
    }

    *pulOutputLength = ( uint32_t ) ulBase64OutputLength;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/
