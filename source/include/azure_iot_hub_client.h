/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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
#include "azure_iot_message.h"
#include "azure_iot_result.h"

#include "azure_iot_mqtt_port.h"
#include "azure_iot_transport_interface.h"

/* Azure SDK for Embedded C includes */
#include "azure/az_core.h"
#include "azure/iot/az_iot_common.h"
#include "azure/iot/az_iot_hub_client.h"
#include "azure/core/_az_cfg_prefix.h"

/**
 * @brief Total number of features which could be subscribed to.
 */
#define azureiothubSUBSCRIBE_FEATURE_COUNT    ( 4 )

/**
 * @brief Macro which should be used to create an array of #AzureIoTHubClientComponent_t
 */
#define azureiothubCREATE_COMPONENT( x )    ( AzureIoTHubClientComponent_t ) AZ_SPAN_LITERAL_FROM_STR( x )

/* Forward declaration for Azure IoT Hub Client */
typedef struct AzureIoTHubClient   AzureIoTHubClient_t;

/**
 * @brief Type for Azure IoT Plug and Play component. The list of component names can be set
 * as an option for the Azure IoT Hub Client
 */
typedef az_span                    AzureIoTHubClientComponent_t;

/**
 * @brief MQTT quality of service values used for messages.
 */
typedef enum AzureIoTHubMessageQoS
{
    eAzureIoTHubMessageQoS0 = 0, /** Delivery at most once. */
    eAzureIoTHubMessageQoS1 = 1  /** Delivery at least once. */
} AzureIoTHubMessageQoS_t;

/**
 * @brief Enumeration to dictate Azure IoT message types.
 */
typedef enum AzureIoTHubMessageType
{
    eAzureIoTHubCloudToDeviceMessage = 1,          /**< The message is a cloud message. */
    eAzureIoTHubCommandMessage,                    /**< The message is a command message. */
    eAzureIoTHubPropertiesRequestedMessage,        /**< The message is a response from a property request (payload contains the property document). */
    eAzureIoTHubPropertiesReportedResponseMessage, /**< The message is a reported property status response. */
    eAzureIoTHubPropertiesWritablePropertyMessage, /**< The message is a writable property message (incoming from the service). */
    eAzureIoTHubCertificateSigningResponseMessage, /**< The message is a certificate signing response. */
} AzureIoTHubMessageType_t;

/**
 * @brief Status codes for Azure IoT Hub responses.
 */
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
 * @brief IoT Hub Command Request struct.
 */
typedef struct AzureIoTHubClientCommandRequest
{
    const void * pvMessagePayload;    /**< The pointer to the message payload. */
    uint32_t ulPayloadLength;         /**< The length of the message payload. */

    const uint8_t * pucRequestID;     /**< The pointer to the request ID. */
    uint16_t usRequestIDLength;       /**< The length of the request ID. */

    const uint8_t * pucComponentName; /**< The name of the component associate with this command. */
    uint16_t usComponentNameLength;   /**< The length of the component name. */

    const uint8_t * pucCommandName;   /**< The name of the command to invoke. */
    uint16_t usCommandNameLength;     /**< The length of the command name. */
} AzureIoTHubClientCommandRequest_t;

/**
 * @brief IoT Hub Properties Response struct.
 */
typedef struct AzureIoTHubClientPropertiesResponse
{
    AzureIoTHubMessageType_t xMessageType;     /**< The type of message received. */

    const void * pvMessagePayload;             /**< The pointer to the message payload. */
    uint32_t ulPayloadLength;                  /**< The length of the message payload. */

    uint32_t ulRequestID;                      /**< The request ID for the property response. */

    AzureIoTHubMessageStatus_t xMessageStatus; /**< The operation status. */
} AzureIoTHubClientPropertiesResponse_t;

/**
 * @brief Response type for certificate signing operations.
 */
typedef enum AzureIoTHubClientCertificateSigningResponseType
{
    eAzureIoTHubClientCertificateSigningResponseAccepted = 1,  /**< 202 - CSR accepted, pending issuance. */
    eAzureIoTHubClientCertificateSigningResponseCompleted = 2, /**< 200 - Certificate issued successfully. */
    eAzureIoTHubClientCertificateSigningResponseError = 3,     /**< 4xx/5xx - Error response. */
} AzureIoTHubClientCertificateSigningResponseType_t;

