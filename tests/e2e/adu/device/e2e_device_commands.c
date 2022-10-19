/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "e2e_device_process_commands.h"

#include <time.h>

#include "azure_iot_http.h"
#include "azure_iot_transport_interface.h"
#include "transport_socket.h"
#include "azure_iot_json_writer.h"
#include "azure_iot_jws.h"
#include "azure_iot_hub_client_properties.h"
#include "azure/core/az_json.h"
#include "azure/core/az_span.h"

#define e2etestMESSAGE_BUFFER_SIZE                         ( 10240 )

#define e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_MSEC           ( 2 * 1000 )

#define e2etestE2E_TEST_SUCCESS                            ( 0 )
#define e2etestE2E_TEST_FAILED                             ( 1 )
#define e2etestE2E_TEST_NOT_FOUND                          ( 2 )

#define e2etestE2E_HMAC_MAX_SIZE                           ( 32 )

#define e2etestMETHOD_KEY                                  "method"
#define e2etestPAYLOAD_KEY                                 "payload"
#define e2etestPROPERTIES_KEY                              "properties"
#define e2etestID_SCOPE_KEY                                "id_scope"
#define e2etestREGISTRATION_ID_KEY                         "registration_id"
#define e2etestSYMMETRIC_KEY                               "symmetric_key"
#define e2etestSERVICE_ENDPOINT_KEY                        "service_endpoint"

#define e2etestE2E_TEST_ECHO_COMMAND                       "echo"
#define e2etestE2E_TEST_EXIT_COMMAND                       "exit"
#define e2etestE2E_TEST_SEND_INITIAL_ADU_STATE_COMMAND     "send_init_adu_state"
#define e2etestE2E_TEST_GET_ADU_TWIN_PROPERTIES_COMMAND    "get_adu_twin"
#define e2etestE2E_TEST_APPLY_ADU_UPDATE_COMMAND           "apply_update"
#define e2etestE2E_TEST_VERIFY_ADU_FINAL_STATE_COMMAND     "verify_final_state"
#define e2etestE2E_TEST_CONNECTED_MESSAGE                  "\"CONNECTED\""
#define e2etestE2E_TEST_ADU_PAYLOAD_RECEIVED               "\"ADU-RECEIVED\""
#define e2etestE2E_TEST_WAITING_FOR_ADU                    "\"Waiting for ADU\""

#define RETURN_IF_FAILED( e, msg )                    \
    if( e != e2etestE2E_TEST_SUCCESS )                \
    {                                                 \
        printf( "[%s][%d]%s : error code: 0x%0x\r\n", \
                __FILE__, __LINE__, msg, e );         \
        return ( e );                                 \
    }
/*-----------------------------------------------------------*/

struct E2E_TEST_COMMAND_STRUCT;
typedef uint32_t ( * EXECUTE_FN ) ( struct E2E_TEST_COMMAND_STRUCT * pxCMD,
                                    AzureIoTHubClient_t * pxAzureIoTHubClient,
                                    AzureIoTADUClient_t * pxAzureIoTAduClient );

typedef struct E2E_TEST_COMMAND_STRUCT
{
    EXECUTE_FN xExecute;
    const uint8_t * pulReceivedData;
    uint32_t ulReceivedDataLength;
} E2E_TEST_COMMAND;

typedef E2E_TEST_COMMAND * E2E_TEST_COMMAND_HANDLE;
/*-----------------------------------------------------------*/

