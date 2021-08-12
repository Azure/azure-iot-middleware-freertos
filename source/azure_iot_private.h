/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef AZURE_IOT_PRIVATE_H
#define AZURE_IOT_PRIVATE_H

#include "azure_iot_result.h"

#include "azure/az_core.h"

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Translate embedded errors to middleware errors
 *
 * @param[in] xCoreError The `az_result` to translate to a middleware error
 *
 * @result The #AzureIoTResult_t translated from \p xCoreError.
 */
AzureIoTResult_t xAzureIoT_TranslateCoreError( az_result xCoreError );

#include <azure/core/_az_cfg_suffix.h>

#endif /* AZURE_IOT_PRIVATE_H */
