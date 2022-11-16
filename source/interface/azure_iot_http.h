/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_http.h
 *
 * @brief The port file for HTTP APIs.
 *
 * Used in ADU.
 *
 */
#ifndef AZURE_IOT_HTTP_H
#define AZURE_IOT_HTTP_H

#include <stdint.h>

#include "azure_iot_http_port.h"
#include "azure_iot_transport_interface.h"

/**
 * @brief Value to request the end of the file.
 *
 */
#define azureiothttpHttpRangeRequestEndOfFile    -1

/**
 * @brief The handle for the Azure HTTP client.
 *
 */
typedef AzureIoTHTTP_t * AzureIoTHTTPHandle_t;

/**
 * @brief Azure HTTP return codes.
 *
 */
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

/**
 * @brief Initialize the Azure HTTP client.
 *
 * @param[in] xHTTPHandle The HTTP handle to use for this operation.
 * @param[in] pxHTTPTransport The Azure IoT Transport interface to use for this operation.
 * @param[in] pucURL The URL to use for this request.
 * @param[in] ulURLLength The length \p pucURL.
 * @param[in] pucPath The path to use for this request.
 * @param[in] ulPathLength The length \p pucPath.
 * @param[out] pucHeaderBuffer The buffer into which the headers for the request will be placed.
 * @param[in] ulHeaderBufferLength The length of \p pucHeaderBuffer.
 * @return AzureIoTHTTPResult_t
 */
AzureIoTHTTPResult_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                                        AzureIoTTransportInterface_t * pxHTTPTransport,
                                        const char * pucURL,
                                        uint32_t ulURLLength,
                                        const char * pucPath,
                                        uint32_t ulPathLength,
                                        char * pucHeaderBuffer,
                                        uint32_t ulHeaderBufferLength );

/**
 * @brief Send an HTTP GET request.
 *
 * @param[in] xHTTPHandle The HTTP handle to use for this operation.
 * @param[in] lRangeStart The start point for the request payload.
 * @param[in] lRangeEnd The end point for the request payload.
 * @param[out] pucDataBuffer The buffer into which the response header and payload will be placed.
 * @param[in] ulDataBufferLength The length of \p pucDataBuffer.
 * @param[out] ppucOutData The pointer to the point in the buffer where the payload starts.
 * @param[out] pulOutDataLength The length of the payload returned by \p ppucOutData.
 * @return AzureIoTHTTPResult_t
 */
AzureIoTHTTPResult_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle,
                                           int32_t lRangeStart,
                                           int32_t lRangeEnd,
                                           char * pucDataBuffer,
                                           uint32_t ulDataBufferLength,
                                           char ** ppucOutData,
                                           uint32_t * pulOutDataLength );

/**
 * @brief Initialize a size request.
 *
 * @param[in] xHTTPHandle The HTTP handle to use for this operation.
 * @param[in] pxHTTPTransport The Azure IoT Transport interface to use for this operation.
 * @param[in] pucURL The URL to use for this request.
 * @param[in] ulURLLength The length \p pucURL.
 * @param[in] pucPath The path to use for this request.
 * @param[in] ulPathLength The length \p pucPath.
 * @param[out] pucHeaderBuffer The buffer into which the response will be placed.
 * @param[in] ulHeaderBufferLength The size of \p pucHeaderBuffer.
 * @return AzureIoTHTTPResult_t
 */
AzureIoTHTTPResult_t AzureIoTHTTP_RequestSizeInit( AzureIoTHTTPHandle_t xHTTPHandle,
                                                   AzureIoTTransportInterface_t * pxHTTPTransport,
                                                   const char * pucURL,
                                                   uint32_t ulURLLength,
                                                   const char * pucPath,
                                                   uint32_t ulPathLength,
                                                   char * pucHeaderBuffer,
                                                   uint32_t ulHeaderBufferLength );

/**
 * @brief Send a size request.
 *
 * @param[in] xHTTPHandle The HTTP handle to use for this operation.
 * @param[out] pucDataBuffer The buffer where the response will be placed.
 * @param[in] ulDataBufferLength The size of \p pucDataBuffer.
 * @return int32_t
 * @retval The size of the file if success.
 * @retval -1 if failure.
 */
int32_t AzureIoTHTTP_RequestSize( AzureIoTHTTPHandle_t xHTTPHandle,
                                  char * pucDataBuffer,
                                  uint32_t ulDataBufferLength );

/**
 * @brief Deinitialize the Azure HTTP client.
 *
 * @param[in] xHTTPHandle The HTTP handle to use for this operation.
 * @return AzureIoTHTTPResult_t
 * @retval eAzureIoTHTTPSuccess if success.
 * @retval Otherwise if failure.
 */
AzureIoTHTTPResult_t AzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle );

#endif /* AZURE_IOT_HTTP_H */
