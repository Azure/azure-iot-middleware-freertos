/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_core_mqtt.c
 * @brief -------.
 */
 
#include <assert.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_mqtt_port.h"

#include "core_mqtt.h"

AzureIoTMQTTStatus_t AzureIoTMQTT_Init( AzureIoTMqttHandle_t  pContext,
                                        const TransportInterface_t * pTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t getTimeFunction,
                                        AzureIoTMQTTEventCallback_t userCallback,
                                        uint8_t * pNetworkBuffer, uint32_t networkBufferLength )
{
    MQTTFixedBuffer_t buffer = { pNetworkBuffer, networkBufferLength };

    /* Check memory equivalence, but ordering is not guarantee */
    assert( sizeof( AzureIoTMQTTConnectInfo_t ) == sizeof( MQTTConnectInfo_t ) );
    assert( sizeof( AzureIoTMQTTSubscribeInfo_t ) == sizeof( MQTTSubscribeInfo_t ) );
    assert( sizeof( AzureIoTMQTTPacketInfo_t ) == sizeof( MQTTPacketInfo_t ) );
    assert( sizeof( AzureIoTMQTTPublishInfo_t ) == sizeof( MQTTPublishInfo_t ) );
    assert( sizeof( AzureIoTMQTTStatus_t ) == sizeof(MQTTStatus_t) );

    return ( MQTTStatus_t ) MQTT_Init( &( pContext->_internal.context ),
                                       pTransportInterface,
                                       ( MQTTGetCurrentTimeFunc_t )getTimeFunction,
                                       ( MQTTEventCallback_t ) userCallback,
                                       &buffer );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Connect( AzureIoTMqttHandle_t  pContext,
                                           const AzureIoTMQTTConnectInfo_t * pConnectInfo,
                                           const AzureIoTMQTTPublishInfo_t * pWillInfo,
                                           uint32_t timeoutMs,
                                           bool * pSessionPresent )
{
    return ( MQTTStatus_t ) MQTT_Connect( &( pContext->_internal.context ),
                                          ( const MQTTConnectInfo_t * ) pConnectInfo,
                                          ( const MQTTPublishInfo_t * ) pWillInfo,
                                          timeoutMs, pSessionPresent );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Subscribe( AzureIoTMqttHandle_t  pContext,
                                             const AzureIoTMQTTSubscribeInfo_t * pSubscriptionList,
                                             size_t subscriptionCount,
                                             uint16_t packetId )
{
    return ( MQTTStatus_t ) MQTT_Subscribe( &( pContext->_internal.context ), ( const MQTTSubscribeInfo_t * )pSubscriptionList,
                                            subscriptionCount, packetId);
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Publish( AzureIoTMqttHandle_t  pContext,
                                           const AzureIoTMQTTPublishInfo_t * pPublishInfo,
                                           uint16_t packetId )
{
    return ( MQTTStatus_t ) MQTT_Publish( &( pContext->_internal.context ),
                                          ( const MQTTPublishInfo_t * ) pPublishInfo,
                                          packetId );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Ping( AzureIoTMqttHandle_t  pContext )
{
    return ( MQTTStatus_t ) MQTT_Ping( &( pContext->_internal.context ) );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Unsubscribe( AzureIoTMqttHandle_t  pContext,
                                               const AzureIoTMQTTSubscribeInfo_t * pSubscriptionList,
                                               size_t subscriptionCount,
                                               uint16_t packetId )
{
    return ( MQTTStatus_t ) MQTT_Unsubscribe( &( pContext->_internal.context ),
                                              ( const MQTTSubscribeInfo_t * )pSubscriptionList,
                                              subscriptionCount, packetId );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_Disconnect( AzureIoTMqttHandle_t  pContext )
{
    return ( MQTTStatus_t ) MQTT_Disconnect( &( pContext->_internal.context ) );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_ProcessLoop( AzureIoTMqttHandle_t  pContext,
                                               uint32_t timeoutMs )
{
    return ( MQTTStatus_t ) MQTT_ProcessLoop( &( pContext->_internal.context ), timeoutMs );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_ReceiveLoop( AzureIoTMqttHandle_t  pContext,
                                               uint32_t timeoutMs )
{
    return ( MQTTStatus_t ) MQTT_ReceiveLoop( &( pContext->_internal.context ), timeoutMs );
}

uint16_t AzureIoTMQTT_GetPacketId( AzureIoTMqttHandle_t  pContext )
{
    return ( MQTTStatus_t ) MQTT_GetPacketId( &( pContext->_internal.context ) );
}

AzureIoTMQTTStatus_t AzureIoTMQTT_GetSubAckStatusCodes( const AzureIoTMQTTPacketInfo_t * pSubackPacket,
                                                        uint8_t ** pPayloadStart,
                                                        size_t * pPayloadSize )
{
    return ( MQTTStatus_t ) MQTT_GetSubAckStatusCodes( (const MQTTPacketInfo_t * ) pSubackPacket,
                                                       pPayloadStart, pPayloadSize );
}