static uint8_t * ucC2DCommandData = NULL;
static uint32_t ulC2DCommandDataLength = 0;
static AzureIoTHubClientPropertiesResponse_t * pxTwinMessage = NULL;
static AzureIoTADUUpdateRequest_t xAzureIoTAduUpdateRequest;
static uint8_t ucMethodNameBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer2[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucMessageBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static const uint8_t ucStatusOKTelemetry[] = "{\"status\": \"OK\"}";
static uint32_t ulContinueProcessingCMD = 1;
static uint8_t ucSharedBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint16_t usReceivedTelemetryPubackID;
/*-----------------------------------------------------------*/

/* ADU Values */
#define e2eADU_DEVICE_MANUFACTURER    "PC"
#define e2eADU_DEVICE_MODEL           "Linux-E2E"
#define e2eADU_UPDATE_PROVIDER        "ADU-E2E-Tests"
#define e2eADU_UPDATE_NAME            "Linux-E2E-Update"
#define e2eADU_UPDATE_VERSION         "1.0"
#define e2eADU_UPDATE_VERSION_NEW     "1.1"
#define e2eADU_UPDATE_ID              "{\"provider\":\"" e2eADU_UPDATE_PROVIDER "\",\"name\":\"" e2eADU_UPDATE_NAME "\",\"version\":\"" e2eADU_UPDATE_VERSION "\"}"
#define e2eADU_UPDATE_ID_NEW          "{\"provider\":\"" e2eADU_UPDATE_PROVIDER "\",\"name\":\"" e2eADU_UPDATE_NAME "\",\"version\":\"" e2eADU_UPDATE_VERSION_NEW "\"}"


static AzureIoTADUClientDeviceProperties_t xADUDeviceProperties =
{
    .ucManufacturer                           = ( const uint8_t * ) e2eADU_DEVICE_MANUFACTURER,
    .ulManufacturerLength                     = sizeof( e2eADU_DEVICE_MANUFACTURER ) - 1,
    .ucModel                                  = ( const uint8_t * ) e2eADU_DEVICE_MODEL,
    .ulModelLength                            = sizeof( e2eADU_DEVICE_MODEL ) - 1,
    .ucCurrentUpdateId                        = ( const uint8_t * ) e2eADU_UPDATE_ID,
    .ulCurrentUpdateIdLength                  = sizeof( e2eADU_UPDATE_ID ) - 1,
    .ucDeliveryOptimizationAgentVersion       = NULL,
    .ulDeliveryOptimizationAgentVersionLength = 0
};

static AzureIoTADUClientDeviceProperties_t xADUDevicePropertiesNew =
{
    .ucManufacturer                           = ( const uint8_t * ) e2eADU_DEVICE_MANUFACTURER,
    .ulManufacturerLength                     = sizeof( e2eADU_DEVICE_MANUFACTURER ) - 1,
    .ucModel                                  = ( const uint8_t * ) e2eADU_DEVICE_MODEL,
    .ulModelLength                            = sizeof( e2eADU_DEVICE_MODEL ) - 1,
    .ucCurrentUpdateId                        = ( const uint8_t * ) e2eADU_UPDATE_ID_NEW,
    .ulCurrentUpdateIdLength                  = sizeof( e2eADU_UPDATE_ID_NEW ) - 1,
    .ucDeliveryOptimizationAgentVersion       = NULL,
    .ulDeliveryOptimizationAgentVersionLength = 0
};

static AzureIoTHTTP_t xHTTPClient;
static bool xAduWasReceived = false;
static uint32_t ulReceivedTwinVersion;
static uint8_t ucAduManifestVerificationBuffer[ azureiotjwsSCRATCH_BUFFER_SIZE ];

/*-----------------------------------------------------------*/

/**
 * Telemetry PUBACK callback
 *
 **/
void prvTelemetryPubackCallback( uint16_t usPacketID )
{
    AZLogInfo( ( "Puback received for packet id in callback: 0x%08x", usPacketID ) );
    usReceivedTelemetryPubackID = usPacketID;
}

/**
 * Telemetry PUBACK callback
 *
 **/
static uint32_t prvWaitForPuback( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                  uint16_t usSentTelemetryPublishID )
{
    uint32_t xResult;

    AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient, e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_MSEC );

    LogInfo( ( "Checking PUBACK packet id: rec[%d] = sent[%d] ", usReceivedTelemetryPubackID, usSentTelemetryPublishID ) );

    if( usReceivedTelemetryPubackID == usSentTelemetryPublishID )
    {
        usReceivedTelemetryPubackID = 0; /* Reset received to 0 */
        xResult = 0;
    }
    else
    {
        xResult = 1;
    }

    return xResult;
}

/*-----------------------------------------------------------*/

/**
 * Free Twin data
 *
 **/
static void prvFreeTwinData( AzureIoTHubClientPropertiesResponse_t * pxTwinData )
{
    if( pxTwinData )
    {
        vPortFree( ( void * ) pxTwinData->pvMessagePayload );
        vPortFree( ( void * ) pxTwinData );
    }
}
/*-----------------------------------------------------------*/

/**
 * Free Method data
 *
 **/
static void prvFreeMethodData( AzureIoTHubClientCommandRequest_t * pxMethodData )
{
    if( pxMethodData )
    {
        vPortFree( ( void * ) pxMethodData->pvMessagePayload );
        vPortFree( ( void * ) pxMethodData->pucCommandName );
        vPortFree( ( void * ) pxMethodData->pucRequestID );
        vPortFree( ( void * ) pxMethodData );
    }
}
/*-----------------------------------------------------------*/

