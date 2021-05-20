/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_json_reader.h
 *
 * @brief The JSON reader used by the middleware for PnP properties.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 *
 */

#ifndef AZURE_IOT_JSON_READER_H
#define AZURE_IOT_JSON_READER_H

#include "azure_iot_hub_client.h"

#include "azure/core/az_json.h"

/**
 * Defines symbols for the various kinds of JSON tokens that make up any JSON text.
 */
typedef enum AzureIoTJSONTokenType
{
    eAzureIoTJSONTokenNONE = AZ_JSON_TOKEN_NONE,
    eAzureIoTJSONTokenBEGIN_OBJECT = AZ_JSON_TOKEN_BEGIN_OBJECT,
    eAzureIoTJSONTokenEND_OBJECT = AZ_JSON_TOKEN_END_OBJECT,
    eAzureIoTJSONTokenBEGIN_ARRAY = AZ_JSON_TOKEN_BEGIN_ARRAY,
    eAzureIoTJSONTokenEND_ARRAY = AZ_JSON_TOKEN_END_ARRAY,
    eAzureIoTJSONTokenPROPERTY_NAME = AZ_JSON_TOKEN_PROPERTY_NAME,
    eAzureIoTJSONTokenSTRING = AZ_JSON_TOKEN_STRING,
    eAzureIoTJSONTokenNUMBER = AZ_JSON_TOKEN_NUMBER,
    eAzureIoTJSONTokenTRUE = AZ_JSON_TOKEN_TRUE,
    eAzureIoTJSONTokenFALSE = AZ_JSON_TOKEN_FALSE,
    eAzureIoTJSONTokenNULL = AZ_JSON_TOKEN_NULL
} AzureIoTJSONTokenType_t;

typedef struct AzureIoTJSONReader
{
    struct
    {
        az_json_reader core_reader;
    } _internal;
} AzureIoTJSONReader_t;

/**
 * @brief Initializes an #AzureIoTJSONReader_t to read the JSON payload contained within the provided
 * buffer.
 *
 * @param[out] reader_ptr A pointer to an #AzureIoTJSONReader_t instance to initialize.
 * @param[in] buffer_ptr An pointer to buffer containing the JSON text to read.
 * @param[in] buffer_len Length of buffer.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The #AzureIoTJSONReader_t is initialized successfully.
 * @retval other Initialization failed.
 *
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_Init( AzureIoTJSONReader_t * reader_ptr,
                                                   const uint8_t * buffer_ptr,
                                                   uint16_t buffer_len );

/**
 * @brief De-initializes an #AzureIoTJSONReader_t
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance to de-initialize
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The #AzureIoTJSONReader_t is de-initialized successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_Deinit( AzureIoTJSONReader_t * reader_ptr );

/**
 * @brief Reads the next token in the JSON text and updates the reader state.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance containing the JSON to
 * read.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The token was read successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_NextToken( AzureIoTJSONReader_t * reader_ptr );

/**
 * @brief Reads and skips over any nested JSON elements.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance containing the JSON to
 * read.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The children of the current JSON token are skipped successfully.
 *
 * @remarks If the current token kind is a property name, the reader first moves to the property
 * value. Then, if the token kind is start of an object or array, the reader moves to the matching
 * end object or array. For all other token kinds, the reader doesn't move and returns #AZURE_IOT_SUCCESS.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_SkipChildren( AzureIoTJSONReader_t * reader_ptr );

/**
 * @brief Gets the JSON token's boolean value.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance.
 * @param[out] value_ptr A pointer to a boolean to receive the value.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The boolean value is returned.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenBoolGet( AzureIoTJSONReader_t * reader_ptr,
                                                           bool * value_ptr );

/**
 * @brief Gets the JSON token's number as a 32-bit signed integer.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance.
 * @param[out] value_ptr A pointer to a variable to receive the value.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The number is returned.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenInt32Get( AzureIoTJSONReader_t * reader_ptr,
                                                            int32_t * value_ptr );

/**
 * @brief Gets the JSON token's number as a `double`.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance.
 * @param[out] value_ptr A pointer to a variable to receive the value.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The number is returned.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenDoubleGet( AzureIoTJSONReader_t * reader_ptr,
                                                             double * value_ptr );

/**
 * @brief Gets the JSON token's string after unescaping it, if required.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance.
 * @param[out] buffer_ptr A pointer to a buffer where the string should be copied into.
 * @param[in] buffer_size The maximum available space within the buffer referred to by buffer_ptr.
 * @param[out] bytes_copied Contains the number of bytes written to the \p
 * destination which denote the length of the unescaped string.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the result of the operation.
 * @retval #AZURE_IOT_SUCCESS The property name was appended successfully.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenStringGet( AzureIoTJSONReader_t * reader_ptr,
                                                             uint8_t * buffer_ptr,
                                                             uint16_t buffer_size,
                                                             uint16_t * bytes_copied );

/**
 * @brief Determines whether the unescaped JSON token value that the #AzureIoTJSONReader_t points to is
 * equal to the expected text within the provided buffer bytes by doing a case-sensitive comparison.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance containing the JSON string token.
 * @param[in] expected_text_ptr A pointer to lookup text to compare the token against.
 * @param[in] expected_text_len Length of expected_text_ptr.
 *
 * @return `1` if the current JSON token value in the JSON source semantically matches the
 * expected lookup text, with the exact casing; otherwise, `0`.
 *
 * @remarks This operation is only valid for the string and property name token kinds. For all other
 * token kinds, it returns 0.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenIsTextEqual( AzureIoTJSONReader_t * reader_ptr,
                                                               uint8_t * expected_text_ptr,
                                                               uint16_t expected_text_len );

/**
 * @brief Determines type of token currently #AzureIoTJSONReader_t points to.
 *
 * @param[in] reader_ptr A pointer to an #AzureIoTJSONReader_t instance.
 *
 * @return An `AzureIoTHubClientResult_t` value indicating the type of token.
 */
AzureIoTHubClientResult_t AzureIoTJSONReader_TokenType( AzureIoTJSONReader_t * reader_ptr );

#endif /* AZURE_IOT_JSON_READER_H */
