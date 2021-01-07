/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/**
 * @file azure_iot_hub_client.h
 * 
 */

#ifndef AZURE_IOT_HUB_CLIENT_H
#define AZURE_IOT_HUB_CLIENT_H

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure_iot.h"

#include "core_mqtt.h"
#include "logging_stack.h"
#include "transport_interface.h"

#ifndef azureIoTHUBDEFAULTTOKENTIMEOUTINSEC
#define azureIoTHUBDEFAULTTOKENTIMEOUTINSEC                     ( 60 * 60U )
#endif

#ifndef azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS
#define azureIoTHubClientKEEP_ALIVE_TIMEOUT_SECONDS             ( 60U )
#endif

#ifndef azureIoTHubClientCONNACK_RECV_TIMEOUT_MS
#define azureIoTHubClientCONNACK_RECV_TIMEOUT_MS                ( 1000U )
#endif

typedef struct AzureIoTHubClient * AzureIoTHubClientHandle_t;

typedef enum AzureIoTHubMessageType
{
    AZURE_IOT_HUB_C2D_MESSAGE = 1,                    ///< The message is a C2D message.
    AZURE_IOT_HUB_DIRECT_METHOD_MESSAGE,                ///< The message is a direct method message.
    AZURE_IOT_HUB_TWIN_GET_MESSAGE,                     ///< The message is a twin get response (payload contains the twin document).
    AZURE_IOT_HUB_TWIN_REPORTED_RESPONSE_MESSAGE,       ///< The message is a twin reported property status response.
    AZURE_IOT_HUB_TWIN_DESIRED_PROPERTY_MESSAGE,        ///< The message is a twin desired property message (incoming from the service).
} AzureIoTHubMessageType_t;

typedef enum AzureIoTHubClientError
{
    AZURE_IOT_HUB_CLIENT_SUCCESS = 0,                   ///< Success.
    AZURE_IOT_HUB_CLIENT_INVALID_ARGUMENT,              ///< Input argument does not comply with the expected range of values.
    AZURE_IOT_HUB_CLIENT_STATUS_PENDING,                ///< The status of the operation is pending.
    AZURE_IOT_HUB_CLIENT_STATUS_OOM,                    ///< The system is out of memory.
    AZURE_IOT_HUB_CLIENT_INIT_FAILED,                   ///< The initialization failed.
    AZURE_IOT_HUB_CLIENT_FAILED,                        ///< There was a failure.
} AzureIoTHubClientError_t;

typedef uint64_t( * AzureIoTGetCurrentTimeFunc_t )( void );

typedef uint32_t( * AzureIoTGetHMACFunc_t )( const uint8_t * pKey, uint32_t keyLength,
                                             const uint8_t * pData, uint32_t dataLength,
                                             uint8_t * pOutput, uint32_t outputLength,
                                             uint32_t * pBytesCopied );

/*
 *  This holds the message type (IE telemetry, methods, etc) and
 *  the publish info which holds payload/length, the topic/length, QOS, dup, etc.
*/
typedef struct AzureIoTHubClientMessage
{
    AzureIoTHubMessageType_t message_type;
    MQTTPublishInfo_t * pxPublishInfo;
    union
    {
        az_iot_hub_client_c2d_request c2d_request;
        az_iot_hub_client_method_request method_request;
        az_iot_hub_client_twin_response twin_response;
    } parsed_message;
} AzureIoTHubClientMessage_t;

/*
 *  The subscription has the details of the topic filter, length, and QOS.
 *  The process function is the function to call for a specific topic.
*/
typedef struct AzureIoTHubClientReceiveContext
{
    MQTTSubscribeInfo_t xMQTTSubscription;
    uint32_t ( * process_function )( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                     MQTTPublishInfo_t * pxPublishInfo );

    void * callback_context;
    void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context );
} AzureIoTHubClientReceiveContext_t;

