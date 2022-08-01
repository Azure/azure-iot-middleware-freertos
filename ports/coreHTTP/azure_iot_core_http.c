/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_http.h"

#include <string.h>

#include "azure_iot.h"

#include "core_http_client.h"
/* Kernel includes. */
#include "FreeRTOS.h"

static uint8_t pucHeaderBuffer[ azureiothttpHEADER_BUFFER_SIZE ];
static uint8_t pucResponseBuffer[ azureiothttpCHUNK_DOWNLOAD_BUFFER_SIZE ];

static AzureIoTHTTPResult_t prvTranslateToAzureIoTHTTPResult( HTTPStatus_t xResult )
{
    switch( xResult )
    {
        case HTTPSuccess:
            return eAzureIoTHTTPSuccess;

        case HTTPInvalidParameter:
            return eAzureIoTHTTPInvalidParameter;

        case HTTPNetworkError:
            return eAzureIoTHTTPNetworkError;

        case HTTPPartialResponse:
            return eAzureIoTHTTPPartialResponse;

        case HTTPNoResponse:
            return eAzureIoTHTTPNoResponse;

        case HTTPInsufficientMemory:
            return eAzureIoTHTTPInsufficientMemory;

        case HTTPSecurityAlertResponseHeadersSizeLimitExceeded:
            return eAzureIoTHTTPSecurityAlertResponseHeadersSizeLimitExceeded;

        case HTTPSecurityAlertExtraneousResponseData:
            return eAzureIoTHTTPSecurityAlertExtraneousResponseData;

        case HTTPSecurityAlertInvalidChunkHeader:
            return eAzureIoTHTTPSecurityAlertInvalidChunkHeader;

        case HTTPSecurityAlertInvalidProtocolVersion:
            return eAzureIoTHTTPSecurityAlertInvalidProtocolVersion;

        case HTTPSecurityAlertInvalidStatusCode:
            return eAzureIoTHTTPSecurityAlertInvalidStatusCode;

        case HTTPSecurityAlertInvalidCharacter:
            return eAzureIoTHTTPSecurityAlertInvalidCharacter;

        case HTTPSecurityAlertInvalidContentLength:
            return eAzureIoTHTTPSecurityAlertInvalidContentLength;

        case HTTPParserInternalError:
            return eAzureIoTHTTPParserInternalError;

        case HTTPHeaderNotFound:
            return eAzureIoTHTTPHeaderNotFound;

        case HTTPInvalidResponse:
            return eAzureIoTHTTPInvalidResponse;

        default:
            return eAzureIoTHTTPError;
    }

    return eAzureIoTHTTPError;
}


AzureIoTHTTPResult_t AzureIoTHTTP_Init( AzureIoTHTTPHandle_t xHTTPHandle,
                                        AzureIoTTransportInterface_t * pxHTTPTransport,
                                        const char * pucURL,
                                        uint32_t ulURLLength,
                                        const char * pucPath,
                                        uint32_t ulPathLength )
{
    HTTPStatus_t xHttpLibraryStatus = HTTPSuccess;

    if( xHTTPHandle == NULL )
    {
        return 1;
    }

    ( void ) memset( &xHTTPHandle->xRequestInfo, 0, sizeof( xHTTPHandle->xRequestInfo ) );
    ( void ) memset( &xHTTPHandle->xRequestHeaders, 0, sizeof( xHTTPHandle->xRequestHeaders ) );
    ( void ) memset( &xHTTPHandle->xResponse, 0, sizeof( xHTTPHandle->xResponse ) );

    xHTTPHandle->xRequestHeaders.pBuffer = pucHeaderBuffer;
    xHTTPHandle->xRequestHeaders.bufferLen = sizeof( pucHeaderBuffer );

    xHTTPHandle->xRequestInfo.pHost = pucURL;
    xHTTPHandle->xRequestInfo.hostLen = ulURLLength;
    xHTTPHandle->xRequestInfo.pPath = pucPath;
    xHTTPHandle->xRequestInfo.pathLen = ulPathLength;
    xHTTPHandle->xRequestInfo.pMethod = "GET";
    xHTTPHandle->xRequestInfo.methodLen = strlen( "GET" );
    xHTTPHandle->xRequestInfo.reqFlags |= HTTP_REQUEST_KEEP_ALIVE_FLAG;

    xHTTPHandle->pxHTTPTransport = pxHTTPTransport;

    HTTPClient_InitializeRequestHeaders( &xHTTPHandle->xRequestHeaders, &xHTTPHandle->xRequestInfo );

    return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
}

