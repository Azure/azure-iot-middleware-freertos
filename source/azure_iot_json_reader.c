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
                                                   uint16_t usBufferLen )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_Deinit( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_NextToken( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}

AzureIoTHubClientResult_t AzureIoTJSONReader_SkipChildren( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenBool( AzureIoTJSONReader_t * pxReader,
                                                           bool * pxValue )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenInt32( AzureIoTJSONReader_t * pxReader,
                                                            int32_t * pilValue )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenDouble( AzureIoTJSONReader_t * pxReader,
                                                             double * pxValue )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_GetTokenString( AzureIoTJSONReader_t * pxReader,
                                                             uint8_t * pucBuffer,
                                                             uint16_t usBufferSize,
                                                             uint16_t * pusBytesCopied )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}


AzureIoTHubClientResult_t AzureIoTJSONReader_TokenIsTextEqual( AzureIoTJSONReader_t * pxReader,
                                                               uint8_t * pucExpectedText,
                                                               uint16_t usExpectedTextLength )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}

AzureIoTHubClientResult_t AzureIoTJSONReader_TokenType( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTHubClientResult_t xResult;

    return result;
}
