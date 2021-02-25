/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_transport_interface.h
 * @brief Transport interface definitions to send and receive data over the
 * network.
 */
#ifndef AZURE_IOT_TRANSPORT_INTERFACE_H_
#define AZURE_IOT_TRANSPORT_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @transportpage
 * @brief The transport interface definition.
 *
 * @transportsectionoverview
 *
 * The transport interface is a set of APIs that must be implemented using an
 * external transport layer protocol. The transport interface is defined in
 * @ref transport_interface.h. This interface allows protocols like MQTT and
 * HTTP to send and receive data over the transport layer. This
 * interface does not handle connection and disconnection to the server of
 * interest. The connection, disconnection, and other transport settings, like
 * timeout and TLS setup, must be handled in the user application.
 * <br>
 *
 * The functions that must be implemented are:<br>
 * - [Transport Receive](@ref TransportRecv_t)
 * - [Transport Send](@ref TransportSend_t)
 *
 * Each of the functions above take in an opaque context @ref struct NetworkContext.
 * The functions above and the context are also grouped together in the
 * @ref AzureIoTTransportInterface_t structure:<br><br>
 * @snippet this define_transportinterface
 * <br>
 *
 * @transportsectionimplementation
 *
 * The following steps give guidance on implementing the transport interface:
 *
 * -# Implementing @ref struct NetworkContext<br><br>
 * @snippet this define_networkcontext
 * <br>
 * The implemented struct NetworkContext must contain all of the information
 * that is needed to receive and send data with the @ref TransportRecv_t
 * and the @ref TransportSend_t implementations.<br>
 * In the case of TLS over TCP, struct NetworkContext is typically implemented
 * with the TCP socket context and a TLS context.<br><br>
 * <b>Example code:</b>
 * @code{c}
 * struct NetworkContext
 * {
 *     struct MyTCPSocketContext tcpSocketContext;
 *     struct MyTLSContext tlsContext;
 * };
 * @endcode
 * <br>
 * -# Implementing @ref TransportRecv_t<br><br>
 * @snippet this define_transportrecv
 * <br>
 * This function is expected to populate a buffer, with bytes received from the
 * transport, and return the number of bytes placed in the buffer.
 * In the case of TLS over TCP, @ref TransportRecv_t is typically implemented by
 * calling the TLS layer function to receive data. In case of plaintext TCP
 * without TLS, it is typically implemented by calling the TCP layer receive
 * function. @ref TransportRecv_t may be invoked multiple times by the protocol
 * library, if fewer bytes than were requested to receive are returned.
 * <br><br>
 * <b>Example code:</b>
 * @code{c}
 * int32_t myNetworkRecvImplementation( struct NetworkContext * pNetworkContext,
 *                                      void * pBuffer,
 *                                      size_t bytesToRecv )
 * {
 *     int32_t bytesReceived = 0;
 *     bytesReceived = TLSRecv( pNetworkContext->tlsContext,
 *                              pBuffer,
 *                              bytesToRecv,
 *                              MY_SOCKET_TIMEOUT );
 *     if( bytesReceived < 0 )
 *     {
 *         // Handle socket error.
 *     }
 *     // Handle other cases.
 *
 *     return bytesReceived;
 * }
 * @endcode
 * <br>
 * -# Implementing @ref TransportSend_t<br><br>
 * @snippet this define_transportsend
 * <br>
 * This function is expected to send the bytes, in the given buffer over the
 * transport, and return the number of bytes sent.
 * In the case of TLS over TCP, @ref TransportSend_t is typically implemented by
 * calling the TLS layer function to send data. In case of plaintext TCP
 * without TLS, it is typically implemented by calling the TCP layer send
 * function. @ref TransportSend_t may be invoked multiple times by the protocol
 * library, if fewer bytes than were requested to send are returned.
 * <br><br>
 * <b>Example code:</b>
 * @code{c}
 * int32_t myNetworkSendImplementation( struct NetworkContext * pNetworkContext,
 *                                      const void * pBuffer,
 *                                      size_t bytesToSend )
 * {
 *     int32_t bytesSent = 0;
 *     bytesSent = TLSSend( pNetworkContext->tlsContext,
 *                          pBuffer,
 *                          bytesToSend,
 *                          MY_SOCKET_TIMEOUT );
 *     if( bytesSent < 0 )
 *     {
 *         // Handle socket error.
 *     }
 *     // Handle other cases.
 *
 *     return bytesSent;
 * }
 * @endcode
 */

/**
 * @transportstruct
 * @brief The NetworkContext is an incomplete type. An implementation of this
 * interface must define struct NetworkContext for the system requirements.
 * This context is passed into the network interface functions.
 */
/* @[define_networkcontext] */
struct NetworkContext;
/* @[define_networkcontext] */

/**
 * @transportcallback
 * @brief Transport interface for receiving data on the network.
 *
 * @param[in] pNetworkContext Implementation-defined network context.
 * @param[in] pBuffer Buffer to receive the data into.
 * @param[in] bytesToRecv Number of bytes requested from the network.
 *
 * @return The number of bytes received or a negative error code.
 */
/* @[define_transportrecv] */
typedef int32_t ( * AzureIoTTransportRecv_t )( struct NetworkContext * pNetworkContext,
                                               void * pBuffer,
                                               size_t bytesToRecv );
/* @[define_transportrecv] */

/**
 * @transportcallback
 * @brief Transport interface for sending data over the network.
 *
 * @param[in] pNetworkContext Implementation-defined network context.
 * @param[in] pBuffer Buffer containing the bytes to send over the network stack.
 * @param[in] bytesToSend Number of bytes to send over the network.
 *
 * @return The number of bytes sent or a negative error code.
 */
/* @[define_transportsend] */
typedef int32_t ( * AzureIoTTransportSend_t )( struct NetworkContext * pNetworkContext,
                                               const void * pBuffer,
                                               size_t bytesToSend );
/* @[define_transportsend] */

/**
 * @azuretransportstruct
 * @brief The transport layer interface.
 */
/* @[define_transportinterface] */
typedef struct AzureIoTTransportInterface
{
    AzureIoTTransportRecv_t recv;               /**< Transport receive interface. */
    AzureIoTTransportSend_t send;               /**< Transport send interface. */
    struct NetworkContext * pNetworkContext; /**< Implementation-defined network context. */
} AzureIoTTransportInterface_t;
/* @[define_transportinterface] */

#endif /* ifndef AZURE_IOT_TRANSPORT_INTERFACE_H_ */
