/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file
 *
 * @brief APIs working with JWS signatures to authenticate an ADU manifest.
 *
 */

#ifndef AZURE_IOT_JWS_H
#define AZURE_IOT_JWS_H

#include <stdint.h>

#include "azure_iot_result.h"

/**
 * @brief Holds the values of the root key used to verify the JWS signature.
 */
typedef struct AzureIoTJWS_RootKey
{
    uint8_t * pucRootKeyId;           /**< The pointer to the root key id. */
    uint32_t ulRootKeyIdLength;       /**< The length of the root key id. */
    uint8_t * pucRootKeyN;            /**< The pointer to the root key modulus. */
    uint32_t ulRootKeyNLength;        /**< The length of the root key modulus. */
    uint8_t * pucRootKeyExponent;     /**< The pointer to the root key exponent. */
    uint32_t ulRootKeyExponentLength; /**< The length of the root key exponent. */
} AzureIoTJWS_RootKey_t;


#define azureiotjwsRSA3072_SIZE                    384                                              /**< Size of the RSA 3072 key. */
#define azureiotjwsSHA256_SIZE                     32                                               /**< Size of the SHA256 hash. */
#define azureiotjwsJWS_HEADER_SIZE                 1400                                             /**< Size of the JWS header. */
#define azureiotjwsJWS_PAYLOAD_SIZE                60                                               /**< Size of the JWS payload. */
#define azureiotjwsJWK_HEADER_SIZE                 48                                               /**< Size of the JWK header. */
#define azureiotjwsJWK_PAYLOAD_SIZE                700                                              /**< Size of the JWK payload. */
#define azureiotjwsSIGNATURE_SIZE                  400                                              /**< Size of the JWS/JWK signature. */
#define azureiotjwsSIGNING_KEY_E_SIZE              10                                               /**< Size of the signed key exponent. */
#define azureiotjwsSIGNING_KEY_N_SIZE              azureiotjwsRSA3072_SIZE                          /**< Size of the signing key modulus. */
#define azureiotjwsSHA_CALCULATION_SCRATCH_SIZE    azureiotjwsRSA3072_SIZE + azureiotjwsSHA256_SIZE /**< Size of the sha calculation scratch space. */

/**
 * @brief The minimum amount of space needed to authenticate a JWS signature.
 *
 * @note azureiotjwsJWS_PAYLOAD_SIZE, one azureiotjwsSIGNATURE_SIZE, and one azureiotjwsSHA256_SIZE are excluded since
 * they will reuse buffer space.
 */
#define azureiotjwsSCRATCH_BUFFER_SIZE                                                            \
    ( azureiotjwsJWS_HEADER_SIZE + azureiotjwsJWK_HEADER_SIZE + azureiotjwsJWK_PAYLOAD_SIZE       \
      + azureiotjwsSIGNATURE_SIZE + azureiotjwsSIGNING_KEY_N_SIZE + azureiotjwsSIGNING_KEY_E_SIZE \
      + azureiotjwsSHA_CALCULATION_SCRATCH_SIZE )

/**
 * @brief Authenticate the manifest from ADU.
 *
 * @param[in] pucManifest The unescaped manifest from the ADU twin property
 * (`pucUpdateManifest` from #AzureIoTADUUpdateRequest_t).
 * @param[in] ulManifestLength The length of \p pucManifest.
 * (`ulUpdateManifestLength` from #AzureIoTADUUpdateRequest_t).
 * @param[in] pucJWS The JWS signature used to authenticate \p pucManifest.
 * (`pucUpdateManifestSignature` from #AzureIoTADUUpdateRequest_t).
 * @param[in] ulJWSLength The length of \p pucJWS.
 * (`ulUpdateManifestSignatureLength` from #AzureIoTADUUpdateRequest_t).
 * @param[in] xADURootKeys An array of root keys that may be used to verify the payload.
 * @param[in] ulADURootKeysLength The length of the array of root keys.
 * @param[out] pucScratchBuffer Scratch buffer space for calculations. It should be
 * `azureiotjwsSCRATCH_BUFFER_SIZE` in length.
 * @param[in] ulScratchBufferLength The length of \p pucScratchBuffer.
 * @return AzureIoTResult_t The return value of this function.
 * @retval eAzureIoTSuccess if successful.
 * @retval Otherwise if failed.
 */
AzureIoTResult_t AzureIoTJWS_ManifestAuthenticate( const uint8_t * pucManifest,
                                                   uint32_t ulManifestLength,
                                                   uint8_t * pucJWS,
                                                   uint32_t ulJWSLength,
                                                   AzureIoTJWS_RootKey_t * xADURootKeys,
                                                   uint32_t ulADURootKeysLength,
                                                   uint8_t * pucScratchBuffer,
                                                   uint32_t ulScratchBufferLength );

#endif /* AZURE_IOT_JWS_H */
