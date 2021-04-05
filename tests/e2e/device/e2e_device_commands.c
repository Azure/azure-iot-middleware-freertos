/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include "e2e_device_process_commands.h"

#include "azure/core/az_json.h"
#include "azure/core/az_span.h"

#define e2etestMESSAGE_BUFFER_SIZE                           10240

#define e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC              4

#define e2etestE2E_TEST_SUCCESS                              0
#define e2etestE2E_TEST_FAILED                               1
#define e2etestE2E_TEST_NOT_FOUND                            2


#define e2etestMETHOD_KEY                                    "method"
#define e2etestPAYLOAD_KEY                                   "payload"
#define e2etestPROPERTIES_KEY                                "properties"
#define e2etestID_SCOPE_KEY                                  "id_scope"
#define e2etestREGISTRATION_ID_KEY                           "registration_id"
#define e2etestSYMMETRIC_KEY                                 "symmetric_key"
#define e2etestSERVICE_ENDPOINT_KEY                          "service_endpoint"
#define e2etestDESIRED_PROPERTY_KEY                          "desired_property_key"
#define e2etestDESIRED_PROPERTY_VALUE                        "desired_property_value"

#define e2etestE2E_TEST_SEND_TELEMETRY_COMMAND               "send_telemetry"
#define e2etestE2E_TEST_ECHO_COMMAND                         "echo"
#define e2etestE2E_TEST_EXIT_COMMAND                         "exit"
#define e2etestE2E_TEST_DEVICE_PROVISIONING_COMMAND          "device_provisioning"
#define e2etestE2E_TEST_REPORTED_PROPERTIES_COMMAND          "reported_properties"
#define e2etestE2E_TEST_VERIFY_DESIRED_PROPERTIES_COMMAND    "verify_desired_properties"
#define e2etestE2E_TEST_GET_TWIN_PROPERTIES_COMMAND          "get_twin_properties"
#define e2etestE2E_TEST_CONNECTED_MESSAGE                    "\"CONNECTED\""

#define RETURN_IF_FAILED( e, msg )                    \
    if( e != e2etestE2E_TEST_SUCCESS )                \
    {                                                 \
        printf( "[%s][%d]%s : error code: 0x%0x\r\n", \
                __FILE__, __LINE__, msg, e );         \
        return ( e );                                 \
    }

typedef struct E2E_TEST_COMMAND_STRUCT
{
    uint32_t ( * execute )( struct E2E_TEST_COMMAND_STRUCT * pxCMD,
                            AzureIoTHubClient_t * xAzureIoTHubClientHandle );
    const uint8_t * pulReceivedData;
    uint32_t ulReceivedDataLength;
} E2E_TEST_COMMAND;

typedef uint32_t ( * EXECUTE_FN ) ( struct E2E_TEST_COMMAND_STRUCT * pxCMD,
                                    AzureIoTHubClient_t * xAzureIoTHubClientHandle );
typedef E2E_TEST_COMMAND * E2E_TEST_COMMAND_HANDLE;