/**
 * @brief IoT Hub Certificate Signing Response struct.
 */
typedef struct AzureIoTHubClientCertificateSigningResponse
{
    AzureIoTHubClientCertificateSigningResponseType_t xResponseType; /**< The type of response (accepted, completed, or error). */

    AzureIoTHubMessageStatus_t xMessageStatus;                       /**< The HTTP-like status code (200, 202, 4xx, 5xx). */

    const void * pvMessagePayload;                                   /**< The pointer to the response payload. */
    uint32_t ulPayloadLength;                                        /**< The length of the response payload. */

    const uint8_t * pucRequestID;                                    /**< The pointer to the request ID. */
    uint16_t usRequestIDLength;                                      /**< The length of the request ID. */
} AzureIoTHubClientCertificateSigningResponse_t;

/**
 * @brief Options for certificate signing requests.
 */
typedef struct AzureIoTHubClientCertificateSigningRequestOptions
{
    const uint8_t * pucReplace; /**< Optional replace field. Use "*" to replace any active request, or pass
                                 *  a prior request ID (see #AzureIoTHubClient_SendCertificateSigningRequest
                                 *  pucRequestID) to replace a specific one. */
    uint16_t usReplaceLength;   /**< The length of the replace field. */
} AzureIoTHubClientCertificateSigningRequestOptions_t;

/**
 * @brief Cloud message callback to be invoked when a cloud message is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 * @param[in] pxMessage The #AzureIoTHubClientCloudToDeviceMessageRequest_t associated with the message.
 * @param[in] pvContext The context passed back to the caller.
 */
typedef void ( * AzureIoTHubClientCloudToDeviceMessageCallback_t ) ( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                                                     void * pvContext );

/**
 * @brief Command callback to be invoked when a command request is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 * @param[in] pxMessage The #AzureIoTHubClientCommandRequest_t associated with the message.
 * @param[in] pvContext The context passed back to the caller.
 *
 */
typedef void ( * AzureIoTHubClientCommandCallback_t ) ( AzureIoTHubClientCommandRequest_t * pxMessage,
                                                        void * pvContext );

/**
 * @brief Properties callback to be invoked when a property message is received in the call to AzureIoTHubClient_ProcessLoop().
 *
 * @param[in] pxMessage The #AzureIoTHubClientPropertiesResponse_t associated with the message.
 * @param[in] pvContext The context passed back to the caller.
 */
typedef void ( * AzureIoTHubClientPropertiesCallback_t ) ( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                                           void * pvContext );

/**
 * @brief Certificate signing callback to be invoked when a certificate signing response is received
 *        in the call to AzureIoTHubClient_ProcessLoop().
 *
 * @param[in] pxResponse The #AzureIoTHubClientCertificateSigningResponse_t associated with the message.
 * @param[in] pvContext The context passed back to the caller.
 */
typedef void ( * AzureIoTHubClientCertificateSigningCallback_t ) ( AzureIoTHubClientCertificateSigningResponse_t * pxResponse,
                                                                   void * pvContext );

/**
 * @brief Receive context to be used internally for the processing of messages.
 *
 * @warning Used internally.
 */
typedef struct AzureIoTHubClientReceiveContext
{
    struct
    {
        uint16_t usState;
        uint16_t usMqttSubPacketID;
        uint32_t ( * pxProcessFunction )( struct AzureIoTHubClientReceiveContext * pxContext,
                                          AzureIoTHubClient_t * pxAzureIoTHubClient,
                                          void * pvPublishInfo );

        void * pvCallbackContext;
        union
        {
            AzureIoTHubClientCloudToDeviceMessageCallback_t xCloudToDeviceMessageCallback;
            AzureIoTHubClientCommandCallback_t xCommandCallback;
            AzureIoTHubClientPropertiesCallback_t xPropertiesCallback;
            AzureIoTHubClientCertificateSigningCallback_t xCertificateSigningCallback;
        } callbacks;
    } _internal; /**< @brief Internal to the SDK */
} AzureIoTHubClientReceiveContext_t;

/**
 * @brief Callback to send notification that puback was received for specific packet ID.
 *
 * @param[in] ulTelemetryPacketID The packet id for the telemetry message which was received. You may use this to match
 * messages with the optional packet id parameter in AzureIoTHubClient_SendTelemetry().
 */
typedef void (* AzureIoTTelemetryAckCallback_t)( uint16_t ulTelemetryPacketID );

