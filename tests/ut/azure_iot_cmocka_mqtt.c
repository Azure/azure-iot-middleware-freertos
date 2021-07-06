/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_cmocka_mqtt.c
 * @brief Unit test dummy MQTT port.
 *
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
/*-----------------------------------------------------------*/

AzureIoTMQTTEventCallback_t xMiddlewareCallback = NULL;
AzureIoTMQTTPacketInfo_t xPacketInfo;
AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
uint16_t usTestPacketId = 1;
const uint8_t * pucPublishPayload = NULL;
uint16_t usSentQOS = 0xFF;
uint32_t ulDelayReceivePacket = 0;
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Init( AzureIoTMQTTHandle_t xContext,
                                        const AzureIoTTransportInterface_t * pxTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t xGetTimeFunction,
                                        AzureIoTMQTTEventCallback_t xUserCallback,
                                        uint8_t * pucNetworkBuffer,
                                        size_t xNetworkBufferLength )
{
    ( void ) xContext;
    ( void ) pxTransportInterface;
    ( void ) xGetTimeFunction;
    ( void ) pucNetworkBuffer;
    ( void ) xNetworkBufferLength;

    xMiddlewareCallback = xUserCallback;
    return ( AzureIoTMQTTResult_t ) mock();
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Connect( AzureIoTMQTTHandle_t xContext,
                                           const AzureIoTMQTTConnectInfo_t * pxConnectInfo,
                                           const AzureIoTMQTTPublishInfo_t * pxWillInfo,
                                           uint32_t ulMilliseconds,
                                           bool * pxSessionPresent )
{
    ( void ) xContext;
    ( void ) pxConnectInfo;
    ( void ) pxWillInfo;
    ( void ) ulMilliseconds;
    ( void ) pxSessionPresent;

    return ( AzureIoTMQTTResult_t ) mock();
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Subscribe( AzureIoTMQTTHandle_t xContext,
                                             const AzureIoTMQTTSubscribeInfo_t * pxSubscriptionList,
                                             size_t xSubscriptionCount,
                                             uint16_t usPacketId )
{
    ( void ) xContext;
    ( void ) pxSubscriptionList;
    ( void ) xSubscriptionCount;
    ( void ) usPacketId;

    return ( AzureIoTMQTTResult_t ) mock();
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Publish( AzureIoTMQTTHandle_t xContext,
                                           const AzureIoTMQTTPublishInfo_t * pxPublishInfo,
                                           uint16_t usPacketId )
{
    ( void ) xContext;
    ( void ) usPacketId;

    AzureIoTMQTTResult_t xReturn = ( AzureIoTMQTTResult_t ) mock();

    if( xReturn )
    {
        return xReturn;
    }

    if( pucPublishPayload )
    {
        assert_memory_equal( pxPublishInfo->pvPayload, pucPublishPayload, pxPublishInfo->xPayloadLength );
    }

    if( usSentQOS != 0xFF )
    {
        assert_int_equal( usSentQOS, pxPublishInfo->xQOS );
    }

    usSentQOS = 0xFF; /* Reset to wrong value after checking */

    return xReturn;
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Unsubscribe( AzureIoTMQTTHandle_t xContext,
                                               const AzureIoTMQTTSubscribeInfo_t * pxSubscriptionList,
                                               size_t xSubscriptionCount,
                                               uint16_t usPacketId )
{
    ( void ) xContext;
    ( void ) pxSubscriptionList;
    ( void ) xSubscriptionCount;
    ( void ) usPacketId;

    return ( AzureIoTMQTTResult_t ) mock();
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_Disconnect( AzureIoTMQTTHandle_t xContext )
{
    ( void ) xContext;

    return ( AzureIoTMQTTResult_t ) mock();
}
/*-----------------------------------------------------------*/

AzureIoTMQTTResult_t AzureIoTMQTT_ProcessLoop( AzureIoTMQTTHandle_t xContext,
                                               uint32_t ulMilliseconds )
{
    ( void ) xContext;
    ( void ) ulMilliseconds;

    AzureIoTMQTTResult_t xReturn = ( AzureIoTMQTTResult_t ) mock();

    if( xReturn )
    {
        return xReturn;
    }

    if( ulDelayReceivePacket > ulMilliseconds )
    {
        ulDelayReceivePacket -= ulMilliseconds;
        return xReturn;
    }

    if( xMiddlewareCallback && ( xPacketInfo.ucType != 0 ) )
    {
        xMiddlewareCallback( xContext, &xPacketInfo, &xDeserializedInfo );
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

uint16_t AzureIoTMQTT_GetPacketId( AzureIoTMQTTHandle_t xContext )
{
    ( void ) xContext;

    return usTestPacketId;
}
