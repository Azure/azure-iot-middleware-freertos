/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file
 *
 * @brief APIs to authenticate an ADU manifest.
 *
 */

#ifndef SAMPLE_ADU_JWS_H
#define SAMPLE_ADU_JWS_H

#include <stdint.h>

#include "azure_iot_result.h"

#define jwsPKCS7_PAYLOAD_OFFSET            19

#define jwsRSA3072_SIZE                    384
#define jwsSHA256_SIZE                     32
#define jwsJWS_HEADER_SIZE                 1400
#define jwsJWS_PAYLOAD_SIZE                60
#define jwsJWK_HEADER_SIZE                 48
#define jwsJWK_PAYLOAD_SIZE                700
#define jwsSIGNATURE_SIZE                  400
#define jwsSIGNING_KEY_E_SIZE              10
#define jwsSIGNING_KEY_N_SIZE              jwsRSA3072_SIZE
#define jwsSHA_CALCULATION_SCRATCH_SIZE    jwsRSA3072_SIZE + jwsSHA256_SIZE

/* This is the minimum amount of space needed to store values which are held at the same time. */
/* jwsJWS_PAYLOAD_SIZE, one jwsSIGNATURE_SIZE, and one jwsSHA256_SIZE are excluded since */
/* they will reuse buffer space. */
#define jwsSCRATCH_BUFFER_SIZE                                            \
    ( jwsJWS_HEADER_SIZE + jwsJWK_HEADER_SIZE + jwsJWK_PAYLOAD_SIZE       \
      + jwsSIGNATURE_SIZE + jwsSIGNING_KEY_N_SIZE + jwsSIGNING_KEY_E_SIZE \
      + jwsSHA_CALCULATION_SCRATCH_SIZE )

/**
 * @brief Authenticate the manifest from ADU.
 *
 * @param[in] pucManifest The escaped manifest from the ADU twin property.
 * @param[in] ulManifestLength The length of \p pucManifest.
 * @param[in] pucJWS The JWS used to authenticate \p pucManifest.
 * @param[in] ulJWSLength The length of \p pucJWS.
 * @param[in] pucScratchBuffer Scratch buffer space for calculations. It should be
 * `jwsSCRATCH_BUFFER_SIZE` in length.
 * @param[in] ulScratchBufferLength The length of \p pucScratchBuffer.
 * @return AzureIoTResult_t The return value of this function.
 * @retval eAzureIoTSuccess if successful.
 * @retval Otherwise if failed.
 */
AzureIoTResult_t AzureIoTJWS_ManifestAuthenticate( const uint8_t * pucManifest,
                                                   uint32_t ulManifestLength,
                                                   uint8_t * pucJWS,
                                                   uint32_t ulJWSLength,
                                                   uint8_t * pucScratchBuffer,
                                                   uint32_t ulScratchBufferLength );

#endif /* SAMPLE_ADU_JWS_H */
