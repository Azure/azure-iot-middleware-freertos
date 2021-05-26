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
                                                   uint16_t usBufferLen )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_Deinit( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithInt32Value( AzureIoTJSONWriter_t * pxWriter,
                                                                           const uint8_t * pucPropertyName,
                                                                           uint16_t usPropertyName,
                                                                           int32_t ilValue )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithDoubleValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * pucPropertyName,
                                                                            uint16_t usPropertyName,
                                                                            double xValue,
                                                                            uint16_t usFractionalDigits )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithBoolValue( AzureIoTJSONWriter_t * pxWriter,
                                                                          const uint8_t * pucPropertyName,
                                                                          uint16_t usPropertyNameLength,
                                                                          bool usValue )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithStringValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * pucPropertyName,
                                                                            uint16_t usPropertyName,
                                                                            const uint8_t * pucValue,
                                                                            uint16_t usValueLen )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_GetBytesUsed( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendString( AzureIoTJSONWriter_t * pxWriter,
                                                           const uint8_t * pucValue,
                                                           uint16_t usValueLen )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendJsonText( AzureIoTJSONWriter_t * pxWriter,
                                                             const uint8_t * pucJSON,
                                                             uint16_t usJSONLen )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyName( AzureIoTJSONWriter_t * pxWriter,
                                                                 const uint8_t * pusValue,
                                                                 uint16_t usValueLen )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBool( AzureIoTJSONWriter_t * pxWriter,
                                                         bool xValue )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendInt32( AzureIoTJSONWriter_t * pxWriter,
                                                          int32_t ilValue )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendDouble( AzureIoTJSONWriter_t * pxWriter,
                                                           double xValue,
                                                           uint16_t usFractionalDigits )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendNull( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginObject( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginArray( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndObject( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndArray( AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTHubClientResult_t xResult;

    return xResult;
}