/**
 * @brief Options list for the hub client.
 */
typedef struct AzureIoTHubClientOptions
{
    const uint8_t * pucModuleID;                       /**< The optional module ID to use for this device. */
    uint32_t ulModuleIDLength;                         /**< The length of the module ID. */

    const uint8_t * pucModelID;                        /**< The Azure Digital Twin Definition Language model ID used to
                                                        *   identify the capabilities of this device based on the Digital Twin document. */
    uint32_t ulModelIDLength;                          /**< The length of the model ID. */

    AzureIoTHubClientComponent_t * pxComponentList;    /**< The list of component names to use for the device. */
    uint32_t ulComponentListLength;                    /**< The number of components in the list. */

    const uint8_t * pucUserAgent;                      /**< The user agent to use for this device. */
    uint32_t ulUserAgentLength;                        /**< The length of the user agent. */

    AzureIoTTelemetryAckCallback_t xTelemetryCallback; /**< The callback to invoke to notify user a puback was received for QOS 1.
                                                        *   Can be NULL if user does not want to be notified.*/
} AzureIoTHubClientOptions_t;

/**
 * @struct AzureIoTHubClient_t
 * @brief Azure IoT Hub Client used to manage connections and features for Azure IoT Hub.
 */
struct AzureIoTHubClient
{
    struct
    {
        AzureIoTMQTT_t xMQTTContext;

        uint8_t * pucWorkingBuffer;
        uint32_t ulWorkingBufferLength;
        az_iot_hub_client xAzureIoTHubClientCore;

        const uint8_t * pucHostname;
        uint16_t ulHostnameLength;
        const uint8_t * pucDeviceID;
        uint16_t ulDeviceIDLength;
        const uint8_t * pucSymmetricKey;
        uint32_t ulSymmetricKeyLength;

        uint32_t ( * pxTokenRefresh )( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                       uint64_t ullExpiryTimeSecs,
                                       const uint8_t * ucKey,
                                       uint32_t ulKeyLen,
                                       uint8_t * pucSASBuffer,
                                       uint32_t ulSasBufferLen,
                                       uint32_t * pulSaSLength );
        AzureIoTGetHMACFunc_t xHMACFunction;
        AzureIoTGetCurrentTimeFunc_t xTimeFunction;
        AzureIoTTelemetryAckCallback_t xTelemetryCallback;

        uint32_t ulCurrentPropertyRequestID;

        AzureIoTHubClientReceiveContext_t xReceiveContext[ azureiothubSUBSCRIBE_FEATURE_COUNT ];

        az_iot_hub_client_certificate_signing_completed_response xCSRCompletedResponse;
    }
    _internal; /**< @brief Internal to the SDK */
};

/**
 * @brief Initialize the Azure IoT Hub Options with default values.
 *
 * @param[out] pxHubClientOptions The #AzureIoTHubClientOptions_t instance to set with default values.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_OptionsInit( AzureIoTHubClientOptions_t * pxHubClientOptions );

/**
 * @brief Initialize the Azure IoT Hub Client.
 *
 * @param[out] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
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
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_Init( AzureIoTHubClient_t * pxAzureIoTHubClient,
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
 * @param pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 */
void AzureIoTHubClient_Deinit( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Set the symmetric key to use for authentication.
 *
 * @note If using X509 authentication, this is not needed and should not be used.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pucSymmetricKey The symmetric key to use for the connection.
 * @param[in] ulSymmetricKeyLength The length of the \p pucSymmetricKey.
 * @param[in] xHMACFunction The #AzureIoTGetHMACFunc_t function pointer to a function which computes the HMAC256 over a set of bytes.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SetSymmetricKey( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                    const uint8_t * pucSymmetricKey,
                                                    uint32_t ulSymmetricKeyLength,
                                                    AzureIoTGetHMACFunc_t xHMACFunction );

