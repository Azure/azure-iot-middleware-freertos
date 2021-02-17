/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client.h
 *
 */

#ifndef AZURE_IOT_HUB_CLIENT_H
#define AZURE_IOT_HUB_CLIENT_H

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure_iot.h"

#include "azure_iot_mqtt.h"
#include "azure_iot_mqtt_port.h"

#ifndef azureIoTHUBDEFAULTTOKENTIMEOUTINSEC
    #define azureIoTHUBDEFAULTTOKENTIMEOUTINSEC    azureIoTDEFAULTTOKENTIMEOUTINSEC
#endif

#ifndef azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS    azureIoTKEEP_ALIVE_TIMEOUT_SECONDS
#endif

#ifndef azureIoTHubClientCONNACK_RECV_TIMEOUT_MS
    #define azureIoTHubClientCONNACK_RECV_TIMEOUT_MS    azureIoTCONNACK_RECV_TIMEOUT_MS
#endif

#ifndef azureIoTHubClientSUBACK_WAIT_INTERVAL_MS
    #define azureIoTHubClientSUBACK_WAIT_INTERVAL_MS    azureIoTSUBACK_WAIT_INTERVAL_MS
#endif

typedef struct AzureIoTHubClient * AzureIoTHubClientHandle_t;

typedef enum AzureIoTHubMessageType
{
    AZURE_IOT_HUB_C2D_MESSAGE = 1,                /*/< The message is a C2D message. */
    AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE,          /*/< The message is a direct method message. */
    AZURE_IOT_HUB_TWIN_GET_MESSAGE,               /*/< The message is a twin get response (payload contains the twin document). */
    AZURE_IOT_HUB_TWIN_REPORTED_RESPONSE_MESSAGE, /*/< The message is a twin reported property status response. */
    AZURE_IOT_HUB_TWIN_DESIRED_PROPERTY_MESSAGE,  /*/< The message is a twin desired property message (incoming from the service). */
} AzureIoTHubMessageType_t;

typedef enum AzureIoTHubClientError
{
    AZURE_IOT_HUB_CLIENT_SUCCESS = 0,          /*/< Success. */
    AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT,     /*/< Input argument does not comply with the expected range of values. */
    AZURE_IOT_HUB_CLIENT_STATUS_PENDING,       /*/< The status of the operation is pending. */
    AZURE_IOT_HUB_CLIENT_STATUS_OUT_OF_MEMORY, /*/< The system is out of memory. */
    AZURE_IOT_HUB_CLIENT_INIT_FAILED,          /*/< The initialization failed. */
    AZURE_IOT_HUB_CLIENT_SUBACK_WAIT_TIMEOUT,  /*/< There was timeout while waiting for SUBACK. */
    AZURE_IOT_HUB_CLIENT_FAILED,               /*/< There was a failure. */
} AzureIoTHubClientError_t;

typedef enum AzureIoTHubMessageStatus
{
    /* Default, unset value */
    AZURE_IOT_STATUS_UNKNOWN = 0,

    /* Service success codes */
    AZURE_IOT_STATUS_OK = 200,
    AZURE_IOT_STATUS_ACCEPTED = 202,
    AZURE_IOT_STATUS_NO_CONTENT = 204,

    /* Service error codes */
    AZURE_IOT_STATUS_BAD_REQUEST = 400,
    AZURE_IOT_STATUS_UNAUTHORIZED = 401,
    AZURE_IOT_STATUS_FORBIDDEN = 403,
    AZURE_IOT_STATUS_NOT_FOUND = 404,
    AZURE_IOT_STATUS_NOT_ALLOWED = 405,
    AZURE_IOT_STATUS_NOT_CONFLICT = 409,
    AZURE_IOT_STATUS_PRECONDITION_FAILED = 412,
    AZURE_IOT_STATUS_REQUEST_TOO_LARGE = 413,
    AZURE_IOT_STATUS_UNSUPPORTED_TYPE = 415,
    AZURE_IOT_STATUS_THROTTLED = 429,
    AZURE_IOT_STATUS_CLIENT_CLOSED = 499,
    AZURE_IOT_STATUS_SERVER_ERROR = 500,
    AZURE_IOT_STATUS_BAD_GATEWAY = 502,
    AZURE_IOT_STATUS_SERVICE_UNAVAILABLE = 503,
    AZURE_IOT_STATUS_TIMEOUT = 504,
} AzureIoTHubMessageStatus_t;

/* Forward declaration */
struct AzureIoTHubClientTwinResponse;
struct AzureIoTHubClientMethodRequest;
struct AzureIoTHubClientC2DRequest;

/* Typedef for the c2d callback */
typedef void ( * AzureIoTHubClientC2DCallback_t ) ( struct AzureIoTHubClientC2DRequest * message,
                                                    void * context );

/* Typedef for the method callback */
typedef void ( * AzureIoTHubClientMethodCallback_t ) ( struct AzureIoTHubClientMethodRequest * message,
                                                       void * context );

