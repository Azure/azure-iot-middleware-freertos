/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_provisioning_client.h"
/*-----------------------------------------------------------*/

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern const uint8_t * pucPublishPayload;
/*-----------------------------------------------------------*/

static const uint8_t ucEndpoint[] = "unittest.azure-devices-provisioning.net";
static const uint8_t ucIdScope[] = "0ne000A247E";
static const uint8_t ucRegistrationId[] = "UnitTest";
static const uint8_t ucDeviceId[] = "UnitTest";
static const uint8_t ucHubEndpoint[] = "unittest.azure-iothub.com";
static const uint8_t ucSymmetricKey[] = "ABC12345";
static const uint8_t ucTestProvisioningServiceResponse[] = "$dps/registrations/res/202/?";
static uint8_t ucBuffer[ 1024 ];
static uint32_t ulExpectedExtendedCode = 400207;
static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};
static const uint8_t ucAssignedHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"assigned\",\"registrationState\": \
         {\"registrationId\":\"reg_id\",\"createdDateTimeUtc\":\"2019-12-27T19:51:41.6630592Z\",\"assignedHub\":\"unittest.azure-iothub.com\", \
          \"deviceId\":\"UnitTest\",\"status\":\"assigned\",\"substatus\":\"initialAssignment\",\"lastUpdatedDateTimeUtc\":\"2019-12-27T19:51:41.8579703Z\", \
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

static const uint8_t ucShortFailureHubResponse[] = "{ \
    \"errorCode\":429001,\"trackingId\":\"\",\"message\":\"Operations are being throttled for this tenant.\", \
    \"timestampUtc\":\"2021-11-10T03:03:52.9034249Z\" \
}";

static const uint8_t ucDisabledHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"disabled\",\"registrationState\": \
         {\"registrationId\":\"reg_id\", \"status\":\"disabled\" }\
}";

/* Invalid JSON: status is invalid, hostname not found and deviceId is not found. */
static const uint8_t ucInvalidHubResponse[] = "{ \
    \"operationId\":\"4.002305f54fc89692.b1f11200-8776-4b5d-867b-dc21c4b59c12\",\"status\":\"Uknown\",\"registrationState\": \
         {\"registrationId\":\"reg_id\",\"createdDateTimeUtc\":\"2019-12-27T19:51:41.6630592Z\",\"assignedHub\":\"unittest.azure-iothub.com\", \
          \"deviceId\":\"UnitTest\",\"status\":\"Uknown\",\"substatus\":\"initialAssignment\",\"lastUpdatedDateTimeUtc\":\"2019-12-27T19:51:41.8579703Z\", \
          \"etag\":\"XXXXXXXXXXX=\"\
         }\
}";
static const uint8_t ucCustomPayload[] = "{\"modelId\":\"UnitTest\"}";
static uint8_t ucTopicBuffer[ 128 ];
static uint32_t ulRequestId = 1;
static uint64_t ullUnixTime = 0;
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();
/*-----------------------------------------------------------*/

static uint32_t prvHmacFunction( const uint8_t * pucKey,
                                 uint32_t ulKeyLength,
                                 const uint8_t * pucData,
                                 uint32_t ulDataLength,
                                 uint8_t * pucOutput,
                                 uint32_t ulOutputLength,
                                 uint32_t * pucBytesCopied );

TickType_t xTaskGetTickCount( void )
{
    return 1;
}
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    return ullUnixTime;
}
/*-----------------------------------------------------------*/

static uint32_t prvHmacFunction( const uint8_t * pucKey,
                                 uint32_t ulKeyLength,
                                 const uint8_t * pucData,
                                 uint32_t ulDataLength,
                                 uint8_t * pucOutput,
                                 uint32_t ulOutputLength,
                                 uint32_t * pucBytesCopied )
{
    ( void ) pucKey;
    ( void ) ulKeyLength;
    ( void ) pucData;
    ( void ) ulDataLength;
    ( void ) pucOutput;
    ( void ) ulOutputLength;
    ( void ) pucBytesCopied;

    return( ( uint32_t ) mock() );
}
/*-----------------------------------------------------------*/

