/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_writer.c
 * @brief Implementation of the Azure IoT JSON writer.
 */

#include "azure_iot_json_writer.h"

/* TODO: remove this dep */
#include "azure_iot_hub_client.h"

AzureIoTHubClientResult_t AzureIoTJSONWriter_Init( AzureIoTJSONWriter_t * pxWriter,
                                                   uint8_t * pucBuffer,
                                                   uint32_t usBufferLen )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucBuffer == NULL ) || ( usBufferLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_Init failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithInt32Value( AzureIoTJSONWriter_t * pxWriter,
                                                                           const uint8_t * pucPropertyName,
                                                                           uint16_t usPropertyNameLength,
                                                                           int32_t ilValue )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucPropertyName == NULL ) || ( usPropertyNameLength == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendPropertyWithInt32Value failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithDoubleValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * pucPropertyName,
                                                                            uint16_t usPropertyNameLength,
                                                                            double xValue,
                                                                            uint16_t usFractionalDigits )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucPropertyName == NULL ) || ( usPropertyNameLength == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendPropertyWithDoubleValue failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithBoolValue( AzureIoTJSONWriter_t * pxWriter,
                                                                          const uint8_t * pucPropertyName,
                                                                          uint16_t usPropertyNameLength,
                                                                          bool usValue )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucPropertyName == NULL ) || ( usPropertyNameLength == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendPropertyWithBoolValue failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithStringValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * pucPropertyName,
                                                                            uint32_t usPropertyNameLength,
                                                                            const uint8_t * pucValue,
                                                                            uint32_t ulValueLen )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucPropertyName == NULL ) || ( usPropertyNameLength == 0 ) ||
        ( pucValue == NULL ) || ( ulValueLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendPropertyWithStringValue failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_GetBytesUsed( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_GetBytesUsed failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendString( AzureIoTJSONWriter_t * pxWriter,
                                                           const uint8_t * pucValue,
                                                           uint32_t ulValueLen )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucValue == NULL ) || ( ulValueLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendString failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendJsonText( AzureIoTJSONWriter_t * pxWriter,
                                                             const uint8_t * pucJSON,
                                                             uint32_t ulJSONLen )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pucJSON == NULL ) || ( ulJSONLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendJsonText failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyName( AzureIoTJSONWriter_t * pxWriter,
                                                                 const uint8_t * pusValue,
                                                                 uint32_t ulValueLen )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) || ( pusValue == NULL ) || ( ulValueLen == 0 ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendPropertyName failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBool( AzureIoTJSONWriter_t * pxWriter,
                                                         bool xValue )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendBool failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendInt32( AzureIoTJSONWriter_t * pxWriter,
                                                          int32_t ilValue )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendInt32 failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendDouble( AzureIoTJSONWriter_t * pxWriter,
                                                           double xValue,
                                                           uint16_t usFractionalDigits )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendDouble failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendNull( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendNull failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginObject( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendBeginObject failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginArray( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendBeginArray failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndObject( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendEndObject failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndArray( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    if( ( pxWriter == NULL ) )
    {
        AZLogError( ( "AzureIoTJSONWriter_AppendEndArray failed: invalid argument" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    return xResult;
}
