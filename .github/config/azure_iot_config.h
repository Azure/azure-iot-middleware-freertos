/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef AZURE_IOT_CONFIG_H
#define AZURE_IOT_CONFIG_H

extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

#define AZLog( x )               vLoggingPrintf x
#define AZLogError( message )    AZLog( ( "[ERROR] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); AZLog( message ); AZLog( ( "\r\n" ) )
#define AZLogWarn( message )     AZLog( ( "[WARN] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); AZLog( message ); AZLog( ( "\r\n" ) )
#define AZLogInfo( message )     AZLog( ( "[INFO] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); AZLog( message ); AZLog( ( "\r\n" ) )
#define AZLogDebug( message )    AZLog( ( "[DEBUG] [AZ IoT] [%s:%d]", __FILE__, __LINE__ ) ); AZLog( message ); AZLog( ( "\r\n" ) )

#endif /* AZURE_IOT_CONFIG_H */
