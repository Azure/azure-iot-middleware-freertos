/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "e2e_device_process_commands.h"

#include "azure_iot_json_writer.h"
#include "azure_iot_hub_client_properties.h"
#include "azure/core/az_json.h"
#include "azure/core/az_span.h"

#define e2etestMESSAGE_BUFFER_SIZE                             ( 10240 )

#define e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC                ( 4 )

#define e2etestE2E_TEST_SUCCESS                                ( 0 )
#define e2etestE2E_TEST_FAILED                                 ( 1 )
#define e2etestE2E_TEST_NOT_FOUND                              ( 2 )

#define e2etestE2E_HMAC_MAX_SIZE                               ( 32 )

#define e2etestMETHOD_KEY                                      "method"
#define e2etestPAYLOAD_KEY                                     "payload"
#define e2etestPROPERTIES_KEY                                  "properties"
#define e2etestID_SCOPE_KEY                                    "id_scope"
#define e2etestREGISTRATION_ID_KEY                             "registration_id"
#define e2etestSYMMETRIC_KEY                                   "symmetric_key"
#define e2etestSERVICE_ENDPOINT_KEY                            "service_endpoint"
#define e2etestDESIRED_PROPERTY_KEY                            "desired_property_key"
#define e2etestDESIRED_PROPERTY_VALUE                          "desired_property_value"
#define e2etestCOMPONENT_KEY                                   "component"

#define e2etestE2E_TEST_SEND_TELEMETRY_COMMAND                 "send_telemetry"
#define e2etestE2E_TEST_ECHO_COMMAND                           "echo"
#define e2etestE2E_TEST_EXIT_COMMAND                           "exit"
#define e2etestE2E_TEST_DEVICE_PROVISIONING_COMMAND            "device_provisioning"
#define e2etestE2E_TEST_REPORTED_PROPERTIES_COMMAND            "reported_properties"
#define e2etestE2E_TEST_VERIFY_DESIRED_PROPERTIES_COMMAND      "verify_desired_properties"
#define e2etestE2E_TEST_GET_TWIN_PROPERTIES_COMMAND            "get_twin_properties"
#define e2etestE2E_TEST_CONNECTED_MESSAGE                      "\"CONNECTED\""
#define e2etestE2E_TEST_WRITEABLE_PROPERTY_RESPONSE_COMMAND    "get_writable_properties_response"
#define e2etestE2E_TEST_COMPONENT_REPORTED_PROPERTY_COMMAND    "report_component_properties"

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
                                    AzureIoTHubClient_t * pxAzureIoTHubClient );

typedef struct E2E_TEST_COMMAND_STRUCT
{
    EXECUTE_FN xExecute;
    const uint8_t * pulReceivedData;
    uint32_t ulReceivedDataLength;
} E2E_TEST_COMMAND;

typedef E2E_TEST_COMMAND * E2E_TEST_COMMAND_HANDLE;
/*-----------------------------------------------------------*/

static const uint64_t ulGlobalEntryTime = 1639093301;
static uint8_t * ucC2DCommandData = NULL;
static uint32_t ulC2DCommandDataLength = 0;
static AzureIoTHubClientCommandRequest_t * pxMethodCommandData = NULL;
static AzureIoTHubClientPropertiesResponse_t * pxTwinMessage = NULL;
static uint8_t ucMethodNameBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer2[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucMessageBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static const uint8_t ucStatusOKTelemetry[] = "{\"status\": \"OK\"}";
static uint32_t ulContinueProcessingCMD = 1;
static uint8_t ucSharedBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
static uint16_t usReceivedTelemetryPubackID;
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

    AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient, e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC );

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

/**
 * Start Device Provisioning registration
 *
 **/
