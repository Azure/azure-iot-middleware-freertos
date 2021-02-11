/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_mqtt.h
 *
 */

#ifndef AZURE_IOT_MQTT_H
#define AZURE_IOT_MQTT_H

#include <stdint.h>
#include <stddef.h>

#if defined( __cplusplus ) || ( defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901L ) )
    #include <stdbool.h>
#elif !defined( bool ) && !defined( false ) && !defined( true )
    #define bool     int8_t
    #define false    ( int8_t ) 0
    #define true     ( int8_t ) 1
#endif

#include "transport_interface.h"

#define AZURE_IOT_MQTT_PACKET_TYPE_CONNECT        ( ( uint8_t ) 0x10U )  /**< @brief CONNECT (client-to-server). */
#define AZURE_IOT_MQTT_PACKET_TYPE_CONNACK        ( ( uint8_t ) 0x20U )  /**< @brief CONNACK (server-to-client). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PUBLISH        ( ( uint8_t ) 0x30U )  /**< @brief PUBLISH (bidirectional). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PUBACK         ( ( uint8_t ) 0x40U )  /**< @brief PUBACK (bidirectional). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PUBREC         ( ( uint8_t ) 0x50U )  /**< @brief PUBREC (bidirectional). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PUBREL         ( ( uint8_t ) 0x62U )  /**< @brief PUBREL (bidirectional). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PUBCOMP        ( ( uint8_t ) 0x70U )  /**< @brief PUBCOMP (bidirectional). */
#define AZURE_IOT_MQTT_PACKET_TYPE_SUBSCRIBE      ( ( uint8_t ) 0x82U )  /**< @brief SUBSCRIBE (client-to-server). */
#define AZURE_IOT_MQTT_PACKET_TYPE_SUBACK         ( ( uint8_t ) 0x90U )  /**< @brief SUBACK (server-to-client). */
#define AZURE_IOT_MQTT_PACKET_TYPE_UNSUBSCRIBE    ( ( uint8_t ) 0xA2U )  /**< @brief UNSUBSCRIBE (client-to-server). */
#define AZURE_IOT_MQTT_PACKET_TYPE_UNSUBACK       ( ( uint8_t ) 0xB0U )  /**< @brief UNSUBACK (server-to-client). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PINGREQ        ( ( uint8_t ) 0xC0U )  /**< @brief PINGREQ (client-to-server). */
#define AZURE_IOT_MQTT_PACKET_TYPE_PINGRESP       ( ( uint8_t ) 0xD0U )  /**< @brief PINGRESP (server-to-client). */
#define AZURE_IOT_MQTT_PACKET_TYPE_DISCONNECT     ( ( uint8_t ) 0xE0U )  /**< @brief DISCONNECT (client-to-server). */


typedef enum AzureIoTMQTTQoS
{
    AzureIoTMQTTQoS0 = 0, /**< Delivery at most once. */
    AzureIoTMQTTQoS1 = 1, /**< Delivery at least once. */
    AzureIoTMQTTQoS2 = 2  /**< Delivery exactly once. */
} AzureIoTMQTTQoS_t;

typedef enum AzureIoTMQTTStatus
{
    AzureIoTMQTTSuccess = 0,     /**< Function completed successfully. */
    AzureIoTMQTTBadParameter,    /**< At least one parameter was invalid. */
    AzureIoTMQTTNoMemory,        /**< A provided buffer was too small. */
    AzureIoTMQTTSendFailed,      /**< The transport send function failed. */
    AzureIoTMQTTRecvFailed,      /**< The transport receive function failed. */
    AzureIoTMQTTBadResponse,     /**< An invalid packet was received from the server. */
    AzureIoTMQTTServerRefused,   /**< The server refused a CONNECT or SUBSCRIBE. */
    AzureIoTMQTTNoDataAvailable, /**< No data available from the transport interface. */
    AzureIoTMQTTIllegalState,    /**< An illegal state in the state record. */
    AzureIoTMQTTStateCollision,  /**< A collision with an existing state record entry. */
    AzureIoTMQTTKeepAliveTimeout /**< Timeout while waiting for PINGRESP. */
} AzureIoTMQTTStatus_t;

typedef struct AzureIoTMQTTConnectInfo
{
    /**
     * @brief Whether to establish a new, clean session or resume a previous session.
     */
    bool cleanSession;

    /**
     * @brief MQTT keep alive period.
     */
    uint16_t keepAliveSeconds;

    /**
     * @brief MQTT client identifier. Must be unique per client.
     */
    const char * pClientIdentifier;

    /**
     * @brief Length of the client identifier.
     */
    uint16_t clientIdentifierLength;

    /**
     * @brief MQTT user name. Set to NULL if not used.
     */
    const char * pUserName;

    /**
     * @brief Length of MQTT user name. Set to 0 if not used.
     */
    uint16_t userNameLength;

    /**
     * @brief MQTT password. Set to NULL if not used.
     */
    const char * pPassword;

    /**
     * @brief Length of MQTT password. Set to 0 if not used.
     */
    uint16_t passwordLength;
} AzureIoTMQTTConnectInfo_t;

