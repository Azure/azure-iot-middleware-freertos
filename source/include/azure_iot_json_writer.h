/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_writer.h
 *
 * @brief The JSON writer used by the middleware for PnP properties.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#ifndef AZURE_IOT_JSON_WRITER_H
#define AZURE_IOT_JSON_WRITER_H

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_hub_client.h"

#include "azure/core/az_json.h"


typedef struct AzureIoTJSONWriter
{
    struct
    {
        az_json_writer xCoreWriter;
    } _internal;
} AzureIoTJSONWriter_t;

/**
 * @brief Initializes an #AzureIoTJSONWriter_t which writes JSON text into a buffer passed.
 *
 * @param[out] pxWriter A pointer to an #AzureIoTJSONWriter_t the instance to initialize.
 * @param[in] buffer_ptr A buffer pointer to which JSON text will be written.
 * @param[in] buffer_len Length of buffer.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Successfully initialized JSON writer.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_Init( AzureIoTJSONWriter_t * pxWriter,
                                                   uint8_t * buffer_ptr,
                                                   uint16_t buffer_len );

/**
 * @brief Deinitializes an #AzureIoTJSONWriter_t.
 *
 * @param[out] pxWriter A pointer to an #AzureIoTJSONWriter_t the instance to de-initialize.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Successfully de-initialized JSON writer.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_Deinit( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the UTF-8 property name and value where value is int32
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] property_name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] property_name_len Length of property_name.
 * @param[in] value The value to be written as a JSON number.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The property name and int32 value was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithInt32Value( AzureIoTJSONWriter_t * pxWriter,
                                                                           const uint8_t * property_name,
                                                                           uint16_t property_name_len,
                                                                           int32_t value );

/**
 * @brief Appends the UTF-8 property name and value where value is double
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] property_name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] property_name_len Length of property_name.
 * @param[in] value The value to be written as a JSON number.
 * @param[in] ulFractionalDigits The number of digits of the value to write after the decimal point and truncate the rest.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The property name and double value was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithDoubleValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * property_name,
                                                                            uint16_t property_name_len,
                                                                            double value,
                                                                            uint16_t ulFractionalDigits );

/**
 * @brief Appends the UTF-8 property name and value where value is boolean
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] property_name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] property_name_len Length of property_name.
 * @param[in] value The value to be written as a JSON literal `true` or `false`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The property name and bool value was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithBoolValue( AzureIoTJSONWriter_t * pxWriter,
                                                                          const uint8_t * property_name,
                                                                          uint16_t property_name_len,
                                                                          uint16_t value );

/**
 * @brief Appends the UTF-8 property name and value where value is string
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] property_name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] property_name_len Length of property_name.
 * @param[in] value The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] usValueLen Length of value.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The property name and string value was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyWithStringValue( AzureIoTJSONWriter_t * pxWriter,
                                                                            const uint8_t * property_name,
                                                                            uint16_t property_name_len,
                                                                            const uint8_t * value,
                                                                            uint16_t usValueLen );

/**
 * @brief Returns the length containing the JSON text written to the underlying buffer.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An uint16_t containing the length of JSON text built so far.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_GetBytesUsed( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the UTF-8 text value (as a JSON string) into the buffer.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] value Pointer of UCHAR buffer that contains UTF-8 encoded value to be written as a JSON string.
 * The value is escaped before writing.
 * @param[in] usValueLen Length of value.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The string value was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendString( AzureIoTJSONWriter_t * pxWriter,
                                                           const uint8_t * value,
                                                           uint16_t usValueLen );

/**
 * @brief Appends an existing UTF-8 encoded JSON text into the buffer, useful for appending nested
 * JSON.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] json A pointer to single, possibly nested, valid, UTF-8 encoded, JSON value to be written as
 * is, without any formatting or spacing changes. No modifications are made to this text, including
 * escaping.
 * @param[in] json_len Length of json
 *
 * @remarks A single, possibly nested, JSON value is one that starts and ends with {} or [] or is a
 * single primitive token. The JSON cannot start with an end object or array, or a property name, or
 * be incomplete.
 *
 * @remarks The function validates that the provided JSON to be appended is valid and properly
 * escaped, and fails otherwise.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The provided json_text was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendJsonText( AzureIoTJSONWriter_t * pxWriter,
                                                             const uint8_t * json,
                                                             uint16_t json_len );

/**
 * @brief Appends the UTF-8 property name (as a JSON string) which is the first part of a name/value
 * pair of a JSON object.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] pusValue The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 * @param[in] usValueLen Length of name.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The property name was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendPropertyName( AzureIoTJSONWriter_t * pxWriter,
                                                                 const uint8_t * pusValue,
                                                                 uint16_t usValueLen );

/**
 * @brief Appends a boolean value (as a JSON literal `true` or `false`).
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] xValue The value to be written as a JSON literal `true` or `false`.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The bool was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBool( AzureIoTJSONWriter_t * pxWriter,
                                                         bool xValue );

/**
 * @brief Appends an `int32_t` number value.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] ilValue The value to be written as a JSON number.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The number was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendInt32( AzureIoTJSONWriter_t * pxWriter,
                                                          int32_t ilValue );

/**
 * @brief Appends a `double` number value.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 * @param[in] value The value to be written as a JSON number.
 * @param[in] ulFractionalDigits The number of digits of the \p value to write after the decimal
 * point and truncate the rest.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess The number was appended successfully.
 *
 * @remark Only finite double values are supported. Values such as `NAN` and `INFINITY` are not
 * allowed and would lead to invalid JSON being written.
 *
 * @remark Non-significant trailing zeros (after the decimal point) are not written, even if \p
 * ulFractionalDigits is large enough to allow the zero padding.
 *
 * @remark The \p ulFractionalDigits must be between 0 and 15 (inclusive). Any value passed in that
 * is larger will be clamped down to 15.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendDouble( AzureIoTJSONWriter_t * pxWriter,
                                                           double value,
                                                           int32_t ulFractionalDigits );

/**
 * @brief Appends the JSON literal `null`.
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess `null` was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendNull( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the beginning of a JSON object (i.e. `{`).
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Object start was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginObject( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the beginning of a JSON array (i.e. `[`).
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Array start was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendBeginArray( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the end of the current JSON object (i.e. `}`).
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Object end was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndObject( AzureIoTJSONWriter_t * pxWriter );

/**
 * @brief Appends the end of the current JSON array (i.e. `]`).
 *
 * @param[in] pxWriter A pointer to an #AzureIoTJSONWriter_t.
 *
 * @return An #AzureIoTHubClientResult_t value indicating the result of the operation.
 * @retval #eAzureIoTHubClientSuccess Array end was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONWriter_AppendEndArray( AzureIoTJSONWriter_t * pxWriter );

#endif /* AZURE_IOT_JSON_WRITER_H */