static uint64_t ulGlobalEntryTime = 1639093301;
static uint8_t * ucC2DCommandData = NULL;
static uint32_t ulC2DCommandDataLength = 0;
static AzureIoTHubClientMethodRequest_t * pxMethodCommandData = NULL;
static AzureIoTHubClientTwinResponse_t * pxTwinMessage = NULL;
static uint8_t ucMethodNameBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucScratchBuffer2[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucMessageBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static uint8_t ucStatusOKTelemetry[] = "{\"status\": \"OK\"}";
static uint32_t ulContinueProcessingCMD = 1;
static uint8_t ucSharedBuffer[ e2etestMESSAGE_BUFFER_SIZE ];
static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;

static void prvFreeTwinData( AzureIoTHubClientTwinResponse_t * pxTwinData )
{
    if( pxTwinData )
    {
        vPortFree( ( void * ) pxTwinData->messagePayload );
        vPortFree( ( void * ) pxTwinData );
    }
}

static void prvFreeMethodData( AzureIoTHubClientMethodRequest_t * pxMethodData )
{
    if( pxMethodData )
    {
        vPortFree( ( void * ) pxMethodData->messagePayload );
        vPortFree( ( void * ) pxMethodData->methodName );
        vPortFree( ( void * ) pxMethodData->requestId );
        vPortFree( ( void * ) pxMethodData );
    }
}

static uint32_t prvStartProvisioning( az_span * pxEndpoint,
                                      az_span * pxIDScope,
                                      az_span * pxRegistrationId,
                                      az_span * pxSymmetricKey,
                                      az_span * pxDeviceId,
                                      az_span * pxHostname )
{
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTProvisioningClientResult_t xResult;
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

    /****************************** Connect. ******************************/

    if( xConnectToServerWithBackoffRetries( az_span_ptr( *pxEndpoint ), 8883,
                                            &xNetworkCredentials,
                                            &xNetworkContext ) != TLS_TRANSPORT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed transport connect!" );
    }

    xTransport.pNetworkContext = &xNetworkContext;
    xTransport.send = TLS_FreeRTOS_send;
    xTransport.recv = TLS_FreeRTOS_recv;

    if( AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                         ( const uint8_t * ) az_span_ptr( *pxEndpoint ),
                                         az_span_size( *pxEndpoint ),
                                         ( const uint8_t * ) az_span_ptr( *pxIDScope ),
                                         az_span_size( *pxIDScope ),
                                         ( const uint8_t * ) az_span_ptr( *pxRegistrationId ),
                                         az_span_size( *pxRegistrationId ),
                                         NULL, ucSharedBuffer,
                                         sizeof( ucSharedBuffer ), ulGetUnixTime,
                                         &xTransport ) != AZURE_IOT_PROVISIONING_CLIENT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed to init provisioning client!" );
    }

    if( AzureIoTProvisioningClient_SymmetricKeySet( &xAzureIoTProvisioningClient,
                                                    ( const uint8_t * ) az_span_ptr( *pxSymmetricKey ),
                                                    az_span_size( *pxSymmetricKey ),
                                                    ulCalculateHMAC ) != AZURE_IOT_PROVISIONING_CLIENT_SUCCESS )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed to set symmetric key!" );
    }

    do
    {
        xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                       e2etestProvisioning_Registration_TIMEOUT_MS );
    } while( xResult == AZURE_IOT_PROVISIONING_CLIENT_PENDING );

    if( xResult != AZURE_IOT_PROVISIONING_CLIENT_SUCCESS )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed to register !" );
    }

    if( AzureIoTProvisioningClient_HubGet( &xAzureIoTProvisioningClient,
                                           az_span_ptr( *pxHostname ),
                                           &ulHostnameLength,
                                           az_span_ptr( *pxDeviceId ),
                                           &ulDeviceIdLength ) != AZURE_IOT_PROVISIONING_CLIENT_SUCCESS )
    {
        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed to get iothub info!" );
    }

    *pxHostname = az_span_create( az_span_ptr( *pxHostname ), ( int32_t ) ulHostnameLength );
    *pxDeviceId = az_span_create( az_span_ptr( *pxDeviceId ), ( int32_t ) ulDeviceIdLength );

    AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

    return e2etestE2E_TEST_SUCCESS;
}

