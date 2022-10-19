/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file sockets_wrapper_freertos_tcpip.c
 * @brief FreeRTOS Sockets wrapper implementation.
 */

#include "sockets_wrapper.h"

/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DNS.h"
/*-----------------------------------------------------------*/

/* Maximum number of times to call FreeRTOS_recv when initiating a graceful shutdown. */
#ifndef FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS
    #define FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS    ( 3 )
#endif

/* A negative error code indicating a network failure. */
#define FREERTOS_SOCKETS_WRAPPER_NETWORK_ERROR    ( -1 )

/*-----------------------------------------------------------*/

BaseType_t Sockets_Init()
{
    return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_DeInit()
{
    return SOCKETS_ERROR_NONE;
}
/*-----------------------------------------------------------*/

SocketHandle Sockets_Open()
{
    Socket_t ulSocketNumber = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    SocketHandle xSocket;

    if( ulSocketNumber == FREERTOS_INVALID_SOCKET )
    {
        xSocket = ( SocketHandle ) SOCKETS_INVALID_SOCKET;
    }
    else
    {
        xSocket = ( SocketHandle ) ulSocketNumber;
    }

    return xSocket;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Close( SocketHandle xSocket )
{
    return ( BaseType_t ) FreeRTOS_closesocket( ( Socket_t ) xSocket );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Connect( SocketHandle xSocket,
                            const char * pcHostName,
                            uint16_t usPort )
{
    Socket_t xTcpSocket = ( Socket_t ) xSocket;
    BaseType_t lRetVal = 0;
    struct freertos_sockaddr xServerAddress = { 0 };
    uint32_t ulIPAddres;

    /* Check for errors from DNS lookup. */
    if( ( ulIPAddres = ( uint32_t ) FreeRTOS_gethostbyname( pcHostName ) ) == 0 )
    {
        lRetVal = SOCKETS_SOCKET_ERROR;
    }
    else
    {
        /* Connection parameters. */
        xServerAddress.sin_family = FREERTOS_AF_INET;
        xServerAddress.sin_port = FreeRTOS_htons( usPort );
        xServerAddress.sin_addr = ulIPAddres;
        xServerAddress.sin_len = ( uint8_t ) sizeof( xServerAddress );

        if( FreeRTOS_connect( xTcpSocket, &xServerAddress, sizeof( xServerAddress ) ) != 0 )
        {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
    }

    return lRetVal;
}
/*-----------------------------------------------------------*/

void Sockets_Disconnect( SocketHandle xSocket )
{
    BaseType_t xWaitForShutdownLoopCount = 0;
    uint8_t pucDummyBuffer[ 2 ];
    Socket_t xTcpSocket = ( Socket_t ) xSocket;

    if( xTcpSocket != FREERTOS_INVALID_SOCKET )
    {
        /* Initiate graceful shutdown. */
        ( void ) FreeRTOS_shutdown( xTcpSocket, FREERTOS_SHUT_RDWR );

        /* Wait for the socket to disconnect gracefully (indicated by FreeRTOS_recv()
         * returning a FREERTOS_EINVAL error) before closing the socket. */
        while( FreeRTOS_recv( xTcpSocket, pucDummyBuffer, sizeof( pucDummyBuffer ), 0 ) >= 0 )
        {
            /* We don't need to delay since FreeRTOS_recv should already have a timeout. */

            if( ++xWaitForShutdownLoopCount >= FREERTOS_SOCKETS_WRAPPER_SHUTDOWN_LOOPS )
            {
                break;
            }
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Recv( SocketHandle xSocket,
                         uint8_t * pucReceiveBuffer,
                         size_t xReceiveBufferLength )
{
    return ( BaseType_t ) FreeRTOS_recv( ( Socket_t ) xSocket,
                                         pucReceiveBuffer, xReceiveBufferLength, 0 );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Send( SocketHandle xSocket,
                         const uint8_t * pucData,
                         size_t xDataLength )
{
    return ( BaseType_t ) FreeRTOS_send( ( Socket_t ) xSocket,
                                         pucData, xDataLength, 0 );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_SetSockOpt( SocketHandle xSocket,
                               int32_t lOptionName,
                               const void * pvOptionValue,
                               size_t xOptionLength )
{
    Socket_t xTcpSocket = ( Socket_t ) xSocket;
    BaseType_t xRetVal;
    int ulRet = 0;
    TickType_t xTimeout;

    switch( lOptionName )
    {
        case SOCKETS_SO_RCVTIMEO:
        case SOCKETS_SO_SNDTIMEO:
            /* Comply with Berkeley standard - a 0 timeout is wait forever. */
            xTimeout = *( ( const TickType_t * ) pvOptionValue );

            if( xTimeout == 0U )
            {
                xTimeout = portMAX_DELAY;
            }

            ulRet = FreeRTOS_setsockopt( xTcpSocket,
                                         0,
                                         lOptionName,
                                         &xTimeout,
                                         xOptionLength );

            if( ulRet != 0 )
            {
                xRetVal = SOCKETS_EINVAL;
            }
            else
            {
                xRetVal = SOCKETS_ERROR_NONE;
            }

            break;

        default:
            xRetVal = SOCKETS_ENOPROTOOPT;
            break;
    }

    return xRetVal;
}
/*-----------------------------------------------------------*/