/* Typedef for the twin callback */
typedef void ( * AzureIoTHubClientTwinCallback_t ) ( struct AzureIoTHubClientTwinResponse * message,
                                                     void * context );

/* Receive context to be used internally to the processing of messages */
typedef struct AzureIoTHubClientReceiveContext
{
    struct
    {
        /* 0: un-initalized, 1: subscribed, 2:subAck */
        uint16_t state;
        uint16_t mqttSubPacketId;
        uint32_t ( * process_function )( struct AzureIoTHubClientReceiveContext * pxContext,
                                         AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                         void * pxPublishInfo );

        void * callback_context;
        union
        {
            AzureIoTHubClientC2DCallback_t c2dCallback;
            AzureIoTHubClientMethodCallback_t methodCallback;
            AzureIoTHubClientTwinCallback_t twinCallback;
        } callbacks;
    } _internal;
} AzureIoTHubClientReceiveContext_t;

/*
 *  C2D STRUCTS
 */
typedef struct AzureIoTHubClientC2DRequest
{
    const void * messagePayload;            /*/< The pointer to the message payload. */
    size_t payloadLength;                   /*/< The length of the message payload. */

    AzureIoTMessageProperties_t properties; /*/< The bag of properties received with the message. */
} AzureIoTHubClientC2DRequest_t;

/*
 *  METHOD STRUCTS
 */
typedef struct AzureIoTHubClientMethodRequest
{
    const void * messagePayload; /*/< The pointer to the message payload. */
    size_t payloadLength;        /*/< The length of the message payload. */

    const uint8_t * requestId;   /*/< The pointer to the request id. */
    size_t requestIdLength;      /*/< The length of the request id. */

    const uint8_t * methodName;  /*/< The name of the method to invoke. */
    size_t methodNameLength;     /*/< The length of the method name. */
} AzureIoTHubClientMethodRequest_t;

/*
 *  TWIN STRUCTS
 */
typedef struct AzureIoTHubClientTwinResponse
{
    AzureIoTHubMessageType_t messageType;     /*/< The type of message received. */

    const void * messagePayload;              /*/< The pointer to the message payload. */
    size_t payloadLength;                     /*/< The length of the message payload. */

    uint32_t requestId;                       /*/< request id. */

    AzureIoTHubMessageStatus_t messageStatus; /*/< The operation status. */

    const uint8_t * version;                  /*/< The pointer to the twin document version. */
    size_t versionLength;                     /*/< The length of the twin document version. */
} AzureIoTHubClientTwinResponse_t;

typedef struct AzureIoTHubClientOptions
{
    const uint8_t * pModuleId;  /*/ The module id to use for this device. */
    uint32_t moduleIdLength;    /*/ The length of the module id. */

    const uint8_t * pModelId;   /*/ The model ID used to identify the capabilities of a device based on the Digital Twin document. */
    uint32_t modelIdLength;     /*/ The length of the model id. */

    const uint8_t * pUserAgent; /*/ The user agent to use for this device. */
    uint32_t userAgentLength;   /*/ The length of the user agent. */
} AzureIoTHubClientOptions_t;

typedef struct AzureIoTHubClient
{
    struct
    {
        AzureIoTMQTT_t xMQTTContext;

        char iot_hub_client_topic_buffer[ azureIoTTOPIC_MAX ];
        az_iot_hub_client iot_hub_client_core;

        const uint8_t * hostname;
        uint32_t hostnameLength;
        const uint8_t * deviceId;
        uint32_t deviceIdLength;
        const uint8_t * azure_iot_hub_client_symmetric_key;
        uint32_t azure_iot_hub_client_symmetric_key_length;

        uint32_t ( * azure_iot_hub_client_token_refresh )( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                                           uint64_t expiryTimeSecs,
                                                           const uint8_t * key,
                                                           uint32_t keyLen,
                                                           uint8_t * pSASBuffer,
                                                           uint32_t sasBufferLen,
                                                           uint32_t * pSaSLength );
        AzureIoTGetHMACFunc_t azure_iot_hub_client_hmac_function;
        AzureIoTGetCurrentTimeFunc_t azure_iot_hub_client_time_function;

        uint32_t currentRequestId;

        AzureIoTHubClientReceiveContext_t xReceiveContext[ 3 ];
    } _internal;
} AzureIoTHubClient_t;

/**
 * @brief Initialize the Azure IoT Hub Client.
 *
 * @param[out] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pHostname The IoT Hub Hostname.
 * @param[in] hostnameLength The length of the IoT Hub Hostname.
 * @param[in] pDeviceId The Device ID. If the ID contains any of the following characters, they must
 * be percent-encoded as follows:
 *         - `/` : `%2F`
 *         - `%` : `%25`
 *         - `#` : `%23`
 *         - `&` : `%26`
 * @param[in] deviceIdLength The length of the device id.
 * @param[in] pHubClientOptions The #AzureIoTHubClientOptions_t for the IoT Hub client instance.
 * @param[in] pBuffer The buffer to use for MQTT messages.
 * @param[in] bufferLength The length of the \p pBuffer.
 * @param[in] getTimeFunction A function pointer to a function which gives the current epoch time.
 * @param[in] pTransportInterface The transport interface to use for the MQTT library.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                 const uint8_t * pHostname,
                                                 uint32_t hostnameLength,
                                                 const uint8_t * pDeviceId,
                                                 uint32_t deviceIdLength,
                                                 AzureIoTHubClientOptions_t * pHubClientOptions,
                                                 uint8_t * pBuffer,
                                                 uint32_t bufferLength,
                                                 AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                 const TransportInterface_t * pTransportInterface );

/**
 * @brief Deinitialize the Azure IoT Hub Client.
 *
 * @param xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 */
