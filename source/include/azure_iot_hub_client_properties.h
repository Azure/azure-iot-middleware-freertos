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

#ifndef AZURE_IOT_HUB_CLIENT_PROPERTIES_H
#define AZURE_IOT_HUB_CLIENT_PROPERTIES_H

#include "azure_iot_hub_client.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

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
 * \p pxJSONWriter to specify the JSON payload.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONWriter The #AzureIoTJSONWriter_t to append the necessary characters for an IoT
 * Plug and Play component.
 * @param[in] component_name The component name associated with the reported properties.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONWriter must not be `NULL`.
 * @pre \p component_name must be a valid, non-empty #az_span.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The JSON payload was prefixed successfully.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_BuilderBeginComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                             AzureIoTJSONWriter_t * pxJSONWriter,
                                                                             uint8_t * const pucComponentName,
                                                                             uint16_t usComponentNameLength );

/**
 * @brief Append the necessary characters to end a reported properties JSON payload belonging to a
 * component.
 *
 * @note This API should be used in conjunction with
 * az_iot_hub_client_properties_builder_begin_component().
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONWriter The #AzureIoTJSONWriter_t to append the necessary characters for an IoT
 * Plug and Play component.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONWriter must not be `NULL`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The JSON payload was suffixed successfully.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_BuilderEndComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                           AzureIoTJSONWriter_t * pxJSONWriter );

/**
 * @brief Begin a property response to a writeable property request from the service.
 *
 * This API should be used in response to an incoming writeable properties. More details can be
 * found here:
 *
 * https://docs.microsoft.com/en-us/azure/iot-pnp/concepts-convention#writable-properties
 *
 * The payload will be of the form:
 *
 * **Without component**
 * @code
 * {
 *   "<property_name>":{
 *     "ac": <ilAckCode>,
 *     "av": <ilAckVersion>,
 *     "ad": "<ack_description>",
 *     "value": <user_value>
 *   }
 * }
 * @endcode
 *
 * **With component**
 * @code
 * {
 *   "<component_name>": {
 *     "__t": "c",
 *     "<property_name>": {
 *       "ac": <ilAckCode>,
 *       "av": <ilAckVersion>,
 *       "ad": "<ack_description>",
 *       "value": <user_value>
 *     }
 *   }
 * }
 * @endcode
 *
 * To send a status for properties belonging to a component, first call the
 * az_iot_hub_client_properties_builder_begin_component() API to prefix the payload with the
 * necessary identification. The API call flow would look like the following with the listed JSON
 * payload being generated.
 *
 * @code
 * az_iot_hub_client_properties_builder_begin_component()
 * az_iot_hub_client_properties_builder_begin_response_status()
 * // Append user value here (<user_value>) using pxJSONWriter directly.
 * az_iot_hub_client_properties_builder_end_response_status()
 * az_iot_hub_client_properties_builder_end_component()
 * @endcode
 *
 * @note This API only builds the metadata for the properties response.  The
 * application itself must specify the payload contents between calls
 * to this API and az_iot_hub_client_properties_builder_end_response_status() using
 * \p pxJSONWriter to specify the JSON payload.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONWriter The initialized #AzureIoTJSONWriter_t to append data to.
 * @param[in] pucPropertyName The name of the property to build a response payload for.
 * @param[in] usPropertyNameLength The length of \p pucPropertyName.
 * @param[in] ilAckCode The HTTP-like status code to respond with. See #az_iot_status for
 * possible supported values.
 * @param[in] ilAckVersion The version of the property the application is acknowledging.
 * This can be retrieved from the service request by
 * calling az_iot_hub_client_properties_get_properties_version.
 * @param[in] pucAckDescription An optional description detailing the context or any details about
 * the acknowledgement. This can be `NULL`.
 * @param[in] usAckDescriptionLength The length of \p pucAckDescription.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONWriter must not be `NULL`.
 * @pre \p property_name must be a valid, non-empty #az_span.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The JSON payload was prefixed successfully.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_BuilderBeginResponseStatus( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                  AzureIoTJSONWriter_t * pxJSONWriter,
                                                                                  const uint8_t * pucPropertyName,
                                                                                  uint16_t usPropertyNameLength,
                                                                                  int32_t ilAckCode,
                                                                                  int32_t ilAckVersion,
                                                                                  const uint8_t * pucAckDescription,
                                                                                  uint16_t usAckDescriptionLength );

/**
 * @brief End a properties response payload with confirmation status.
 *
 * @note This API should be used in conjunction with
 * az_iot_hub_client_properties_builder_begin_response_status().
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONWriter The initialized #AzureIoTJSONWriter_t to append data to.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONWriter must not be `NULL`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The JSON payload was suffixed successfully.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_BuilderEndResponseStatus( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                AzureIoTJSONWriter_t * pxJSONWriter );

/**
 * @brief Read the Azure IoT Plug and Play property version.
 *
 * @warning This modifies the state of the json reader. To then use the same json reader
 * with az_iot_hub_client_properties_get_next_component_property(), you must call
 * az_json_reader_init() again after this call and before the call to
 * az_iot_hub_client_properties_get_next_component_property() or make an additional copy before
 * calling this API.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONReader The pointer to the #AzureIoTJSONReader_t used to parse through the JSON
 * payload.
 * @param[in] xResponseType The #az_iot_hub_client_properties_response_type representing the message
 * type associated with the payload.
 * @param[out] pilVersion The numeric version of the properties in the JSON payload.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONReader must not be `NULL`.
 * @pre \p pilVersion must not be `NULL`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess If the function returned a valid version.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_GetPropertiesVersion( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                            AzureIoTJSONReader_t * pxJSONReader,
                                                                            az_iot_hub_client_properties_response_type xResponseType,
                                                                            int32_t * pilVersion );

/**
 * @brief Property type
 *
 */