typedef struct AzureIoTMQTTSubscribeInfo
{
    /**
     * @brief Quality of Service for subscription.
     */
    AzureIoTMQTTQoS_t qos;

    /**
     * @brief Topic filter to subscribe to.
     */
    const char * pTopicFilter;

    /**
     * @brief Length of subscription topic filter.
     */
    uint16_t topicFilterLength;
} AzureIoTMQTTSubscribeInfo_t;

typedef struct AzureIoTMQTTPublishInfo
{
    /**
     * @brief Quality of Service for message.
     */
    AzureIoTMQTTQoS_t qos;

    /**
     * @brief Whether this is a retained message.
     */
    bool retain;

    /**
     * @brief Whether this is a duplicate publish message.
     */
    bool dup;

    /**
     * @brief Topic name on which the message is published.
     */
    const char * pTopicName;

    /**
     * @brief Length of topic name.
     */
    uint16_t topicNameLength;

    /**
     * @brief Message payload.
     */
    const void * pPayload;

    /**
     * @brief Message payload length.
     */
    size_t payloadLength;
} AzureIoTMQTTPublishInfo_t;

typedef struct AzureIoTMQTTDeserializedInfo
{
    uint16_t packetIdentifier;                  /**< @brief Packet ID of deserialized packet. */
    AzureIoTMQTTPublishInfo_t * pPublishInfo;   /**< @brief Pointer to deserialized publish info. */
    AzureIoTMQTTStatus_t deserializationResult; /**< @brief Return code of deserialization. */
} AzureIoTMQTTDeserializedInfo_t;

typedef struct AzureIoTMQTTPacketInfo
{
    /**
     * @brief Type of incoming MQTT packet.
     */
    uint8_t type;

    /**
     * @brief Remaining serialized data in the MQTT packet.
     */
    uint8_t * pRemainingData;

    /**
     * @brief Length of remaining serialized data.
     */
    size_t remainingLength;
} AzureIoTMQTTPacketInfo_t;

typedef struct AzureIoTMQTT AzureIoTMQTT_t;
typedef struct AzureIoTMQTT * AzureIoTMQTTHandle_t;

typedef uint32_t (* AzureIoTMQTTGetCurrentTimeFunc_t )( void );

typedef void (* AzureIoTMQTTEventCallback_t )( AzureIoTMQTTHandle_t  pContext,
                                               struct AzureIoTMQTTPacketInfo * pPacketInfo,
                                               struct AzureIoTMQTTDeserializedInfo * pDeserializedInfo );


AzureIoTMQTTStatus_t AzureIoTMQTT_Init( AzureIoTMQTTHandle_t  pContext,
                                        const TransportInterface_t * pTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t getTimeFunction,
                                        AzureIoTMQTTEventCallback_t userCallback,
                                        uint8_t * pNetworkBuffer, uint32_t networkBufferLength );

AzureIoTMQTTStatus_t AzureIoTMQTT_Connect( AzureIoTMQTTHandle_t  pContext,
                                           const AzureIoTMQTTConnectInfo_t * pConnectInfo,
                                           const AzureIoTMQTTPublishInfo_t * pWillInfo,
                                           uint32_t timeoutMs,
                                           bool * pSessionPresent );

AzureIoTMQTTStatus_t AzureIoTMQTT_Subscribe( AzureIoTMQTTHandle_t pContext,
                                             const AzureIoTMQTTSubscribeInfo_t * pSubscriptionList,
                                             size_t subscriptionCount,
                                             uint16_t packetId );

AzureIoTMQTTStatus_t AzureIoTMQTT_Publish( AzureIoTMQTTHandle_t  pContext,
                                           const AzureIoTMQTTPublishInfo_t * pPublishInfo,
                                           uint16_t packetId );

AzureIoTMQTTStatus_t AzureIoTMQTT_Ping( AzureIoTMQTTHandle_t  pContext );

AzureIoTMQTTStatus_t AzureIoTMQTT_Unsubscribe( AzureIoTMQTTHandle_t  pContext,
                                               const AzureIoTMQTTSubscribeInfo_t * pSubscriptionList,
                                               size_t subscriptionCount,
                                               uint16_t packetId );

AzureIoTMQTTStatus_t AzureIoTMQTT_Disconnect( AzureIoTMQTTHandle_t  pContext );

AzureIoTMQTTStatus_t AzureIoTMQTT_ProcessLoop( AzureIoTMQTTHandle_t  pContext,
                                               uint32_t timeoutMs );

AzureIoTMQTTStatus_t AzureIoTMQTT_ReceiveLoop( AzureIoTMQTTHandle_t  pContext,
                                               uint32_t timeoutMs );

uint16_t AzureIoTMQTT_GetPacketId( AzureIoTMQTTHandle_t  pContext );

AzureIoTMQTTStatus_t AzureIoTMQTT_GetSubAckStatusCodes( const AzureIoTMQTTPacketInfo_t * pSubackPacket,
                                                        uint8_t ** pPayloadStart,
                                                        size_t * pPayloadSize );

#endif /* AZURE_IOT_MQTT_H */
