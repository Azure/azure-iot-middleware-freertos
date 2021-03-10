/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_hub_client.h"

#define testCLOUD_CALLBACK_ID 1
#define testMETHOD_CALLBACK_ID 2
#define testTWIN_CALLBACK_ID 3
#define testEMPTY_JSON "{}"
#define testCLOUD_MESSAGE_TOPIC "devices/unittest/messages/devicebound/test=1"
#define testCLOUD_MESSAGE "Hello"
#define testMETHOD_MESSAGE_TOPIC "$iothub/methods/POST/echo/?$rid=1"
#define testMETHOD_MESSAGE testEMPTY_JSON
#define testTWIN_GET_MESSAGE_TOPIC "$iothub/twin/res/200/?$rid=2"
#define testTWIN_MESSAGE "{\"desired\":{\"telemetrySendFrequency\":\"5m\"},\"reported\":{\"telemetrySendFrequency\":\"5m\"}}"
#define testTWIN_DESIRED_MESSAGE_TOPIC "$iothub/twin/PATCH/properties/desired/?$version=1"
#define testTWIN_DESIRED_MESSAGE "{\"telemetrySendFrequency\":\"5m\"}"

typedef struct ReceviceTestData
{
    const uint8_t * pucTopic;
    uint32_t ulTopicLength;
    const uint8_t * pucPayload;
    uint32_t ulPayloadLength;
    uint32_t ulCallbackFunctionId;
} ReceviceTestData_t;

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern const uint8_t * pucPublishPayload;

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static const uint8_t ucSymmetricKey[] = "ABC12345";
static const uint8_t ucTestTelemetryPayload[] = "Unit Test Payload";
static const uint8_t ucTestMethodResponsePayload[] = "Unit Test Method Reponse Payload";
static const uint8_t ucTestTwinReportedPayload[] = "{\"Property\":\"Unit Test Payload\"}";
static uint8_t ucBuffer[512];
static AzureIoTTransportInterface_t xTransportInterface = {
    .pNetworkContext = NULL,
    .send = (AzureIoTTransportSend_t)0xA5A5A5A5,
    .recv = (AzureIoTTransportRecv_t)0xACACACAC
};
static uint32_t ulReceivedCallbackFunctionId;
static const ReceviceTestData_t xTestReceiveData[] = {
    {.pucTopic = (const uint8_t *)testCLOUD_MESSAGE_TOPIC,
     .ulTopicLength = sizeof(testCLOUD_MESSAGE_TOPIC) - 1,
     .pucPayload = (const uint8_t *)testCLOUD_MESSAGE,
     .ulPayloadLength = sizeof(testCLOUD_MESSAGE) - 1,
     .ulCallbackFunctionId = testCLOUD_CALLBACK_ID},
    {.pucTopic = (const uint8_t *)testMETHOD_MESSAGE_TOPIC,
     .ulTopicLength = sizeof(testMETHOD_MESSAGE_TOPIC) - 1,
     .pucPayload = (const uint8_t *)testMETHOD_MESSAGE,
     .ulPayloadLength = sizeof(testMETHOD_MESSAGE) - 1,
     .ulCallbackFunctionId = testMETHOD_CALLBACK_ID},
    {.pucTopic = (const uint8_t *)testTWIN_GET_MESSAGE_TOPIC,
     .ulTopicLength = sizeof(testTWIN_GET_MESSAGE_TOPIC) - 1,
     .pucPayload = (const uint8_t *)testTWIN_MESSAGE,
     .ulPayloadLength = sizeof(testTWIN_MESSAGE) - 1,
     .ulCallbackFunctionId = testTWIN_CALLBACK_ID},
    {.pucTopic = (const uint8_t *)testTWIN_DESIRED_MESSAGE_TOPIC,
     .ulTopicLength = sizeof(testTWIN_DESIRED_MESSAGE_TOPIC) - 1,
     .pucPayload = (const uint8_t *)testTWIN_DESIRED_MESSAGE,
     .ulPayloadLength = sizeof(testTWIN_DESIRED_MESSAGE) - 1,
     .ulCallbackFunctionId = testTWIN_CALLBACK_ID}
};
/*-----------------------------------------------------------*/
TickType_t xTaskGetTickCount(void);
int get_all_tests();

TickType_t xTaskGetTickCount(void)
{
    return 1;
}

