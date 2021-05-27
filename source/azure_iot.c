/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot.c
 *
 * @brief Common Azure IoT code for FreeRTOS middleware.
 *
 */

#include <string.h>

#include "azure_iot.h"

#include "azure_iot_result.h"

/* Using SHA256 hash - needs 32 bytes */
#define azureiotBASE64_HASH_BUFFER_SIZE    ( 33 )

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
 * Note: pucDecodedBytes buffer needs enough space for a NULL terminator.
 *
 * TODO: Remove in favor of embedded SDK implementation.
 *
 * */
static AzureIoTResult_t prvAzureIoTBase64Decode( const char * pcEncodedBytes,
                                                 uint32_t ulEncodedBytesLength,
                                                 uint8_t * pucDecodedBytes,
                                                 uint32_t ulDecodedBytesLength,
                                                 uint32_t * pulOutDecodedBytesLength )
{
    uint32_t i, j;
    uint32_t ulValue1, ulValue2;
    uint32_t ulStep;
    uint32_t ulSourceLength = ulEncodedBytesLength;

    /* Adjust the ulEncodedBytesLength to represent the ASCII name.  */
    ulEncodedBytesLength = ( ( ulEncodedBytesLength * 6 ) / 8 );

    if( pcEncodedBytes[ ulSourceLength - 1 ] == '=' )
    {
        if( pcEncodedBytes[ ulSourceLength - 2 ] == '=' )
        {
            ulEncodedBytesLength--;
        }

        ulEncodedBytesLength--;
    }

    if( ulDecodedBytesLength < ulEncodedBytesLength )
    {
        return eAzureIoTErrorOutOfMemory;
    }

    /* Setup index into the ASCII name.  */
    j = 0;

    /* Compute the ASCII name.  */
    ulStep = 0;
    i = 0;

    while( ( j < ulEncodedBytesLength ) && ( pcEncodedBytes[ i ] ) && ( pcEncodedBytes[ i ] != '=' ) )
    {
        /* Derive values of the Base64 name.  */
        if( ( pcEncodedBytes[ i ] >= 'A' ) && ( pcEncodedBytes[ i ] <= 'Z' ) )
        {
            ulValue1 = ( uint32_t ) ( pcEncodedBytes[ i ] - 'A' );
        }
        else if( ( pcEncodedBytes[ i ] >= 'a' ) && ( pcEncodedBytes[ i ] <= 'z' ) )
        {
            ulValue1 = ( uint32_t ) ( pcEncodedBytes[ i ] - 'a' ) + 26;
        }
        else if( ( pcEncodedBytes[ i ] >= '0' ) && ( pcEncodedBytes[ i ] <= '9' ) )
        {
            ulValue1 = ( uint32_t ) ( pcEncodedBytes[ i ] - '0' ) + 52;
        }
        else if( pcEncodedBytes[ i ] == '+' )
        {
            ulValue1 = 62;
        }
        else if( pcEncodedBytes[ i ] == '/' )
        {
            ulValue1 = 63;
        }
        else
        {
            ulValue1 = 0;
        }

        /* Derive value for the next character.  */
        if( ( pcEncodedBytes[ i + 1 ] >= 'A' ) && ( pcEncodedBytes[ i + 1 ] <= 'Z' ) )
        {
            ulValue2 = ( uint32_t ) ( pcEncodedBytes[ i + 1 ] - 'A' );
        }
        else if( ( pcEncodedBytes[ i + 1 ] >= 'a' ) && ( pcEncodedBytes[ i + 1 ] <= 'z' ) )
        {
            ulValue2 = ( uint32_t ) ( pcEncodedBytes[ i + 1 ] - 'a' ) + 26;
        }
        else if( ( pcEncodedBytes[ i + 1 ] >= '0' ) && ( pcEncodedBytes[ i + 1 ] <= '9' ) )
        {
            ulValue2 = ( uint32_t ) ( pcEncodedBytes[ i + 1 ] - '0' ) + 52;
        }
        else if( pcEncodedBytes[ i + 1 ] == '+' )
        {
            ulValue2 = 62;
        }
        else if( pcEncodedBytes[ i + 1 ] == '/' )
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
            pucDecodedBytes[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 0x3f ) << 2 ) | ( ( ulValue2 >> 4 ) & 3 ) );
            i++;
            ulStep++;
        }
        else if( ulStep == 1 )
        {
            /* Use last 4 bits of first value and first 4 bits of next value.  */
            pucDecodedBytes[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 0xF ) << 4 ) | ( ulValue2 >> 2 ) );
            i++;
            ulStep++;
        }
        else if( ulStep == 2 )
        {
            /* Use first 2 bits and following 6 bits of next value.  */
            pucDecodedBytes[ j++ ] = ( uint8_t ) ( ( ( ulValue1 & 3 ) << 6 ) | ( ulValue2 & 0x3f ) );
            i++;
            i++;
            ulStep = 0;
        }
    }

    /* Need space for a NULL terminator */
    if( j >= ulDecodedBytesLength )
    {
        return eAzureIoTErrorOutOfMemory;
    }

    /* Put a NULL character in.  */
    pucDecodedBytes[ j ] = 0;
    *pulOutDecodedBytesLength = j;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