static uint32_t prvStartProvisioning( az_span * pxEndpoint,
                                      az_span * pxIDScope,
                                      az_span * pxRegistrationId,
                                      az_span * pxSymmetricKey,
                                      az_span * pxDeviceId,
                                      az_span * pxHostname )
{
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    AzureIoTTransportInterface_t xTransport;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    uint32_t ulHostnameLength = ( uint32_t ) az_span_size( *pxHostname );
    uint32_t ulDeviceIdLength = ( uint32_t ) az_span_size( *pxDeviceId );

    xNetworkCredentials.disableSni = true;
    /* Set the credentials for establishing a TLS connection. */
    xNetworkCredentials.pRootCa = ( const unsigned char * ) azureiotROOT_CA_PEM;
    xNetworkCredentials.rootCaSize = sizeof( azureiotROOT_CA_PEM );

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pParams = &xTlsTransportParams;

    if( xConnectToServerWithBackoffRetries( az_span_ptr( *pxEndpoint ), 8883,
                                            &xNetworkCredentials,
                                            &xNetworkContext ) != TLS_TRANSPORT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed transport connect!" );
    }

    xTransport.pxNetworkContext = &xNetworkContext;
    xTransport.xSend = TLS_FreeRTOS_send;
    xTransport.xRecv = TLS_FreeRTOS_recv;

    if( AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                         ( const uint8_t * ) az_span_ptr( *pxEndpoint ),
                                         az_span_size( *pxEndpoint ),
                                         ( const uint8_t * ) az_span_ptr( *pxIDScope ),
                                         az_span_size( *pxIDScope ),
                                         ( const uint8_t * ) az_span_ptr( *pxRegistrationId ),
                                         az_span_size( *pxRegistrationId ),
                                         NULL, ucSharedBuffer,
                                         sizeof( ucSharedBuffer ), ulGetUnixTime,
                                         &xTransport ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to init provisioning client!" );
    }

    if( AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                    ( const uint8_t * ) az_span_ptr( *pxSymmetricKey ),
                                                    az_span_size( *pxSymmetricKey ),
                                                    ulCalculateHMAC ) != eAzureIoTSuccess )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to set symmetric key!" );
    }

    do
    {
        xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                       e2etestProvisioning_Registration_TIMEOUT_MS );
    } while( xResult == eAzureIoTErrorPending );

    if( xResult != eAzureIoTSuccess )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to register !" );
    }

    if( AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                    az_span_ptr( *pxHostname ),
                                                    &ulHostnameLength,
                                                    az_span_ptr( *pxDeviceId ),
                                                    &ulDeviceIdLength ) != eAzureIoTSuccess )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to get iothub info!" );
    }

    *pxHostname = az_span_create( az_span_ptr( *pxHostname ), ( int32_t ) ulHostnameLength );
    *pxDeviceId = az_span_create( az_span_ptr( *pxDeviceId ), ( int32_t ) ulDeviceIdLength );

    AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

    return e2etestE2E_TEST_SUCCESS;
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

/**
 * Execute telemetry command
 *
 * e.g of command issued from service:
 *   {"method":"send_telemetry","payload":"","properties":{"propertyA":"valueA","propertyB":"valueB"}}
 *
 **/
