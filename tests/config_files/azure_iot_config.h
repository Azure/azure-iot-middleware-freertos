// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

#define Log( x )               vLoggingPrintf x
#define LogError( message )    Log( ( "[ERROR] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogWarn( message )     Log( ( "[WARN] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogInfo( message )     Log( ( "[INFO] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogDebug( message )    Log( ( "[DEBUG] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )

/************ End of logging configuration ****************/

#endif /* AZURE_IOT_CONFIG_H */