static uint64_t prvGetUnixTime(void)
{
    return 0xFFFFFFFFFFFFFFFF;
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

static void prvSetupTestIoTHubClient(AzureIoTHubClient_t * pxTestIoTHubClient)
{
    AzureIoTHubClientOptions_t xHubClientOptions = {0};

    will_return(AzureIoTMQTT_Init, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_Init(pxTestIoTHubClient,
                                            ucHostname, sizeof(ucHostname) - 1,
                                            ucDeviceId, sizeof(ucDeviceId) - 1,
                                            &xHubClientOptions,
                                            ucBuffer,
                                            sizeof(ucBuffer),
                                            prvGetUnixTime,
                                            &xTransportInterface),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
    assert_int_equal(AzureIoTHubClient_SymmetricKeySet(pxTestIoTHubClient,
                                                       ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                       prvHmacFunction),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void prvTestCloudMessage(struct AzureIoTHubClientCloudMessageRequest * pxMessage,
                                void * pvContext)
{
    /* Dummy function */
    (void)pxMessage;
    (void)pvContext;

    ulReceivedCallbackFunctionId = testCLOUD_CALLBACK_ID;
}

static void prvTestDirectMethod(struct AzureIoTHubClientMethodRequest * pxMessage,
                                void * pvContext)
{
    /* Dummy function */
    (void)pxMessage;
    (void)pvContext;
    
    ulReceivedCallbackFunctionId = testMETHOD_CALLBACK_ID;
}

static void prvTestDeviceTwin(struct AzureIoTHubClientTwinResponse * pxMessage,
                              void * pvContext)
{
    /* Dummy function */
    (void)pxMessage;
    (void)pvContext;

    ulReceivedCallbackFunctionId = testTWIN_CALLBACK_ID;
}
/*-----------------------------------------------------------*/

static void testAzureIoTHubClient_Init_Failure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = {0};

    (void)state;

    /* Fail init when null hostname passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                NULL, 0,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                ucBuffer,
                                                sizeof(ucBuffer),
                                                prvGetUnixTime,
                                                &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail init when null deviceId passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                NULL, 0,
                                                &xHubClientOptions,
                                                ucBuffer,
                                                sizeof(ucBuffer),
                                                prvGetUnixTime,
                                                &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail init when null buffer passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                NULL, 0,
                                                prvGetUnixTime,
                                                &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail init when null AzureIoTGetCurrentTimeFunc_t passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof(ucBuffer),
                                                NULL, &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail init when null Transport passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof(ucBuffer),
                                                prvGetUnixTime, NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail when smaller buffer is passed */
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, azureiotUSERNAME_MAX,
                                                prvGetUnixTime, &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail init when AzureIoTMQTT_Init fails */
    will_return(AzureIoTMQTT_Init, AzureIoTMQTTNoMemory);
    assert_int_not_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                                ucHostname, sizeof(ucHostname) - 1,
                                                ucDeviceId, sizeof(ucDeviceId) - 1,
                                                &xHubClientOptions,
                                                ucBuffer, sizeof(ucBuffer),
                                                prvGetUnixTime, &xTransportInterface),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Init_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTHubClientOptions_t xHubClientOptions = {0};

    (void)state;
    will_return(AzureIoTMQTT_Init, AzureIoTMQTTSuccess);

    assert_int_equal(AzureIoTHubClient_Init(&xTestIoTHubClient,
                                            ucHostname, sizeof(ucHostname) - 1,
                                            ucDeviceId, sizeof(ucDeviceId) - 1,
                                            &xHubClientOptions,
                                            ucBuffer,
                                            sizeof(ucBuffer),
                                            prvGetUnixTime,
                                            &xTransportInterface),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Deinit_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    AzureIoTHubClient_Deinit(&xTestIoTHubClient);
}

static void testAzureIoTHubClient_SymmetricKeySet_Failure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    /* Fail AzureIoTHubClient_SymmetricKeySet when null symmetric key is passed */
    assert_int_not_equal(AzureIoTHubClient_SymmetricKeySet(&xTestIoTHubClient,
                                                           NULL, 0,
                                                           prvHmacFunction),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* Fail AzureIoTHubClient_SymmetricKeySet when null hashing function is passed */
    assert_int_not_equal(AzureIoTHubClient_SymmetricKeySet(&xTestIoTHubClient,
                                                           ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                           NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_SymmetricKeySet_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    assert_int_equal(AzureIoTHubClient_SymmetricKeySet(&xTestIoTHubClient,
                                                       ucSymmetricKey, sizeof(ucSymmetricKey) - 1,
                                                       prvHmacFunction),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Connect_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_Connect(NULL,
                                                   false,
                                                   60),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Connect_MQTTConnectFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTServerRefused);
    assert_int_not_equal(AzureIoTHubClient_Connect(&xTestIoTHubClient,
                                                   false,
                                                   60),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Connect_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(prvHmacFunction, 0);
    will_return(AzureIoTMQTT_Connect, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_Connect(&xTestIoTHubClient,
                                               false,
                                               60),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Disconnect_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_Disconnect(NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Disconnect_MQTTDisconnectFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Disconnect, AzureIoTMQTTIllegalState);
    assert_int_not_equal(AzureIoTHubClient_Disconnect(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_Disconnect_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Disconnect, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_Disconnect(&xTestIoTHubClient),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_TelemetrySend_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_TelemetrySend(NULL,
                                                         ucTestTelemetryPayload,
                                                         sizeof(ucTestTelemetryPayload) - 1, NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_TelemetrySend_BigTopicFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMessageProperties_t properties = {
        ._internal.properties._internal.current_property_index = 0,
        ._internal.properties._internal.properties_written = sizeof( ucBuffer ) + 1
    };

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    assert_int_not_equal(AzureIoTHubClient_TelemetrySend(&xTestIoTHubClient,
                                                         ucTestTelemetryPayload,
                                                         sizeof(ucTestTelemetryPayload) - 1, &properties),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_TelemetrySend_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_TelemetrySend(&xTestIoTHubClient,
                                                         ucTestTelemetryPayload,
                                                         sizeof(ucTestTelemetryPayload) - 1, NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_TelemetrySend_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_TelemetrySend(&xTestIoTHubClient,
                                                     ucTestTelemetryPayload,
                                                     sizeof(ucTestTelemetryPayload) - 1, NULL),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_ProcessLoop_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_ProcessLoop(NULL,
                                                       1234),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_ProcessLoop_MQTTProcessFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTRecvFailed);
    assert_int_not_equal(AzureIoTHubClient_ProcessLoop(&xTestIoTHubClient,
                                                       1234),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_ProcessLoop_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_ProcessLoop(&xTestIoTHubClient,
                                                   1234),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageSubscribe_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_CloudMessageSubscribe(NULL,
                                                                 prvTestCloudMessage,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* NULL function callback */
    assert_int_not_equal(AzureIoTHubClient_CloudMessageSubscribe(&xTestIoTHubClient,
                                                                 NULL, NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageSubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_CloudMessageSubscribe(&xTestIoTHubClient,
                                                                 prvTestCloudMessage,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageSubscribe_ReceiveFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTRecvFailed);
    assert_int_not_equal(AzureIoTHubClient_CloudMessageSubscribe(&xTestIoTHubClient,
                                                                 prvTestCloudMessage,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageSubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_CloudMessageSubscribe(&xTestIoTHubClient,
                                                             prvTestCloudMessage,
                                                             NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodSubscribe_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_DirectMethodSubscribe(NULL,
                                                                 prvTestDirectMethod,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* NULL function callback */
    assert_int_not_equal(AzureIoTHubClient_DirectMethodSubscribe(&xTestIoTHubClient,
                                                                 NULL, NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodSubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_DirectMethodSubscribe(&xTestIoTHubClient,
                                                                 prvTestDirectMethod,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodSubscribe_ReceiveFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTRecvFailed);
    assert_int_not_equal(AzureIoTHubClient_DirectMethodSubscribe(&xTestIoTHubClient,
                                                                 prvTestDirectMethod,
                                                                 NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodSubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DirectMethodSubscribe(&xTestIoTHubClient,
                                                             prvTestDirectMethod,
                                                             NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinSubscribe_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    (void)state;

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinSubscribe(NULL,
                                                               prvTestDeviceTwin,
                                                               NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* NULL function callback */
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                               NULL, NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinSubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                               prvTestDeviceTwin,
                                                               NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinSubscribe_ReceiveFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTRecvFailed);
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                               prvTestDeviceTwin,
                                                               NULL, (uint32_t)-1),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinSubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageUnsubscribe_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_CloudMessageUnsubscribe(NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageUnsubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_CloudMessageUnsubscribe(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_CloudMessageUnsubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_CloudMessageUnsubscribe(&xTestIoTHubClient),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodUnsubscribe_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_DirectMethodUnsubscribe(NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodUnsubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_DirectMethodUnsubscribe(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DirectMethodUnsubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_DirectMethodUnsubscribe(&xTestIoTHubClient),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinUnsubscribe_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_DeviceTwinUnsubscribe(NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinUnsubscribe_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinUnsubscribe(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinUnsubscribe_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Unsubscribe, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_DeviceTwinUnsubscribe(&xTestIoTHubClient),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_SendMethodResponse_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientMethodRequest_t req = {
        .requestId = &req_id,
        .requestIdLength = sizeof(req_id)};

    (void)state;

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_SendMethodResponse(NULL,
                                                              &req, 200, ucTestMethodResponsePayload,
                                                              sizeof(ucTestMethodResponsePayload)),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
    /* NULL Request */
    assert_int_not_equal(AzureIoTHubClient_SendMethodResponse(&xTestIoTHubClient,
                                                              NULL, 200, ucTestMethodResponsePayload,
                                                              sizeof(ucTestMethodResponsePayload)),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_SendMethodResponse_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientMethodRequest_t req = {
        .requestId = &req_id,
        .requestIdLength = sizeof(req_id)};

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    assert_int_not_equal(AzureIoTHubClient_SendMethodResponse(&xTestIoTHubClient,
                                                              &req, 200, ucTestMethodResponsePayload,
                                                              sizeof(ucTestMethodResponsePayload)),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_SendMethodResponse_EmptyResponseSuccess(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientMethodRequest_t req = {
        .requestId = &req_id,
        .requestIdLength = sizeof(req_id)};

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    pucPublishPayload = (const uint8_t *)testEMPTY_JSON;
    assert_int_equal(AzureIoTHubClient_SendMethodResponse(&xTestIoTHubClient,
                                                          &req, 200, NULL, 0),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_SendMethodResponse_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint8_t req_id = 1;
    AzureIoTHubClientMethodRequest_t req = {
        .requestId = &req_id,
        .requestIdLength = sizeof(req_id)};

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    pucPublishPayload = ucTestMethodResponsePayload;
    assert_int_equal(AzureIoTHubClient_SendMethodResponse(&xTestIoTHubClient,
                                                          &req, 200, ucTestMethodResponsePayload,
                                                          sizeof(ucTestMethodResponsePayload) - 1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinReportedSend_InvalidArgFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    /* NULL handler */
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinReportedSend(NULL,
                                                                  ucTestTwinReportedPayload,
                                                                  sizeof(ucTestTwinReportedPayload) - 1,
                                                                  &requestId),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

    /* NULL reported payload */
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinReportedSend(&xTestIoTHubClient,
                                                                  NULL, 0,
                                                                  &requestId),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinReportedSend_NotSubscribeFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    assert_int_not_equal(AzureIoTHubClient_DeviceTwinReportedSend(&xTestIoTHubClient,
                                                                  ucTestTwinReportedPayload,
                                                                  sizeof(ucTestTwinReportedPayload) - 1,
                                                                  &requestId),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinReportedSend_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    pucPublishPayload = ucTestTwinReportedPayload;
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinReportedSend(&xTestIoTHubClient,
                                                                  ucTestTwinReportedPayload,
                                                                  sizeof(ucTestTwinReportedPayload) - 1,
                                                                  &requestId),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinReportedSend_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t requestId = 0;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    pucPublishPayload = ucTestTwinReportedPayload;
    assert_int_equal(AzureIoTHubClient_DeviceTwinReportedSend(&xTestIoTHubClient,
                                                              ucTestTwinReportedPayload,
                                                              sizeof(ucTestTwinReportedPayload) - 1,
                                                              &requestId),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinGet_InvalidArgFailure(void ** state)
{
    (void)state;

    assert_int_not_equal(AzureIoTHubClient_DeviceTwinGet(NULL),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinGet_NotSubscribeFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    assert_int_not_equal(AzureIoTHubClient_DeviceTwinGet(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinGet_SendFailure(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSendFailed);
    pucPublishPayload = NULL;
    assert_int_not_equal(AzureIoTHubClient_DeviceTwinGet(&xTestIoTHubClient),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_DeviceTwinGet_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    will_return(AzureIoTMQTT_Publish, AzureIoTMQTTSuccess);
    pucPublishPayload = NULL;
    assert_int_equal(AzureIoTHubClient_DeviceTwinGet(&xTestIoTHubClient),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);
}

static void testAzureIoTHubClient_ReceiveMessages_Success(void ** state)
{
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTMQTTPublishInfo_t publishInfo;

    (void)state;

    prvSetupTestIoTHubClient(&xTestIoTHubClient);

    xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_SUBACK;
    xDeserializedInfo.packetIdentifier = usTestPacketId;
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_CloudMessageSubscribe(&xTestIoTHubClient,
                                                             prvTestCloudMessage,
                                                             NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    xDeserializedInfo.packetIdentifier = ++usTestPacketId;
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_DirectMethodSubscribe(&xTestIoTHubClient,
                                                             prvTestDirectMethod,
                                                             NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    xDeserializedInfo.packetIdentifier = ++usTestPacketId;
    will_return(AzureIoTMQTT_Subscribe, AzureIoTMQTTSuccess);
    will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
    assert_int_equal(AzureIoTHubClient_DeviceTwinSubscribe(&xTestIoTHubClient,
                                                           prvTestDeviceTwin,
                                                           NULL, (uint32_t)-1),
                     AZURE_IOT_HUB_CLIENT_SUCCESS);

    for (size_t index = 0; index < (sizeof(xTestReceiveData) / sizeof(ReceviceTestData_t)); index++)
    {
        will_return(AzureIoTMQTT_ProcessLoop, AzureIoTMQTTSuccess);
        xPacketInfo.type = AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH;
        xDeserializedInfo.packetIdentifier = 1;
        publishInfo.pTopicName = (const char *)xTestReceiveData[index].pucTopic;
        publishInfo.topicNameLength = (uint16_t)xTestReceiveData[index].ulTopicLength;
        publishInfo.pPayload = xTestReceiveData[index].pucPayload;
        publishInfo.payloadLength = xTestReceiveData[index].ulPayloadLength;
        xDeserializedInfo.pPublishInfo = &publishInfo;
        ulReceivedCallbackFunctionId = 0;

        assert_int_equal(AzureIoTHubClient_ProcessLoop(&xTestIoTHubClient, 60),
                         AZURE_IOT_HUB_CLIENT_SUCCESS);

        assert_int_equal(ulReceivedCallbackFunctionId, xTestReceiveData[index].ulCallbackFunctionId);
    }
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testAzureIoTHubClient_Init_Failure),
        cmocka_unit_test(testAzureIoTHubClient_Init_Success),
        cmocka_unit_test(testAzureIoTHubClient_Deinit_Success),
        cmocka_unit_test(testAzureIoTHubClient_SymmetricKeySet_Failure),
        cmocka_unit_test(testAzureIoTHubClient_SymmetricKeySet_Success),
        cmocka_unit_test(testAzureIoTHubClient_Connect_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_Connect_MQTTConnectFailure),
        cmocka_unit_test(testAzureIoTHubClient_Connect_Success),
        cmocka_unit_test(testAzureIoTHubClient_Disconnect_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_Disconnect_MQTTDisconnectFailure),
        cmocka_unit_test(testAzureIoTHubClient_Disconnect_Success),
        cmocka_unit_test(testAzureIoTHubClient_TelemetrySend_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_TelemetrySend_BigTopicFailure),
        cmocka_unit_test(testAzureIoTHubClient_TelemetrySend_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_TelemetrySend_Success),
        cmocka_unit_test(testAzureIoTHubClient_ProcessLoop_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_ProcessLoop_MQTTProcessFailure),
        cmocka_unit_test(testAzureIoTHubClient_ProcessLoop_Success),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageSubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageSubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageSubscribe_ReceiveFailure),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageSubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodSubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodSubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodSubscribe_ReceiveFailure),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodSubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinSubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinSubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinSubscribe_ReceiveFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinSubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageUnsubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageUnsubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_CloudMessageUnsubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodUnsubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodUnsubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DirectMethodUnsubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinUnsubscribe_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinUnsubscribe_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinUnsubscribe_Success),
        cmocka_unit_test(testAzureIoTHubClient_SendMethodResponse_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_SendMethodResponse_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_SendMethodResponse_EmptyResponseSuccess),
        cmocka_unit_test(testAzureIoTHubClient_SendMethodResponse_Success),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinReportedSend_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinReportedSend_NotSubscribeFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinReportedSend_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinReportedSend_Success),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinGet_InvalidArgFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinGet_NotSubscribeFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinGet_SendFailure),
        cmocka_unit_test(testAzureIoTHubClient_DeviceTwinGet_Success),
        cmocka_unit_test(testAzureIoTHubClient_ReceiveMessages_Success)
    };

    return cmocka_run_group_tests_name("azure_iot_hub_client_ut", tests, NULL, NULL);
}
