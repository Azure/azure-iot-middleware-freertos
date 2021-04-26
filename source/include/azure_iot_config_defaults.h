/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef AZURE_IOT_CONFIG_DEFAULTS_H
#define AZURE_IOT_CONFIG_DEFAULTS_H

/**
 * @brief Default timeout used for generating SAS token.
 *
 */
#ifndef azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC
    #define azureiotDEFAULT_TOKEN_TIMEOUT_IN_SEC        ( 60 * 60U )
#endif

/**
 * @brief MQTT keep alive.
 *
 */
#ifndef azureiotKEEP_ALIVE_TIMEOUT_SECONDS
    #define azureiotKEEP_ALIVE_TIMEOUT_SECONDS          ( 60U )
#endif

/**
 * @brief Receive timeout for MQTT CONNACK.
 *
 */
#ifndef azureiotCONNACK_RECV_TIMEOUT_MS
    #define azureiotCONNACK_RECV_TIMEOUT_MS             ( 1000U )
#endif

/**
 * @brief Wait timeout of MQTT SUBACK.
 */
#ifndef azureiotSUBACK_WAIT_INTERVAL_MS
    #define azureiotSUBACK_WAIT_INTERVAL_MS             ( 10U )
#endif

/**
 * @brief Max MQTT username.
 */
#ifndef azureiotUSERNAME_MAX
    #define azureiotUSERNAME_MAX                        ( 256U )
#endif

/**
 * @brief Max MQTT password.
 */
#ifndef azureiotPASSWORD_MAX
    #define azureiotPASSWORD_MAX                        ( 256U )
#endif

/**
 * @brief Max MQTT topic length.
 */
#ifndef azureiotTOPIC_MAX
    #define azureiotTOPIC_MAX                           ( 128U )
#endif

/**
 * @brief Max provisioning response payload supported.
 *
 */
#ifndef azureiotPROVISIONING_REQUEST_PAYLOAD_MAX
    #define azureiotPROVISIONING_REQUEST_PAYLOAD_MAX    ( 512U )
#endif

/**
 * @brief Macro that is called in the Azure IoT Middleware library for logging "Error" level
 * messages.
 *
 * To enable error level logging in the AzureIoT Middleware library:
 *  - Map the macro to the application-specific logging implementation that supports error logging.
 *
 * @note This logging macro is called in the Azure IoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 *
 * <b>Default value</b>: Error logging is turned off, and no code is generated for calls
 * to the macro in the Azure IoT Middleware library on compilation.
 */
#ifndef AZLogError
    #define AZLogError( message )
#endif

/**
 * @brief Macro that is called in the Azure IoT Middleware library for logging "Warning" level
 * messages.
 *
 * To enable warning level logging in the AzureIoT Middleware library:
 *  - Map the macro to the application-specific logging implementation that supports error logging.
 *
 * @note This logging macro is called in the Azure IoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 *
 * <b>Default value</b>: Warning logs are turned off, and no code is generated for calls
 * to the macro in the Azure IoT Middleware library on compilation.
 */
#ifndef AZLogWarn
    #define AZLogWarn( message )
#endif

/**
 * @brief Macro that is called in the Azure IoT Middleware library for logging "Info" level
 * messages.
 *
 * To enable info level logging in the AzureIoT Middleware library:
 *  - Map the macro to the application-specific logging implementation that supports error logging.
 *
 * @note This logging macro is called in the Azure IoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 *
 * <b>Default value</b>: Info logging is turned off, and no code is generated for calls
 * to the macro in the Azure IoT Middleware library on compilation.
 */
#ifndef AZLogInfo
    #define AZLogInfo( message )
#endif

/**
 * @brief Macro that is called in the Azure IoT Middleware library for logging "Debug" level
 * messages.
 *
 * To enable info level logging in the AzureIoT Middleware library:
 *  - Map the macro to the application-specific logging implementation that supports error logging.
 *
 * @note This logging macro is called in the Azure IoT Middleware library with parameters wrapped in
 * double parentheses to be ISO C89/C90 standard compliant.
 *
 * <b>Default value</b>: Debug logging is turned off, and no code is generated for calls
 * to the macro in the Azure IoT Middleware library on compilation.
 */
#ifndef AZLogDebug
    #define AZLogDebug( message )
#endif

#endif /* AZURE_IOT_CONFIG_DEFAULTS_H */
