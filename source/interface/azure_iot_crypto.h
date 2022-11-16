/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_crypto.h
 *
 * @brief The port file for crypto APIs
 *
 * Used for verifying the ADU image payload.
 *
 */

#include <stdint.h>

#include "azure_iot_result.h"

/**
 * @brief Calculate a SHA256 hash.
 *
 * @param[in] pucInputPtr The input to calculate the SHA over.
 * @param[in] ulInputSize The size of \p pucInputPtr.
 * @param[out] pucOutputPtr The buffer into which the calculation will be placed.
 * @param[in] ulOutputSize The length of \p pucOutputPtr.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCrypto_SHA256Calculate( const char * pucInputPtr,
                                                 uint64_t ulInputSize,
                                                 const char * pucOutputPtr,
                                                 uint64_t ulOutputSize );

/**
 * @brief Verify an RS256 signed payload.
 *
 * @param[in] pucInputPtr The input to verify.
 * @param[in] ulInputSize The length of \p pucInputPtr.
 * @param[in] pucSignaturePtr The signature of the \p pucInputPtr payload.
 * @param[in] ulSignatureSize The length of \p pucSignaturePtr.
 * @param[in] pucN The pointer to the key modulus.
 * @param[in] ullNSize The length of \p pucN.
 * @param[in] pucE The pointer to the key exponent.
 * @param[in] ullESize The length of \p pucE.
 * @param[out] pucBufferPtr The buffer which will be used to make the calculation.
 * @param[in] ulBufferSize The size of \p pucBufferPtr.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCrypto_RS256Verify( const char * pucInputPtr,
                                             uint64_t ulInputSize,
                                             const char * pucSignaturePtr,
                                             uint64_t ulSignatureSize,
                                             const char * pucN,
                                             uint64_t ullNSize,
                                             const char * pucE,
                                             uint64_t ullESize,
                                             const char * pucBufferPtr,
                                             uint32_t ulBufferSize );
