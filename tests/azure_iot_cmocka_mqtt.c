/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_cmocka_mqtt.c
 * @brief -------.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_mqtt_port.h"

AzureIoTMQTTEventCallback_t xTestUserCallback = NULL;
AzureIoTMQTTPacketInfo_t xPacketInfo;
AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
uint16_t usTestPacketId = 1;
const uint8_t *pucPublishPayload = NULL;

AzureIoTMQTTStatus_t AzureIoTMQTT_Init(AzureIoTMQTTHandle_t pContext,
                                       const AzureIoTTransportInterface_t *pTransportInterface,
                                       AzureIoTMQTTGetCurrentTimeFunc_t getTimeFunction,
                                       AzureIoTMQTTEventCallback_t userCallback,
                                       uint8_t *pNetworkBuffer, uint32_t networkBufferLength)
{
    xTestUserCallback = userCallback;
    return (AzureIoTMQTTStatus_t)mock();
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Connect(AzureIoTMQTTHandle_t pContext,
                                          const AzureIoTMQTTConnectInfo_t *pConnectInfo,
                                          const AzureIoTMQTTPublishInfo_t *pWillInfo,
                                          uint32_t timeoutMs,
                                          bool *pSessionPresent)
{
    return (AzureIoTMQTTStatus_t)mock();
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Disconnect(AzureIoTMQTTHandle_t pContext)
{
    return (AzureIoTMQTTStatus_t)mock();
}

uint16_t AzureIoTMQTT_GetPacketId(AzureIoTMQTTHandle_t pContext)
{
    return usTestPacketId;
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Publish(AzureIoTMQTTHandle_t pContext,
                                          const AzureIoTMQTTPublishInfo_t *pPublishInfo,
                                          uint16_t packetId)
{
    return (AzureIoTMQTTStatus_t)mock();
    AzureIoTMQTTStatus_t ret = (AzureIoTMQTTStatus_t)mock();

    if (ret)
    {
        return ret;
    }

    if (pucPublishPayload)
    {
        assert_memory_equal(pPublishInfo->pPayload, pucPublishPayload, pPublishInfo->payloadLength);
    }

    return ret;
}

AzureIoTMQTTStatus_t AzureIoTMQTT_ProcessLoop(AzureIoTMQTTHandle_t pContext,
                                              uint32_t timeoutMs)
{
    AzureIoTMQTTStatus_t ret = (AzureIoTMQTTStatus_t)mock();

    if (ret)
    {
        return ret;
    }

    if (xTestUserCallback && (xPacketInfo.type != 0))
    {
        xTestUserCallback(pContext, &xPacketInfo, &xDeserializedInfo);
    }

    return ret;
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Subscribe(AzureIoTMQTTHandle_t pContext,
                                            const AzureIoTMQTTSubscribeInfo_t *pSubscriptionList,
                                            size_t subscriptionCount,
                                            uint16_t packetId)
{
    return (AzureIoTMQTTStatus_t)mock();
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Unsubscribe(AzureIoTMQTTHandle_t pContext,
                                              const AzureIoTMQTTSubscribeInfo_t *pSubscriptionList,
                                              size_t subscriptionCount,
                                              uint16_t packetId)
{
    return (AzureIoTMQTTStatus_t)mock();
}
