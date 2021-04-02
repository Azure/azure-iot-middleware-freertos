/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client.h
 *
 * @brief The middleware IoT Hub Client used to connect a device to Azure IoT Hub.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#ifndef AZURE_IOT_HUB_CLIENT_H
#define AZURE_IOT_HUB_CLIENT_H

#include "azure_iot.h"

#include "azure_iot_mqtt.h"
#include "azure_iot_mqtt_port.h"
#include "azure_iot_transport_interface.h"

#define hubclientSUBSCRIBE_FEATURE_COUNT    3

typedef struct AzureIoTHubClient * AzureIoTHubClientHandle_t;

typedef enum AzureIoTHubMessageType
{
    eAzureIoTHubCloudToDeviceMessage = 1,    /**< The message is a cloud message. */
    eAzureIoTHubDirectMethodMessage,         /**< The message is a direct method message. */
    eAzureIoTHubTwinGetMessage,              /**< The message is a twin get response (payload contains the twin document). */
    eAzureIoTHubTwinReportedResponseMessage, /**< The message is a twin reported property status response. */
    eAzureIoTHubTwinDesiredPropertyMessage,  /**< The message is a twin desired property message (incoming from the service). */
} AzureIoTHubMessageType_t;

typedef enum AzureIoTHubClientResult
{
    eAzureIoTHubClientSuccess = 0,       /**< Success. */
    eAzureIoTHubClientInvalidArgument,   /**< Input argument does not comply with the expected range of values. */
    eAzureIoTHubClientPending,           /**< The status of the operation is pending. */
    eAzureIoTHubClientOutOfMemory,       /**< The system is out of memory. */
    eAzureIoTHubClientInitFailed,        /**< The initialization failed. */
    eAzureIoTHubClientSubackWaitTimeout, /**< There was timeout while waiting for SUBACK. */
    eAzureIoTHubClientFailed,            /**< There was a failure. */
} AzureIoTHubClientResult_t;

typedef enum AzureIoTHubMessageStatus
{
    /* Default, unset value */
    eAzureIoTStatusUnknown = 0,

    /* Service success codes */
    eAzureIoTStatusOk = 200,
    eAzureIoTStatusAccepted = 202,
    eAzureIoTStatusNoContent = 204,

    /* Service error codes */
    eAzureIoTStatusBadRequest = 400,
    eAzureIoTStatusUnauthorized = 401,
    eAzureIoTStatusForbidden = 403,
    eAzureIoTStatusNotFound = 404,
    eAzureIoTStatusNotAllowed = 405,
    eAzureIoTStatusNotConflict = 409,
    eAzureIoTStatusPreconditionFailed = 412,
    eAzureIoTStatusRequestTooLarge = 413,
    eAzureIoTStatusUnsupportedType = 415,
    eAzureIoTStatusThrottled = 429,
    eAzureIoTStatusClientClosed = 499,
    eAzureIoTStatusServerError = 500,
    eAzureIoTStatusBadGateway = 502,
    eAzureIoTStatusServiceUnavailable = 503,
    eAzureIoTStatusTimeout = 504,
} AzureIoTHubMessageStatus_t;

/**
 * @brief IoT Hub Cloud Message Request struct.
 */
typedef struct AzureIoTHubClientCloudToDeviceMessageRequest
{
    const void * pvMessagePayload;           /**< The pointer to the message payload. */
    uint32_t ulPayloadLength;                /**< The length of the message payload. */

    AzureIoTMessageProperties_t xProperties; /**< The bag of properties received with the message. */
} AzureIoTHubClientCloudToDeviceMessageRequest_t;

/**
 * @brief IoT Hub Method Request struct.
 */
typedef struct AzureIoTHubClientMethodRequest
{
    const void * pvMessagePayload; /**< The pointer to the message payload. */
    uint32_t ulPayloadLength;      /**< The length of the message payload. */

    const uint8_t * pucRequestID;  /**< The pointer to the request ID. */
    uint16_t usRequestIDLength;     /**< The length of the request ID. */

    const uint8_t * pucMethodName; /**< The name of the method to invoke. */
    uint16_t usMethodNameLength;   /**< The length of the method name. */
} AzureIoTHubClientMethodRequest_t;

/**
 * @brief IoT Hub Twin Response struct.
 */
typedef struct AzureIoTHubClientTwinResponse
{
    AzureIoTHubMessageType_t messageType;     /**< The type of message received. */

    const void * pvMessagePayload;            /**< The pointer to the message payload. */
    uint32_t ulPayloadLength;                 /**< The length of the message payload. */

    uint32_t ulRequestID;                     /**< The request ID for the twin response. */

    AzureIoTHubMessageStatus_t messageStatus; /**< The operation status. */

    const uint8_t * pucVersion;               /**< The pointer to the twin document version. */
    uint16_t usVersionLength;                 /**< The length of the twin document version. */
} AzureIoTHubClientTwinResponse_t;

/**
 * @brief Cloud message callback to be invoked when a cloud message is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 */
typedef void ( * AzureIoTHubClientCloudToDeviceMessageCallback_t ) ( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                                                     void * pvContext );

