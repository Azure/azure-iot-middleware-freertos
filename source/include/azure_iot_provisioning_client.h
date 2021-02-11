/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_provisioning_client.h
 *
 */

#ifndef AZURE_IOT_PROVISIONING_CLIENT_H
#define AZURE_IOT_PROVISIONING_CLIENT_H

/* Transport interface include. */
#include "FreeRTOS.h"

#include "azure_iot.h"

#include "azure_iot_mqtt.h"
#include "azure_iot_mqtt_port.h"

#ifndef azureIoTPROVISIONINGDEFAULTTOKENTIMEOUTINSEC
    #define azureIoTPROVISIONINGDEFAULTTOKENTIMEOUTINSEC    azureIoTDEFAULTTOKENTIMEOUTINSEC
#endif

#ifndef azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureIoTPROVISIONINGClientKEEP_ALIVE_TIMEOUT_SECONDS    azureIoTKEEP_ALIVE_TIMEOUT_SECONDS
#endif

#ifndef azureIoTHubClientSUBACK_WAIT_INTERVAL_MS
    #define azureIoTPROVISIONINGClientCONNACK_RECV_TIMEOUT_MS    azureIoTCONNACK_RECV_TIMEOUT_MS
#endif

typedef struct AzureIoTProvisioningClient * AzureIoTProvisioningClientHandle_t;

typedef enum AzureIoTProvisioningClientError
{
    AZURE_IOT_PROVISIONING_CLIENT_SUCCESS = 0,      /*/< Success. */
    AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT, /*/< Input argument does not comply with the expected range of values. */
    AZURE_IOT_PROVISIONING_CLIENT_STATUS_PENDING,   /*/< The status of the operation is pending. */
    AZURE_IOT_PROVISIONING_CLIENT_STATUS_OOM,       /*/< The system is out of memory. */
    AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED,      /*/< The initialization failed. */
    AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR,     /*/< There was a server error in registration. */
    AZURE_IOT_PROVISIONING_CLIENT_FAILED,           /*/< There was a failure. */
    AZURE_IOT_PROVISIONING_CLIENT_PENDING,          /*/< Registration is still pending. */
} AzureIoTProvisioningClientError_t;

typedef struct AzureIoTProvisioningClient
{
    struct
    {
        AzureIoTMQTT_t xMQTTContext;

        uint8_t * azure_iot_provisioning_client_last_response_payload;
        size_t azure_iot_provisioning_client_last_response_payload_length;
        uint8_t * azure_iot_provisioning_client_last_response_topic;
        uint32_t azure_iot_provisioning_client_last_response_topic_length;

        az_iot_provisioning_client_register_response register_response;

        az_iot_provisioning_client iot_dps_client_core;

        const uint8_t * pRegistrationId;
        uint32_t registrationIdLength;
        const uint8_t * pEndpoint;
        uint32_t endpointLength;
        const uint8_t * pIdScope;
        uint32_t idScopeLength;

        uint32_t workflowState;
        uint32_t lastOperationResult;
        uint64_t azure_iot_provisioning_client_req_timeout;
        AzureIoTGetCurrentTimeFunc_t getTimeFunction;

        const uint8_t * azure_iot_provisioning_client_symmetric_key;
        uint32_t azure_iot_provisioning_client_symmetric_key_length;

        uint32_t ( * azure_iot_provisioning_client_token_refresh )( struct AzureIoTProvisioningClient * xAzureIoTProvisioningClientHandle,
                                                                    uint64_t expiryTimeSecs,
                                                                    const uint8_t * key,
                                                                    uint32_t keyLen,
                                                                    uint8_t * pSASBuffer,
                                                                    uint32_t sasBufferLen,
                                                                    uint32_t * pSaSLength );
        AzureIoTGetHMACFunc_t azure_iot_provisioning_client_hmac_function;
    } _internal;
} AzureIoTProvisioningClient_t;

