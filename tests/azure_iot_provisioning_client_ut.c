/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_provisioning_client.h"

#define testPROVISIONING_SERVICE_REPONSE_TOPIC      "$dps/registrations/res/202/?"

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern const uint8_t * pucPublishPayload;

static const uint8_t ucEndpoint[] = "unittest.azure-devices-provisioning.net";
static const uint8_t ucIdScope[] = "0ne000A247E";
static const uint8_t ucRegistrationId[] = "UnitTest";
static const uint8_t ucSymmetricKey[] = "ABC12345";
static uint8_t ucBuffer[1024];
static AzureIoTTransportInterface_t xTransportInterface = {
    .pNetworkContext = NULL,
    .send = (AzureIoTTransportSend_t)0xA5A5A5A5,
    .recv = (AzureIoTTransportRecv_t)0xACACACAC
};
static const uint8_t ucAssignedHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"assigned\",\"registrationState\": \
         {\"registrationId\":\"reg_id\",\"createdDateTimeUtc\":\"2019-12-27T19:51:41.6630592Z\",\"assignedHub\":\"test.azure-iothub.com\", \
          \"deviceId\":\"testId\",\"status\":\"assigned\",\"substatus\":\"initialAssignment\",\"lastUpdatedDateTimeUtc\":\"2019-12-27T19:51:41.8579703Z\", \
          \"etag\":\"XXXXXXXXXXX=\"\
         }\
}";
static const uint8_t ucAssigningHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"assigning\" \
}";
static const uint8_t ucFailureHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"failed\",\"registrationState\": \
         {\"registrationId\":\"reg_id\",\"createdDateTimeUtc\":\"2019-12-27T19:51:41.6630592Z\",\
          \"status\":\"failed\",\"errorCode\":400207,\"errorMessage\":\"Custom allocation failed with status code: 400\",\
          \"lastUpdatedDateTimeUtc\":\"2019-12-27T19:51:41.8579703Z\", \
          \"etag\":\"XXXXXXXXXXX=\"\
         }\
}";
static uint8_t ucTopicBuffer[128];
static uint32_t ulRequestId = 1;
static uint64_t ulUnixTime = 0;
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount(void);
int get_all_tests();

TickType_t xTaskGetTickCount(void)
{
    return 1;
}

static uint64_t prvGetUnixTime(void)
{
    return ulUnixTime;
}

static uint32_t prvHmacFunction(const uint8_t * pucKey,
                                uint32_t ulKeyLength,
                                const uint8_t * pucData,
                                uint32_t ulDataLength,
                                uint8_t * pucOutput,
                                uint32_t ulOutputLength,
                                uint32_t * pucBytesCopied)
{
    (void)pucKey;
    (void)ulKeyLength;
    (void)pucData;
    (void)ulDataLength;
    (void)pucOutput;
    (void)ulOutputLength;
    (void)pucBytesCopied;

    return ((uint32_t)mock());
}

static void prvGenerateGoodResponse(AzureIoTMQTTPublishInfo_t * pxPublishInfo,
                                    uint32_t ulAssignedResponseAfter)
{
    int length;
    if (ulAssignedResponseAfter == 0)
    {

        xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH;
        xDeserializedInfo.packetIdentifier = 1;
        length = snprintf((char *)ucTopicBuffer, sizeof(ucTopicBuffer),
                          "%s$rid=%u", testPROVISIONING_SERVICE_REPONSE_TOPIC,
                          ulRequestId);
        pxPublishInfo->topicNameLength = (uint16_t)length;
        pxPublishInfo->pTopicName = (const char *)ucTopicBuffer;
        pxPublishInfo->pPayload = ucAssignedHubResponse;
        pxPublishInfo->payloadLength = sizeof(ucAssignedHubResponse) - 1;
        xDeserializedInfo.pPublishInfo = pxPublishInfo;
        will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    }
    else
    {
        xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH;
        xDeserializedInfo.packetIdentifier = 1;
        length = snprintf((char *)ucTopicBuffer, sizeof(ucTopicBuffer),
                          "%s$rid=%u&retry-after=%u",
                          testPROVISIONING_SERVICE_REPONSE_TOPIC,
                          ulRequestId, ulAssignedResponseAfter);
        pxPublishInfo->topicNameLength = (uint16_t)length;
        pxPublishInfo->pTopicName = (const char *)ucTopicBuffer;
        pxPublishInfo->pPayload = ucAssigningHubResponse;
        pxPublishInfo->payloadLength = sizeof(ucAssigningHubResponse) - 1;
        xDeserializedInfo.pPublishInfo = pxPublishInfo;
        will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    }
}

static void prvGenerateFailureResponse(AzureIoTMQTTPublishInfo_t * pxPublishInfo)
{
    int length;

    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH;
    xDeserializedInfo.packetIdentifier = 1;
    length = snprintf((char *)ucTopicBuffer, sizeof(ucTopicBuffer),
                      "%s$rid=%u", testPROVISIONING_SERVICE_REPONSE_TOPIC,
                      ulRequestId);
    pxPublishInfo->topicNameLength = (uint16_t)length;
    pxPublishInfo->pTopicName = (const char *)ucTopicBuffer;
    pxPublishInfo->pPayload = ucFailureHubResponse;
    pxPublishInfo->payloadLength = sizeof(ucFailureHubResponse) - 1;
    xDeserializedInfo.pPublishInfo = pxPublishInfo;
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
}