/**
 * @brief Method callback to be invoked when a method request is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 */
typedef void ( * AzureIoTHubClientMethodCallback_t ) ( AzureIoTHubClientMethodRequest_t * pxMessage,
                                                       void * pvContext );

/**
 * @brief Twin callback to be invoked when a twin message is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 */
typedef void ( * AzureIoTHubClientTwinCallback_t ) ( AzureIoTHubClientTwinResponse_t * pxMessage,
                                                     void * pvContext );

/* Receive context to be used internally to the processing of messages */
typedef struct AzureIoTHubClientReceiveContext
{
    struct
    {
        uint16_t usState;
        uint16_t usMqttSubPacketID;
        uint32_t ( * pxProcessFunction )( struct AzureIoTHubClientReceiveContext * pxContext,
                                          AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                          void * pvPublishInfo );

        void * pvCallbackContext;
        union
        {
            AzureIoTHubClientCloudToDeviceMessageCallback_t xCloudToDeviceMessageCallback;
            AzureIoTHubClientMethodCallback_t xMethodCallback;
            AzureIoTHubClientTwinCallback_t xTwinCallback;
        } callbacks;
    } _internal;
} AzureIoTHubClientReceiveContext_t;

typedef struct AzureIoTHubClientOptions
{
    const uint8_t * pucModuleID;  /**< The optional module ID to use for this device. */
    uint32_t ulModuleIDLength;    /**< The length of the module ID. */

    const uint8_t * pucModelID;   /**< The Azure Digital Twin Definition Language model ID used to
                                   *   identify the capabilities of this device based on the Digital Twin document. */
    uint32_t ulModelIDLength;     /**< The length of the model ID. */

    const uint8_t * pucUserAgent; /**< The user agent to use for this device. */
    uint32_t ulUserAgentLength;   /**< The length of the user agent. */
} AzureIoTHubClientOptions_t;

typedef struct AzureIoTHubClient
{
    struct
    {
        AzureIoTMQTT_t xMQTTContext;

        uint8_t * pucAzureIoTHubClientWorkingBuffer;
        uint32_t ulAzureIoTHubClientWorkingBufferLength;
        az_iot_hub_client xAzureIoTHubClientCore;

        const uint8_t * pucHostname;
        uint16_t ulHostnameLength;
        const uint8_t * pucDeviceID;
        uint16_t ulDeviceIDLength;
        const uint8_t * pucAzureIoTHubClientSymmetricKey;
        uint32_t ulAzureIoTHubClientSymmetricKeyLength;

        uint32_t ( * pxAzureIoTHubClientTokenRefresh )( struct AzureIoTHubClient * pxAzureIoTHubClientHandle,
                                                        uint64_t ullExpiryTimeSecs,
                                                        const uint8_t * ucKey,
                                                        uint32_t ulKeyLen,
                                                        uint8_t * pucSASBuffer,
                                                        uint32_t ulSasBufferLen,
                                                        uint32_t * pulSaSLength );
        AzureIoTGetHMACFunc_t xAzureIoTHubClientHMACFunction;
        AzureIoTGetCurrentTimeFunc_t xAzureIoTHubClientTimeFunction;

        uint32_t ulCurrentRequestID;

        AzureIoTHubClientReceiveContext_t xReceiveContext[ hubclientSUBSCRIBE_FEATURE_COUNT ];
    } _internal;
} AzureIoTHubClient_t;

/**
 * @brief Initialize the Azure IoT Hub Options with default values.
 *
 * @param[out] pxHubClientOptions The #AzureIoTHubClientOptions_t instance to set with default values.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions );

/**
 * @brief Initialize the Azure IoT Hub Client.
 *
 * @param[out] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pucHostname The IoT Hub Hostname.
 * @param[in] ulHostnameLength The length of the IoT Hub Hostname.
 * @param[in] pucDeviceID The device ID. If the ID contains any of the following characters, they must
 * be percent-encoded as follows:
 *         - `/` : `%2F`
 *         - `%` : `%25`
 *         - `#` : `%23`
 *         - `&` : `%26`
 * @param[in] ulDeviceIDLength The length of the device ID.
 * @param[in] pxHubClientOptions The #AzureIoTHubClientOptions_t for the IoT Hub client instance.
 * @param[in] pucBuffer The static buffer to use for middleware operations and MQTT messages until AzureIoTHubClient_Deinit is called.
 * @param[in] ulBufferLength The length of the \p pucBuffer.
 * @param[in] xGetTimeFunction A function pointer to a function which gives the current epoch time.
 * @param[in] pxTransportInterface The #AzureIoTTransportInterface_t to use for the MQTT library.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                  const uint8_t * pucHostname,
                                                  uint16_t ulHostnameLength,
                                                  const uint8_t * pucDeviceID,
                                                  uint16_t ulDeviceIDLength,
                                                  AzureIoTHubClientOptions_t * pxHubClientOptions,
                                                  uint8_t * pucBuffer,
                                                  uint32_t ulBufferLength,
                                                  AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                  const AzureIoTTransportInterface_t * pxTransportInterface );

/**
 * @brief Deinitialize the Azure IoT Hub Client.
 *
 * @param xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 */
