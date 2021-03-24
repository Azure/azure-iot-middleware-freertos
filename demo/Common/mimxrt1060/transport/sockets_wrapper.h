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
 * @file sockets_wrapper.h
 * @brief Wrap platform specific socket.
 */

#ifndef SOCKETS_WRAPPER_H
#define SOCKETS_WRAPPER_H

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging related header files are required to be included in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define LIBRARY_LOG_NAME and  LIBRARY_LOG_LEVEL.
 * 3. Include the header file "logging_stack.h".
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "Sockets"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

typedef void * Socket_t;

#define SOCKETS_ERROR_NONE          ( 0 )          /*!< No error. */
#define SOCKETS_SOCKET_ERROR        ( -1 )         /*!< Catch-all sockets error code. */
#define SOCKETS_EWOULDBLOCK         ( -11 )        /*!< A resource is temporarily unavailable. */
#define SOCKETS_ENOMEM              ( -12 )        /*!< Memory allocation failed. */
#define SOCKETS_EINVAL              ( -22 )        /*!< Invalid argument. */
#define SOCKETS_ENOPROTOOPT         ( -109 )       /*!< A bad option was specified . */
#define SOCKETS_ENOTCONN            ( -126 )       /*!< The supplied socket is not connected. */
#define SOCKETS_EISCONN             ( -127 )       /*!< The supplied socket is already connected. */
#define SOCKETS_ECLOSED             ( -128 )       /*!< The supplied socket has already been closed. */
#define SOCKETS_PERIPHERAL_RESET    ( -1006 )      /*!< Communications peripheral has been reset. */
/**@} */

#define SOCKETS_INVALID_SOCKET      ( ( Socket_t ) ~0U )

/**
 * @brief Establish a connection to server.
 *
 * @param[out] pTcpSocket The output parameter to return the created socket descriptor.
 * @param[in] pHostName Server hostname to connect to.
 * @param[in] pServerInfo Server port to connect to.
 * @param[in] receiveTimeoutMs Timeout (in milliseconds) for transport receive.
 * @param[in] sendTimeoutMs Timeout (in milliseconds) for transport send.
 *
 * @note A timeout of 0 means infinite timeout.
 *
 * @return Non-zero value on error, 0 on success.
 */
BaseType_t Sockets_Connect( Socket_t * pTcpSocket,
                            const char * pHostName,
                            uint16_t port,
                            uint32_t receiveTimeoutMs,
                            uint32_t sendTimeoutMs );

/**
 * @brief End connection to server.
 *
 * @param[in] tcpSocket The socket descriptor.
 */
void Sockets_Disconnect( Socket_t tcpSocket );

/**
 * @brief Receive data into buffer.
 *
 * @param[in] tcpSocket The socket descriptor.
 * @param[in] pucReceiveBuffer Buffer to receive data.
 * @param[in] xReceiveBufferLength Length of the buffer.
 *
 * @return Number of bytes received, if error negative value is return.
 */
BaseType_t Sockets_Recv( Socket_t tcpSocket,
                         unsigned char * pucReceiveBuffer,
                         size_t xReceiveBufferLength );

/**
 * @brief Send data over socket.
 *
 * @param[in] tcpSocket The socket descriptor.
 * @param[in] pucData Data to send.
 * @param[in] xDataLength Length of the data.
 *
 * @return Non-zero value on error, 0 on success.
 */
BaseType_t Sockets_Send( Socket_t tcpSocket,
                         const unsigned char * pucData,
                         size_t xDataLength );

/**
 * @brief Initialize sockets.
 *
 * @return Non-zero value on error, 0 on success.
 */
BaseType_t SOCKETS_Init();

#endif /* ifndef SOCKETS_WRAPPER_H */
