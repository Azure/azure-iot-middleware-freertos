/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_http.h
 *
 * @brief The port file for http APIs.
 *
 * Used in ADU.
 *
 */
#ifndef AZURE_IOT_HTTP_H
#define AZURE_IOT_HTTP_H

#include <stdint.h>

#include "azure_iot_http_port.h"
#include "azure_iot_transport_interface.h"

#define azureiothttpHEADER_BUFFER_SIZE            512
#define azureiothttpCHUNK_DOWNLOAD_SIZE           4096
#define azureiothttpCHUNK_DOWNLOAD_BUFFER_SIZE    ( azureiothttpCHUNK_DOWNLOAD_SIZE + 1024 )
#define azureiothttpHttpRangeRequestEndOfFile     -1

typedef AzureIoTHTTP_t * AzureIoTHTTPHandle_t;

typedef enum AzureIoTHTTPResult
{
    eAzureIoTHTTPSuccess = 0,
    eAzureIoTHTTPInvalidParameter,
    eAzureIoTHTTPNetworkError,
    eAzureIoTHTTPPartialResponse,
    eAzureIoTHTTPNoResponse,
    eAzureIoTHTTPInsufficientMemory,
    eAzureIoTHTTPSecurityAlertResponseHeadersSizeLimitExceeded,
    eAzureIoTHTTPSecurityAlertExtraneousResponseData,
    eAzureIoTHTTPSecurityAlertInvalidChunkHeader,
    eAzureIoTHTTPSecurityAlertInvalidProtocolVersion,
    eAzureIoTHTTPSecurityAlertInvalidStatusCode,
    eAzureIoTHTTPSecurityAlertInvalidCharacter,
    eAzureIoTHTTPSecurityAlertInvalidContentLength,
    eAzureIoTHTTPParserInternalError,
    eAzureIoTHTTPHeaderNotFound,
    eAzureIoTHTTPInvalidResponse,
    eAzureIoTHTTPError
} AzureIoTHTTPResult_t;

AzureIoTHTTPResult_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                                        AzureIoTTransportInterface_t * pxHTTPTransport,
                                        const char * pucURL,
                                        uint32_t ulURLLength,
                                        const char * pucPath,
                                        uint32_t ulPathLength );

AzureIoTHTTPResult_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle,
                                           int32_t lRangeStart,
                                           int32_t lRangeEnd,
                                           char ** ppucDataBuffer,
                                           uint32_t * pulDataLength );

AzureIoTHTTPResult_t AzureIoTHTTP_RequestSizeInit( AzureIoTHTTPHandle_t xHTTPHandle,
                                                   AzureIoTTransportInterface_t * pxHTTPTransport,
                                                   const char * pucURL,
                                                   uint32_t ulURLLength,
                                                   const char * pucPath,
                                                   uint32_t ulPathLength );

int32_t AzureIoTHTTP_RequestSize( AzureIoTHTTPHandle_t xHTTPHandle );

AzureIoTHTTPResult_t AzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle );

#endif /* AZURE_IOT_HTTP_H */
