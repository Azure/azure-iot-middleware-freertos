// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZURE_IOT_CONFIG_DEFAULT_H
#define AZURE_IOT_CONFIG_DEFAULT_H

#ifndef azureIoTDEFAULTTOKENTIMEOUTINSEC
    #define azureIoTDEFAULTTOKENTIMEOUTINSEC        ( 60 * 60U )
#endif

#ifndef azureIoTKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureIoTKEEP_ALIVE_TIMEOUT_SECONDS      ( 60U )
#endif

#ifndef azureIoTCONNACK_RECV_TIMEOUT_MS
    #define azureIoTCONNACK_RECV_TIMEOUT_MS         ( 1000U )
#endif

#ifndef azureIoTSUBACK_WAIT_INTERVAL_MS
    #define azureIoTSUBACK_WAIT_INTERVAL_MS         ( 10U )
#endif

/**
 * @brief Macro that is called in the AzureIoT Middleware library for logging "Error" level
 * messages.
 *
 * To enable error level logging in the AzureIoT Middleware library, this macro should be mapped to the
 * application-specific logging implementation that supports error logging.
 *
 * @note This logging macro is called in the AzureIoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant. 
 *
 * <b>Default value</b>: Error logging is turned off, and no code is generated for calls
 * to the macro in the AzureIoT Middleware library on compilation.
 */
#ifndef LogError
    #define AZLogError( message )
#else
    #define AZLogError       LogError
#endif

/**
 * @brief Macro that is called in the AzureIoT Middleware library for logging "Warning" level
 * messages.
 *
 * To enable warning level logging in the AzureIoT Middleware library, this macro should be mapped to the
 * application-specific logging implementation that supports warning logging.
 *
 * @note This logging macro is called in the AzureIoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant. 
 * 
 * <b>Default value</b>: Warning logs are turned off, and no code is generated for calls
 * to the macro in the AzureIoT Middleware library on compilation.
 */
#ifndef LogWarn
    #define AZLogWarn( message )
#else
    #define AZLogWarn       LogWarn
#endif

/**
 * @brief Macro that is called in the AzureIoT Middleware library for logging "Info" level
 * messages.
 *
 * To enable info level logging in the AzureIoT Middleware library, this macro should be mapped to the
 * application-specific logging implementation that supports info logging.
 *
 * @note This logging macro is called in the AzureIoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 * 
 * <b>Default value</b>: Info logging is turned off, and no code is generated for calls
 * to the macro in the AzureIoT Middleware library on compilation.
 */
#ifndef LogInfo
    #define AZLogInfo( message )
#else
    #define AZLogInfo       LogInfo
#endif

/**
 * @brief Macro that is called in the AzureIoT Middleware library for logging "Debug" level
 * messages.
 *
 * To enable debug level logging from AzureIoT Middleware library, this macro should be mapped to the
 * application-specific logging implementation that supports debug logging.
 *
 * @note This logging macro is called in the AzureIoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 *
 * <b>Default value</b>: Debug logging is turned off, and no code is generated for calls
 * to the macro in the AzureIoT Middleware library on compilation.
 */
#ifndef LogDebug
    #define LogDebug( message )
#else
    #define AZLogDebug       LogDebug
#endif

#endif /* AZURE_IOT_CONFIG_DEFAULT_H */