void AzureIoTHubClient_Deinit( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Set the symmetric key to use for authentication.
 *
 * @note If using X509 authentication, this is not needed and should not be used.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pucSymmetricKey The symmetric key to use for the connection.
 * @param[in] ulSymmetricKeyLength The length of the \p pucSymmetricKey.
 * @param[in] xHmacFunction The #AzureIoTGetHMACFunc_t function pointer to a function which computes the HMAC256 over a set of bytes.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SetSymmetricKey( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             const uint8_t * pucSymmetricKey,
                                                             uint32_t ulSymmetricKeyLength,
                                                             AzureIoTGetHMACFunc_t xHmacFunction );

/**
 * @brief Connect via MQTT to the IoT Hub endpoint.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] cleanSession A boolean dictating whether to connect with a clean session or not.
 * @param[in] ulTimeoutMilliseconds The maximum time in milliseconds to wait for a CONNACK.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_Connect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                     bool cleanSession,
                                                     uint32_t ulTimeoutMilliseconds );

/**
 * @brief Disconnect from the IoT Hub endpoint.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_Disconnect( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Send telemetry data to IoT Hub.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pucTelemetryData The pointer to the buffer of telemetry data.
 * @param[in] ulTelemetryDataLength The length of the buffer to send as telemetry.
 * @param[in] pxProperties The property bag to send with the message.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SendTelemetry( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                           const uint8_t * pucTelemetryData,
                                                           uint32_t ulTelemetryDataLength,
                                                           AzureIoTMessageProperties_t * pxProperties );

/**
 * @brief Receive any incoming MQTT messages from and manage the MQTT connection to IoT Hub.
 *
 * @note This API will receive any messages sent to the device and manage the connection such as sending
 * `PING` messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] ulTimeoutMilliseconds Minimum time (in milliseconds) for the loop to run. If `0` is passed, it will only run once.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                         uint32_t ulTimeoutMilliseconds );

/**
 * @brief Subscribe to cloud to device messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] xCloudToDeviceCallback The #AzureIoTHubClientCloudToDeviceMessageCallback_t to invoke when a CloudToDevice messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for subscribe operation to complete.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeCloudToDeviceMessage( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                           AzureIoTHubClientCloudToDeviceMessageCallback_t xCloudToDeviceMessageCallback,
                                                                           void * prvCallbackContext,
                                                                           uint32_t ulTimeoutMilliseconds );

/**
 * @brief Subscribe to direct methods.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] xMethodCallback The #AzureIoTHubClientMethodCallback_t to invoke when direct method messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for Subscribe operation to complete.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeDirectMethod( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   AzureIoTHubClientMethodCallback_t xMethodCallback,
                                                                   void * prvCallbackContext,
                                                                   uint32_t ulTimeoutMilliseconds );

/**
 * @brief Subscribe to device twin.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] xTwinCallback The #AzureIoTHubClientTwinCallback_t to invoke when device twin messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for Subscribe operation to complete.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SubscribeDeviceTwin( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                 AzureIoTHubClientTwinCallback_t xTwinCallback,
                                                                 void * prvCallbackContext,
                                                                 uint32_t ulTimeoutMilliseconds );

/**
 * @brief Unsubscribe from cloud to device messages.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Unsubscribe from direct methods.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_UnsubscribeDirectMethod( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Unsubscribe from device twin.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinUnsubscribe( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

/**
 * @brief Send a response to a received direct method message.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pxMessage The pointer to the #AzureIoTHubClientMethodRequest_t to which a response is being sent.
 * @param[in] ulStatus A code that indicates the result of the method, as defined by the user.
 * @param[in] pucMethodPayload __[nullable]__ An optional method response payload.
 * @param[in] ulMethodPayloadLength The length of the method response payload.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                const AzureIoTHubClientMethodRequest_t * pxMessage,
                                                                uint32_t ulStatus,
                                                                const uint8_t * pucMethodPayload,
                                                                uint32_t ulMethodPayloadLength );

/**
 * @brief Send reported device twin properties to Azure IoT Hub.
 *
 * @note AzureIoTHubClient_SubscribeDeviceTwin() must be called before calling this function.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pucReportedPayload The payload of properly formatted, reported properties.
 * @param[in] ulReportedPayloadLength The length of the reported property payload.
 * @param[out] pulRequestID Pointer to request ID used to send the twin reported property.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_SendDeviceTwinReported( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                    const uint8_t * pucReportedPayload,
                                                                    uint32_t ulReportedPayloadLength,
                                                                    uint32_t * pulRequestID );

/**
 * @brief Request to get the device twin document.
 *
 * @note AzureIoTHubClient_SubscribeDeviceTwin() must be called before calling this function.
 *
 * The response to the request will be returned via the #AzureIoTHubClientTwinCallback_t which was passed
 * in the AzureIoTHubClient_SubscribeDeviceTwin() call. The type of message will be #eAzureIoTHubTwinGetMessage
 * and the payload (on success) will be the twin document.
 *
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientResult_t with the result of the operation.
 */
AzureIoTHubClientResult_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle );

#endif /* AZURE_IOT_HUB_CLIENT_H */
