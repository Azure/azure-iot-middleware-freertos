/*
 * FreeRTOS V202011.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * Demo for showing use of the MQTT API using a mutually authenticated
 * network connection.
 *
 * The Example shown below uses MQTT APIs to create MQTT messages and send them
 * over the mutually authenticated network connection established with the
 * MQTT broker. This example is single threaded and uses statically allocated
 * memory. It uses QoS1 for sending to and receiving messages from the broker.
 *
 * A mutually authenticated TLS connection is used to connect to the
 * MQTT message broker in this example. Define HOSTNAME,
 * democonfigROOT_CA_PEM, democonfigCLIENT_CERTIFICATE_PEM,
 * and democonfigCLIENT_PRIVATE_KEY_PEM in demo_config.h to establish a
 * mutually authenticated connection.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Azure IoTHub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "using_mbedtls.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined(HOSTNAME) && !defined(ENABLE_DPS_SAMPLE)
    #error "Define the config HOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined(ENDPOINT) && defined(ENABLE_DPS_SAMPLE)
#error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(democonfigROOT_CA_PEM) in demo_config.h."
#endif

/*-----------------------------------------------------------*/

#ifndef democonfigMQTT_BROKER_PORT

/**
 * @brief The port to use for the demo.
 */
    #define democonfigMQTT_BROKER_PORT    ( 8883 )
#endif

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
 * @brief The MQTT message published in this example.
 */
#define mqttexampleMESSAGE                                "Hello World!"

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
#define mqttexampleTRANSPORT_SEND_RECV_TIMEOUT_MS         ( 200U )

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND    ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK      ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/*-----------------------------------------------------------*/

/* Define buffer for IoTHub info.  */
#ifdef ENABLE_DPS_SAMPLE
static uint8_t sampleIotHubHostname[128];
static uint8_t sampleIotHubDeviceId[128];
#endif /* ENABLE_DPS_SAMPLE */

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

/*-----------------------------------------------------------*/

#ifdef ENABLE_DPS_SAMPLE
static uint32_t prvDPSDemo( NetworkCredentials_t * pXNetworkCredentials,
                        uint8_t ** pIothubHostname, uint32_t * iothubHostnameLength,
                        uint8_t ** piothubDeviceId, uint32_t * iothubDeviceIdLength );
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
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pHostName, uint32_t port,
                                                                  NetworkCredentials_t * pxNetworkCredentials,
                                                                  NetworkContext_t * pxNetworkContext );
/**
 * @brief The timer query function provided to the MQTT context.
 *
 * @return Time in milliseconds.
 */
static uint32_t prvGetTimeMs( void );

/**
 * @brief Process a response or ack to an MQTT request (PING, PUBLISH,
 * SUBSCRIBE or UNSUBSCRIBE). This function processes PINGRESP, PUBACK,
 * SUBACK, and UNSUBACK.
 *
 * @param[in] pxIncomingPacket is a pointer to structure containing deserialized
 * MQTT response.
 * @param[in] usPacketId is the packet identifier from the ack received.
 */
static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId );

/**
 * @brief Process incoming Publish message.
 *
 * @param[in] pxPublishInfo is a pointer to structure containing deserialized
 * Publish message.
 */
static void prvMQTTProcessIncomingPublish( MQTTPublishInfo_t * pxPublishInfo );

/**
 * @brief The application callback function for getting the incoming publishes,
 * incoming acks, and ping responses reported from the MQTT library.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 * @param[in] pxPacketInfo Packet Info pointer for the incoming packet.
 * @param[in] pxDeserializedInfo Deserialized information from the incoming packet.
 */
static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo );

/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Global entry time into the application to use as a reference timestamp
 * in the #prvGetTimeMs function. #prvGetTimeMs will always return the difference
 * between the current time and the global entry time. This will reduce the chances
 * of overflow for the 32 bit unsigned integer used for holding the timestamp.
 */
static uint32_t ulGlobalEntryTimeMs;

/** @brief Static buffer used to hold MQTT messages being sent and received. */
static MQTTFixedBuffer_t xBuffer =
{
    ucSharedBuffer,
    democonfigNETWORK_BUFFER_SIZE
};

/*-----------------------------------------------------------*/

static void handle_c2d_message(AzureIoTHubClientMessage_t * message, void * context )
{
    (void)context;

    LogInfo(("C2D message payload : %.*s \r\n",
             message->pxPublishInfo->payloadLength,
             message->pxPublishInfo->pPayload));
}

