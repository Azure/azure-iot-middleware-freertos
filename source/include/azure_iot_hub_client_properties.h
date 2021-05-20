/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client_properties.h
 *
 * @brief The middleware IoT Hub Client property functions used for Azure IoT Plug and Play.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#include "azure_iot_hub_client.h"

/**
 * @brief Append the necessary characters to a reported properties JSON payload belonging to a
 * component.
 *
 * The payload will be of the form:
 *
 * @code
 * "reported": {
 *     "<component_name>": {
 *         "__t": "c",
 *         "temperature": 23
 *     }
 * }
 * @endcode
 *
 * @note This API only builds the metadata for a component's properties.  The
 * application itself must specify the payload contents between calls
 * to this API and az_iot_hub_client_properties_builder_end_component() using
 * \p ref_json_writer to specify the JSON payload.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in,out] ref_json_writer The #az_json_writer to append the necessary characters for an IoT
 * Plug and Play component.
 * @param[in] component_name The component name associated with the reported properties.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 * @pre \p component_name must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The JSON payload was prefixed successfully.
 */
AzureIoTHubClientResult_t AzureIoTHubClientProperties_BuilderBeginComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                             const uint8_t * pucComponentName,
                                                                             uint16_t ulComponentNameLength, );
AZ_NODISCARD az_result az_iot_hub_client_properties_builder_begin_component( az_iot_hub_client const * client,
                                                                             az_json_writer * ref_json_writer,
                                                                             az_span component_name );
