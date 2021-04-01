/*
 * FreeRTOS V202011.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file sockets_wrapper.c
 * @brief ST socket wrapper.
 */

/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

#include "semphr.h"
#include "sockets_wrapper.h"

/* Wifi module */
#include "es_wifi.h"
#include "wifi.h"

/*-----------------------------------------------------------*/

/**
 * @brief A Flag to indicate whether or not a socket is
 * secure i.e. it uses TLS or not.
 */
#define stsecuresocketsSOCKET_SECURE_FLAG          ( 1UL << 0 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for receive.
 */
#define stsecuresocketsSOCKET_READ_CLOSED_FLAG     ( 1UL << 1 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for send.
 */
#define stsecuresocketsSOCKET_WRITE_CLOSED_FLAG    ( 1UL << 2 )

/**
 * @brief A flag to indicate whether or not the socket is connected.
 */
#define stsecuresocketsSOCKET_IS_CONNECTED_FLAG    ( 1UL << 3 )

/**
 * @brief The maximum timeout accepted by the Inventek module.
 *
 * This value is dictated by the hardware and should not be
 * modified.
 */
#define stsecuresocketsMAX_TIMEOUT                 ( 30000 )

/**
 * @brief Delay used between network read attempts when effecting a receive timeout.
 *
 * If receive timeouts are implemented by the Inventek module then
 * the SPI driver will poll for extended periods, preventing lower
 * priority tasks from executing.  Therefore timeouts are mocked in
 * the secure sockets layer, and this constant sets the sleep time
 * between each read attempt during the receive timeout period.
 */
#define stsecuresocketsFIVE_MILLISECONDS           ( pdMS_TO_TICKS( 5 ) )

/**
 * @brief The timeout supplied to the Inventek module in receive operation.
 *
 * Receive timeout are emulated in secure sockets layer and therefore we
 * do not want the Inventek module to block. Setting to zero means
 * no timeout, so one is the smallest value we can set it to.
 */
#define stsecuresocketsONE_MILLISECOND             ( 1 )

/**
 * @brief Maximum number of sockets that can be created simultaneously.
 */
#define wificonfigMAX_SOCKETS                 ( 4 )

/**
 * @brief Default socket send timeout.
 */
#define socketsconfigDEFAULT_SEND_TIMEOUT    ( 10000 )

/**
 * @brief Default socket receive timeout.
 */
#define socketsconfigDEFAULT_RECV_TIMEOUT    ( 10000 )

typedef struct STSecureSocket
{
    uint8_t ucInUse;                    /**< Tracks whether the socket is in use or not. */
    uint8_t esWifiSocketNumber;         /**< Socket number used in eswifi layer. */
    uint32_t ulFlags;                   /**< Various properties of the socket (secured etc.). */
    uint32_t ulSendTimeout;             /**< Send timeout. */
    uint32_t ulReceiveTimeout;          /**< Receive timeout. */
} STSecureSocket_t;

static STSecureSocket_t xSockets[ wificonfigMAX_SOCKETS ];
static const TickType_t xSemaphoreWaitTicks = pdMS_TO_TICKS( 60000 );
extern xSemaphoreHandle xWifiSemaphoreHandle;

/*-----------------------------------------------------------*/

static uint32_t prvGetFreeSocket( void )
{
    uint32_t ulIndex;

    /* Iterate over xSockets array to see if any free socket
     * is available. */
    for( ulIndex = 0; ulIndex < ( uint32_t ) wificonfigMAX_SOCKETS; ulIndex++ )
    {
        /* Since multiple tasks can be accessing this simultaneously,
         * this has to be in critical section. */
        taskENTER_CRITICAL();

        if( xSockets[ ulIndex ].ucInUse == 0U )
        {
            /* Mark the socket as "in-use". */
            xSockets[ ulIndex ].ucInUse = 1;
            taskEXIT_CRITICAL();

            /* We have found a free socket, so stop. */
            break;
        }
        else
        {
            taskEXIT_CRITICAL();
        }
    }

    /* Did we find a free socket? */
    if( ulIndex == ( uint32_t ) wificonfigMAX_SOCKETS )
    {
        /* Return SOCKETS_INVALID_SOCKET if we fail to
         * find a free socket. */
        ulIndex = ( uint32_t ) SOCKETS_INVALID_SOCKET;
    }

    return ulIndex;
}

/*-----------------------------------------------------------*/

static void prvReturnSocket( uint32_t ulSocketNumber )
{
    /* Since multiple tasks can be accessing this simultaneously,
     * this has to be in critical section. */
    taskENTER_CRITICAL();
    {
        /* Mark the socket as free. */
        xSockets[ ulSocketNumber ].ucInUse = 0;
    }
    taskEXIT_CRITICAL();
}