/**
 *
 * Encode Base64 bytes.
 *
 * Note: pcEncodedBytes buffer needs enough space for a NULL terminator.
 *
 * TODO: Remove in favor of embedded SDK implementation.
 *
 * */
static AzureIoTResult_t prvAzureIoTBase64Encode( uint8_t * pucBytes,
                                                 uint32_t ulBytesLength,
                                                 char * pcEncodedBytes,
                                                 uint32_t ulEncodedBytesLength,
                                                 uint32_t * pulOutEncodedBytesLength )
{
    uint32_t ulPad;
    uint32_t i, j;
    uint32_t ulStep;

    /* Adjust the length to represent the base64 name.  */
    ulBytesLength = ( ( ulBytesLength * 8 ) / 6 );

    /* Default padding to none.  */
    ulPad = 0;

    /* Determine if an extra conversion is needed.  */
    if( ( ulBytesLength * 6 ) % 24 )
    {
        /* Some padding is needed.  */

        /* Calculate the number of pad characters.  */
        ulPad = ( ulBytesLength * 6 ) % 24;
        ulPad = ( 24 - ulPad ) / 6;
        ulPad = ulPad - 1;

        /* Adjust the ulBytesLength to pickup the character fraction.  */
        ulBytesLength++;
    }

    if( ulEncodedBytesLength <= ulBytesLength )
    {
        return eAzureIoTErrorOutOfMemory;
    }

    /* Setup index into the pcEncodedBytes.  */
    j = 0;

    /* Compute the pcEncodedBytes.  */
    ulStep = 0;
    i = 0;

    while( j < ulBytesLength )
    {
        /* Determine which step we are in.  */
        if( ulStep == 0 )
        {
            /* Use first 6 bits of encoded bytes character for index.  */
            pcEncodedBytes[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( uint8_t ) pucBytes[ i ] ) >> 2 ];
            ulStep++;
        }
        else if( ulStep == 1 )
        {
            /* Use last 2 bits of encoded bytes character and first 4 bits of next encoded bytes character for index.  */
            pcEncodedBytes[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( ( uint8_t ) pucBytes[ i ] ) & 0x3 ) << 4 ) |
                                                                    ( ( ( uint8_t ) pucBytes[ i + 1 ] ) >> 4 ) ];
            i++;
            ulStep++;
        }
        else if( ulStep == 2 )
        {
            /* Use last 4 bits of encoded bytes character and first 2 bits of next encoded bytes character for index.  */
            pcEncodedBytes[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( ( uint8_t ) pucBytes[ i ] ) & 0xF ) << 2 ) |
                                                                    ( ( ( uint8_t ) pucBytes[ i + 1 ] ) >> 6 ) ];
            i++;
            ulStep++;
        }
        else /* Step 3 */
        {
            /* Use last 6 bits of encoded bytes character for index.  */
            pcEncodedBytes[ j++ ] = ( char ) _cAzureIoTBase64Array[ ( ( ( uint8_t ) pucBytes[ i ] ) & 0x3F ) ];
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
    while( ( ulPad-- ) && ( j < ulEncodedBytesLength ) )
    {
        /* Pad pcEncodedBytes with '=' characters.  */
        pcEncodedBytes[ j++ ] = '=';
    }

    /* Need space for a NULL terminator */
    if( j >= ulEncodedBytesLength )
    {
        return eAzureIoTErrorOutOfMemory;
    }

    /* Put a NULL character in.  */
    pcEncodedBytes[ j ] = 0;
    *pulOutEncodedBytesLength = j;

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
}
/*-----------------------------------------------------------*/

