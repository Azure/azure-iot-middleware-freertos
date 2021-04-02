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

#include "lwip/api.h"
#include "lwip/ip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/err.h"

/* FreeRTOS includes. */
#include "FreeRTOSConfig.h"

#include "sockets_wrapper.h"

#include "task.h"

#include <stdbool.h>

/*-----------------------------------------------------------*/

uint32_t prvGetHostByName( const char * pcHostName )
{
    uint32_t addr = 0; /* 0 indicates failure to caller */
    err_t xLwipError = ERR_OK;
    ip_addr_t xLwipIpv4Address;

    xLwipError = netconn_gethostbyname( pcHostName, &xLwipIpv4Address );

    switch( xLwipError )
    {
        case ERR_OK:
            addr = *( ( uint32_t * ) &xLwipIpv4Address ); /* NOTE: IPv4 addresses only */
            break;

        default:
            configPRINTF( ( "Unexpected error (%lu) from netconn_gethostbyname() while resolving (%s)!",
                            ( uint32_t ) xLwipError, pcHostName ) );
            break;
    }

    return addr;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Connect( Socket_t * xSocket,
                            const char * pHostName,
                            uint16_t port,
                            uint32_t receiveTimeoutMs,
                            uint32_t sendTimeoutMs )
{
    int32_t ulSocketNumber = 0;
    int32_t lRetVal = SOCKETS_ERROR_NONE;
    uint32_t ulIPAddres = 0;
    struct sockaddr_in sa_addr = { 0 };
    struct timeval tv = { 0 };

    ulSocketNumber = lwip_socket( AF_INET, SOCK_STREAM, IP_PROTO_TCP );

    if( ulSocketNumber < 0 )
    {
        lRetVal = SOCKETS_ENOMEM;
    }
    else if( ( ulIPAddres = prvGetHostByName( pHostName ) ) == 0 )
    {
        lRetVal = SOCKETS_SOCKET_ERROR;
    }
    else
    {
        sa_addr.sin_family = AF_INET;
        sa_addr.sin_addr.s_addr = ulIPAddres;
        sa_addr.sin_port = lwip_htons( port );

        if( lwip_connect( ulSocketNumber, ( struct sockaddr * ) &sa_addr, sizeof( sa_addr ) ) < 0 )
        {
            lRetVal = SOCKETS_SOCKET_ERROR;
        }
        else
        {
            tv.tv_sec = receiveTimeoutMs / 1000;
            tv.tv_usec = ( receiveTimeoutMs % 1000 ) * 1000;
            lRetVal = lwip_setsockopt( ulSocketNumber,
                                       SOL_SOCKET,
                                       SO_RCVTIMEO,
                                       ( struct timeval * ) &tv,
                                       sizeof( tv ) );
        }
    }

    if( lRetVal == SOCKETS_ERROR_NONE )
    {
        *xSocket = ( void * ) ulSocketNumber;
    }
    else if( ulSocketNumber >= 0 )
    {
        /* Return the socket back to the free socket pool. */
        lwip_close( ulSocketNumber );
    }

    return lRetVal;
}
/*-----------------------------------------------------------*/

void Sockets_Disconnect( Socket_t tcpSocket )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;

    lwip_close( ulSocketNumber );
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Send( Socket_t tcpSocket,
                         const unsigned char * pucData,
                         size_t xDataLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;
    int ret = lwip_send( ulSocketNumber,
                         pucData,
                         xDataLength,
                         0 );

    return ( BaseType_t ) ret;
}
/*-----------------------------------------------------------*/

BaseType_t Sockets_Recv( Socket_t tcpSocket,
                         unsigned char * pucReceiveBuffer,
                         size_t xReceiveBufferLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) tcpSocket;

    int ret = lwip_recv( ulSocketNumber,
                         pucReceiveBuffer,
                         xReceiveBufferLength,
                         0 );

    if( -1 == ret )
    {
        /*
         * 1. EWOULDBLOCK if the socket is NON-blocking, but there is no data
         *    when recv is called.
         * 2. EAGAIN if the socket would block and have waited long enough but
         *    packet is not received.
         */
        if( ( errno == EWOULDBLOCK ) || ( errno == EAGAIN ) )
        {
            return SOCKETS_ERROR_NONE; /* timeout or would block */
        }

        /*
         * socket is not connected.
         */
        if( errno == EBADF )
        {
            return SOCKETS_ECLOSED;
        }
    }

    if( ( 0 == ret ) && ( errno == ENOTCONN ) )
    {
        ret = SOCKETS_ECLOSED;
    }

    return ( BaseType_t ) ret;
}
/*-----------------------------------------------------------*/