/**
 * @brief Initialize the Azure IoT Provisioning Client.
 *
 * @param[out] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[in] pEndpoint The IoT Provisioning Hostname.
 * @param[in] endpointLength The length of the IoT Provisioning Hostname.
 * @param[in] pIdScope The id scope to use for provisioning.
 * @param[in] idScopeLength The length of the scope id.
 * @param[in] pRegistrationId The registration id to use for provisioning.
 * @param[in] registrationIdLength The length of the registration id.
 * @param[in] pBuffer The buffer to use for MQTT messages.
 * @param[in] bufferLength bufferLength The length of the \p pBuffer.
 * @param[in] getTimeFunction A function pointer to a function which gives the current epoch time.
 * @param[in] pTransportInterface The transport interface to use for the MQTT library.
 * @return AzureIoTProvisioningClientError_t
 */
AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                   const uint8_t * pEndpoint,
                                                                   uint32_t endpointLength,
                                                                   const uint8_t * pIdScope,
                                                                   uint32_t idScopeLength,
                                                                   const uint8_t * pRegistrationId,
                                                                   uint32_t registrationIdLength,
                                                                   uint8_t * pBuffer,
                                                                   uint32_t bufferLength,
                                                                   AzureIoTGetCurrentTimeFunc_t getTimeFunction,
                                                                   const TransportInterface_t * pTransportInterface );

/**
 * @brief Deinitialize the Azure IoT Provisioning Client.
 *
 * @param xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 */
void AzureIoTProvisioningClient_Deinit( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle );

/**
 * @brief Set the symmetric key to use for authentication.
 *
 * @param[in] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[in] pSymmetricKey The symmetric key to use for the connection.
 * @param[in] pSymmetricKeyLength The length of the \p pSymmetricKey.
 * @param[in] hmacFunction The #AzureIoTGetHMACFunc_t function pointer to a function which computes the HMAC256 over a set of bytes.
 * @return An #AzureIoTProvisioningClientError_t with the result of the operation.
 */
AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_SymmetricKeySet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                              const uint8_t * pSymmetricKey,
                                                                              uint32_t pSymmetricKeyLength,
                                                                              AzureIoTGetHMACFunc_t hmacFunction );

/**
 * @brief Begin the provisioning process.
 *
 * This call will issue the request to the service to provision this device. It will then poll the service
 * to check if an IoT Hub and device id have been provided. After a successful result, AzureIoTProvisioningClient_HubGet() can
 * be called to get the IoT Hub and device id. Otherwise an appropriate error result will be returned.
 *
 * Note: If result return is AZURE_IOT_PROVISIONING_CLIENT_PENDING, then registation is still in progess and
 * user need to call this function to resume the registartion.
 *
 * @param[in] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[in] timeout Timeout in ms to wait for registration to finish.
 * @return An #AzureIoTProvisioningClientError_t with the result of the operation.
 *      - AZURE_IOT_PROVISIONING_CLIENT_PENDING registration is still in progess.
 *      - AZURE_IOT_PROVISIONING_CLIENT_SUCCESS registration is completed.
 */
AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                       uint32_t timeout );

/**
 * @brief After a registration has been completed, get the IoT Hub host name and device id.
 *
 * @param[in] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[out] pHubHostname The pointer to a buffer which will be populated with the IoT Hub hostname.
 * @param[out] pHostnameLength The pointer to the `uint32_t` which will be populated with the length of the hostname.
 * @param[out] pDeviceId The pointer to a buffer which will be populated with the device id.
 * @param[out] pDeviceIdLength The pointer to the uint32_t which will be populated with the length of the device id.
 * @return An #AzureIoTProvisioningClientError_t with the result of the operation.
 */
AzureIoTProvisioningClientError_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                     uint8_t * pHubHostname,
                                                                     uint32_t * pHostnameLength,
                                                                     uint8_t * pDeviceId,
                                                                     uint32_t * pDeviceIdLength );

#endif /* AZURE_IOT_PROVISIONING_CLIENT_H */