static void prvGenerateResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo,
                                 uint32_t ulAssignedResponseAfter,
                                 const uint8_t * pucAssignedResponse,
                                 uint32_t ulAssignedResponseLength )
{
    int xLength;

    if( ulAssignedResponseAfter == 0 )
    {
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
        xDeserializedInfo.usPacketIdentifier = 1;
        xLength = snprintf( ( char * ) ucTopicBuffer, sizeof( ucTopicBuffer ),
                            "%s$rid=%u", ucTestProvisioningServiceResponse,
                            ulRequestId );
        pxPublishInfo->usTopicNameLength = ( uint16_t ) xLength;
        pxPublishInfo->pcTopicName = ( const uint8_t * ) ucTopicBuffer;
        pxPublishInfo->pvPayload = pucAssignedResponse;
        pxPublishInfo->xPayloadLength = ulAssignedResponseLength;
        xDeserializedInfo.pxPublishInfo = pxPublishInfo;
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    }
    else
    {
        xPacketInfo.ucType = azureiotmqttPACKET_TYPE_PUBLISH;
        xDeserializedInfo.usPacketIdentifier = 1;
        xLength = snprintf( ( char * ) ucTopicBuffer, sizeof( ucTopicBuffer ),
                            "%s$rid=%u&retry-after=%u",
                            ucTestProvisioningServiceResponse,
                            ulRequestId, ulAssignedResponseAfter );
        pxPublishInfo->usTopicNameLength = ( uint16_t ) xLength;
        pxPublishInfo->pcTopicName = ( const uint8_t * ) ucTopicBuffer;
        pxPublishInfo->pvPayload = ucAssigningHubResponse;
        pxPublishInfo->xPayloadLength = sizeof( ucAssigningHubResponse ) - 1;
        xDeserializedInfo.pxPublishInfo = pxPublishInfo;
        will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    }
}
/*-----------------------------------------------------------*/