static void prvSetupTestProvisioningClient(AzureIoTProvisioningClient_t * pxTestProvisioningClient)
{
    AzureIoTProvisioningClientOptions_t xProvisioningOptions = {0};

    will_return(AzureIoTMQTT_Init, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Init(pxTestProvisioningClient,
                                                     &ucEndpoint[0], sizeof(ucEndpoint),
                                                     &ucIdScope[0], sizeof(ucIdScope),
                                                     &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                     &xProvisioningOptions, ucBuffer, sizeof(ucBuffer),
                                                     prvGetUnixTime,
                                                     &xTransportInterface),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
    assert_int_equal(AzureIoTProvisioningClient_SymmetricKeySet(pxTestProvisioningClient,
                                                                ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                                prvHmacFunction),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Init_Failure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    (void)state;

    /* Fail init when null endpoint passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         NULL, 0,
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when null IDScope passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         NULL, 0,
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when null RegistrationId passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         NULL, 0,
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when null Buffer passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, NULL, 0,
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when null null AzureIoTGetCurrentTimeFunc_t passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         NULL,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when null Transport passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         prvGetUnixTime,
                                                         NULL),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail when smaller buffer is passed */
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, azureiotUSERNAME_MAX,
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail init when AzureIoTMQTT_Init fails */
    will_return(AzureIoTMQTT_Init, AzureIoTMQTTNoMemory);
    assert_int_not_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                         &ucEndpoint[0], sizeof(ucEndpoint),
                                                         &ucIdScope[0], sizeof(ucIdScope),
                                                         &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                         NULL, ucBuffer, sizeof(ucBuffer),
                                                         prvGetUnixTime,
                                                         &xTransportInterface),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
}

static void testAzureIoTProvisioningClient_Init_Success(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    (void)state;
    will_return(AzureIoTMQTT_Init, AzureIoTMQTTSuccess);

    assert_int_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                     &ucEndpoint[0], sizeof(ucEndpoint),
                                                     &ucIdScope[0], sizeof(ucIdScope),
                                                     &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                     NULL, ucBuffer, sizeof(ucBuffer),
                                                     prvGetUnixTime,
                                                     &xTransportInterface),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
}

static void testAzureIoTProvisioningClient_Deinit_Success(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    (void)state;

    will_return(AzureIoTMQTT_Init, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Init(&xTestProvisioningClient,
                                                     &ucEndpoint[0], sizeof(ucEndpoint),
                                                     &ucIdScope[0], sizeof(ucIdScope),
                                                     &ucRegistrationId[0], sizeof(ucRegistrationId),
                                                     NULL, ucBuffer, sizeof(ucBuffer),
                                                     prvGetUnixTime,
                                                     &xTransportInterface),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_SymmetricKeySet_Failure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    (void)state;

    /* Fail AzureIoTProvisioningClient_SymmetricKeySet when null symmetric key is passed */
    assert_int_not_equal(AzureIoTProvisioningClient_SymmetricKeySet(&xTestProvisioningClient,
                                                                    NULL, 0,
                                                                    prvHmacFunction),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    /* Fail AzureIoTProvisioningClient_SymmetricKeySet when null hashing function is passed */
    assert_int_not_equal(AzureIoTProvisioningClient_SymmetricKeySet(&xTestProvisioningClient,
                                                                    ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                                    NULL),
                         AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
}

static void testAzureIoTProvisioningClient_SymmetricKeySet_Success(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    (void)state;

    assert_int_equal(AzureIoTProvisioningClient_SymmetricKeySet(&xTestProvisioningClient,
                                                                ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                                prvHmacFunction),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);
}

static void testAzureIoTProvisioningClient_Register_ConnectFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTServerRefused);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_SubscribeFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSendFailed);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_SubscribeAckFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* AckSubscribe */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTServerRefused);
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_PublishFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* AckSubscribe */
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Request */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_RegistrationFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t publishInfo;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* AckSubscribe */
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Request */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Registration response */
    prvGenerateFailureResponse(&publishInfo);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Read response */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_QueryFailure(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t publishInfo;
    AzureIoTProvisioningClientResult_t ret;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* AckSubscribe */
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Request */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Registration response */
    prvGenerateGoodResponse(&publishInfo, 1);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Read response */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Wait for timeout */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    ulUnixTime += 2;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Query */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    ret = AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_PENDING);
    assert_int_not_equal(ret, AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

static void testAzureIoTProvisioningClient_Register_Success(void ** state)
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t publishInfo;

    (void)state;

    prvSetupTestProvisioningClient(&xTestProvisioningClient);

    /* Connect */
    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Subscribe */
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* AckSubscribe */
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Request */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Registration response */
    prvGenerateGoodResponse(&publishInfo, 1);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Read response */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Wait for timeout */
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    ulUnixTime += 2;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Publish Registration Query */
    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    /* Restration response */
    prvGenerateGoodResponse(&publishInfo, 0);
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_PENDING);

    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = 0;
    assert_int_equal(AzureIoTProvisioningClient_Register(&xTestProvisioningClient, 0),
                     AZURE_IOT_PROVISIONING_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xTestProvisioningClient);
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testAzureIoTProvisioningClient_Init_Failure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Init_Success),
        cmocka_unit_test(testAzureIoTProvisioningClient_Deinit_Success),
        cmocka_unit_test(testAzureIoTProvisioningClient_SymmetricKeySet_Failure),
        cmocka_unit_test(testAzureIoTProvisioningClient_SymmetricKeySet_Success),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_ConnectFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_SubscribeFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_SubscribeAckFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_PublishFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_RegistrationFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_QueryFailure),
        cmocka_unit_test(testAzureIoTProvisioningClient_Register_Success)};

    return cmocka_run_group_tests_name("azure_iot_provisioning_client_ut", tests, NULL, NULL);
}
