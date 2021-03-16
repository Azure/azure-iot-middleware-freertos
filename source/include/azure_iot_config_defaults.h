/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef AZURE_IOT_CONFIG_DEFAULT_H
#define AZURE_IOT_CONFIG_DEFAULT_H

#ifndef azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC
    #define azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC        ( 60 * 60U )
#endif

#ifndef azureiotKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureiotKEEP_ALIVE_TIMEOUT_SECONDS          ( 60U )
#endif

#ifndef azureiotCONNACK_RECV_TIMEOUT_MS
    #define azureiotCONNACK_RECV_TIMEOUT_MS             ( 1000U )
#endif

#ifndef azureiotSUBACK_WAIT_INTERVAL_MS
    #define azureiotSUBACK_WAIT_INTERVAL_MS             ( 10U )
#endif

#ifndef azureiotUSERNAME_MAX
    #define azureiotUSERNAME_MAX                        ( 128U )
#endif

#ifndef azureiotPASSWORD_MAX
    #define azureiotPASSWORD_MAX                        ( 256U )
#endif

#ifndef azureiotTOPIC_MAX
    #define azureiotTOPIC_MAX                           ( 128U )
#endif

#ifndef azureiotPROVISIONING_RESPONSE_MAX
    #define azureiotPROVISIONING_RESPONSE_MAX           ( 512U )
#endif

#ifndef azureiotPROVISIONING_REQUEST_PAYLOAD_MAX
    #define azureiotPROVISIONING_REQUEST_PAYLOAD_MAX    ( 512U )
#endif

/**
 * @brief Milliseconds per second.
 */
#define azureiotMILLISECONDS_PER_SECOND             ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define azureiotMILLISECONDS_PER_TICK               ( azureiotMILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

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
    #define AZLogDebug( message )
#else
    #define AZLogDebug       LogDebug
#endif

#endif /* AZURE_IOT_CONFIG_DEFAULT_H */