static uint32_t prvE2ETestSendTelemetryCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                       AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_json_reader xState;
    az_span xPayloadSpan = az_span_create( ucScratchBuffer, sizeof( ucScratchBuffer ) );
    AzureIoTMessageProperties_t xProperties;
    uint16_t usTelemetryPacketID;


    if( AzureIoTMessage_PropertiesInit( &xProperties,
                                        ucScratchBuffer2,
                                        0,
                                        sizeof( ucScratchBuffer2 ) ) != eAzureIoTSuccess )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Failed to init properties, error code : %d", ulStatus ) );
    }
    else if( az_json_reader_init( &xState, xJsonSpan, NULL ) != AZ_OK )
    {
        ulStatus = e2etestE2E_TEST_FAILED;
        LogError( ( "Failed to parse json, error code : %d", ulStatus ) );
    }
    else
    {
        if( prvMoveToObject( &xState, e2etestPROPERTIES_KEY ) == e2etestE2E_TEST_SUCCESS )
        {
            if( ( ulStatus = prvAddAllProperties( &xState, pxAzureIoTHubClient,
                                                  xCMD, &xProperties ) ) != e2etestE2E_TEST_SUCCESS )
            {
                LogError( ( "Failed to parse json properties, error code : %d", ulStatus ) );
            }
        }

        if( ulStatus == e2etestE2E_TEST_SUCCESS )
        {
            if( az_json_reader_init( &xState, xJsonSpan, NULL ) != AZ_OK )
            {
                ulStatus = e2etestE2E_TEST_SUCCESS;
                LogError( ( "Failed to parse json", ulStatus ) );
            }
            else if( ( ulStatus = prvGetJsonValueForKey( &xState,
                                                         e2etestPAYLOAD_KEY,
                                                         &xPayloadSpan ) ) != e2etestE2E_TEST_SUCCESS )
            {
                LogError( ( "Telemetry message send failed!, error code : %d", ulStatus ) );
            }
        }

        if( ( ulStatus == e2etestE2E_TEST_SUCCESS ) &&
            ( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                               az_span_ptr( xPayloadSpan ),
                                               az_span_size( xPayloadSpan ),
                                               &xProperties, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess ) )
        {
            ulStatus = e2etestE2E_TEST_FAILED;
            LogError( ( "Telemetry message send failed!", ulStatus ) );
        }

        if( ( ulStatus == e2etestE2E_TEST_SUCCESS ) &&
            ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
        {
            ulStatus = e2etestE2E_TEST_FAILED;
            LogError( ( "Telemetry message PUBACK never received!", ulStatus ) );
        }

        if( ulStatus == e2etestE2E_TEST_SUCCESS )
        {
            LogInfo( ( "Successfully done with prvE2ETestSendTelemetryCommandExecute" ) );
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
                                              AzureIoTHubClient_t * pxAzureIoTHubClient )
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
                                              AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    ulContinueProcessingCMD = 0;

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * Execute DPS provisioning command
 *
 * e.g of command issued from service:
 *   {"method":"device_provisioning","id_scope":"<>","service_endpoint":"<>","registration_id":"<>","symmetric_key":"<>"}
 *
 **/
static uint32_t prvE2ETestDeviceProvisioningCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                            AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint32_t ulMessageLength;
    uint16_t usTelemetryPacketID;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xIdScope = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xRegistrationId = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xSymmetricKey = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xServiceEndpoint = az_span_create( ucScratchBuffer + 3 * e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xDeviceId = az_span_create( ucScratchBuffer2, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xEndpoint = az_span_create( ucScratchBuffer2 + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xHostname = az_span_create( ucScratchBuffer2 + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestID_SCOPE_KEY, &xIdScope );
    RETURN_IF_FAILED( ulStatus, "Failed to find id_scope!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestREGISTRATION_ID_KEY, &xRegistrationId );
    RETURN_IF_FAILED( ulStatus, "Failed to find registration_id!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestSYMMETRIC_KEY, &xSymmetricKey );
    RETURN_IF_FAILED( ulStatus, "Failed to find symmetric key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestSERVICE_ENDPOINT_KEY, &xServiceEndpoint );
    RETURN_IF_FAILED( ulStatus, "Failed to find service endpoint key!" );

    if( az_span_size( xServiceEndpoint ) >= az_span_size( xEndpoint ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "buffer too small to copy dps endpoint!" );
    }

    /* hostname required to be NULL terminated */
    memcpy( ( void * ) az_span_ptr( xEndpoint ),
            ( void * ) az_span_ptr( xServiceEndpoint ),
            ( uint32_t ) az_span_size( xServiceEndpoint ) );
    az_span_ptr( xEndpoint )[ ( uint32_t ) az_span_size( xServiceEndpoint ) ] = 0;

    ulStatus = prvStartProvisioning( &xEndpoint, &xIdScope,
                                     &xRegistrationId, &xSymmetricKey,
                                     &xDeviceId, &xHostname );
    RETURN_IF_FAILED( ulStatus, "Fail to start Provisioning!" )

    ulMessageLength = ( uint32_t ) snprintf( ucMessageBuffer, sizeof( ucMessageBuffer ),
                                             "{\"device\":\"%.*s\", \"status\":\"OK\"}",
                                             az_span_size( xDeviceId ), az_span_ptr( xDeviceId ) );

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         ucMessageBuffer, ulMessageLength,
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Fail to send telemetry" );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
    }

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * Execute Reported property command
 *
 * e.g of command issued from service:
 *   {"method":"reported_properties","payload":"{\"temp\":\"1106197372\"}"}
 *
 **/
static uint32_t prvE2ETestReportedPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                            AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint32_t ulRequestId;
    uint16_t usTelemetryPacketID;

    if( AzureIoTHubClient_SendPropertiesReported( pxAzureIoTHubClient,
                                                  xCMD->pulReceivedData,
                                                  xCMD->ulReceivedDataLength,
                                                  &ulRequestId ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to send reported properties" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != eAzureIoTSuccess )
    {
        LogError( ( "recevice timeout" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( pxTwinMessage == NULL ) ||
             ( pxTwinMessage->ulRequestID != ulRequestId ) ||
             ( pxTwinMessage->xMessageStatus < eAzureIoTStatusOk ) ||
             ( pxTwinMessage->xMessageStatus > eAzureIoTStatusNoContent ) )
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
 * Execute GetTwin command
 *
 * e.g of command issued from service:
 *   {"method":"get_twin_properties"}
 *
 **/
static uint32_t prvE2ETestGetTwinPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                           AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;

    if( AzureIoTHubClient_RequestPropertiesAsync( pxAzureIoTHubClient ) != eAzureIoTSuccess )
    {
        LogError( ( "Failed to request twin properties" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != eAzureIoTSuccess )
    {
        LogError( ( "recevice timeout" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( pxTwinMessage == NULL )
    {
        LogError( ( "Invalid response from server" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                              pxTwinMessage->pvMessagePayload,
                                              pxTwinMessage->ulPayloadLength,
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
 * Execute Verifiy desired command
 *
 * e.g of command issued from service:
 *   {"method":"verify_desired_properties","desired_property_key":"temp","desired_property_value":"7664262703"}
 *
 **/
static uint32_t prvE2ETestVerifyDesiredPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                                 AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xKey = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValue = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValueReceived = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_KEY, &xKey );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_VALUE, &xValue );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_value!" );

    if( ( pxTwinMessage == NULL ) &&
        ( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                         e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != eAzureIoTSuccess ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to receive desired properties" );
    }

    xJsonSpan = az_span_create( ( uint8_t * ) pxTwinMessage->pvMessagePayload,
                                pxTwinMessage->ulPayloadLength );
    ulStatus = prvGetValueForKey( xJsonSpan, az_span_ptr( xKey ), &xValueReceived );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_key!" );

    if( !az_span_is_content_equal( xValueReceived, xValue ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to find desired_property_value!" );
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    snprintf( ucScratchBuffer2, sizeof( ucScratchBuffer2 ), "{\"%.*s\": \"%.*s\"}",
              az_span_size( xKey ), az_span_ptr( xKey ), az_span_size( xValue ), az_span_ptr( xValue ) );

    if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                         ucScratchBuffer2,
                                         strlen( ucScratchBuffer2 ),
                                         NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send response" );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
    }

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * Execute Writeable property response command
 *
 * e.g of command issued from service:
 *   {"method":"get_writable_properties_repsonse","desired_property_key":"temp","desired_property_value":"7664262703"}
 *
 **/
static uint32_t prvE2ETestWriteablePropertyResponseCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                                   AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;
    uint32_t ulRequestId;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xKey = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValue = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValueReceived = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    AzureIoTJSONWriter_t xJSONWriter;

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_KEY, &xKey );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_VALUE, &xValue );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_value!" );

    if( ( AzureIoTJSONWriter_Init( &xJSONWriter, ucScratchBuffer2,
                                   sizeof( ucScratchBuffer2 ) ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendBeginObject( &xJSONWriter ) != eAzureIoTSuccess ) ||
        ( AzureIoTHubClientProperties_BuilderBeginResponseStatus( pxAzureIoTHubClient,
                                                                  &xJSONWriter,
                                                                  az_span_ptr( xKey ),
                                                                  az_span_size( xKey ),
                                                                  200, 1, NULL, 0 ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendString( &xJSONWriter,
                                           az_span_ptr( xValue ),
                                           ( uint32_t ) az_span_size( xValue ) ) != eAzureIoTSuccess ) ||
        ( AzureIoTHubClientProperties_BuilderEndResponseStatus( pxAzureIoTHubClient,
                                                                &xJSONWriter ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendEndObject( &xJSONWriter ) != eAzureIoTSuccess ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to build response with status" );
    }
    else if( AzureIoTHubClient_SendPropertiesReported( pxAzureIoTHubClient,
                                                       ucScratchBuffer2,
                                                       ( uint32_t ) AzureIoTJSONWriter_GetBytesUsed( &xJSONWriter ),
                                                       &ulRequestId ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send reported properties" );
    }
    else if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Test failed: receive" );
    }
    else if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                              xCMD->pulReceivedData,
                                              xCMD->ulReceivedDataLength,
                                              NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send response" );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
    }

    return e2etestE2E_TEST_SUCCESS;
}
/*-----------------------------------------------------------*/

/**
 * Execute Report component properties command
 *
 * e.g of command issued from service:
 *   {"method":"report_component_properties","component":"test","desired_property_key":"temp","desired_property_value":"7664262703"}
 *
 **/
static uint32_t prvE2ETestReportComponentPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                                   AzureIoTHubClient_t * pxAzureIoTHubClient )
{
    uint32_t ulStatus;
    uint16_t usTelemetryPacketID;
    uint32_t ulRequestId;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xKey = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValue = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xComponent = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    AzureIoTJSONWriter_t xJSONWriter;

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestCOMPONENT_KEY, &xComponent );
    RETURN_IF_FAILED( ulStatus, "Failed to find component!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_KEY, &xKey );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_VALUE, &xValue );
    RETURN_IF_FAILED( ulStatus, "Failed to find desired_property_value!" );

    if( ( AzureIoTJSONWriter_Init( &xJSONWriter, ucScratchBuffer2,
                                   sizeof( ucScratchBuffer2 ) ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendBeginObject( &xJSONWriter ) != eAzureIoTSuccess ) ||
        ( AzureIoTHubClientProperties_BuilderBeginComponent( pxAzureIoTHubClient,
                                                             &xJSONWriter,
                                                             az_span_ptr( xComponent ),
                                                             az_span_size( xComponent ) ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendPropertyWithStringValue( &xJSONWriter,
                                                            az_span_ptr( xKey ),
                                                            ( uint32_t ) az_span_size( xKey ),
                                                            az_span_ptr( xValue ),
                                                            ( uint32_t ) az_span_size( xValue ) ) != eAzureIoTSuccess ) ||
        ( AzureIoTHubClientProperties_BuilderEndComponent( pxAzureIoTHubClient,
                                                           &xJSONWriter ) != eAzureIoTSuccess ) ||
        ( AzureIoTJSONWriter_AppendEndObject( &xJSONWriter ) != eAzureIoTSuccess ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to build reported property" );
    }
    else if( AzureIoTHubClient_SendPropertiesReported( pxAzureIoTHubClient,
                                                       ucScratchBuffer2,
                                                       ( uint32_t ) AzureIoTJSONWriter_GetBytesUsed( &xJSONWriter ),
                                                       &ulRequestId ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send reported properties" );
    }
    else if( AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Test failed: receive" );
    }
    else if( AzureIoTHubClient_SendTelemetry( pxAzureIoTHubClient,
                                              xCMD->pulReceivedData,
                                              xCMD->ulReceivedDataLength,
                                              NULL, eAzureIoTHubMessageQoS1, &usTelemetryPacketID ) != eAzureIoTSuccess )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send response" );
    }
    else if( ( prvWaitForPuback( pxAzureIoTHubClient, usTelemetryPacketID ) ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Telemetry message PUBACK never received!" );
    }

    return e2etestE2E_TEST_SUCCESS;
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
 * Find the command and intialized command handle
 *
 **/
static uint32_t prvE2ETestInitializeCMD( const char * pcMethodName,
                                         const char * ucData,
                                         uint32_t ulDataLength,
                                         E2E_TEST_COMMAND_HANDLE xCMD )
{
    uint32_t ulStatus;

    if( !strncmp( e2etestE2E_TEST_SEND_TELEMETRY_COMMAND,
                  pcMethodName, sizeof( e2etestE2E_TEST_SEND_TELEMETRY_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestSendTelemetryCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_ECHO_COMMAND,
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
    else if( !strncmp( e2etestE2E_TEST_DEVICE_PROVISIONING_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_DEVICE_PROVISIONING_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestDeviceProvisioningCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_REPORTED_PROPERTIES_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_REPORTED_PROPERTIES_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestReportedPropertiesCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_GET_TWIN_PROPERTIES_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_GET_TWIN_PROPERTIES_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestGetTwinPropertiesCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_VERIFY_DESIRED_PROPERTIES_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_VERIFY_DESIRED_PROPERTIES_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestVerifyDesiredPropertiesCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_WRITEABLE_PROPERTY_RESPONSE_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_WRITEABLE_PROPERTY_RESPONSE_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestWriteablePropertyResponseCommandExecute );
    }
    else if( !strncmp( e2etestE2E_TEST_COMPONENT_REPORTED_PROPERTY_COMMAND,
                       pcMethodName, sizeof( e2etestE2E_TEST_COMPONENT_REPORTED_PROPERTY_COMMAND ) - 1 ) )
    {
        ulStatus = prvE2EDeviceCommandInit( xCMD, ucData, ulDataLength,
                                            prvE2ETestReportComponentPropertiesCommandExecute );
    }
    else
    {
        ulStatus = e2etestE2E_TEST_FAILED;
    }

    RETURN_IF_FAILED( ulStatus, "Failed find a command" );

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
 * Direct method message callback
 *
 * */
void vHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                     void * pvContext )
{
    if( pxMethodCommandData == NULL )
    {
        pxMethodCommandData = pvPortMalloc( sizeof( AzureIoTHubClientCommandRequest_t ) );

        if( pxMethodCommandData == NULL )
        {
            printf( "Failed to allocated memory for direct method message" );
            configASSERT( false );
            return;
        }

        memcpy( pxMethodCommandData, pxMessage, sizeof( AzureIoTHubClientCommandRequest_t ) );

        if( pxMessage->pucRequestID )
        {
            pxMethodCommandData->pucRequestID = pvPortMalloc( pxMessage->usRequestIDLength );

            if( pxMethodCommandData->pucRequestID != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->pucRequestID,
                        pxMessage->pucRequestID, pxMessage->usRequestIDLength );
            }
        }

        if( pxMessage->pucCommandName )
        {
            pxMethodCommandData->pucCommandName = pvPortMalloc( pxMessage->usCommandNameLength );

            if( pxMethodCommandData->pucCommandName != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->pucCommandName,
                        pxMessage->pucCommandName, pxMessage->usCommandNameLength );
            }
        }

        if( pxMessage->pvMessagePayload )
        {
            pxMethodCommandData->pvMessagePayload = pvPortMalloc( pxMessage->ulPayloadLength );

            if( pxMethodCommandData->pvMessagePayload != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->pvMessagePayload,
                        pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
            }
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
uint32_t ulE2EDeviceProcessCommands( AzureIoTHubClient_t * pxAzureIoTHubClient )
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

    do
    {
        xResult = AzureIoTHubClient_ProcessLoop( pxAzureIoTHubClient,
                                                 e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC );

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
                if( ( ulStatus = xCMD.xExecute( &xCMD, pxAzureIoTHubClient ) ) != e2etestE2E_TEST_SUCCESS )
                {
                    LogError( ( "Failed to execute, error code : %d", ulStatus ) );
                }

                prvE2EDeviceCommandDeinit( &xCMD );
            }
        }
        else if( pxMethodCommandData != NULL )
        {
            pucMethodData = pxMethodCommandData;
            pxMethodCommandData = NULL;

            if( ( ulStatus = prvE2ETestInitializeCMD( pucMethodData->pucCommandName,
                                                      pucMethodData->pvMessagePayload,
                                                      pucMethodData->ulPayloadLength, &xCMD ) ) )
            {
                LogError( ( "Failed to parse the command, error code : %d", ulStatus ) );
                ucErrorReport = "{\"msg\" : \"Failed to parse the command\"}";
            }
            else
            {
                pucMethodData->pvMessagePayload = NULL;

                if( ( ulStatus = xCMD.xExecute( &xCMD, pxAzureIoTHubClient ) ) != e2etestE2E_TEST_SUCCESS )
                {
                    LogError( ( "Failed to execute, error code : %d", ulStatus ) );
                    ucErrorReport = "{\"msg\" : \"Failed to execute\"}";
                }
                else
                {
                    ucErrorReport = "";
                }

                prvE2EDeviceCommandDeinit( &xCMD );
            }

            AzureIoTHubClient_SendCommandResponse( pxAzureIoTHubClient,
                                                   pucMethodData, ulStatus,
                                                   ucErrorReport, strlen( ucErrorReport ) );
            prvFreeMethodData( pucMethodData );
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
    TickType_t xTickCount = 0;
    uint64_t ulTime = 0UL;

    xTickCount = xTaskGetTickCount();
    ulTime = ( uint64_t ) xTickCount / configTICK_RATE_HZ;
    ulTime = ( uint64_t ) ( ulTime + ulGlobalEntryTime );

    return ulTime;
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