/*-----------------------------------------------------------*/

static BaseType_t prvIsValidSocket( uint32_t ulSocketNumber )
{
    BaseType_t xValid = pdFALSE;

    /* Check that the provided socket number is within the valid
     * index range. */
    if( ulSocketNumber < ( uint32_t ) wificonfigMAX_SOCKETS )
    {
        /* Since multiple tasks can be accessing this simultaneously,
         * this has to be in critical section. */
        taskENTER_CRITICAL();
        {
            /* Check that this socket is in use. */
            if( xSockets[ ulSocketNumber ].ucInUse == 1U )
            {
                /* This is a valid socket number. */
                xValid = pdTRUE;
            }
        }
        taskEXIT_CRITICAL();
    }

    return xValid;
}

/*-----------------------------------------------------------*/

static uint32_t prvGetHostByName( const char * pcHostName )
{
    uint32_t ulIPAddres = 0;

    /* Try to acquire the semaphore. */
    if( xSemaphoreTake( xWifiSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
    {
        /* Do a DNS Lookup. */
        if( WIFI_GetHostAddress( pcHostName, ( uint8_t * ) &( ulIPAddres ) ) != WIFI_STATUS_OK )
        {
            /* Return 0 if the DNS lookup fails. */
            ulIPAddres = 0;
        }

        /* Return the semaphore. */
        ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
    }

    return ulIPAddres;
}

/*-----------------------------------------------------------*/

BaseType_t Sockets_Connect( Socket_t * xSocket,
                            const char * pHostName,
                            uint16_t port,
                            uint32_t receiveTimeoutMs,
                            uint32_t sendTimeoutMs )
{
    uint32_t ulSocketNumber = 0;
    STSecureSocket_t * pxSecureSocket;
    int32_t lRetVal = SOCKETS_ERROR_NONE;
    uint32_t ulIPAddres = 0;

    ulSocketNumber = prvGetFreeSocket();

    if ( ulSocketNumber == ( uint32_t )SOCKETS_INVALID_SOCKET )
    {
        lRetVal = SOCKETS_ENOMEM;
    }
    else
    {
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );
        pxSecureSocket->ulFlags = stsecuresocketsSOCKET_SECURE_FLAG;
        pxSecureSocket->ulSendTimeout = socketsconfigDEFAULT_SEND_TIMEOUT;
        pxSecureSocket->ulReceiveTimeout = socketsconfigDEFAULT_RECV_TIMEOUT;

        if( ( ulIPAddres = prvGetHostByName( pHostName ) ) == 0 )
        {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
        else if ( xSemaphoreTake( xWifiSemaphoreHandle, xSemaphoreWaitTicks ) != pdTRUE )
        {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
        else
        {
            /* Start the client connection. */
            if( WIFI_OpenClientConnection( ulSocketNumber, WIFI_TCP_PROTOCOL,
                                           NULL, (uint8_t *)&ulIPAddres, port, 0 ) == WIFI_STATUS_OK )
            {
                /* Successful connection is established. */
                lRetVal = SOCKETS_ERROR_NONE;

                /* Mark that the socket is connected. */
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_IS_CONNECTED_FLAG;
            }
            else
            {
                /* Connection failed. */
                lRetVal = SOCKETS_SOCKET_ERROR;
            }

            /* Return the semaphore. */
            ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
        }
    }

    if ( lRetVal == SOCKETS_ERROR_NONE )
    {
        *xSocket = ( void * ) ulSocketNumber;
    }
    else if ( ulSocketNumber != ( uint32_t )SOCKETS_INVALID_SOCKET )
    {
        /* Return the socket back to the free socket pool. */
        prvReturnSocket( ulSocketNumber );
    }
    
    return lRetVal;
}

/*-----------------------------------------------------------*/

void Sockets_Disconnect( Socket_t tcpSocket )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;
    STSecureSocket_t * pxSecureSocket;

    /* Ensure that a valid socket was passed. */
    if( prvIsValidSocket( ulSocketNumber ) == pdTRUE )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        /* Mark the socket as closed. */
        pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;
        pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;

        /* Try to acquire the semaphore. */
        if( xSemaphoreTake( xWifiSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
        {
            /* Stop the client connection. */
            WIFI_CloseClientConnection( ulSocketNumber );

            /* Return the semaphore. */
            ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
        }

        /* Return the socket back to the free socket pool. */
        prvReturnSocket( ulSocketNumber );
    }
}

/*-----------------------------------------------------------*/

BaseType_t Sockets_Send( Socket_t tcpSocket,
                         const unsigned char * pucData,
                         size_t xDataLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;
    STSecureSocket_t * pxSecureSocket;
    uint16_t usSentBytes = 0;
    BaseType_t xRetVal = SOCKETS_SOCKET_ERROR;
    WIFI_Status_t xWiFiResult;

    /* Shortcut for easy access. */
    pxSecureSocket = &( xSockets[ ulSocketNumber ] );

    /* Try to acquire the semaphore. */
    if( xSemaphoreTake( xWifiSemaphoreHandle, xSemaphoreWaitTicks ) == pdTRUE )
    {
        // /* Since WiFi module has only one timeout, this needs
        //  * to be set per send and receive operation to the
        //  * respective send or receive timeout. Also, this
        //  * must be done after acquiring the semaphore as the
        //  * xWiFiModule is a shared object.*/
        // if( pxSecureSocket->ulSendTimeout == 0 )
        // {
        //     /* Set the SPI timeout to the maximum uint32_t value.
        //      * This is a little over 49 days. */
        //     xWiFiModule.xWifiObject.Timeout = 0xFFFFFFFF;
        // }
        // else
        // {
        //     /* The maximum timeout for Inventek module is 30 seconds.
        //      * This timeout is about 65 seconds, so the module should
        //      * timeout before the SPI. */
        //     xWiFiModule.xWifiObject.Timeout = ES_WIFI_TIMEOUT;
        // }

        /* Send the data. */
        xWiFiResult = WIFI_SendData( ( uint8_t ) ulSocketNumber,
                                     ( uint8_t * ) pucData, /*lint !e9005 STM function does not use const. */
                                     ( uint16_t ) xDataLength,
                                     &( usSentBytes ),
                                     pxSecureSocket->ulSendTimeout );

        if( xWiFiResult == WIFI_STATUS_OK )
        {
            /* If the data was successfully sent, return the actual
             * number of bytes sent. Otherwise return SOCKETS_SOCKET_ERROR. */
            xRetVal = ( BaseType_t ) usSentBytes;
        }

        /* Return the semaphore. */
        ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
    }

    /* The following code attempts to revive the Inventek WiFi module
     * from its unusable state.*/
    if( xWiFiResult == WIFI_STATUS_ERROR )
    {
        /* Reset the WiFi Module. Since the WIFI_Reset function
         * acquires the same semaphore, we must not acquire
         * it. */
        if( WIFI_ResetModule() == WIFI_STATUS_OK )
        {
            /* Try to acquire the semaphore. */
            if( xSemaphoreTake( xWifiSemaphoreHandle, portMAX_DELAY ) == pdTRUE )
            {
                /* Reinitialize the socket structures which
                 * marks all sockets as closed and free. */
                SOCKETS_Init();

                /* Return the semaphore. */
                ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
            }

            /* Set the error code to indicate that
             * WiFi needs to be reconnected to network. */
            xRetVal = SOCKETS_PERIPHERAL_RESET;
        }
    }

    /* To allow other tasks of equal priority that are using this API to run as
     * a switch to an equal priority task that is waiting for the mutex will
     * only otherwise occur in the tick interrupt - at which point the mutex
     * might have been taken again by the currently running task.
     */
    taskYIELD();

    return xRetVal;
}

/*-----------------------------------------------------------*/

BaseType_t Sockets_Recv( Socket_t tcpSocket,
                         unsigned char * pucReceiveBuffer,
                         size_t xReceiveBufferLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;
    STSecureSocket_t * pxSecureSocket;
    uint16_t usReceivedBytes = 0;
    BaseType_t xRetVal;
    WIFI_Status_t xWiFiResult;
    TickType_t xTimeOnEntering = xTaskGetTickCount(), xSemaphoreWait;

    /* Shortcut for easy access. */
    pxSecureSocket = &( xSockets[ ulSocketNumber ] );

    /* WiFi module does not support receiving more than ES_WIFI_PAYLOAD_SIZE
     * bytes at a time. */
    if( xReceiveBufferLength > ( uint32_t ) ES_WIFI_PAYLOAD_SIZE )
    {
        xReceiveBufferLength = ( uint32_t ) ES_WIFI_PAYLOAD_SIZE;
    }

    xSemaphoreWait = pxSecureSocket->ulReceiveTimeout + stsecuresocketsFIVE_MILLISECONDS;

    for( ; ; )
    {
        /* Try to acquire the semaphore. */
        if( xSemaphoreTake( xWifiSemaphoreHandle, xSemaphoreWait ) == pdTRUE )
        {
            // /* Since WiFi module has only one timeout, this needs
            //  * to be set per send and receive operation to the
            //  * respective send or receive timeout. Also, this
            //  * must be done after acquiring the semaphore as the
            //  * xWiFiModule is a shared object.*/
            // if( pxSecureSocket->ulReceiveTimeout == 0 )
            // {
            //     /* Set the SPI timeout to the maximum uint32_t value.
            //      * This is a little over 49 days. */
            //     xWiFiModule.xWifiObject.Timeout = 0xFFFFFFFF;
            // }
            // else
            // {
            //     /* The maximum timeout for Inventek module is 30 seconds.
            //      * This timeout is about 65 seconds, so the module should
            //      * timeout before the SPI. */
            //     xWiFiModule.xWifiObject.Timeout = ES_WIFI_TIMEOUT;
            // }

            /* Receive the data. */
            xWiFiResult = WIFI_ReceiveData( ( uint8_t ) ulSocketNumber,
                                            ( uint8_t * ) pucReceiveBuffer,
                                            ( uint16_t ) xReceiveBufferLength,
                                            &( usReceivedBytes ),
                                            stsecuresocketsONE_MILLISECOND );

            /* Return the semaphore. */
            ( void ) xSemaphoreGive( xWifiSemaphoreHandle );

            if( ( xWiFiResult == WIFI_STATUS_OK ) && ( usReceivedBytes != 0 ) )
            {
                /* Success, return the number of bytes received. */
                xRetVal = ( BaseType_t ) usReceivedBytes;
                break;
            }
            else if( ( xWiFiResult == WIFI_STATUS_TIMEOUT ) || ( ( xWiFiResult == WIFI_STATUS_OK ) && ( usReceivedBytes == 0 ) ) )
            {
                /* The WiFi poll timed out, but has the socket timeout expired
                 * too? */
                if( ( xTaskGetTickCount() - xTimeOnEntering ) < pxSecureSocket->ulReceiveTimeout )
                {
                    /* The socket has not timed out, but the driver supplied
                     * with the board is polling, which would block other tasks, so
                     * block for a short while to allow other tasks to run before
                     * trying again. */
                    vTaskDelay( stsecuresocketsFIVE_MILLISECONDS );
                }
                else
                {
                    /* The socket read has timed out too. Returning
                     * SOCKETS_EWOULDBLOCK will cause mBedTLS to fail
                     * and so we must return zero. */
                    xRetVal = 0;
                    break;
                }
            }
            else
            {
                /* xWiFiResult contains an error status. */
                xRetVal = SOCKETS_SOCKET_ERROR;
                break;
            }
        }
        else
        {
            /* Semaphore wait time was longer than the receive timeout so this
             * is also a socket timeout. Returning SOCKETS_EWOULDBLOCK will
             * cause mBedTLS to fail and so we must return zero.*/
            xRetVal = 0;
            break;
        }
    }

    /* The following code attempts to revive the Inventek WiFi module
     * from its unusable state.*/
    if( xWiFiResult == WIFI_STATUS_ERROR )
    {
        /* Reset the WiFi Module. Since the WIFI_Reset function
         * acquires the same semaphore, we must not acquire
         * it. */
        if( WIFI_ResetModule() == WIFI_STATUS_OK )
        {
            /* Try to acquire the semaphore. */
            if( xSemaphoreTake( xWifiSemaphoreHandle, portMAX_DELAY ) == pdTRUE )
            {
                /* Reinitialize the socket structures which
                 * marks all sockets as closed and free. */
                SOCKETS_Init();

                /* Return the semaphore. */
                ( void ) xSemaphoreGive( xWifiSemaphoreHandle );
            }

            /* Set the error code to indicate that
             * WiFi needs to be reconnected to network. */
            xRetVal = SOCKETS_PERIPHERAL_RESET;
        }
    }

    return xRetVal;
}

/*-----------------------------------------------------------*/

BaseType_t SOCKETS_Init()
{
    uint32_t ulIndex;

    /* Mark all the sockets as free and closed. */
    for( ulIndex = 0; ulIndex < ( uint32_t ) wificonfigMAX_SOCKETS; ulIndex++ )
    {
        xSockets[ ulIndex ].ucInUse = 0;
        xSockets[ ulIndex ].ulFlags = 0;

        xSockets[ ulIndex ].ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;
        xSockets[ ulIndex ].ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;
    }

    /* Empty initialization for ST board. */
    return pdPASS;
}

/*-----------------------------------------------------------*/
