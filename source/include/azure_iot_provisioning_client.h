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

typedef struct AzureIoTProvisioningClient * AzureIoTProvisioningClientHandle_t;

typedef enum AzureIoTProvisioningClientError
{
    AZURE_IOT_PROVISIONING_CLIENT_SUCCESS = 0,      /*/< Success. */
    AZURE_IOT_PROVISIONING_CLIENT_INVALID_ARGUMENT, /*/< Input argument does not comply with the expected range of values. */
    AZURE_IOT_PROVISIONING_CLIENT_PENDING,          /*/< The status of the operation is pending. */
    AZURE_IOT_PROVISIONING_CLIENT_OUT_OF_MEMORY,    /*/< The system is out of memory. */
    AZURE_IOT_PROVISIONING_CLIENT_INIT_FAILED,      /*/< The initialization failed. */
    AZURE_IOT_PROVISIONING_CLIENT_SERVER_ERROR,     /*/< There was a server error in registration. */
    AZURE_IOT_PROVISIONING_CLIENT_FAILED,           /*/< There was a failure. */
} AzureIoTProvisioningClientResult_t;

typedef struct AzureIoTProvisioningClientOptions
{
    const uint8_t * pUserAgent; /*/ The user agent to use for this device. */
    uint32_t userAgentLength;   /*/ The length of the user agent. */
} AzureIoTProvisioningClientOptions_t;

typedef struct AzureIoTProvisioningClient
{
    struct
    {
        AzureIoTMQTT_t xMQTTContext;

        char provisioning_topic_buffer[ azureiotTOPIC_MAX ];
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
                                                                    uint64_t ullExpiryTimeSecs,
                                                                    const uint8_t * ucKey,
                                                                    uint32_t ulKeyLen,
                                                                    uint8_t * pucSASBuffer,
                                                                    uint32_t ulSasBufferLen,
                                                                    uint32_t * pulSaSLength );
        AzureIoTGetHMACFunc_t azure_iot_provisioning_client_hmac_function;
    } _internal;
} AzureIoTProvisioningClient_t;

/**
 * @brief Initialize the Azure IoT Provisioning Options with default values.
 *
 * @param[out] pxProvisioningClientOptions The #AzureIoTProvisioningClientOptions_t instance to set with default values.
 * @return An #AzureIoTProvisioningClientResult_t with the result of the operation.
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_OptionsInit( AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions );

/**
 * @brief Initialize the Azure IoT Provisioning Client.
 *
 * @param[out] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[in] pucEndpoint The IoT Provisioning Hostname.
 * @param[in] ulEndpointLength The length of the IoT Provisioning Hostname.
 * @param[in] pucIdScope The id scope to use for provisioning.
 * @param[in] ulIdScopeLength The length of the scope id.
 * @param[in] pucRegistrationId The registration id to use for provisioning.
 * @param[in] ulRegistrationIdLength The length of the registration id.
 * @param[in] pxProvisioningClientOptions The #AzureIoTProvisioningClientOptions_t for the IoT Provisioning client instance.
 * @param[in] pucBuffer The buffer to use for MQTT messages.
 * @param[in] ulBufferLength bufferLength The length of the \p pucBuffer.
 * @param[in] xGetTimeFunction A function pointer to a function which gives the current epoch time.
 * @param[in] pxTransportInterface The transport interface to use for the MQTT library.
 * @return AzureIoTProvisioningClientResult_t
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Init( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                    const uint8_t * pucEndpoint,
                                                                    uint32_t ulEndpointLength,
                                                                    const uint8_t * pucIdScope,
                                                                    uint32_t ulIdScopeLength,
                                                                    const uint8_t * pucRegistrationId,
                                                                    uint32_t ulRegistrationIdLength,
                                                                    AzureIoTProvisioningClientOptions_t * pxProvisioningClientOptions,
                                                                    uint8_t * pucBuffer,
                                                                    uint32_t ulBufferLength,
                                                                    AzureIoTGetCurrentTimeFunc_t xGetTimeFunction,
                                                                    const AzureIoTTransportInterface_t * pxTransportInterface );

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
 * @param[in] pucSymmetricKey The symmetric key to use for the connection.
 * @param[in] ulSymmetricKeyLength The length of the \p pucSymmetricKey.
 * @param[in] xHmacFunction The #AzureIoTGetHMACFunc_t function pointer to a function which computes the HMAC256 over a set of bytes.
 * @return An #AzureIoTProvisioningClientResult_t with the result of the operation.
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_SymmetricKeySet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                               const uint8_t * pucSymmetricKey,
                                                                               uint32_t ulSymmetricKeyLength,
                                                                               AzureIoTGetHMACFunc_t xHmacFunction );

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
 * @param[in] ulTimeoutMilliseconds Timeout in millisecond to wait for registration to finish.
 * @return An #AzureIoTProvisioningClientResult_t with the result of the operation.
 *      - AZURE_IOT_PROVISIONING_CLIENT_PENDING registration is still in progess.
 *      - AZURE_IOT_PROVISIONING_CLIENT_SUCCESS registration is completed.
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_Register( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                        uint32_t ulTimeoutMilliseconds );

/**
 * @brief After a registration has been completed, get the IoT Hub host name and device id.
 *
 * @param[in] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[out] pucHubHostname The pointer to a buffer which will be populated with the IoT Hub hostname.
 * @param[out] pulHostnameLength The pointer to the `uint32_t` which will be populated with the length of the hostname.
 * @param[out] pucDeviceId The pointer to a buffer which will be populated with the device id.
 * @param[out] pulDeviceIdLength The pointer to the uint32_t which will be populated with the length of the device id.
 * @return An #AzureIoTProvisioningClientResult_t with the result of the operation.
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_HubGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                      uint8_t * pucHubHostname,
                                                                      uint32_t * pulHostnameLength,
                                                                      uint8_t * pucDeviceId,
                                                                      uint32_t * pulDeviceIdLength );

/**
 * @brief Get extended code for Provisioning failure.
 *
 * @note Extended code is 6 digit error code.
 *
 * @param[in] xAzureIoTProvisioningClientHandle The #AzureIoTProvisioningClientHandle_t to use for this call.
 * @param[out] pulExtendedErrorCode The pointer to the uint32_t which will be populated with the extended code.
 * @return An #AzureIoTProvisioningClientResult_t with the result of the operation.
 */
AzureIoTProvisioningClientResult_t AzureIoTProvisioningClient_ExtendedCodeGet( AzureIoTProvisioningClientHandle_t xAzureIoTProvisioningClientHandle,
                                                                               uint32_t * pulExtendedErrorCode );

#endif /* AZURE_IOT_PROVISIONING_CLIENT_H */
