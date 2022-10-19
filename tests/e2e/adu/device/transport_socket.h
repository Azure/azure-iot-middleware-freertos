/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Socket transport for plaintext writing.
 *
 */

#ifndef TRANSPORT_SOCKET_H
#define TRANSPORT_SOCKET_H

#include "azure_iot_transport_interface.h"

#include "sockets_wrapper.h"

#include "transport_abstraction.h"

/**
 * @brief Socket Connect / Disconnect return status.
 */
typedef enum SocketTransportStatus
{
    eSocketTransportSuccess = 0,        /**< Function successfully completed. */
    eSocketTransportInvalidParameter,   /**< At least one parameter was invalid. */
    eSocketTransportInSufficientMemory, /**< Insufficient memory required to establish connection. */
    eSocketTransportHandshakeFailed,    /**< Performing Socket handshake with server failed. */
    eSocketTransportInternalError,      /**< A call to a system API resulted in an internal error. */
    eSocketTransportConnectFailure      /**< Initial connection to the server failed. */
} SocketTransportStatus_t;

SocketTransportStatus_t Azure_Socket_Connect( NetworkContext_t * pxNetworkContext,
                                              const char * pHostName,
                                              uint16_t usPort,
                                              uint32_t ulReceiveTimeoutMs,
                                              uint32_t ulSendTimeoutMs );

void Azure_Socket_Close( NetworkContext_t * pNetworkContext );

int32_t Azure_Socket_Send( NetworkContext_t * pxNetworkContext,
                           const void * pvBuffer,
                           size_t xBytesToSend );

int32_t Azure_Socket_Recv( NetworkContext_t * pxNetworkContext,
                           void * pvBuffer,
                           size_t xBytesToRecv );

#endif /* TRANSPORT_SOCKET_H */
