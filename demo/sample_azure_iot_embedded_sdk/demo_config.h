/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "MQTTDemo"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

#define ENABLE_DPS_SAMPLE

#ifdef ENABLE_DPS_SAMPLE

#define ENDPOINT                "testbarnacle.azure-devices-provisioning.net"
#define ID_SCOPE                "0ne000A247E"
#define REGISTRATION_ID         "fabrikam"

#endif // ENABLE_DPS_SAMPLE

#define DEVICE_ID				"fabrikam"
#define MODULE_ID               ""
#define HOSTNAME                "kartos.azure-devices.net"

#define democonfigROOT_CA_PEM "-----BEGIN CERTIFICATE-----\r\n" \
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n" \
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n" \
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n" \
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n" \
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n" \
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n" \
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n" \
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n" \
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n" \
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n" \
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n" \
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n" \
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n" \
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n" \
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n" \
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n" \
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n" \
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n" \
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n" \
"-----END CERTIFICATE-----\r\n"

#define democonfigCLIENT_CERTIFICATE_PEM "-----BEGIN CERTIFICATE-----\r\n"\
"MIIBUTCB+QIUQIuXoZrzE0biEdFp3eE2LEvk1w4wCgYIKoZIzj0EAwIwRTELMAkG\r\n"\
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"\
"IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMDA5MDEwNzUyMzlaFw0yMTA5MDEwNzUyMzla\r\n"\
"MBMxETAPBgNVBAMMCGZhYnJpa2FtMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE\r\n"\
"SC1Trg8ZZsGLJBp5EnXHGOaDD1JP1EpciWr+Li7IALORJK/OoNo2OgM3WROTGKB1\r\n"\
"zIw+6Kj4aQh23WmVshJQIjAKBggqhkjOPQQDAgNHADBEAiAp4X50SDkaficRJq2D\r\n"\
"XeuhuMiaB7QVzKcs0fznoBxr1gIgQx95ZX33/8k/3zf2Q+myPIeZmvd/AX3BaJSN\r\n"\
"1F3VICc=\r\n"\
"-----END CERTIFICATE-----\r\n"

 /**
  * @brief Client's private key.
  *
  * For AWS IoT MQTT broker, refer to the AWS documentation below for details
  * regarding clientauthentication.
  * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
  *
  * @note This private key should be PEM-encoded.
  *
  * Must include the PEM header and footer:
  * "-----BEGIN RSA PRIVATE KEY-----\n"\
  * "...base64 data...\n"\
  * "-----END RSA PRIVATE KEY-----\n"
  *
  * #define democonfigCLIENT_PRIVATE_KEY_PEM    "...insert here..."
  */

#define democonfigCLIENT_PRIVATE_KEY_PEM "-----BEGIN EC PARAMETERS-----\r\n"\
"BggqhkjOPQMBBw==\r\n"\
"-----END EC PARAMETERS-----\r\n"\
"-----BEGIN EC PRIVATE KEY-----\r\n"\
"MHcCAQEEIAGertgK003ewL7wePlhwqdgsD2uwkcMW8B3JmFqzKCVoAoGCCqGSM49\r\n"\
"AwEHoUQDQgAESC1Trg8ZZsGLJBp5EnXHGOaDD1JP1EpciWr+Li7IALORJK/OoNo2\r\n"\
"OgM3WROTGKB1zIw+6Kj4aQh23WmVshJQIg==\r\n"\
"-----END EC PRIVATE KEY-----\r\n"

/**
 * @brief An option to disable Server Name Indication.
 *
 * @note When using a local Mosquitto server setup, SNI needs to be disabled
 * for an MQTT broker that only has an IP address but no hostname. However,
 * SNI should be enabled whenever possible.
 */
#define democonfigDISABLE_SNI    ( pdFALSE )

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define democonfigOS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define democonfigOS_VERSION                tskKERNEL_VERSION_NUMBER

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define democonfigHARDWARE_PLATFORM_NAME    "WinSim"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#define democonfigMQTT_LIB                  "core-mqtt@1.0.0"

/**
 * @brief Set the stack size of the main demo task.
 *
 * In the Windows port, this stack only holds a structure. The actual
 * stack is created by an operating system thread.
 */
#define democonfigDEMO_STACKSIZE            configMINIMAL_STACK_SIZE

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE       ( 5 * 1024U )

#endif /* DEMO_CONFIG_H */