static uint32_t prvInitHttp( void )
{
    /*HTTP Connection */
    AzureIoTTransportInterface_t xHTTPTransport;
    NetworkContext_t xHTTPNetworkContext = { 0 };
    TlsTransportParams_t xHTTPTlsTransportParams = { 0 };

    /* Fill in Transport Interface send and receive function pointers. */
    xHTTPTransport.pxNetworkContext = &xHTTPNetworkContext;
    xHTTPTransport.xSend = Azure_Socket_Send;
    xHTTPTransport.xRecv = Azure_Socket_Recv;

    xHTTPNetworkContext.pParams = &xHTTPTlsTransportParams;
}
/*-----------------------------------------------------------*/

/**
 * Scan forward till key is not found
 *
 **/
static uint32_t prvGetJsonValueForKey( az_json_reader * pxState,
                                       const char * pcKey,
                                       az_span * pxValue )
{
    uint32_t ulStatus = e2etestE2E_TEST_NOT_FOUND;
    az_span xKeySpan = az_span_create( ( uint8_t * ) pcKey, strlen( pcKey ) );
    int32_t lLength;

    while( az_result_succeeded( az_json_reader_next_token( pxState ) ) )
    {
        if( az_json_token_is_text_equal( &( pxState->token ), xKeySpan ) )
        {
            if( az_result_failed( az_json_reader_next_token( pxState ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                LogError( ( "Failed next token, error code : %d", ulStatus ) );
                break;
            }

            if( az_result_failed( az_json_token_get_string( &pxState->token,
                                                            ( char * ) az_span_ptr( *pxValue ),
                                                            az_span_size( *pxValue ), &lLength ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                LogError( ( "Failed get string value, error code : %d", ulStatus ) );
                break;
            }

            *pxValue = az_span_create( az_span_ptr( *pxValue ), lLength );
            ulStatus = e2etestE2E_TEST_SUCCESS;
            break;
        }
    }

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Do find on JSON buffer
 *
 **/
static uint32_t prvGetValueForKey( az_span xJsonSpan,
                                   const char * pcKey,
                                   az_span * pxValue )
{
    az_json_reader xState;

    if( az_json_reader_init( &xState, xJsonSpan, NULL ) != AZ_OK )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to parse json" );
    }

    return prvGetJsonValueForKey( &xState, pcKey, pxValue );
}
/*-----------------------------------------------------------*/

/**
 * Move JSON reader curser to particular key
 *
 **/
static uint32_t prvMoveToObject( az_json_reader * pxState,
                                 const char * pcKey )
{
    uint32_t ulStatus = e2etestE2E_TEST_NOT_FOUND;
    az_span xKeySpan = az_span_create( ( uint8_t * ) pcKey, strlen( pcKey ) );

    while( az_result_succeeded( az_json_reader_next_token( pxState ) ) )
    {
        if( pxState->token.kind != AZ_JSON_TOKEN_PROPERTY_NAME )
        {
            continue;
        }

        if( az_json_token_is_text_equal( &( pxState->token ), xKeySpan ) )
        {
            if( az_result_succeeded( az_json_reader_next_token( pxState ) ) &&
                ( pxState->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT ) )
            {
                ulStatus = e2etestE2E_TEST_SUCCESS;
            }
            else
            {
                ulStatus = e2etestE2E_TEST_FAILED;
            }

            break;
        }
    }

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Append all test properties
 *
 **/
static uint32_t prvAddAllProperties( az_json_reader * pxState,
                                     AzureIoTHubClient_t * pxAzureIoTHubClient,
                                     E2E_TEST_COMMAND_HANDLE xCMD,
                                     AzureIoTMessageProperties_t * pxProperties )
{
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;
    uint32_t ulPropertyValueLength;
    uint32_t ulPropertyNameLength;

    while( az_result_succeeded( az_json_reader_next_token( pxState ) ) )
    {
        if( pxState->token.kind != AZ_JSON_TOKEN_PROPERTY_NAME )
        {
            continue;
        }

        if( az_result_failed( az_json_token_get_string( &pxState->token, ucScratchBuffer,
                                                        sizeof( ucScratchBuffer ),
                                                        ( int32_t * ) &ulPropertyNameLength ) ) )
        {
            ulStatus = e2etestE2E_TEST_FAILED;
            RETURN_IF_FAILED( ulStatus, "Failed to get property name" );
        }

        if( az_result_failed( az_json_reader_next_token( pxState ) ) )
        {
            ulStatus = e2etestE2E_TEST_FAILED;
            RETURN_IF_FAILED( ulStatus, "Failed to get property value" );
        }

        if( pxState->token.kind == AZ_JSON_TOKEN_STRING )
        {
            if( az_result_failed( az_json_token_get_string( &pxState->token, ucScratchBuffer + ulPropertyNameLength,
                                                            sizeof( ucScratchBuffer ) - ulPropertyNameLength,
                                                            ( int32_t * ) &ulPropertyValueLength ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                RETURN_IF_FAILED( ulStatus, "Failed to get property value" );
            }

            if( AzureIoTMessage_PropertiesAppend( pxProperties,
                                                  ucScratchBuffer,
                                                  ulPropertyNameLength,
                                                  ucScratchBuffer + ulPropertyNameLength,
                                                  ulPropertyValueLength ) != eAzureIoTSuccess )
            {
                RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to add property" );
            }

            ulStatus = e2etestE2E_TEST_SUCCESS;
        }
        else if( pxState->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT )
        {
            if( az_result_failed( az_json_reader_skip_children( pxState ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                RETURN_IF_FAILED( ulStatus, "Failed to skip object" );
            }
        }
    }

    return ulStatus;
}
/*-----------------------------------------------------------*/

static void prvSkipPropertyAndValue( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_SkipChildren( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );
}

/*-----------------------------------------------------------*/

static uint32_t prvParseADUTwin( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                 AzureIoTADUClient_t * pxAzureIoTAduClient,
                                 AzureIoTHubClientPropertiesResponse_t * pxPropertiesResponse )
{
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;

    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;

    if( pxPropertiesResponse->xMessageType != eAzureIoTHubPropertiesReportedResponseMessage )
    {
        uint32_t ulWritablePropertyResponseBufferLength;
        uint32_t * pulWritablePropertyResponseBufferLength = &ulWritablePropertyResponseBufferLength;

        LogInfo( ( "Writable properties received: %.*s\r\n",
                   pxPropertiesResponse->ulPayloadLength, ( char * ) pxPropertiesResponse->pvMessagePayload ) );

        xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxPropertiesResponse->pvMessagePayload, pxPropertiesResponse->ulPayloadLength );

        if( xAzIoTResult != eAzureIoTSuccess )
        {
            LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
            *pulWritablePropertyResponseBufferLength = 0;
            return e2etestE2E_TEST_FAILED;
        }

        xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( pxAzureIoTHubClient, &xJsonReader, pxPropertiesResponse->xMessageType, &ulReceivedTwinVersion );

        if( xAzIoTResult != eAzureIoTSuccess )
        {
            LogError( ( "AzureIoTHubClientProperties_GetPropertiesVersion failed: result 0x%08x", xAzIoTResult ) );
            *pulWritablePropertyResponseBufferLength = 0;
            return e2etestE2E_TEST_FAILED;
        }

        xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxPropertiesResponse->pvMessagePayload, pxPropertiesResponse->ulPayloadLength );

        if( xAzIoTResult != eAzureIoTSuccess )
        {
            LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
            *pulWritablePropertyResponseBufferLength = 0;
            return e2etestE2E_TEST_FAILED;
        }

        /**
         * If the PnP component is for Azure Device Update, function
         * AzureIoTADUClient_SendResponse shall be used to publish back the
         * response for the ADU writable properties.
         * Thus, to prevent this callback to publish a response in duplicate,
         * pulWritablePropertyResponseBufferLength must be set to zero.
         */
        *pulWritablePropertyResponseBufferLength = 0;

        while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( pxAzureIoTHubClient, &xJsonReader,
                                                                                      pxPropertiesResponse->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                      &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
        {
            LogInfo( ( "Properties component name: %.*s", ulComponentNameLength, pucComponentName ) );

            if( AzureIoTADUClient_IsADUComponent( pxAzureIoTAduClient, pucComponentName, ulComponentNameLength ) )
            {
                AzureIoTADURequestDecision_t xRequestDecision;

                xAzIoTResult = AzureIoTADUClient_ParseRequest(
                    pxAzureIoTAduClient,
                    &xJsonReader,
                    &xAzureIoTAduUpdateRequest,
                    ucScratchBuffer,
                    sizeof( ucScratchBuffer ) );
            }
            else
            {
                LogInfo( ( "Component not ADU: %.*s", ulComponentNameLength, pucComponentName ) );
                prvSkipPropertyAndValue( &xJsonReader );
            }
        }
    }

    return ulStatus;
}

/*-----------------------------------------------------------*/

static uint32_t prvCheckTwinForADU( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                    AzureIoTADUClient_t * pxAzureIoTAduClient,
                                    AzureIoTHubClientPropertiesResponse_t * pxPropertiesResponse )
{
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;

    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;

    if( pxPropertiesResponse->xMessageType != eAzureIoTHubPropertiesReportedResponseMessage )
    {
        uint32_t ulWritablePropertyResponseBufferLength;
        uint32_t * pulWritablePropertyResponseBufferLength = &ulWritablePropertyResponseBufferLength;

        LogInfo( ( "Writable properties received: %.*s\r\n",
                   pxPropertiesResponse->ulPayloadLength, ( char * ) pxPropertiesResponse->pvMessagePayload ) );

        xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxPropertiesResponse->pvMessagePayload, pxPropertiesResponse->ulPayloadLength );

        if( xAzIoTResult != eAzureIoTSuccess )
        {
            LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
            *pulWritablePropertyResponseBufferLength = 0;
            return e2etestE2E_TEST_FAILED;
        }

        xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxPropertiesResponse->pvMessagePayload, pxPropertiesResponse->ulPayloadLength );

        if( xAzIoTResult != eAzureIoTSuccess )
        {
            LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
            *pulWritablePropertyResponseBufferLength = 0;
            return e2etestE2E_TEST_FAILED;
        }

        /**
         * If the PnP component is for Azure Device Update, function
         * AzureIoTADUClient_SendResponse shall be used to publish back the
         * response for the ADU writable properties.
         * Thus, to prevent this callback to publish a response in duplicate,
         * pulWritablePropertyResponseBufferLength must be set to zero.
         */
        *pulWritablePropertyResponseBufferLength = 0;

        while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( pxAzureIoTHubClient, &xJsonReader,
                                                                                      pxPropertiesResponse->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                      &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
        {
            LogInfo( ( "Properties component name: %.*s", ulComponentNameLength, pucComponentName ) );

            if( AzureIoTADUClient_IsADUComponent( pxAzureIoTAduClient, pucComponentName, ulComponentNameLength ) )
            {
                /* Mark as received so we can begin with the test process */
                xAduWasReceived = true;
                break;
            }
            else
            {
                LogInfo( ( "Component not ADU: %.*s", ulComponentNameLength, pucComponentName ) );
                prvSkipPropertyAndValue( &xJsonReader );
            }
        }
    }

    return ulStatus;
}


/*-----------------------------------------------------------*/

/**
 * Execute echo command
 *
 * e.g of command issued from service:
 *   {"method":"echo","payload":"hello"}
 *
 **/
static uint32_t prvE2ETestEchoCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                              AzureIoTHubClient_t * pxAzureIoTHubClient,
                                              AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    AzureIoTResult_t xStatus;
    uint16_t usTelemetryPacketID;

    if( ( xStatus = AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                                     xCMD->pulReceivedData,
                                                     xCMD->ulReceivedDataLength,
                                                     NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Telemetry message send failed!, error code %d", xStatus ) );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        LogError( ( "Telemetry message PUBACK never received!", xStatus ) );
    }
    else
    {
        LogInfo( ( "Successfully done with prvE2ETestEchoCommandExecute" ) );
    }

    return ( uint32_t ) xStatus;
}
/*-----------------------------------------------------------*/

/**
 * Execute exit command
 *
 * e.g of command issued from service:
 *   {"method":"exit"}
 *
 **/
static uint32_t prvE2ETestExitCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                              AzureIoTHubClient_t * pxAzureIoTHubClient,
                                              AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    ulContinueProcessingCMD = 0;

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * Execute send initial ADU state command
 *
 * e.g of command issued from service:
 *   {"method":"send_init_adu_state"}
 *
 **/
static uint32_t prvE2ETestSendInitialADUStateExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                      AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                      AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;

    if( AzureIoTADUClient_SendAgentState( pxAzureIoTAduClient, pxAzureIoTHubClient, &xADUDeviceProperties,
                                          NULL, eAzureIoTADUAgentStateIdle, NULL, ucScratchBuffer, sizeof( ucScratchBuffer ), NULL ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Unable to send initial ADU client state!" );
    }

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         xCMD->pulReceivedData,
                                         xCMD->ulReceivedDataLength,
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Telemetry message PUBACK never received!", ulStatus ) );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Execute GetTwin for ADU properties command
 *
 * e.g of command issued from service:
 *   {"method":"get_adu_twin"}
 *
 **/
static uint32_t prvE2ETestGetADUTwinPropertiesExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                       AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                       AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;

    if( AzureIoTHubClient_RequestPropertiesAsync( pxAzureIoTHubClient ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to request twin properties" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_MSEC ) != eAzureIoTSuccess )
    {
        LogError( ( "receive timeout" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( pxTwinMessage == NULL )
    {
        LogError( ( "Invalid response from server" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                              xCMD->pulReceivedData,
                                              xCMD->ulReceivedDataLength,
                                              NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Telemetry message PUBACK never received!", ulStatus ) );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Execute application of the update command
 *
 * e.g of command issued from service:
 *   {"method":"apply_update"}
 *
 **/
static uint32_t prvE2ETestApplyADUUpdateExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                 AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                 AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;

    LogInfo( ( "Getting the ADU twin to parse" ) );

    if( AzureIoTHubClient_RequestPropertiesAsync( pxAzureIoTHubClient ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to request twin properties" );
    }

    if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                       e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_MSEC ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "receive timeout" );
    }

    if( pxTwinMessage == NULL )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Invalid response from server" );
    }

    LogInfo( ( "Parsing the ADU twin" ) );

    if( prvParseADUTwin( pxAzureIoTHubClient, pxAzureIoTAduClient, pxTwinMessage ) != e2etestE2E_TEST_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to parse ADU twin!" );
    }

    LogInfo( ( "Authenticating the ADU twin (JWS)" ) );

    if( AzureIoTJWS_ManifestAuthenticate( xAzureIoTAduUpdateRequest.pucUpdateManifest,
                                          xAzureIoTAduUpdateRequest.ulUpdateManifestLength,
                                          xAzureIoTAduUpdateRequest.pucUpdateManifestSignature,
                                          xAzureIoTAduUpdateRequest.ulUpdateManifestSignatureLength,
                                          ucAduManifestVerificationBuffer,
                                          sizeof( ucAduManifestVerificationBuffer ) ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to authenticate the JWS manifest!" );
    }

    LogInfo( ( "Sending response to accept the ADU twin" ) );

    if( AzureIoTADUClient_SendResponse(
            pxAzureIoTAduClient,
            pxAzureIoTHubClient,
            eAzureIoTADURequestDecisionAccept,
            ulReceivedTwinVersion,
            ucScratchBuffer,
            sizeof( ucScratchBuffer ),
            NULL ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send response accepting the properties!" );
    }

    LogInfo( ( "Sending the ADU state as in progress" ) );

    if( AzureIoTADUClient_SendAgentState( pxAzureIoTAduClient, pxAzureIoTHubClient, &xADUDeviceProperties,
                                          NULL, eAzureIoTADUAgentStateDeploymentInProgress, NULL, ucScratchBuffer, sizeof( ucScratchBuffer ), NULL ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Unable to send in progress ADU client state!" );
    }

    LogInfo( ( "Sending completion telemetry" ) );

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         xCMD->pulReceivedData,
                                         xCMD->ulReceivedDataLength,
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Telemetry message PUBACK never received!", ulStatus ) );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}

/*-----------------------------------------------------------*/

/**
 * Execute verification of the ADU update
 *
 * e.g of command issued from service:
 *   {"method":"verify_final_state"}
 *
 **/
static uint32_t prvE2ETestVerifyADUUpdateExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                  AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                  AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;

    if( AzureIoTADUClient_SendAgentState( pxAzureIoTAduClient, pxAzureIoTHubClient, &xADUDevicePropertiesNew,
                                          NULL, eAzureIoTADUAgentStateIdle, NULL, ucScratchBuffer, sizeof( ucScratchBuffer ), NULL ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Unable to send initial ADU client state!" );
    }

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         xCMD->pulReceivedData,
                                         xCMD->ulReceivedDataLength,
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Telemetry message PUBACK never received!", ulStatus ) );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}

/*-----------------------------------------------------------*/

/**
 * Initialize device command
 *
 **/
static uint32_t prvE2EDeviceCommandInit( E2E_TEST_COMMAND_HANDLE xCMD,
                                         const char * ucData,
                                         uint32_t ulDataLength,
                                         EXECUTE_FN xExecute )
{
    xCMD->xExecute = xExecute;
    xCMD->pulReceivedData = ( const uint8_t * ) ucData;
    xCMD->ulReceivedDataLength = ulDataLength;

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * DeInitialize device command
 *
 **/
static void prvE2EDeviceCommandDeinit( E2E_TEST_COMMAND_HANDLE xCMD )
{
    vPortFree( ( void * ) xCMD->pulReceivedData );
    memset( xCMD, 0, sizeof( E2E_TEST_COMMAND ) );
}
/*-----------------------------------------------------------*/

/**
 * Find the command and initialized command handle
 *
 **/
static uint32_t prvE2ETestInitializeCMD( const char * pcMethodName,
                                         const char * ucData,
                                         uint32_t ulDataLength,
                                         E2E_TEST_COMMAND_HANDLE xCMD )
{
    uint32_t ulStatus;

    if( !strncmp( e2etestE2E_TEST_ECHO_COMMAND,
                  pcMethodName, sizeof( e2etestE2E_TEST_ECHO_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestEchoCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_EXIT_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_EXIT_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestExitCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_SEND_INITIAL_ADU_STATE_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_SEND_INITIAL_ADU_STATE_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestSendInitialADUStateExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_GET_ADU_TWIN_PROPERTIES_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_GET_ADU_TWIN_PROPERTIES_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestGetADUTwinPropertiesExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_APPLY_ADU_UPDATE_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_APPLY_ADU_UPDATE_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestApplyADUUpdateExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_VERIFY_ADU_FINAL_STATE_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_VERIFY_ADU_FINAL_STATE_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestVerifyADUUpdateExecute );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_FAILED;
    }

    RETURN_IF_FAILED( ulStatus, "Failed to find a command" );

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Parse test command received from service
 *
 * */
static uint32_t prvE2ETestParseCommand( uint8_t * pucData,
                                        uint32_t ulDataLength,
                                        E2E_TEST_COMMAND_HANDLE xCMD )
{
    uint32_t ulStatus;
    az_span xJsonSpan;
    az_json_reader xState;
    az_span xMethodName = az_span_create( ucMethodNameBuffer, sizeof( ucMethodNameBuffer ) );

    xJsonSpan = az_span_create( pucData, ulDataLength );

    LogInfo( ( "Command %.*s\r\n", ulDataLength, pucData ) );

    if( az_json_reader_init( &xState, xJsonSpan, NULL ) != AZ_OK )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to parse json" );
    }

    ulStatus = prvGetJsonValueForKey( &xState, e2etestMETHOD_KEY, &xMethodName );
    RETURN_IF_FAILED( ulStatus, "Failed to parse the command" );

    ulStatus = prvE2ETestInitializeCMD( az_span_ptr( xMethodName ), pucData, ulDataLength, xCMD );
    RETURN_IF_FAILED( ulStatus, "Failed to initialize the command" );

    return( ulStatus );
}
/*-----------------------------------------------------------*/

/**
 * Cloud message callback
 *
 * */
void vHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                          void * pvContext )
{
    if( ucC2DCommandData == NULL )
    {
        ucC2DCommandData = pvPortMalloc( pxMessage->ulPayloadLength );
        ulC2DCommandDataLength = pxMessage->ulPayloadLength;

        if( ucC2DCommandData != NULL )
        {
            memcpy( ucC2DCommandData, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
        }
        else
        {
            printf( "Failed to allocated memory for cloud message message" );
            configASSERT( false );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Device twin message callback
 *
 * */
void vHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                               void * pvContext )
{
    if( pxTwinMessage == NULL )
    {
        pxTwinMessage = pvPortMalloc( sizeof( AzureIoTHubClientPropertiesResponse_t ) );

        if( pxTwinMessage == NULL )
        {
            printf( "Failed to allocated memory for Twin message" );
            configASSERT( false );
            return;
        }

        memcpy( pxTwinMessage, pxMessage, sizeof( AzureIoTHubClientPropertiesResponse_t ) );

        if( pxMessage->ulPayloadLength != 0 )
        {
            pxTwinMessage->pvMessagePayload = pvPortMalloc( pxMessage->ulPayloadLength );

            if( pxTwinMessage->pvMessagePayload != NULL )
            {
                memcpy( ( void * ) pxTwinMessage->pvMessagePayload,
                        pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
            }
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * Poll for commands and execute commands
 *
 * */
uint32_t ulE2EDeviceProcessCommands( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                     AzureIoTADUClient_t * pxAzureIoTAduClient )
{
    AzureIoTResult_t xResult;
    E2E_TEST_COMMAND xCMD = { 0 };
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;
    uint8_t * pucData = NULL;
    AzureIoTHubClientCommandRequest_t * pucMethodData = NULL;
    uint32_t ulDataLength;
    uint8_t * ucErrorReport = NULL;
    uint16_t usTelemetryPacketID;

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         e2etestE2E_TEST_CONNECTED_MESSAGE,
                                         sizeof( e2etestE2E_TEST_CONNECTED_MESSAGE ) - 1,
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Report connected failed!" );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
    }

    /* Send initial state for device */
    if( AzureIoTADUClient_SendAgentState( pxAzureIoTAduClient, pxAzureIoTHubClient, &xADUDeviceProperties,
                                          NULL, eAzureIoTADUAgentStateIdle, NULL, ucScratchBuffer, sizeof( ucScratchBuffer ), NULL ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Unable to send initial ADU client state!" );
    }

    /* We will wait until the device receives the ADU payload since the service is inconsistent */
    /* with actually sending the update payload. */
    do
    {
        xResult = AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                                 e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_MSEC );

        if( xResult )
        {
            LogError( ( "Failed to receive data from IoTHUB, error code : %d", xResult ) );
            break;
        }

        if( ucC2DCommandData != NULL )
        {
            pucData = ucC2DCommandData;
            ulDataLength = ulC2DCommandDataLength;
            ucC2DCommandData = NULL;

            if( ( ulStatus = prvE2ETestParseCommand( pucData, ulDataLength, &xCMD ) ) )
            {
                vPortFree( pucData );
                LogError( ( "Failed to parse the command, error code : %d", ulStatus ) );
            }
            else
            {
                if( ( ulStatus = xCMD.xExecute( &xCMD, pxAzureIoTHubClient, pxAzureIoTAduClient ) ) != e2etestE2E_TEST_SUCCESS )
                {
                    LogError( ( "Failed to execute, error code : %d", ulStatus ) );
                }

                prvE2EDeviceCommandDeinit( &xCMD );
            }
        }
        else if( pxTwinMessage != NULL )
        {
            LogInfo( ( "Twin message received\r\n" ) );

            if( !xAduWasReceived )
            {
                prvCheckTwinForADU( pxAzureIoTHubClient, pxAzureIoTAduClient, pxTwinMessage );

                if( xAduWasReceived )
                {
                    LogInfo( ( "ADU message received\r\n" ) );

                    /* Once we receive a twin message, it will have been the ADU payload. */
                    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                                         e2etestE2E_TEST_ADU_PAYLOAD_RECEIVED,
                                                         sizeof( e2etestE2E_TEST_ADU_PAYLOAD_RECEIVED ) - 1,
                                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
                    {
                        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Report ADU received failed!" );
                    }
                    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
                    {
                        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
                    }
                }
            }

            free( pxTwinMessage );
            pxTwinMessage = NULL;
        }
        else
        {
            if( !xAduWasReceived )
            {
                if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                                     e2etestE2E_TEST_WAITING_FOR_ADU,
                                                     sizeof( e2etestE2E_TEST_WAITING_FOR_ADU ) - 1,
                                                     NULL, eAzureIoTHubMessageQoS0, NULL ) != eAzureIoTSuccess )
                {
                    RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send ADU waiting telem!" );
                }
            }

            vTaskDelay( 10 * 1000 / portTICK_PERIOD_MS );
        }
    } while( ( ulStatus == e2etestE2E_TEST_SUCCESS ) &&
             ulContinueProcessingCMD );

    return ulStatus;
}
/*-----------------------------------------------------------*/

/**
 * Get time from fix start point
 *
 * */
uint64_t ulGetUnixTime( void )
{
    return time( NULL );
}
/*-----------------------------------------------------------*/

/**
 * Connect to endpoint with backoff
 *
 * */
TlsTransportStatus_t xConnectToServerWithBackoffRetries( const char * pHostName,
                                                         uint32_t port,
                                                         NetworkCredentials_t * pxNetworkCredentials,
                                                         NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       e2etestRETRY_BACKOFF_BASE_MS,
                                       e2etestRETRY_MAX_BACKOFF_DELAY_MS,
                                       e2etestRETRY_MAX_ATTEMPTS );

    do
    {
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pHostName, port ) );
        xNetworkStatus = TLS_FreeRTOS_Connect( pxNetworkContext,
                                               pHostName, port,
                                               pxNetworkCredentials,
                                               e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                               e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != TLS_TRANSPORT_SUCCESS )
        {
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, uxRand(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the broker failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the broker failed. "
                           "Retrying connection with backoff and jitter." ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != TLS_TRANSPORT_SUCCESS ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus;
}
/*-----------------------------------------------------------*/

/**
 * Calculate HMAC using mbedtls crypto API's
 *
 **/
uint32_t ulCalculateHMAC( const uint8_t * pucKey,
                          uint32_t ulKeyLength,
                          const uint8_t * pucData,
                          uint32_t ulDataLength,
                          uint8_t * pucOutput,
                          uint32_t ulOutputLength,
                          uint32_t * pulBytesCopied )
{
    uint32_t ulResult;

    if( ulOutputLength < e2etestE2E_HMAC_MAX_SIZE )
    {
        return 1;
    }

    mbedtls_md_context_t xContext;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init( &xContext );

    if( mbedtls_md_setup( &xContext, mbedtls_md_info_from_type( md_type ), 1 ) ||
        mbedtls_md_hmac_starts( &xContext, pucKey, ulKeyLength ) ||
        mbedtls_md_hmac_update( &xContext, pucData, ulDataLength ) ||
        mbedtls_md_hmac_finish( &xContext, pucOutput ) )
    {
        ulResult = 1;
    }
    else
    {
        ulResult = 0;
    }

    mbedtls_md_free( &xContext );
    *pulBytesCopied = e2etestE2E_HMAC_MAX_SIZE;

    return ulResult;
}
/*-----------------------------------------------------------*/