static void prvGenerateShortFailureResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    prvGenerateResponse(
        pxPublishInfo,
        0,
        ucShortFailureHubResponse,
        sizeof( ucShortFailureHubResponse ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvGenerateDisabledResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    prvGenerateResponse(
        pxPublishInfo,
        0,
        ucDisabledHubResponse,
        sizeof( ucDisabledHubResponse ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvGenerateFailureResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    prvGenerateResponse(
        pxPublishInfo,
        0,
        ucFailureHubResponse,
        sizeof( ucFailureHubResponse ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvGenerateInvalidResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo )
{
    prvGenerateResponse( pxPublishInfo,
                         0,
                         ucInvalidHubResponse,
                         sizeof( ucInvalidHubResponse ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvGenerateGoodResponse( AzureIoTMQTTPublishInfo_t * pxPublishInfo,
                                     uint32_t ulAssignedResponseAfter )
{
    prvGenerateResponse( pxPublishInfo,
                         ulAssignedResponseAfter,
                         ucAssignedHubResponse,
                         sizeof( ucAssignedHubResponse ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvSetupTestProvisioningClient( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    AzureIoTProvisioningClientOptions_t xProvisioningOptions = { 0 };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Init( pxTestProvisioningClient,
                                                       &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                       &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                       &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                       &xProvisioningOptions, ucBuffer, sizeof( ucBuffer ),
                                                       prvGetUnixTime,
                                                       &xTransportInterface ),
                      eAzureIoTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_SetSymmetricKey( pxTestProvisioningClient,
                                                                  ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                                  prvHmacFunction ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void prvRegistrationConnectStep( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    xPacketInfo.ucType = 0;
    will_return( prvHmacFunction, 0 );
    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );
}
/*-----------------------------------------------------------*/

static void prvRegistrationSubscribeStep( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    xPacketInfo.ucType = 0;
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );
}
/*-----------------------------------------------------------*/

static void prvRegistrationAckSubscribeStep( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );
}
/*-----------------------------------------------------------*/

static void prvRegistrationPublishStep( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );
}
/*-----------------------------------------------------------*/

static void prvRegister( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    /* Connect */
    prvRegistrationConnectStep( pxTestProvisioningClient );

    /* Subscribe */
    prvRegistrationSubscribeStep( pxTestProvisioningClient );

    /* AckSubscribe */
    prvRegistrationAckSubscribeStep( pxTestProvisioningClient );

    /* Publish Registration Request */
    prvRegistrationPublishStep( pxTestProvisioningClient );

    /* Registration response */
    prvGenerateGoodResponse( &xPublishInfo, 1 );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Read response */
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Wait for timeout */
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    ullUnixTime += 2;
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );
}
/*-----------------------------------------------------------*/

static void prvQuery( AzureIoTProvisioningClient_t * pxTestProvisioningClient )
{
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Registration response */
    prvGenerateGoodResponse( &xPublishInfo, 0 );
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Process response */
    assert_int_equal( AzureIoTProvisioningClient_Register( pxTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Init_Failure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    /* Fail init when null endpoint passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           NULL, 0,
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when null IDScope passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           NULL, 0,
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when null RegistrationId passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           NULL, 0,
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when null Buffer passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, NULL, 0,
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when null null AzureIoTGetCurrentTimeFunc_t passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           NULL,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when null Transport passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           prvGetUnixTime,
                                                           NULL ),
                          eAzureIoTSuccess );

    /* Fail when smaller buffer is passed */
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, azureiotconfigUSERNAME_MAX,
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );

    /* Fail init when AzureIoTMQTT_Init fails */
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTNoMemory );
    assert_int_not_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                           &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                           &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                           &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                           NULL, ucBuffer, sizeof( ucBuffer ),
                                                           prvGetUnixTime,
                                                           &xTransportInterface ),
                          eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Init_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;
    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                       &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                       &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                       &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                       NULL, ucBuffer, sizeof( ucBuffer ),
                                                       prvGetUnixTime,
                                                       &xTransportInterface ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Deinit_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTProvisioningClient_Init( &xTestProvisioningClient,
                                                       &ucEndpoint[ 0 ], sizeof( ucEndpoint ),
                                                       &ucIdScope[ 0 ], sizeof( ucIdScope ),
                                                       &ucRegistrationId[ 0 ], sizeof( ucRegistrationId ),
                                                       NULL, ucBuffer, sizeof( ucBuffer ),
                                                       prvGetUnixTime,
                                                       &xTransportInterface ),
                      eAzureIoTSuccess );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_SymmetricKeySet_Failure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    /* Fail AzureIoTProvisioningClient_SetSymmetricKey when null symmetric key is passed */
    assert_int_not_equal( AzureIoTProvisioningClient_SetSymmetricKey( &xTestProvisioningClient,
                                                                      NULL, 0,
                                                                      prvHmacFunction ),
                          eAzureIoTSuccess );

    /* Fail AzureIoTProvisioningClient_SetSymmetricKey when null hashing function is passed */
    assert_int_not_equal( AzureIoTProvisioningClient_SetSymmetricKey( &xTestProvisioningClient,
                                                                      ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                                      NULL ),
                          eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_SymmetricKeySet_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    assert_int_equal( AzureIoTProvisioningClient_SetSymmetricKey( &xTestProvisioningClient,
                                                                  ucSymmetricKey, sizeof( ucSymmetricKey ) - 1,
                                                                  prvHmacFunction ),
                      eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_ConnectFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Connect */
    will_return( prvHmacFunction, 0 );
    will_return( AzureIoTMQTT_Connect, eAzureIoTMQTTServerRefused );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorServerError );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_SubscribeFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Connect */
    prvRegistrationConnectStep( &xTestProvisioningClient );

    /* Subscribe */
    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorSubscribeFailed );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_SubscribeAckFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Connect */
    prvRegistrationConnectStep( &xTestProvisioningClient );

    /* Subscribe */
    prvRegistrationSubscribeStep( &xTestProvisioningClient );

    /* AckSubscribe */
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTServerRefused );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorFailed );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_PublishFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Connect */
    prvRegistrationConnectStep( &xTestProvisioningClient );

    /* Subscribe */
    prvRegistrationSubscribeStep( &xTestProvisioningClient );

    /* AckSubscribe */
    prvRegistrationAckSubscribeStep( &xTestProvisioningClient );

    /* Publish Registration Request */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPublishFailed );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_RegistrationFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;
    uint32_t ulExtendedCode;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Connect */
    prvRegistrationConnectStep( &xTestProvisioningClient );

    /* Subscribe */
    prvRegistrationSubscribeStep( &xTestProvisioningClient );

    /* AckSubscribe */
    prvRegistrationAckSubscribeStep( &xTestProvisioningClient );

    /* Publish Registration Request */
    prvRegistrationPublishStep( &xTestProvisioningClient );

    /* Registration response */
    prvGenerateFailureResponse( &xPublishInfo );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Read response */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorServerError );

    /* Read extended code */
    assert_int_equal( AzureIoTProvisioningClient_GetExtendedCode( &xTestProvisioningClient,
                                                                  &ulExtendedCode ),
                      eAzureIoTSuccess );
    assert_int_equal( ulExpectedExtendedCode, ulExtendedCode );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_QueryPublishFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );
    prvRegister( &xTestProvisioningClient );

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSendFailed );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPublishFailed );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_QueryShortFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );
    prvRegister( &xTestProvisioningClient );

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Invalid registration response */
    prvGenerateShortFailureResponse( &xPublishInfo );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Process invalid response */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorServerError );
    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_QueryFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );
    prvRegister( &xTestProvisioningClient );

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Invalid registration response */
    prvGenerateFailureResponse( &xPublishInfo );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Process invalid response */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorServerError );
    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/