/**
 * @brief Connect via MQTT to the IoT Hub endpoint.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] xCleanSession A boolean dictating whether to connect with a clean session or not.
 * @param[in] pxOutSessionPresent Whether a previous session was present.
 * Only relevant if not establishing a clean session.
 * @param[in] ulTimeoutMilliseconds The maximum time in milliseconds to wait for a CONNACK.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_Connect( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                            bool xCleanSession,
                                            bool * pxOutSessionPresent,
                                            uint32_t ulTimeoutMilliseconds );

/**
 * @brief Disconnect from the IoT Hub endpoint.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_Disconnect( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Send telemetry data to IoT Hub.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pucTelemetryData The pointer to the buffer of telemetry data.
 * @param[in] ulTelemetryDataLength The length of the buffer to send as telemetry.
 * @param[in] pxProperties The property bag to send with the message.
 * @param[in] xQOS The QOS to use for the telemetry. Only QOS `0` and `1` are supported.
 * @param[out] pusTelemetryPacketID The packet id for the sent telemetry.
 *                                  Can be notified of PUBACK for QOS 1 using the #AzureIoTHubClientOptions_t `xTelemetryCallback` option.
 *                                  If xQOS is `eAzureIoTHubMessageQoS0` this value will not be sent on return.
 *                                  Can be `NULL`.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SendTelemetry( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                  const uint8_t * pucTelemetryData,
                                                  uint32_t ulTelemetryDataLength,
                                                  AzureIoTMessageProperties_t * pxProperties,
                                                  AzureIoTHubMessageQoS_t xQOS,
                                                  uint16_t * pusTelemetryPacketID );

/**
 * @brief Receive any incoming MQTT messages from and manage the MQTT connection to IoT Hub.
 *
 * @note This API will receive any messages sent to the device and manage the connection such as sending
 * `PING` messages.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] ulTimeoutMilliseconds Minimum time (in milliseconds) for the loop to run. If `0` is passed, it will only run once.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_ProcessLoop( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                uint32_t ulTimeoutMilliseconds );

/**
 * @brief Subscribe to cloud to device messages.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] xCloudToDeviceMessageCallback The #AzureIoTHubClientCloudToDeviceMessageCallback_t to invoke when a CloudToDevice messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for subscribe operation to complete.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SubscribeCloudToDeviceMessage( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                  AzureIoTHubClientCloudToDeviceMessageCallback_t xCloudToDeviceMessageCallback,
                                                                  void * prvCallbackContext,
                                                                  uint32_t ulTimeoutMilliseconds );

/**
 * @brief Unsubscribe from cloud to device messages.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Subscribe to Azure IoT Hub Direct Methods.
 * @remark Azure IoT Direct methods may also be referred to as Commands.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] xCommandCallback The #AzureIoTHubClientCommandCallback_t to invoke when command messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for Subscribe operation to complete.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SubscribeCommand( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                     AzureIoTHubClientCommandCallback_t xCommandCallback,
                                                     void * prvCallbackContext,
                                                     uint32_t ulTimeoutMilliseconds );

/**
 * @brief Unsubscribe from Azure IoT Hub Direct Methods.
 * @remark Azure IoT Direct methods may also be referred to as Commands.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_UnsubscribeCommand( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Send a response to a received command message.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pxMessage The pointer to the #AzureIoTHubClientCommandRequest_t to which a response is being sent.
 * @param[in] ulStatus A code that indicates the result of the command, as defined by the user.
 * @param[in] pucCommandPayload __[nullable]__ An optional command response payload.
 * @param[in] ulCommandPayloadLength The length of the command response payload.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SendCommandResponse( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                        const AzureIoTHubClientCommandRequest_t * pxMessage,
                                                        uint32_t ulStatus,
                                                        const uint8_t * pucCommandPayload,
                                                        uint32_t ulCommandPayloadLength );

/**
 * @brief Subscribe to device properties.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] xPropertiesCallback The #AzureIoTHubClientPropertiesCallback_t to invoke when device property messages arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for Subscribe operation to complete.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SubscribeProperties( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                        AzureIoTHubClientPropertiesCallback_t xPropertiesCallback,
                                                        void * prvCallbackContext,
                                                        uint32_t ulTimeoutMilliseconds );

/**
 * @brief Unsubscribe from device properties.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_UnsubscribeProperties( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Send reported device properties to Azure IoT Hub.
 *
 * @note AzureIoTHubClient_SubscribeProperties() must be called before calling this function.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pucReportedPayload The payload of properly formatted, reported properties.
 * @param[in] ulReportedPayloadLength The length of the reported property payload.
 * @param[out] pulRequestID Pointer to request ID used to send the reported property.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SendPropertiesReported( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                           const uint8_t * pucReportedPayload,
                                                           uint32_t ulReportedPayloadLength,
                                                           uint32_t * pulRequestID );

/**
 * @brief Request to get the device property document.
 *
 * @note AzureIoTHubClient_SubscribeProperties() must be called before calling this function.
 *
 * The response to the request will be returned via the #AzureIoTHubClientPropertiesCallback_t which was passed
 * in the AzureIoTHubClient_SubscribeProperties() call. The type of message will be #eAzureIoTHubPropertiesRequestedMessage
 * and the payload (on success) will be the property document.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_RequestPropertiesAsync( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Subscribe to certificate signing responses.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] xCSRCallback The #AzureIoTHubClientCertificateSigningCallback_t to invoke when certificate signing responses arrive.
 * @param[in] prvCallbackContext A pointer to a context to pass to the callback.
 * @param[in] ulTimeoutMilliseconds Timeout in milliseconds for Subscribe operation to complete.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SubscribeCertificateSigningResponse( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                        AzureIoTHubClientCertificateSigningCallback_t xCSRCallback,
                                                                        void * prvCallbackContext,
                                                                        uint32_t ulTimeoutMilliseconds );

/**
 * @brief Unsubscribe from certificate signing responses.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_UnsubscribeCertificateSigningResponse( AzureIoTHubClient_t * pxAzureIoTHubClient );

/**
 * @brief Send a certificate signing request to IoT Hub.
 *
 * @note AzureIoTHubClient_SubscribeCertificateSigningResponse() must be called before calling this function.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pucCSR The pointer to the base64-encoded PKCS#10 CSR (without PEM headers).
 * @param[in] ulCSRLength The length of the CSR.
 * @param[in] pucRequestID The pointer to the request ID string. Must be 4 to 36 ASCII characters
 *                         inclusive, containing only alphanumerics and dashes. Must not begin or
 *                         end with a dash. A UUID/GUID (e.g. "550e8400-e29b-41d4-a716-446655440000")
 *                         is a suitable choice for production applications. Store this request ID durably;
 *                         on reconnect or retry, pass it as #AzureIoTHubClientCertificateSigningRequestOptions_t::pucReplace
 *                         to replace the prior in-progress operation.
 * @param[in] usRequestIDLength The length of the request ID (4 to 36 inclusive).
 * @param[in] pxOptions __[nullable]__ Optional #AzureIoTHubClientCertificateSigningRequestOptions_t for extra options (e.g., replace).
 * @param[in] pucPayloadBuffer The buffer to use for building the JSON request payload.
 * @param[in] ulPayloadBufferLength The length of the payload buffer.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_SendCertificateSigningRequest( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                  const uint8_t * pucCSR,
                                                                  uint32_t ulCSRLength,
                                                                  const uint8_t * pucRequestID,
                                                                  uint16_t usRequestIDLength,
                                                                  const AzureIoTHubClientCertificateSigningRequestOptions_t * pxOptions,
                                                                  uint8_t * pucPayloadBuffer,
                                                                  uint32_t ulPayloadBufferLength );

/**
 * @brief Get the number of certificates in the issued certificate chain from the last CSR response.
 *
 * @note Must be called after receiving a 200 Completed CSR response and before the
 *       next AzureIoTHubClient_ProcessLoop() call (the internal spans reference the MQTT buffer).
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[out] pulIssuedCertificateChainLength The pointer to the `uint32_t` which will be populated
 *             with the number of issued certificates.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_GetIssuedCertificateChainLength( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                    uint32_t * pulIssuedCertificateChainLength );

/**
 * @brief Get a certificate from the issued certificate chain at the given position.
 *
 * @note The certificate data is base64-encoded DER (not PEM). The caller is
 *       responsible for any conversion (e.g., wrapping with PEM headers).
 * @note Must be called after receiving a 200 Completed CSR response and before the
 *       next AzureIoTHubClient_ProcessLoop() call.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] ulCertificatePositionNumber The index of the certificate in the issued chain.
 * @param[out] pucIssuedCertificate The pointer to a buffer which will be populated with the
 *             issued certificate data.
 * @param[in,out] pulIssuedCertificateLength On input, the size of \p pucIssuedCertificate buffer.
 *                On output, the length of the certificate data written.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTHubClient_GetIssuedCertificate( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                         uint32_t ulCertificatePositionNumber,
                                                         uint8_t * pucIssuedCertificate,
                                                         uint32_t * pulIssuedCertificateLength );

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZURE_IOT_HUB_CLIENT_H */