typedef enum AzureIoTHubClientPropertyType_t
{
    /** @brief Property was originally reported from the device. */
    eAzureIoTHubClientReportedFromDevice = 1,
    /** @brief Property was received from the service. */
    eAzureIoTHubClientPropertyWriteable = 2
} AzureIoTHubClientPropertyType_t;

/**
 * @brief Iteratively read the Azure IoT Plug and Play component properties.
 *
 * Note that between calls, the #az_span pointed to by \p out_component_name shall not be modified,
 * only checked and compared. Internally, the #az_span is only changed if the component name changes
 * in the JSON document and is not necessarily set every invocation of the function.
 *
 * On success, the `pxJSONReader` will be set on a valid property name. After checking the
 * property name, the reader can be advanced to the property value by calling
 * az_json_reader_next_token(). Note that on the subsequent call to this API, it is expected that
 * the json reader will be placed AFTER the read property name and value. That means that after
 * reading the property value (including single values or complex objects), the user must call
 * az_json_reader_next_token().
 *
 * Below is a code snippet which you can use as a starting point:
 *
 * @code
 *
 * while (az_result_succeeded(az_iot_hub_client_properties_get_next_component_property(
 *       &hub_client, &jr, xResponseType, AZ_IOT_HUB_CLIENT_PROPERTY_WRITEABLE, &component_name)))
 * {
 *   // Check if property is of interest (substitute user_property for your own property name)
 *   if (az_json_token_is_text_equal(&jr.token, user_property))
 *   {
 *     az_json_reader_next_token(&jr);
 *
 *     // Get the property value here
 *     // Example: az_json_token_get_int32(&jr.token, &user_int);
 *
 *     // Skip to next property value
 *     az_json_reader_next_token(&jr);
 *   }
 *   else
 *   {
 *     // The JSON reader must be advanced regardless of whether the property
 *     // is of interest or not.
 *     az_json_reader_next_token(&jr);
 *
 *     // Skip children in case the property value is an object
 *     az_json_reader_skip_children(&jr);
 *     az_json_reader_next_token(&jr);
 *   }
 * }
 *
 * @endcode
 *
 * @warning If you need to retrieve more than one \p xPropertyType, you should first complete the
 * scan of all components for the first property type (until the API returns
 * #AZ_ERROR_IOT_END_OF_PROPERTIES). Then you must call az_json_reader_init() again after this call
 * and before the next call to az_iot_hub_client_properties_get_next_component_property with the
 * different \p xPropertyType.
 *
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t to use for this call.
 * @param[in,out] pxJSONReader The #AzureIoTJSONReader_t to parse through. The ownership of iterating
 * through this json reader is shared between the user and this API.
 * @param[in] xResponseType The #az_iot_hub_client_properties_response_type representing the message
 * type associated with the payload.
 * @param[in] xPropertyType The #AzureIoTHubClientPropertyType_t to scan for.
 * @param[out] out_component_name The #az_span* representing the value of the component.
 *
 * @pre \p pxAzureIoTHubClient must not be `NULL`.
 * @pre \p pxJSONReader must not be `NULL`.
 * @pre \p out_component_name must not be `NULL`. It must point to an #az_span instance.
 * @pre \p If `eAzureIoTHubClientReportedFromDevice` is specified in \p xPropertyType,
 * then \p xResponseType must be `AZ_IOT_HUB_CLIENT_PROPERTIES_RESPONSE_TYPE_GET`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess If the function returned a valid #AzureIoTJSONReader_t pointing to the property name and
 * the #az_span with a component name.
 * @retval #AZ_ERROR_JSON_INVALID_STATE If the json reader is passed in at an unexpected location.
 * @retval #AZ_ERROR_IOT_END_OF_PROPERTIES If there are no more properties left for the component.
 */
AzureIoTHubClientResult_t AzureIotHubClientProperties_GetNextComponentProperty( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                AzureIoTJSONReader_t * pxJSONReader,
                                                                                az_iot_hub_client_properties_response_type xResponseType,
                                                                                AzureIoTHubClientPropertyType_t xPropertyType,
                                                                                uint8_t * ppucComponentName,
                                                                                uint16_t * pusComponentNameLength );

#include <azure/core/_az_cfg_suffix.h>

#endif /*AZURE_IOT_HUB_CLIENT_PROPERTIES_H */