static uint32_t prvGetJsonValueForKey( az_json_reader * pxState,
                                       const char * pcKey,
                                       az_span * pxValue )
{
    uint32_t ulStatus = e2etestE2E_TEST_NOT_FOUND;
    az_span xKeySpan = az_span_create( ( uint8_t * ) pcKey, strlen( pcKey ) );
    int32_t ulLength;

    while( az_result_succeeded( az_json_reader_next_token( pxState ) ) )
    {
        if( az_json_token_is_text_equal( &( pxState->token ), xKeySpan ) )
        {
            if( az_result_failed( az_json_reader_next_token( pxState ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                LogError( ( "failed next token, error code : %d", ulStatus ) );
                break;
            }

            if( az_result_failed( az_json_token_get_string( &pxState->token,
                                                            ( char * ) az_span_ptr( *pxValue ),
                                                            az_span_size( *pxValue ), &ulLength ) ) )
            {
                ulStatus = e2etestE2E_TEST_FAILED;
                LogError( ( "failed get string value, error code : %d", ulStatus ) );
                break;
            }

            *pxValue = az_span_create( az_span_ptr( *pxValue ), ulLength );
            ulStatus = e2etestE2E_TEST_SUCCESS;
            break;
        }
    }

    return ulStatus;
}

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

static uint32_t prvMoveToObject( az_json_reader * pxState,
                                 const char * pcKey )
{
    uint32_t ulStatus = e2etestE2E_TEST_NOT_FOUND;
    az_span key_span = az_span_create( ( uint8_t * ) pcKey, strlen( pcKey ) );

    while( az_result_succeeded( az_json_reader_next_token( pxState ) ) )
    {
        if( pxState->token.kind != AZ_JSON_TOKEN_PROPERTY_NAME )
        {
            continue;
        }

        if( az_json_token_is_text_equal( &( pxState->token ), key_span ) )
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

static uint32_t prvAddAllProperties( az_json_reader * pxState,
                                     AzureIoTHubClient_t * xAzureIoTHubClientHandle,
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

            if( AzureIoT_MessagePropertiesAppend( pxProperties,
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

static uint32_t prvE2ETestSendTelemertyCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                       AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_json_reader xState;
    az_span xPayloadSpan = az_span_create( ucScratchBuffer, sizeof( ucScratchBuffer ) );
    AzureIoTMessageProperties_t xProperties;


    if( AzureIoT_MessagePropertiesInit( &xProperties,
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
            if( ( ulStatus = prvAddAllProperties( &xState, xAzureIoTHubClientHandle,
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
            ( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                               az_span_ptr( xPayloadSpan ),
                                               az_span_size( xPayloadSpan ),
                                               &xProperties ) != AZURE_IOT_HUB_CLIENT_SUCCESS ) )
        {
            ulStatus = e2etestE2E_TEST_FAILED;
            LogError( ( "Telemetry message send failed!", ulStatus ) );
        }

        if( ulStatus == e2etestE2E_TEST_SUCCESS )
        {
            LogInfo( ( "Successfully done with prvE2ETestSendTelemertyCommandExecute" ) );
        }
    }

    return ulStatus;
}

static uint32_t prvE2ETestEchoCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                              AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    AzureIoTHubClientResult_t xStatus;

    if( ( xStatus = AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                                     xCMD->pulReceivedData,
                                                     xCMD->ulReceivedDataLength,
                                                     NULL ) ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "Telemetry message send failed!, error code %d", xStatus ) );
    }
    else
    {
        LogInfo( ( "Successfully done with prvE2ETestEchoCommandExecute" ) );
    }

    return ( uint32_t ) xStatus;
}

static uint32_t prvE2ETestExitCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                              AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    ulContinueProcessingCMD = 0;

    return e2etestE2E_TEST_SUCCESS;
}

static uint32_t prvE2ETestDeviceProvisioningCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                            AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    uint32_t ulStatus;
    uint32_t ulMessageLength;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xIdScope = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xRegistrationId = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xSymmetricKey = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xServiceEndpoint = az_span_create( ucScratchBuffer + 3 * e2etestMESSAGE_BUFFER_SIZE / 4, e2etestMESSAGE_BUFFER_SIZE / 4 );
    az_span xDeviceId = az_span_create( ucScratchBuffer2, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xEndpoint = az_span_create( ucScratchBuffer2 + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xHostname = az_span_create( ucScratchBuffer2 + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestID_SCOPE_KEY, &xIdScope );
    RETURN_IF_FAILED( ulStatus, "failed to find id_scope!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestREGISTRATION_ID_KEY, &xRegistrationId );
    RETURN_IF_FAILED( ulStatus, "failed to find registration_id!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestSYMMETRIC_KEY, &xSymmetricKey );
    RETURN_IF_FAILED( ulStatus, "failed to find symmetric key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestSERVICE_ENDPOINT_KEY, &xServiceEndpoint );
    RETURN_IF_FAILED( ulStatus, "failed to find service endpoint key!" );

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

    if( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                         ucMessageBuffer, ulMessageLength,
                                         NULL ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Fail to send telemetry" );
    }

    return e2etestE2E_TEST_SUCCESS;
}

static uint32_t prvE2ETestReportedPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                            AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    uint32_t ulStatus;
    uint32_t ulRequestId;

    if( AzureIoTHubClient_DeviceTwinReportedSend( xAzureIoTHubClientHandle,
                                                  xCMD->pulReceivedData,
                                                  xCMD->ulReceivedDataLength,
                                                  &ulRequestId ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "Failed to send reported properties" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_ProcessLoop( xAzureIoTHubClientHandle,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "recevice timeout" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( ( pxTwinMessage == NULL ) ||
             ( pxTwinMessage->requestId != ulRequestId ) ||
             ( pxTwinMessage->messageStatus < 200 ) ||
             ( pxTwinMessage->messageStatus >= 300 ) )
    {
        LogError( ( "Invalid response from server" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                              xCMD->pulReceivedData,
                                              xCMD->ulReceivedDataLength,
                                              NULL ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}

static uint32_t prvE2ETestGetTwinPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                           AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    uint32_t ulStatus;

    if( AzureIoTHubClient_DeviceTwinGet( xAzureIoTHubClientHandle ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "Failed to request twin properties" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_ProcessLoop( xAzureIoTHubClientHandle,
                                            e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "recevice timeout" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( pxTwinMessage == NULL )
    {
        LogError( ( "Invalid response from server" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else if( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                              pxTwinMessage->messagePayload,
                                              pxTwinMessage->payloadLength,
                                              NULL ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogError( ( "Failed to send response" ) );
        ulStatus = e2etestE2E_TEST_FAILED;
    }
    else
    {
        ulStatus = e2etestE2E_TEST_SUCCESS;
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    return ulStatus;
}

static uint32_t prvE2ETestVerifyDesiredPropertiesCommandExecute( E2E_TEST_COMMAND_HANDLE xCMD,
                                                                 AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    uint32_t ulStatus;
    az_span xJsonSpan = az_span_create( ( uint8_t * ) xCMD->pulReceivedData, xCMD->ulReceivedDataLength );
    az_span xKey = az_span_create( ucScratchBuffer, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValue = az_span_create( ucScratchBuffer + e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );
    az_span xValueReceived = az_span_create( ucScratchBuffer + 2 * e2etestMESSAGE_BUFFER_SIZE / 3, e2etestMESSAGE_BUFFER_SIZE / 3 );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_KEY, &xKey );
    RETURN_IF_FAILED( ulStatus, "failed to find desired_property_key!" );

    ulStatus = prvGetValueForKey( xJsonSpan, e2etestDESIRED_PROPERTY_VALUE, &xValue );
    RETURN_IF_FAILED( ulStatus, "failed to find desired_property_value!" );

    if( ( pxTwinMessage == NULL ) &&
        ( AzureIoTHubClient_ProcessLoop( xAzureIoTHubClientHandle,
                                         e2etestPROCESS_LOOP_WAIT_TIMEOUT_IN_SEC ) != AZURE_IOT_HUB_CLIENT_SUCCESS ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to receive desired properties" );
    }

    xJsonSpan = az_span_create( ( uint8_t * ) pxTwinMessage->messagePayload, pxTwinMessage->payloadLength );
    ulStatus = prvGetValueForKey( xJsonSpan, az_span_ptr( xKey ), &xValueReceived );
    RETURN_IF_FAILED( ulStatus, "failed to find desired_property_key!" );

    if( !az_span_is_content_equal( xValueReceived, xValue ) )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "failed to find desired_property_value!" );
    }

    prvFreeTwinData( pxTwinMessage );
    pxTwinMessage = NULL;

    snprintf( ucScratchBuffer2, sizeof( ucScratchBuffer2 ), "{\"%.*s\": \"%.*s\"}",
              az_span_size( xKey ), az_span_ptr( xKey ), az_span_size( xValue ), az_span_ptr( xValue ) );

    if( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                         ucScratchBuffer2,
                                         strlen( ucScratchBuffer2 ),
                                         NULL ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to send response" );
    }

    return e2etestE2E_TEST_SUCCESS;
}

static uint32_t prvE2EDeviceCommandInit( E2E_TEST_COMMAND_HANDLE xCMD,
                                         const char * ucData,
                                         uint32_t ulDataLength,
                                         EXECUTE_FN xExecute )
{
    xCMD->execute = xExecute;
    xCMD->pulReceivedData = ( const uint8_t * ) ucData;
    xCMD->ulReceivedDataLength = ulDataLength;

    return e2etestE2E_TEST_SUCCESS;
}

static void prvE2EDeviceCommandDeinit( E2E_TEST_COMMAND_HANDLE xCMD )
{
    vPortFree( ( void * ) xCMD->pulReceivedData );
    memset( xCMD, 0, sizeof( E2E_TEST_COMMAND ) );
}

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
                                            prvE2ETestSendTelemertyCommandExecute );
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
    else
    {
        ulStatus = e2etestE2E_TEST_FAILED;
    }

    RETURN_IF_FAILED( ulStatus, "Failed find a command" );

    return ulStatus;
}

static uint32_t prvE2ETestParseCommand( uint8_t * pucData,
                                        uint32_t ulDataLength,
                                        E2E_TEST_COMMAND_HANDLE xCMD )
{
    uint32_t status;
    az_span json_span;
    az_json_reader state;
    az_span method_name_span = az_span_create( ucMethodNameBuffer, sizeof( ucMethodNameBuffer ) );

    json_span = az_span_create( pucData, ulDataLength );

    LogInfo( ( "Command %.*s\r\n", ulDataLength, pucData ) );

    if( az_json_reader_init( &state, json_span, NULL ) != AZ_OK )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Failed to parse json" );
    }

    status = prvGetJsonValueForKey( &state, e2etestMETHOD_KEY, &method_name_span );
    RETURN_IF_FAILED( status, "Failed to parse the command" );

    status = prvE2ETestInitializeCMD( az_span_ptr( method_name_span ), pucData, ulDataLength, xCMD );
    RETURN_IF_FAILED( status, "Failed to initialize the command" );

    return( status );
}

void vHandleCloudMessage( AzureIoTHubClientCloudMessageRequest_t * message,
                          void * pvContext )
{
    if( ucC2DCommandData == NULL )
    {
        ucC2DCommandData = pvPortMalloc( message->payloadLength );
        ulC2DCommandDataLength = message->payloadLength;

        if( ucC2DCommandData != NULL )
        {
            memcpy( ucC2DCommandData, message->messagePayload, message->payloadLength );
        }
    }
}

void vHandleDirectMethod( AzureIoTHubClientMethodRequest_t * message,
                          void * pvContext )
{
    if( pxMethodCommandData == NULL )
    {
        pxMethodCommandData = pvPortMalloc( sizeof( AzureIoTHubClientMethodRequest_t ) );

        if( pxMethodCommandData != NULL )
        {
            memcpy( pxMethodCommandData, message, sizeof( AzureIoTHubClientMethodRequest_t ) );
        }

        if( message->requestId )
        {
            pxMethodCommandData->requestId = pvPortMalloc( message->requestIdLength );

            if( pxMethodCommandData->requestId != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->requestId, message->requestId, message->requestIdLength );
            }
        }

        if( message->methodName )
        {
            pxMethodCommandData->methodName = pvPortMalloc( message->methodNameLength );

            if( pxMethodCommandData->methodName != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->methodName, message->methodName, message->methodNameLength );
            }
        }

        if( message->payloadLength != 0 )
        {
            pxMethodCommandData->messagePayload = pvPortMalloc( message->payloadLength );

            if( pxMethodCommandData->messagePayload != NULL )
            {
                memcpy( ( void * ) pxMethodCommandData->messagePayload, message->messagePayload, message->payloadLength );
            }
        }
    }
}
void vHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * message,
                               void * pvContext )
{
    if( pxTwinMessage == NULL )
    {
        pxTwinMessage = pvPortMalloc( sizeof( AzureIoTHubClientTwinResponse_t ) );

        if( pxTwinMessage != NULL )
        {
            memcpy( pxTwinMessage, message, sizeof( AzureIoTHubClientTwinResponse_t ) );
        }

        if( message->payloadLength != 0 )
        {
            pxTwinMessage->messagePayload = pvPortMalloc( message->payloadLength );

            if( pxTwinMessage->messagePayload != NULL )
            {
                memcpy( ( void * ) pxTwinMessage->messagePayload, message->messagePayload, message->payloadLength );
            }
        }
    }
}

uint32_t ulE2EDeviceProcessCommands( AzureIoTHubClient_t * xAzureIoTHubClientHandle )
{
    AzureIoTHubClientResult_t xResult;
    E2E_TEST_COMMAND xCMD = { 0 };
    uint32_t ulStatus = e2etestE2E_TEST_SUCCESS;
    uint8_t * pucData = NULL;
    AzureIoTHubClientMethodRequest_t * pucMethodData = NULL;
    uint32_t ulDataLength;
    uint8_t * ucErrorReport = NULL;

    if( AzureIoTHubClient_TelemetrySend( xAzureIoTHubClientHandle,
                                         e2etestE2E_TEST_CONNECTED_MESSAGE,
                                         sizeof( e2etestE2E_TEST_CONNECTED_MESSAGE ) - 1,
                                         NULL ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        RETURN_IF_FAILED( e2etestE2E_TEST_FAILED, "Report connected failed!" );
    }

    do
    {
        xResult = AzureIoTHubClient_ProcessLoop( xAzureIoTHubClientHandle,
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
                if( ( ulStatus = xCMD.execute( &xCMD, xAzureIoTHubClientHandle ) ) != e2etestE2E_TEST_SUCCESS )
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

            if( ( ulStatus = prvE2ETestInitializeCMD( pucMethodData->methodName,
                                                      pucMethodData->messagePayload,
                                                      pucMethodData->payloadLength, &xCMD ) ) )
            {
                LogError( ( "Failed to parse the command, error code : %d", ulStatus ) );
                ucErrorReport = "{\"msg\" : \"Failed to parse the command\"}";
            }
            else
            {
                pucMethodData->messagePayload = NULL;

                if( ( ulStatus = xCMD.execute( &xCMD, xAzureIoTHubClientHandle ) ) != e2etestE2E_TEST_SUCCESS )
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

            AzureIoTHubClient_SendMethodResponse( xAzureIoTHubClientHandle,
                                                  pucMethodData, ulStatus,
                                                  ucErrorReport, strlen( ucErrorReport ) );
            prvFreeMethodData( pucMethodData );
        }
    } while( ( ulStatus == e2etestE2E_TEST_SUCCESS ) &&
             ulContinueProcessingCMD );

    return ulStatus;
}

uint64_t ulGetUnixTime( void )
{
    TickType_t xTickCount = 0;
    uint64_t ulTime = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTime = ( uint64_t ) xTickCount / configTICK_RATE_HZ;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTime = ( uint64_t ) ( ulTime + ulGlobalEntryTime );

    return ulTime;
}


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

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects to
         * the MQTT broker as specified in HOSTNAME and
         * democonfigMQTT_BROKER_PORT at the top of this file. */
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_FreeRTOS_Connect( pxNetworkContext,
                                               pHostName, port,
                                               pxNetworkCredentials,
                                               e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                               e2etestTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != TLS_TRANSPORT_SUCCESS )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
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

uint32_t ulCalculateHMAC( const uint8_t * pKey,
                          uint32_t keyLength,
                          const uint8_t * pData,
                          uint32_t dataLength,
                          uint8_t * pOutput,
                          uint32_t outputLength,
                          uint32_t * pBytesCopied )
{
    uint32_t ret;

    if( outputLength < 32 )
    {
        return 1;
    }

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init( &ctx );

    if( mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 1 ) ||
        mbedtls_md_hmac_starts( &ctx, pKey, keyLength ) ||
        mbedtls_md_hmac_update( &ctx, pData, dataLength ) ||
        mbedtls_md_hmac_finish( &ctx, pOutput ) )
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    mbedtls_md_free( &ctx );
    *pBytesCopied = 32;
    return ret;
}
