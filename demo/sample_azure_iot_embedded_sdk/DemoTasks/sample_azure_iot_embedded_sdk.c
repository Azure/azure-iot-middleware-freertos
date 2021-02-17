// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Azure Provisioning/IoTHub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "using_mbedtls.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( HOSTNAME ) && !defined( ENABLE_DPS_SAMPLE )
    #error "Define the config HOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( ENDPOINT ) && defined( ENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(democonfigROOT_CA_PEM) in demo_config.h."
#endif

#if defined( DEVICE_SYMMETRIC_KEY ) && defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define only one auth DEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

#if !defined( DEVICE_SYMMETRIC_KEY ) && !defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define one auth DEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

/*-----------------------------------------------------------*/

#define democonfigMQTT_BROKER_PORT    ( 8883 )

/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define mqttexampleRETRY_MAX_ATTEMPTS                     ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define mqttexampleRETRY_MAX_BACKOFF_DELAY_MS             ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define mqttexampleRETRY_BACKOFF_BASE_MS                  ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define mqttexampleCONNACK_RECV_TIMEOUT_MS                ( 1000U )

/**
 * @brief The number of topic filters to subscribe.
 */
#define mqttexampleTOPIC_COUNT                            ( 1 )

/**
 * @brief The Telemetry message published in this example.
 */