static void testAzureIoTProvisioningClient_Register_QueryDeviceDisabledResponseFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );
    prvRegister( &xTestProvisioningClient );

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Invalid registration response */
    prvGenerateDisabledResponse( &xPublishInfo );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Process invalid response */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorServerError );
    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_QueryInvalidResponseFailure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    AzureIoTMQTTPublishInfo_t xPublishInfo;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );
    prvRegister( &xTestProvisioningClient );

    /* Publish Registration Query */
    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = 0;
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Invalid registration response */
    prvGenerateInvalidResponse( &xPublishInfo );
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorPending );

    /* Process invalid response */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorFailed );
    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_Register_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    prvRegister( &xTestProvisioningClient );
    prvQuery( &xTestProvisioningClient );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_GetDeviceAndHub_Failure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    uint8_t ucTestDevice[ 128 ];
    uint32_t ulTestDeviceLength = sizeof( ucTestDevice );
    uint8_t ucTestHostname[ 128 ];
    uint32_t ulTestHostnameLength = sizeof( ucTestHostname );

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    /* Registration is not yet started */
    assert_int_not_equal( AzureIoTProvisioningClient_GetDeviceAndHub( &xTestProvisioningClient,
                                                                      ucTestHostname, &ulTestHostnameLength,
                                                                      ucTestDevice, &ulTestDeviceLength ),
                          eAzureIoTSuccess );

    prvRegister( &xTestProvisioningClient );
    prvQuery( &xTestProvisioningClient );

    /* Not enough memory passed to copy the result */
    ulTestDeviceLength = 0;
    assert_int_not_equal( AzureIoTProvisioningClient_GetDeviceAndHub( &xTestProvisioningClient,
                                                                      ucTestHostname, &ulTestHostnameLength,
                                                                      ucTestDevice, &ulTestDeviceLength ),
                          eAzureIoTSuccess );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_GetDeviceAndHub_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    uint8_t ucTestDevice[ 128 ];
    uint32_t ulTestDeviceLength = sizeof( ucTestDevice );
    uint8_t ucTestHostname[ 128 ];
    uint32_t ulTestHostnameLength = sizeof( ucTestHostname );

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    prvRegister( &xTestProvisioningClient );
    prvQuery( &xTestProvisioningClient );

    assert_int_equal( AzureIoTProvisioningClient_GetDeviceAndHub( &xTestProvisioningClient,
                                                                  ucTestHostname, &ulTestHostnameLength,
                                                                  ucTestDevice, &ulTestDeviceLength ),
                      eAzureIoTSuccess );

    assert_int_equal( ulTestDeviceLength, sizeof( ucDeviceId ) - 1 );
    assert_memory_equal( ucTestDevice, ucDeviceId, ulTestDeviceLength );
    assert_int_equal( ulTestHostnameLength, sizeof( ucHubEndpoint ) - 1 );
    assert_memory_equal( ucTestHostname, ucHubEndpoint, ulTestHostnameLength );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_WithCustomPayload_Failure( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    uint8_t ucTestDevice[ 128 ];
    uint32_t ulTestDeviceLength = sizeof( ucTestDevice );
    uint8_t ucTestHostname[ 128 ];
    uint32_t ulTestHostnameLength = sizeof( ucTestHostname );

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    assert_int_equal( AzureIoTProvisioningClient_SetRegistrationPayload( &xTestProvisioningClient,
                                                                         ucCustomPayload, sizeof( ucCustomPayload ) - 2 ),
                      eAzureIoTSuccess );

    /* Connect */
    prvRegistrationConnectStep( &xTestProvisioningClient );

    /* Subscribe */
    prvRegistrationSubscribeStep( &xTestProvisioningClient );

    /* AckSubscribe */
    prvRegistrationAckSubscribeStep( &xTestProvisioningClient );

    /* Publish payload generation failed */
    assert_int_equal( AzureIoTProvisioningClient_Register( &xTestProvisioningClient,
                                                           azureiotprovisioningNO_WAIT ),
                      eAzureIoTErrorFailed );

    /* After failed registration, AzureIoTProvisioningClient_GetDeviceAndHub should return error */
    assert_int_not_equal( AzureIoTProvisioningClient_GetDeviceAndHub( &xTestProvisioningClient,
                                                                      ucTestHostname, &ulTestHostnameLength,
                                                                      ucTestDevice, &ulTestDeviceLength ),
                          eAzureIoTSuccess );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

static void testAzureIoTProvisioningClient_WithCustomPayload_Success( void ** ppvState )
{
    AzureIoTProvisioningClient_t xTestProvisioningClient;
    uint8_t ucTestDevice[ 128 ];
    uint32_t ulTestDeviceLength = sizeof( ucTestDevice );
    uint8_t ucTestHostname[ 128 ];
    uint32_t ulTestHostnameLength = sizeof( ucTestHostname );

    ( void ) ppvState;

    prvSetupTestProvisioningClient( &xTestProvisioningClient );

    assert_int_equal( AzureIoTProvisioningClient_SetRegistrationPayload( &xTestProvisioningClient,
                                                                         ucCustomPayload, sizeof( ucCustomPayload ) - 1 ),
                      eAzureIoTSuccess );

    prvRegister( &xTestProvisioningClient );
    prvQuery( &xTestProvisioningClient );

    assert_int_equal( AzureIoTProvisioningClient_GetDeviceAndHub( &xTestProvisioningClient,
                                                                  ucTestHostname, &ulTestHostnameLength,
                                                                  ucTestDevice, &ulTestDeviceLength ),
                      eAzureIoTSuccess );

    assert_int_equal( ulTestDeviceLength, sizeof( ucDeviceId ) - 1 );
    assert_memory_equal( ucTestDevice, ucDeviceId, ulTestDeviceLength );
    assert_int_equal( ulTestHostnameLength, sizeof( ucHubEndpoint ) - 1 );
    assert_memory_equal( ucTestHostname, ucHubEndpoint, ulTestHostnameLength );

    AzureIoTProvisioningClient_Deinit( &xTestProvisioningClient );
}
/*-----------------------------------------------------------*/

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTProvisioningClient_Init_Failure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Init_Success ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Deinit_Success ),
        cmocka_unit_test( testAzureIoTProvisioningClient_SymmetricKeySet_Failure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_SymmetricKeySet_Success ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_ConnectFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_SubscribeFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_SubscribeAckFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_PublishFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_RegistrationFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_QueryPublishFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_QueryShortFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_QueryFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_QueryDeviceDisabledResponseFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_QueryInvalidResponseFailure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_Register_Success ),
        cmocka_unit_test( testAzureIoTProvisioningClient_GetDeviceAndHub_Failure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_GetDeviceAndHub_Success ),
        cmocka_unit_test( testAzureIoTProvisioningClient_WithCustomPayload_Failure ),
        cmocka_unit_test( testAzureIoTProvisioningClient_WithCustomPayload_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_provisioning_client_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