static void handle_direct_method(AzureIoTHubClientMessage_t * message, void * context )
{
    LogInfo(("Method payload : %.*s \r\n",
             message->pxPublishInfo->payloadLength,
             message->pxPublishInfo->pPayload));

    AzureIoTHubClient_t* xAzureIoTHubClient = (AzureIoTHubClient_t*)context;
    if( AzureIoTHubClient_SendMethodResponse( xAzureIoTHubClient, message, 200,
                       NULL, 0) != AZURE_IOT_HUB_CLIENT_SUCCESS )
    {
        LogInfo( ("Error sending method response\r\n") );
    }
}

static void handle_device_twin_message( AzureIoTHubClientMessage_t * message, void * context )
{
    (void)context;

    LogInfo(("Twin document payload : %.*s \r\n",
             message->pxPublishInfo->payloadLength,
             message->pxPublishInfo->pPayload));
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
 * @brief Create the task that demonstrates the MQTT API Demo over a
 * mutually authenticated network connection with MQTT broker.
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
    AzureIoTHubClient_t xAzureIoTHubClient = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientError_t xResult;
    uint32_t status;
    #ifdef ENABLE_DPS_SAMPLE
    uint8_t * iotHubHostname = NULL;
    uint8_t * iotHubDeviceId = NULL;
    uint32_t iotHubHostnameLength = 0;
    uint32_t iotHubDeviceIdLength = 0;
    #else
    uint8_t* iotHubHostname = ( uint8_t * )HOSTNAME;
    uint8_t* iotHubDeviceId = ( uint8_t * )DEVICE_ID;
    uint32_t iotHubHostnameLength = sizeof( HOSTNAME) - 1;
    uint32_t iotHubDeviceIdLength = sizeof( DEVICE_ID ) - 1;
    #endif /* ENABLE_DPS_SAMPLE */

    ( void )pvParameters;

    ulGlobalEntryTimeMs = prvGetTimeMs();

    status = prvSetupNetworkCredentials( &xNetworkCredentials );
    configASSERT(status == 0);

#ifdef ENABLE_DPS_SAMPLE

    /* Run DPS.  */
    if ( ( status = prvDPSDemo( &xNetworkCredentials, &iotHubHostname,
                                &iotHubHostnameLength, &iotHubDeviceId,
                                &iotHubDeviceIdLength ) ) != 0)
    {
        printf("Failed on sample_dps_entry!: error code = 0x%08x\r\n", status);
        return(status);
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
        xNetworkStatus = prvConnectToServerWithBackoffRetries( (const char * )iotHubHostname, democonfigMQTT_BROKER_PORT,
                                                               &xNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", iotHubHostname ) );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pNetworkContext = &xNetworkContext;
        xTransport.send = TLS_FreeRTOS_send;
        xTransport.recv = TLS_FreeRTOS_recv;

        /* Initialize MQTT library. */
        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                             iotHubHostname, iotHubHostnameLength,
                                             iotHubDeviceId, iotHubDeviceIdLength,
                                             (const uint8_t * )MODULE_ID, sizeof( MODULE_ID ) - 1,
                                             ucSharedBuffer, sizeof( ucSharedBuffer ),
                                             prvGetTimeMs,
                                             &xTransport );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                false, mqttexampleCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /**************************** Enable features. ******************************/

        xResult = AzureIoTHubClient_CloudMessageEnable( &xAzureIoTHubClient, handle_c2d_message, &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_DirectMethodEnable( &xAzureIoTHubClient, handle_direct_method, &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        xResult = AzureIoTHubClient_DeviceTwinEnable( &xAzureIoTHubClient, handle_device_twin_message, &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /****************** Publish and Keep Alive Loop. **********************/
        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ulPublishCount = 0; ulPublishCount < ulMaxPublishCount; ulPublishCount++ )
        {
            xResult = AzureIoTHubClient_TelemetrySend( &xAzureIoTHubClient,
                                                           mqttexampleMESSAGE,
                                                           sizeof( mqttexampleMESSAGE ) - 1 );
            configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

            LogInfo( ( "Attempt to receive publish message from IoTHub.\r\n" ) );
            xResult = AzureIoTHubClient_DoWork( &xAzureIoTHubClient,
                                                    mqttexamplePROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( mqttexampleDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        /**************************** Disconnect. *****************************/

        /* Send an MQTT Disconnect packet over the already connected TLS over
         * TCP connection. There is no corresponding response for the disconnect
         * packet. After sending disconnect, client must close the network
         * connection. */
        LogInfo( ( "Disconnecting the MQTT connection with %s.\r\n",
                   HOSTNAME ) );
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        configASSERT( xResult == AZURE_IOT_HUB_CLIENT_SUCCESS );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the broker. */
        LogInfo( ( "prvMQTTDemoTask() completed an iteration successfully. "
                   "Total free heap is %u.\r\n",
                   xPortGetFreeHeapSize() ) );
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( mqttexampleDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}

#ifdef ENABLE_DPS_SAMPLE

static uint32_t prvDPSDemo( NetworkCredentials_t * pXNetworkCredentials,
                        uint8_t ** pIothubHostname, uint32_t * iothubHostnameLength,
                        uint8_t ** piothubDeviceId, uint32_t * iothubDeviceIdLength )
{
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTProvisioningClient_t xAzureIoTProvisioningClient = { 0 };
    TlsTransportStatus_t xNetworkStatus;
    AzureIoTProvisioningClientError_t xResult;
    TransportInterface_t xTransport;
    uint32_t sampleIotHubHostnameLength = sizeof(sampleIotHubHostname);
    uint32_t sampleIotHubDeviceIdLength = sizeof(sampleIotHubDeviceId);

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pParams = &xTlsTransportParams;

    /****************************** Connect. ******************************/

    xNetworkStatus = prvConnectToServerWithBackoffRetries( ENDPOINT, democonfigMQTT_BROKER_PORT,
                                                           pXNetworkCredentials, &xNetworkContext);
    configASSERT(xNetworkStatus == TLS_TRANSPORT_SUCCESS);

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pNetworkContext = &xNetworkContext;
    xTransport.send = TLS_FreeRTOS_send;
    xTransport.recv = TLS_FreeRTOS_recv;

    /* Sends an MQTT Connect packet over the already established TLS connection,
     * and waits for connection acknowledgment (CONNACK) packet. */
    LogInfo(("Creating an MQTT connection to %s.\r\n", ENDPOINT));

    /* Initialize MQTT library. */
    xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                  ENDPOINT, sizeof(ENDPOINT) - 1,
                                                  ID_SCOPE, sizeof(ID_SCOPE) - 1,
                                                  REGISTRATION_ID, sizeof(REGISTRATION_ID) - 1,
                                                  ucSharedBuffer, sizeof(ucSharedBuffer),
                                                  prvGetTimeMs,
                                                  &xTransport );
    configASSERT(xResult == AZURE_IOT_HUB_CLIENT_SUCCESS);

    /****************** Publish and Keep Alive Loop. **********************/
    xResult = AzureIoTProvisioningClient_Register(&xAzureIoTProvisioningClient);
    configASSERT(xResult == AZURE_IOT_HUB_CLIENT_SUCCESS);
    
    xResult = AzureIoTProvisioningClient_HubGet( &xAzureIoTProvisioningClient,   
                                                     sampleIotHubHostname, &sampleIotHubHostnameLength,
                                                     sampleIotHubDeviceId, &sampleIotHubDeviceIdLength );
    configASSERT(xResult == AZURE_IOT_HUB_CLIENT_SUCCESS);

    AzureIoTProvisioningClient_Deinit(&xAzureIoTProvisioningClient);
    
    /* Close the network connection.  */
    TLS_FreeRTOS_Disconnect(&xNetworkContext);

    *pIothubHostname = sampleIotHubHostname;
    *iothubHostnameLength = sampleIotHubHostnameLength;
    *piothubDeviceId = sampleIotHubDeviceId;
    *iothubDeviceIdLength = sampleIotHubDeviceIdLength;

    return 0;
}

#endif // ENABLE_DPS_SAMPLE

/*-----------------------------------------------------------*/

static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pHostName, uint32_t port,
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
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pHostName, port) );
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

static uint32_t prvGetTimeMs(void)
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = (uint32_t)xTickCount * MILLISECONDS_PER_TICK;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTimeMs = (uint32_t)(ulTimeMs - ulGlobalEntryTimeMs);

    return ulTimeMs;
}

/*-----------------------------------------------------------*/