#define mqttexampleMESSAGE                                "Hello World!"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define mqttexampleTWIN_PROPERTY                          " { \"hello\": \"world\" } "

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define mqttexampleDELAY_BETWEEN_DEMO_ITERATIONS_TICKS    ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define mqttexamplePROCESS_LOOP_TIMEOUT_MS                ( 500U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define mqttexampleDELAY_BETWEEN_PUBLISHES_TICKS          ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define mqttexampleTRANSPORT_SEND_RECV_TIMEOUT_MS         ( 2000U )

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND                           ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK                             ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define provisioning_Registration_TIMEOUT_MS              ( 20U )

/*-----------------------------------------------------------*/

/* Define buffer for IoTHub info.  */
#ifdef ENABLE_DPS_SAMPLE
    static uint8_t sampleIotHubHostname[ 128 ];
    static uint8_t sampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* ENABLE_DPS_SAMPLE */

static uint8_t propertyBuffer[ 32 ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;

/*-----------------------------------------------------------*/

#ifdef ENABLE_DPS_SAMPLE

/**
 * @brief Gets the IoTHub endpoint and deviceId from Provisioning service.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] pIothubHostname  Pointer to uint8_t* IoTHub hostname return from Provisioning Service
 * @param[inout] iothubHostnameLength  Length of hostname
 * @param[out] piothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[inout] iothubDeviceIdLength  Length of deviceId
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** pIothubHostname,
                                      uint32_t * iothubHostnameLength,
                                      uint8_t ** piothubDeviceId,
                                      uint32_t * iothubDeviceIdLength );

#endif /* ENABLE_DPS_SAMPLE */

/**
 * @brief The task used to demonstrate the MQTT API.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters );


/**
 * @brief Connect to MQTT broker with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param[out] pxNetworkContext The parameter to return the created network context.
 *
 * @return The status of the final connection attempt.
 */
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pHostName,
                                                                  uint32_t port,
                                                                  NetworkCredentials_t * pxNetworkCredentials,
                                                                  NetworkContext_t * pxNetworkContext );

/**
 * @brief Unix time.
 *
 * @return Time in milliseconds.
 */
static uint64_t prvGetUnixTime( void );

/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Global start unix time
 */
static uint64_t ulGlobalEntryTime = 1639093301;

/*-----------------------------------------------------------*/

#ifdef DEVICE_SYMMETRIC_KEY

    static uint32_t calculateHMAC( const uint8_t * pKey,
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

#endif // DEVICE_SYMMETRIC_KEY

static void handle_c2d_message( AzureIoTHubClientC2DRequest_t * message,
                                void * context )
{
    ( void ) context;

    LogInfo( ( "C2D message payload : %.*s \r\n",
               message->payloadLength,
               message->messagePayload ) );
}

static void handle_direct_method( AzureIoTHubClientMethodRequest_t * message,
                                  void * context )
{
    LogInfo( ( "Method payload : %.*s \r\n",
               message->payloadLength,
               message->messagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) context;

    if( AzureIoTHubClient_SendMethodResponse( xHandle, message, 200,
                                              NULL, 0 ) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogInfo( ( "Error sending method response\r\n" ) );
    }
}

static void handle_device_twin_message( AzureIoTHubClientTwinResponse_t * message,
                                        void * context )
{
    ( void ) context;

    switch( message->messageType )
    {
        case AZURE_IOT_HUB_TWIN_GET_MESSAGE:
            LogInfo( ( "Device twin document GET received" ) );
            break;

        case AZURE_IOT_HUB_TWIN_REPORTED_RESPONSE_MESSAGE:
            LogInfo( ( "Device twin reported property response received" ) );
            break;

        case AZURE_IOT_HUB_TWIN_DESIRED_PROPERTY_MESSAGE:
            LogInfo( ( "Device twin desired property received" ) );
            break;

        default:
            LogError( ( "Unknown twin message" ) );
    }

    LogInfo( ( "Twin document payload : %.*s \r\n",
               message->payloadLength,
               message->messagePayload ) );
}

static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->disableSni = democonfigDISABLE_SNI;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->rootCaSize = sizeof( democonfigROOT_CA_PEM );
    #ifdef democonfigCLIENT_CERTIFICATE_PEM
        pxNetworkCredentials->pClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
        pxNetworkCredentials->clientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
        pxNetworkCredentials->pPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->privateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #endif

    return 0;
}

/*
 * @brief Create the task that demonstrates the AzureIoTHub demo
 */
void vStartSimpleMQTTDemo( void )
{
    /* This example uses a single application task, which in turn is used to
     * connect, subscribe, publish, unsubscribe and disconnect from the MQTT
     * broker. */
    xTaskCreate( prvAzureDemoTask,         /* Function that implements the task. */
                 "AzureDemoTask",          /* Text name for the task - only used for debugging. */
                 democonfigDEMO_STACKSIZE, /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                     /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,         /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                   /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/

static void prvAzureDemoTask( void * pvParameters )
{
    uint32_t ulPublishCount = 0U;
    const uint32_t ulMaxPublishCount = 5UL;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    TransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportStatus_t xNetworkStatus;
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientError_t xResult;
    uint32_t status;

    #ifdef ENABLE_DPS_SAMPLE
        uint8_t * iotHubHostname = NULL;
        uint8_t * iotHubDeviceId = NULL;
        uint32_t iotHubHostnameLength = 0;
        uint32_t iotHubDeviceIdLength = 0;
    #else
        uint8_t * iotHubHostname = ( uint8_t * ) HOSTNAME;
        uint8_t * iotHubDeviceId = ( uint8_t * ) DEVICE_ID;
        uint32_t iotHubHostnameLength = sizeof( HOSTNAME ) - 1;
        uint32_t iotHubDeviceIdLength = sizeof( DEVICE_ID ) - 1;
    #endif /* ENABLE_DPS_SAMPLE */

    ( void ) pvParameters;

    configASSERT( AzureIoTInit() == AZURE_IOT_SUCCESS );

    status = prvSetupNetworkCredentials( &xNetworkCredentials );
    configASSERT( status == 0 );

    #ifdef ENABLE_DPS_SAMPLE
        /* Run DPS.  */
        if( ( status = prvIoTHubInfoGet( &xNetworkCredentials, &iotHubHostname,
                                         &iotHubHostnameLength, &iotHubDeviceId,
                                         &iotHubDeviceIdLength ) ) != 0 )
        {
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", status ) );
            return;
        }
    #endif /* ENABLE_DPS_SAMPLE */

    xNetworkContext.pParams = &xTlsTransportParams;

    for( ; ; )
    {
        /****************************** Connect. ******************************/

        /* Attempt to establish TLS session with MQTT broker. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the broker after the configured
         * number of attempts. */
        xNetworkStatus = prvConnectToServerWithBackoffRetries( ( const char * ) iotHubHostname, democonfigMQTT_BROKER_PORT,
                                                               &xNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", iotHubHostname ) );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pNetworkContext = &xNetworkContext;
        xTransport.send = TLS_FreeRTOS_send;
        xTransport.recv = TLS_FreeRTOS_recv;

        AzureIoTHubClientOptions_t hubOptions = { 0 };
        hubOptions.pModuleId = ( const uint8_t * ) MODULE_ID;
        hubOptions.moduleIdLength = sizeof(MODULE_ID) - 1;

        /* Initialize MQTT library. */
        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          iotHubHostname, iotHubHostnameLength,
                                          iotHubDeviceId, iotHubDeviceIdLength,
                                          &hubOptions,
                                          ucSharedBuffer, sizeof( ucSharedBuffer ),
                                          prvGetUnixTime,
                                          &xTransport );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        #ifdef DEVICE_SYMMETRIC_KEY
            xResult = AzureIoTHubClient_SymmetricKeySet( &xAzureIoTHubClient,
                                                         ( const uint8_t * ) DEVICE_SYMMETRIC_KEY,
                                                         sizeof( DEVICE_SYMMETRIC_KEY ) - 1,
                                                         calculateHMAC );
            configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );
        #endif // DEVICE_SYMMETRIC_KEY

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                             false, mqttexampleCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /**************************** Enable features. ******************************/

        xResult = AzureIoTHubClient_CloudMessageEnable( &xAzureIoTHubClient, handle_c2d_message, &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_DirectMethodEnable( &xAzureIoTHubClient, handle_direct_method, &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_DeviceTwinEnable( &xAzureIoTHubClient, handle_device_twin_message, &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /* Get the device twin on boot */
        xResult = AzureIoTHubClient_DeviceTwinGet( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /* Create a bag of properties for the telemetry */
        AzureIoTMessageProperties_t propertyBag;
        xResult = AzureIoTMessagePropertiesInit( &propertyBag, propertyBuffer, 0, sizeof( propertyBag ) );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTMessagePropertiesAppend( &propertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                   ( uint8_t * ) "value", sizeof( "value" ) - 1 );

        /****************** Publish and Keep Alive Loop. **********************/
        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ulPublishCount = 0; ulPublishCount < ulMaxPublishCount; ulPublishCount++ )
        {
            xResult = AzureIoTHubClient_TelemetrySend( &xAzureIoTHubClient,
                                                       ( const uint8_t * ) mqttexampleMESSAGE,
                                                       sizeof( mqttexampleMESSAGE ) - 1,
                                                       &propertyBag );
            configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

            LogInfo( ( "Attempt to receive publish message from IoTHub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     mqttexamplePROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

            if( ulPublishCount % 2 == 0 )
            {
                /* Send reported property every other cycle */
                xResult = AzureIoTHubClient_DeviceTwinReportedSend( &xAzureIoTHubClient,
                                                                    ( const uint8_t * ) mqttexampleTWIN_PROPERTY,
                                                                    sizeof( mqttexampleTWIN_PROPERTY ) - 1,
                                                                    NULL );
                configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( mqttexampleDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        /**************************** Disconnect. *****************************/

        xResult = AzureIoTHubClient_DeviceTwinDisable( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_DirectMethodDisable( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_CloudMessageDisable( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /* Send an MQTT Disconnect packet over the already connected TLS over
         * TCP connection. There is no corresponding response for the disconnect
         * packet. After sending disconnect, client must close the network
         * connection. */
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the broker. */
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( mqttexampleDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}

#ifdef ENABLE_DPS_SAMPLE

    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** pIothubHostname,
                                      uint32_t * iothubHostnameLength,
                                      uint8_t ** piothubDeviceId,
                                      uint32_t * iothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        TlsTransportStatus_t xNetworkStatus;
        AzureIoTProvisioningClientError_t xResult;
        TransportInterface_t xTransport;
        uint32_t sampleIotHubHostnameLength = sizeof( sampleIotHubHostname );
        uint32_t sampleIotHubDeviceIdLength = sizeof( sampleIotHubDeviceId );

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        /****************************** Connect. ******************************/

        xNetworkStatus = prvConnectToServerWithBackoffRetries( ENDPOINT, democonfigMQTT_BROKER_PORT,
                                                               pXNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pNetworkContext = &xNetworkContext;
        xTransport.send = TLS_FreeRTOS_send;
        xTransport.recv = TLS_FreeRTOS_recv;

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", ENDPOINT ) );

        /* Initialize MQTT library. */
        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) ENDPOINT, sizeof( ENDPOINT ) - 1,
                                                   ( const uint8_t * ) ID_SCOPE, sizeof( ID_SCOPE ) - 1,
                                                   ( const uint8_t * ) REGISTRATION_ID, sizeof( REGISTRATION_ID ) - 1,
                                                   ucSharedBuffer, sizeof( ucSharedBuffer ),
                                                   prvGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );

        #ifdef DEVICE_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SymmetricKeySet( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) DEVICE_SYMMETRIC_KEY,
                                                                  sizeof( DEVICE_SYMMETRIC_KEY ) - 1,
                                                                  calculateHMAC );
            configASSERT( xResult == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );
        #endif // DEVICE_SYMMETRIC_KEY

        /****************** Publish and Keep Alive Loop. **********************/
        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           provisioning_Registration_TIMEOUT_MS );
        } while( xResult == AZURE_IOT_PROVISIONING_CLIENT_PENDING );

        configASSERT( xResult == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );

        xResult = AzureIoTProvisioningClient_HubGet( &xAzureIoTProvisioningClient,
                                                     sampleIotHubHostname, &sampleIotHubHostnameLength,
                                                     sampleIotHubDeviceId, &sampleIotHubDeviceIdLength );
        configASSERT( xResult == AZURE_IOT_PROVISIONING_CLIENT_SUCCESS );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        *pIothubHostname = sampleIotHubHostname;
        *iothubHostnameLength = sampleIotHubHostnameLength;
        *piothubDeviceId = sampleIotHubDeviceId;
        *iothubDeviceIdLength = sampleIotHubDeviceIdLength;

        return 0;
    }

#endif // ENABLE_DPS_SAMPLE

/*-----------------------------------------------------------*/

static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pHostName,
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
                                       mqttexampleRETRY_BACKOFF_BASE_MS,
                                       mqttexampleRETRY_MAX_BACKOFF_DELAY_MS,
                                       mqttexampleRETRY_MAX_ATTEMPTS );

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
                                               mqttexampleTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                               mqttexampleTRANSPORT_SEND_RECV_TIMEOUT_MS );

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
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
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

/*-----------------------------------------------------------*/