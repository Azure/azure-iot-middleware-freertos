/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

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
#include "using_mbedtls_pkcs11.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( democonfigHOSTNAME ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config democonfigHOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( democonfigENDPOINT ) && defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(democonfigROOT_CA_PEM) in demo_config.h."
#endif

/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleazureiotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS                 ( 1000U )

/**
 * @brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                                 "Hello World!"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotTWIN_PROPERTY                           "{ \"hello\": \"world\" }"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define sampleazureiotPROCESS_LOOP_TIMEOUT_MS                 ( 500U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 20U )

/*-----------------------------------------------------------*/

/* Define buffer for IoTHub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

static uint8_t ucPropertyBuffer[ 32 ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;

/*-----------------------------------------------------------*/

extern uint32_t DemoKeyProvision();

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Gets the IoTHub endpoint and deviceId from Provisioning service.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoTHub hostname return from Provisioning Service
 * @param[inout] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[inout] pulIothubDeviceIdLength  Length of deviceId
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength );

#endif /* democonfigENABLE_DPS_SAMPLE */

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
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                                  uint32_t ulPort,
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


static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

static void prvHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
                                   void * pvContext )
{
    LogInfo( ( "Method payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if( AzureIoTHubClient_SendMethodResponse( xHandle, pxMessage, 200,
                                              NULL, 0 ) != eAzureIoTHubClientSuccess )
    {
        LogInfo( ( "Error sending method response\r\n" ) );
    }
}
/*-----------------------------------------------------------*/

static void prvHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubTwinGetMessage:
            LogInfo( ( "Device twin document GET received" ) );
            break;

        case eAzureIoTHubTwinReportedResponseMessage:
            LogInfo( ( "Device twin reported property response received" ) );
            break;

        case eAzureIoTHubTwinDesiredPropertyMessage:
            LogInfo( ( "Device twin desired property received" ) );
            break;

        default:
            LogError( ( "Unknown twin message" ) );
    }

    LogInfo( ( "Twin document payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

static uint32_t prvInitMbedtls( void )
{
    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );

    return 0;
}
/*-----------------------------------------------------------*/

static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->disableSni = democonfigDISABLE_SNI;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->rootCaSize = sizeof( democonfigROOT_CA_PEM );

    /* Provision Demo keys */
    configASSERT( DemoKeyProvision() == 0 );

    return 0;
}
/*-----------------------------------------------------------*/

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
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportStatus_t xNetworkStatus;
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTMessageProperties_t xPropertyBag;
    bool xSessionPresent;

    #ifdef democonfigENABLE_DPS_SAMPLE
        uint8_t * pucIotHubHostname = NULL;
        uint8_t * pucIotHubDeviceId = NULL;
        uint32_t pulIothubHostnameLength = 0;
        uint32_t pulIothubDeviceIdLength = 0;
    #else
        uint8_t * pucIotHubHostname = ( uint8_t * ) democonfigHOSTNAME;
        uint8_t * pucIotHubDeviceId = ( uint8_t * ) democonfigDEVICE_ID;
        uint32_t pulIothubHostnameLength = sizeof( democonfigHOSTNAME ) - 1;
        uint32_t pulIothubDeviceIdLength = sizeof( democonfigDEVICE_ID ) - 1;
    #endif /* democonfigENABLE_DPS_SAMPLE */

    ( void ) pvParameters;

    /* Init crypto */
    prvInitMbedtls();

    configASSERT( AzureIoT_Init() == eAzureIoTSuccess );

    ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials );
    configASSERT( ulStatus == 0 );

    #ifdef democonfigENABLE_DPS_SAMPLE
        /* Run DPS.  */
        if( ( ulStatus = prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname,
                                           &pulIothubHostnameLength, &pucIotHubDeviceId,
                                           &pulIothubDeviceIdLength ) ) != 0 )
        {
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ulStatus ) );
            return;
        }
    #endif /* democonfigENABLE_DPS_SAMPLE */

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
        xNetworkStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                               democonfigMQTT_BROKER_PORT,
                                                               &xNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_FreeRTOS_send;
        xTransport.xRecv = TLS_FreeRTOS_recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
        xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;

        /* Initialize MQTT library. */
        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucSharedBuffer, sizeof( ucSharedBuffer ),
                                          prvGetUnixTime,
                                          &xTransport );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                             false, &xSessionPresent,
                                             sampleazureiotCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /**************************** Enable features. ******************************/

        xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                   &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_SubscribeDirectMethod( &xAzureIoTHubClient, prvHandleDirectMethod,
                                                           &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_SubscribeDeviceTwin( &xAzureIoTHubClient, prvHandleDeviceTwinMessage,
                                                         &xAzureIoTHubClient, ULONG_MAX );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Get the device twin on boot */
        xResult = AzureIoTHubClient_GetDeviceTwin( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Create a bag of properties for the telemetry */
        xResult = AzureIoT_MessagePropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( xPropertyBag ) );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoT_MessagePropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                    ( uint8_t * ) "value", sizeof( "value" ) - 1 );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /****************** Publish and Keep Alive Loop. **********************/
        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ulPublishCount = 0; ulPublishCount < ulMaxPublishCount; ulPublishCount++ )
        {
            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                       ( const uint8_t * ) sampleazureiotMESSAGE,
                                                       sizeof( sampleazureiotMESSAGE ) - 1,
                                                       &xPropertyBag );
            configASSERT( xResult == eAzureIoTHubClientSuccess );

            LogInfo( ( "Attempt to receive publish message from IoTHub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTHubClientSuccess );

            if( ulPublishCount % 2 == 0 )
            {
                /* Send reported property every other cycle */
                xResult = AzureIoTHubClient_SendDeviceTwinReported( &xAzureIoTHubClient,
                                                                    ( const uint8_t * ) sampleazureiotTWIN_PROPERTY,
                                                                    sizeof( sampleazureiotTWIN_PROPERTY ) - 1,
                                                                    NULL );
                configASSERT( xResult == eAzureIoTHubClientSuccess );
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        /**************************** Disconnect. *****************************/

        xResult = AzureIoTHubClient_UnsubscribeDeviceTwin( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_UnsubscribeDirectMethod( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Send an MQTT Disconnect packet over the already connected TLS over
         * TCP connection. There is no corresponding response for the disconnect
         * packet. After sending disconnect, client must close the network
         * connection. */
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the broker. */
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        TlsTransportStatus_t xNetworkStatus;
        AzureIoTProvisioningClientResult_t xResult;
        AzureIoTTransportInterface_t xTransport;
        uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
        uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        /****************************** Connect. ******************************/

        xNetworkStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigMQTT_BROKER_PORT,
                                                               pXNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_FreeRTOS_send;
        xTransport.xRecv = TLS_FreeRTOS_recv;

        /* Initialize MQTT library. */
        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   sizeof( democonfigREGISTRATION_ID ) - 1,
                                                   NULL, ucSharedBuffer, sizeof( ucSharedBuffer ),
                                                   prvGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTProvisioningPending );

        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                              ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                              ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        *ppucIothubHostname = ucSampleIotHubHostname;
        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

        return 0;
    }

#endif // democonfigENABLE_DPS_SAMPLE
/*-----------------------------------------------------------*/

static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
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
                                       sampleazureiotRETRY_BACKOFF_BASE_MS,
                                       sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleazureiotRETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects to
         * the MQTT broker as specified in democonfigHOSTNAME and
         * democonfigMQTT_BROKER_PORT at the top of this file. */
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pcHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_FreeRTOS_Connect( pxNetworkContext,
                                               pcHostName, port,
                                               pxNetworkCredentials,
                                               sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                               sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS );

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