AzureIoTResult_t AzureIoT_MessagePropertiesInit( AzureIoTMessageProperties_t * pxMessageProperties,
                                                 uint8_t * pucBuffer,
                                                 uint32_t ulAlreadyWrittenLength,
                                                 uint32_t ulBufferLength )
{
    az_span xPropertyBufferSpan = az_span_create( pucBuffer, ( int32_t ) ulBufferLength );
    az_result xResult;

    if( ( pxMessageProperties == NULL ) ||
        ( pucBuffer == NULL ) )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesInit failed: Invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xResult = az_iot_message_properties_init( &pxMessageProperties->_internal.xProperties,
                                              xPropertyBufferSpan, ( int32_t ) ulAlreadyWrittenLength );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTErrorFailed;
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

    if( ( pxMessageProperties == NULL ) ||
        ( pucName == NULL ) || ( ulNameLength == 0 ) ||
        ( pucValue == NULL ) || ( ulValueLength == 0 ) )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesAppend failed: Invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xResult = az_iot_message_properties_append( &pxMessageProperties->_internal.xProperties,
                                                xNameSpan, xValueSpan );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTErrorFailed;
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
        ( pucName == NULL ) || ( ulNameLength == 0 ) ||
        ( ppucOutValue == NULL ) || ( pulOutValueLength == NULL ) )
    {
        AZLogError( ( "AzureIoT_MessagePropertiesFind failed: Invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xResult = az_iot_message_properties_find( &pxMessageProperties->_internal.xProperties,
                                              xNameSpan, &xOutValueSpan );

    if( az_result_failed( xResult ) )
    {
        return eAzureIoTErrorItemNotFound;
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
    uint8_t * pucDecodedKeyBuf = pucBuffer;
    uint32_t ulHashBufSize = azureiotBASE64_HASH_BUFFER_SIZE;
    uint32_t ulBinaryKeyBufSize;
    uint32_t ulBase64OutputLength;

    if( ( xAzureIoTHMACFunction == NULL ) ||
        ( pucKey == NULL ) || ( ulKeySize == 0 ) ||
        ( pucMessage == NULL ) || ( ulMessageSize == 0 ) ||
        ( pucBuffer == NULL ) || ( ulBufferLength == 0 ) ||
        ( pucOutput == NULL ) || ( pulOutputLength == NULL ) )
    {
        AZLogError( ( "AzureIoT_Base64HMACCalculate failed: Invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xStatus = prvAzureIoTBase64Decode( ( char * ) pucKey, ulKeySize,
                                       pucDecodedKeyBuf, ulBufferLength, &ulBinaryKeyBufSize );

    if( xStatus )
    {
        return xStatus;
    }

    /* Decoded key is less than total decoded buffer size */
    ulBufferLength -= ulBinaryKeyBufSize;

    if( ulHashBufSize > ulBufferLength )
    {
        return eAzureIoTErrorOutOfMemory;
    }

    pucHashBuf = pucDecodedKeyBuf + ulBinaryKeyBufSize;
    memset( pucHashBuf, 0, ulHashBufSize );

    if( xAzureIoTHMACFunction( pucDecodedKeyBuf, ulBinaryKeyBufSize,
                               pucMessage, ( uint32_t ) ulMessageSize,
                               pucHashBuf, ulHashBufSize, &ulHashBufSize ) )
    {
        return eAzureIoTErrorFailed;
    }

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