void AzureIoTHubClient_Deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Set the symmetric key to use for authentication.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pSymmetricKey The symmetric key to use for the connection.
 * @param[in] pSymmetricKeyLength The length of the \p pSymmetricKey.
 * @param[in] hmacFunction The #AzureIoTGetHMACFunc_t function pointer to a function which computes the HMAC256 over a set of bytes.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_SymmetricKeySet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                            const uint8_t * pSymmetricKey,
                                                            uint32_t pSymmetricKeyLength,
                                                            AzureIoTGetHMACFunc_t hmacFunction );

/**
 * @brief Connect via MQTT to the IoT Hub endpoint.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] cleanSession A boolean dictating whether to connect with a clean session or not.
 * @param[in] xTimeoutTicks The maximum time in milliseconds to wait for a CONNACK.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                    bool cleanSession,
                                                    uint32_t xTimeoutTicks );

/**
 * @brief Disconnect from the IoT Hub endpoint
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Send telemetry data to IoT Hub.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pTelemetryData The pointer to the buffer of telemetry data.
 * @param[in] telemetryDataLength The length of the buffer to send as telemetry.
 * @param[in] pProperties The property bag to send with the message.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          const uint8_t * pTelemetryData,
                                                          uint32_t telemetryDataLength,
                                                          AzureIoTMessageProperties_t * pProperties );

/**
 * @brief Receive any incoming MQTT messages from and manage the MQTT connection to IoT Hub.
 *
 * @note This API will receive any messages sent to the device and manage the connection such as send
 * `PING` messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] timeoutMs Minimum time (in ms) for the loop to run. If `0` is passed, it will only run once.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                        uint32_t timeoutMs );

/**
 * @brief Enable cloud to device (C2D) messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The #AzureIoTHubClientC2DCallback_t to invoke when a C2D messages arrive.
 * @param[in] pCallbackContext A pointer to a context to pass to the callback.
 * @param[in] timeout Timeout in ms for enable operation to complete.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientC2DCallback_t c2dCallback,
                                                               void * pCallbackContext,
                                                               uint32_t timeout );

/**
 * @brief Enable direct methods.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The #AzureIoTHubClientMethodCallback_t to invoke when direct method messages arrive.
 * @param[in] pCallbackContext A pointer to a context to pass to the callback.
 * @param[in] timeout Timeout in ms for enable operation to complete.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMethodCallback_t methodCallback,
                                                               void * pCallbackContext,
                                                               uint32_t timeout );

/**
 * @brief Enable device twin.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The #AzureIoTHubClientTwinCallback_t to invoke when device twin messages arrive.
 * @param[in] pCallbackContext A pointer to a context to pass to the callback.
 * @param[in] timeout Timeout in ms for enable operation to complete.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             AzureIoTHubClientTwinCallback_t twinCallback,
                                                             void * pCallbackContext,
                                                             uint32_t timeout );

/**
 * @brief Disable cloud to device (C2D) messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Disable direct methods.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Disable device twin.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinDisable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Send a response to a received direct method message.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pMessage The pointer to the #AzureIoTHubClientMethodRequest_t to which a response is being sent.
 * @param[in] status A code that indicates the result of the method, as defined by the user.
 * @param[in] pMethodPayload __[nullable]__ An optional method response payload.
 * @param[in] methodPayloadLength The length of the method response payload.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMethodRequest_t * pMessage,
                                                               uint32_t status,
                                                               const uint8_t * pMethodPayload,
                                                               uint32_t methodPayloadLength );

/**
 * @brief Send reported device twin properties to Azure IoT Hub.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pReportedPayload The payload of properly formatted, reported properties.
 * @param[in] reportedPayloadLength The length of the reported property payload.
 * @param[out] pRequestId Pointer to request Id used to send the twin reported property.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinReportedSend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   const uint8_t * pReportedPayload,
                                                                   uint32_t reportedPayloadLength,
                                                                   uint32_t * pRequestId );

/**
 * @brief Request to get the device twin document.
 *
 * The answer to the request will be returned via the #AzureIoTHubClientTwinCallback_t which was passed
 * in the AzureIoTHubClient_DeviceTwinEnable() call. The type of message will be #AZURE_IOT_HUB_TWIN_GET_MESSAGE
 * and the payload (on success) will be the twin document.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

#endif /* AZURE_IOT_HUB_CLIENT_H */
