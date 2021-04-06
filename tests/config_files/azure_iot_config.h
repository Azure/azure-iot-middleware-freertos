// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H

extern void vLoggingPrintf( const char * pcFormatString, ... );

#define Log( x )               vLoggingPrintf x
#define LogError( message )    Log( ( "[ERROR] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogWarn( message )     Log( ( "[WARN] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogInfo( message )     Log( ( "[INFO] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )
#define LogDebug( message )    Log( ( "[DEBUG] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); Log( message ); Log( ( "\r\n" ) )

#endif /* AZURE_IOT_CONFIG_H */
