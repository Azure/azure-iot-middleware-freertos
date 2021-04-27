// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H


/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for AzureIoT middleware.
 * 3. Include the header file "logging_stack.h", if logging is enabled for AzureIoT middleware.
 */

#include "logging_levels.h"

/* Logging configuration for the AzureIoT middleware library. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "AZ IOT"
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

#endif /* AZURE_IOT_CONFIG_H */