typedef struct AzureIoTHubClient
{
    MQTTContext_t xMQTTContext;

    az_iot_hub_client iot_hub_client_core;

    const uint8_t * hostname;
    uint32_t hostnameLength;
    const uint8_t *deviceId;
    uint32_t deviceIdLength;
    const uint8_t * azure_iot_hub_client_symmetric_key;
    uint32_t azure_iot_hub_client_symmetric_key_length;

    uint32_t ( * azure_iot_hub_client_token_refresh )( struct AzureIoTHubClient * xAzureIoTHubClientHandle,
                                                       uint64_t expiryTimeSecs, const uint8_t * key, uint32_t keyLen,
                                                       uint8_t * pSASBuffer, uint32_t sasBufferLen, uint32_t * pSaSLength );
    AzureIoTGetHMACFunc_t azure_iot_hub_client_hmac_function;
    AzureIoTGetCurrentTimeFunc_t azure_iot_hub_client_time_function;

    uint32_t currentRequestId;

    //TODO: Look to make this a possible define to save space
    AzureIoTHubClientReceiveContext_t xReceiveContext[3];
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
 * @param[in] pModuleId The model ID used to identify the capabilities of a device based on the Digital Twin document.
 * @param[in] moduleIdLength The length of the model id.
 * @param[in] pBuffer The buffer to use for MQTT messages.
 * @param[in] bufferLength The length of the \p pBuffer.
 * @param[in] getTimeFunction A function pointer to a function which gives the current epoch time.
 * @param[in] pTransportInterface The transport interface to use for the MQTT library.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_Init( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                 const uint8_t * pHostname, uint32_t hostnameLength,
                                                 const uint8_t * pDeviceId, uint32_t deviceIdLength,
                                                 const uint8_t * pModuleId, uint32_t moduleIdLength,
                                                 uint8_t * pBuffer, uint32_t bufferLength,
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
                                                            const uint8_t * pSymmetricKey, uint32_t pSymmetricKeyLength, 
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
                                                    bool cleanSession, TickType_t xTimeoutTicks );

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
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_TelemetrySend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                          const char * pTelemetryData, uint32_t telemetryDataLength );

/**
 * @brief Do work receiving MQTT messages from IoT Hub.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] timeoutMs Minimum time for the loop to run. If `0` is passed, it will only run once.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DoWork( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                   uint32_t timeoutMs );

/**
 * @brief Enable cloud to device (C2D) messages.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The callback to invoke when a C2D messages arrive.
 * @param[in] callback_context A pointer to a context to pass to the callback.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_CloudMessageEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context );

/**
 * @brief Enable direct methods.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The callback to invoke when direct method messages arrive.
 * @param[in] callback_context A pointer to a context to pass to the callback.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DirectMethodEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                               void * callback_context );

/**
 * @brief Enable device twin.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] callback The callback to invoke when device twin messages arrive.
 * @param[in] callback_context A pointer to a context to pass to the callback.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinEnable( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                             void ( * callback ) ( AzureIoTHubClientMessage_t * message, void * context ),
                                                             void * callback_context );

/**
 * @brief Send a response to a received direct method message.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] message The pointer to the #AzureIoTHubClientMessage_t to which a response is being sent.
 * @param[in] status A code that indicates the result of the method, as defined by the user.
 * @param[in] pMethodPayload __[nullable]__ An optional method response payload.
 * @param[in] methodPayloadLength The length of the method response payload.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_SendMethodResponse( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                               AzureIoTHubClientMessage_t* message, uint32_t status,
                                                               const char * pMethodPayload, uint32_t methodPayloadLength);

/**
 * @brief Send reported device twin properties to Azure IoT Hub.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @param[in] pReportedPayload The payload of properly formatted, reported properties.
 * @param[in] reportedPayloadLength The length of the reported property payload.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinReportedSend( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle,
                                                                   const char* pReportedPayload, uint32_t reportedPayloadLength);

/**
 * @brief Request to get the device twin document.
 * 
 * @param[in] xAzureIoTHubClientHandle The #AzureIoTHubClientHandle_t to use for this call.
 * @return An #AzureIoTHubClientError_t with the result of the operation.
 */
AzureIoTHubClientError_t AzureIoTHubClient_DeviceTwinGet( AzureIoTHubClientHandle_t xAzureIoTHubClientHandle);

#endif /* AZURE_IOT_HUB_CLIENT_H */