AzureIoTHTTPResult_t AzureIoTHTTP_Request( AzureIoTHTTPHandle_t xHTTPHandle,
                                           int32_t lRangeStart,
                                           int32_t lRangeEnd,
                                           char ** ppucDataBuffer,
                                           uint32_t * pulDataLength )
{
    HTTPStatus_t xHttpLibraryStatus = HTTPSuccess;

    xHTTPHandle->xResponse.pBuffer = pucResponseBuffer;
    xHTTPHandle->xResponse.bufferLen = sizeof( pucResponseBuffer );

    if( !( ( lRangeStart == 0 ) && ( lRangeEnd == azureiothttpHttpRangeRequestEndOfFile ) ) )
    {
        /* Add range headers if not the whole image. */

        xHttpLibraryStatus = HTTPClient_AddRangeHeader( &xHTTPHandle->xRequestHeaders, lRangeStart, lRangeEnd );

        if( xHttpLibraryStatus != HTTPSuccess )
        {
            return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
        }
    }

    xHttpLibraryStatus = HTTPClient_Send( ( TransportInterface_t * ) xHTTPHandle->pxHTTPTransport, &xHTTPHandle->xRequestHeaders, NULL, 0, &xHTTPHandle->xResponse, 0 );

    if( xHttpLibraryStatus != HTTPSuccess )
    {
        AZLogError( ( "[HTTP] ERROR: %d", xHttpLibraryStatus ) );
        return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
    }

    if( xHttpLibraryStatus == HTTPSuccess )
    {
        if( xHTTPHandle->xResponse.statusCode == 200 )
        {
            /* Handle a response Status-Code of 200 OK. */
            AZLogInfo( ( "[HTTP] Success 200" ) );
        }
        else if( xHTTPHandle->xResponse.statusCode == 206 )
        {
            /* Handle a response Status-Code of 200 OK. */
            AZLogInfo( ( "[HTTP] [Status 206] Received range %i to %i", ( int ) lRangeStart, ( int ) lRangeEnd ) );

            *ppucDataBuffer = ( char * ) xHTTPHandle->xResponse.pBody;
            *pulDataLength = ( uint32_t ) xHTTPHandle->xResponse.bodyLen;
        }
        else
        {
            /* Handle an error */
            AZLogError( ( "[HTTP] Failed %d.", xHTTPHandle->xResponse.statusCode ) );
            xHttpLibraryStatus = 1;
        }
    }

    return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
}

AzureIoTHTTPResult_t AzureIoTHTTP_RequestSizeInit( AzureIoTHTTPHandle_t xHTTPHandle,
                                                   AzureIoTTransportInterface_t * pxHTTPTransport,
                                                   const char * pucURL,
                                                   uint32_t ulURLLength,
                                                   const char * pucPath,
                                                   uint32_t ulPathLength )
{
    HTTPStatus_t xHttpLibraryStatus = HTTPSuccess;

    if( xHTTPHandle == NULL )
    {
        return 1;
    }

    ( void ) memset( &xHTTPHandle->xRequestInfo, 0, sizeof( xHTTPHandle->xRequestInfo ) );
    ( void ) memset( &xHTTPHandle->xRequestHeaders, 0, sizeof( xHTTPHandle->xRequestHeaders ) );
    ( void ) memset( &xHTTPHandle->xResponse, 0, sizeof( xHTTPHandle->xResponse ) );

    xHTTPHandle->xRequestHeaders.pBuffer = pucHeaderBuffer;
    xHTTPHandle->xRequestHeaders.bufferLen = sizeof( pucHeaderBuffer );

    xHTTPHandle->xRequestInfo.pHost = pucURL;
    xHTTPHandle->xRequestInfo.hostLen = ulURLLength;
    xHTTPHandle->xRequestInfo.pPath = pucPath;
    xHTTPHandle->xRequestInfo.pathLen = ulPathLength;
    xHTTPHandle->xRequestInfo.pMethod = "HEAD";
    xHTTPHandle->xRequestInfo.methodLen = strlen( "HEAD" );
    xHTTPHandle->xRequestInfo.reqFlags |= HTTP_REQUEST_KEEP_ALIVE_FLAG;

    xHTTPHandle->pxHTTPTransport = pxHTTPTransport;

    HTTPClient_InitializeRequestHeaders( &xHTTPHandle->xRequestHeaders, &xHTTPHandle->xRequestInfo );

    return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
}

int32_t AzureIoTHTTP_RequestSize( AzureIoTHTTPHandle_t xHTTPHandle )
{
    HTTPStatus_t xHttpLibraryStatus = HTTPSuccess;

    xHTTPHandle->xResponse.pBuffer = pucResponseBuffer;
    xHTTPHandle->xResponse.bufferLen = sizeof( pucResponseBuffer );

    xHttpLibraryStatus = HTTPClient_Send( ( TransportInterface_t * ) xHTTPHandle->pxHTTPTransport, &xHTTPHandle->xRequestHeaders, NULL, 0, &xHTTPHandle->xResponse, 0 );

    if( xHttpLibraryStatus != HTTPSuccess )
    {
        return -1;
    }

    if( xHttpLibraryStatus == HTTPSuccess )
    {
        if( xHTTPHandle->xResponse.statusCode == 200 )
        {
            /* Handle a response Status-Code of 200 OK. */
            AZLogDebug( ( "[HTTP] Size Request Success 200" ) );
            return ( int32_t ) xHTTPHandle->xResponse.contentLength;
        }
        else
        {
            /* Handle an error */
            AZLogError( ( "[HTTP] Size Request Failed %d.", xHTTPHandle->xResponse.statusCode ) );
            return -1;
        }
    }

    return -1;
}

AzureIoTHTTPResult_t AzureIoTHTTP_Deinit( AzureIoTHTTPHandle_t xHTTPHandle )
{
    ( void ) xHTTPHandle;

    HTTPStatus_t xHttpLibraryStatus = HTTPSuccess;

    return prvTranslateToAzureIoTHTTPResult( xHttpLibraryStatus );
}
