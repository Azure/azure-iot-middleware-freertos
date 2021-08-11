/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_internal.h
 *
 * @brief Azure IoT FreeRTOS middleware internal APIs and structs
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#ifndef AZURE_IOT_INTERNAL_H
#define AZURE_IOT_INTERNAL_H

#include "azure_iot_result.h"

#include "azure/az_core.h"
#include "azure/az_iot.h"

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Translate embedded errors to middleware errors
 *
 * @param[in] xCoreError The `az_result` to translate to a middleware error
 *
 * @result The #AzureIoTResult_t translated from \p xCoreError.
 */
AzureIoTResult_t prvAzureIoT_TranslateCoreError( az_result xCoreError );

#include <azure/core/_az_cfg_suffix.h>

#endif /* AZURE_IOT_INTERNAL_H */
