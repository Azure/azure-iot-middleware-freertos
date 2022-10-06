/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_sample_adu_jws.h"

#include "azure/az_core.h"
#include "azure/az_iot.h"

#include "azure_iot_result.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_adu_client.h"

#include "mbedtls/base64.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/cipher.h"

/* For logging */
#include "demo_config.h"

/**
 * @brief Convenience macro to return if an operation failed.
 */
#define azureiotresultRETURN_IF_FAILED( exp )        \
    do                                               \
    {                                                \
        AzureIoTResult_t const _xAzResult = ( exp ); \
        if( _xAzResult != eAzureIoTSuccess )         \
        {                                            \
            return _xAzResult;                       \
        }                                            \
    } while( 0 )

const uint8_t jws_sha256_json_value[] = "sha256";
const uint8_t jws_sjwk_json_value[] = "sjwk";
const uint8_t jws_kid_json_value[] = "kid";
const uint8_t jws_n_json_value[] = "n";
const uint8_t jws_e_json_value[] = "e";
const uint8_t jws_alg_json_value[] = "alg";
const uint8_t jws_alg_rs256[] = "RS256";

typedef struct prvJWSValidationContext
{
    int32_t outParsedManifestShaSize;
    uint8_t * ucJWSHeader;
    uint8_t * ucJWKHeader;
    uint8_t * ucJWKPayload;
    uint8_t * ucJWKSignature;
    uint8_t * ucScratchCalculationBuffer;
    uint8_t * ucJWSPayload;
    uint8_t * ucJWSSignature;
    uint8_t * ucSigningKeyN;
    uint8_t * ucSigningKeyE;
    uint8_t * ucManifestSHACalculation;
    uint8_t * ucParsedManifestSha;
    uint8_t * pucBase64EncodedHeader;
    uint8_t * pucBase64EncodedPayload;
    uint8_t * pucBase64EncodedSignature;
    uint8_t * pucJWKBase64EncodedHeader;
    uint8_t * pucJWKBase64EncodedPayload;
    uint8_t * pucJWKBase64EncodedSignature;
    uint32_t ulBase64EncodedHeaderLength;
    uint32_t ulBase64EncodedPayloadLength;
    uint32_t ulBase64SignatureLength;
    uint32_t ulJWKBase64EncodedHeaderLength;
    uint32_t ulJWKBase64EncodedPayloadLength;
    uint32_t ulJWKBase64EncodedSignatureLength;
    int32_t outSigningKeyELength;
    int32_t outSigningKeyNLength;
    int32_t outJWSHeaderLength;
    int32_t outJWSPayloadLength;
    int32_t outJWSSignatureLength;
    int32_t outJWKHeaderLength;
    int32_t outJWKPayloadLength;
    int32_t outJWKSignatureLength;
    az_span kidSpan;
    az_span sha256Span;
    az_span xBase64EncodedNSpan;
    az_span xBase64EncodedESpan;
    az_span xAlgSpan;
    az_span xJWKManifestSpan;
} prvJWSValidationContext_t;

/* prvSplitJWS takes a JWS payload and returns pointers to its constituent header, payload, and signature parts. */
static AzureIoTResult_t prvSplitJWS( uint8_t * pucJWS,
                                     uint32_t ulJWSLength,
                                     uint8_t ** ppucHeader,
                                     uint32_t * pulHeaderLength,
                                     uint8_t ** ppucPayload,
                                     uint32_t * pulPayloadLength,
                                     uint8_t ** ppucSignature,
                                     uint32_t * pulSignatureLength )
{
    uint8_t * pucFirstDot;
    uint8_t * pucSecondDot;
    uint32_t ulDotCount = 0;
    uint32_t ulIndex = 0;

    *ppucHeader = pucJWS;

    while( ulIndex < ulJWSLength )
    {
        if( *pucJWS == '.' )
        {
            ulDotCount++;

            if( ulDotCount == 1 )
            {
                pucFirstDot = pucJWS;
            }
            else if( ulDotCount == 2 )
            {
                pucSecondDot = pucJWS;
            }
            else if( ulDotCount > 2 )
            {
                LogError( ( "JWS had more '.' than required (2)" ) );
                return eAzureIoTErrorFailed;
            }
        }

        pucJWS++;
        ulIndex++;
    }

    if( ( ulDotCount != 2 ) || ( pucSecondDot >= ( *ppucHeader + ulJWSLength - 1 ) ) )
    {
        return eAzureIoTErrorFailed;
    }

    *pulHeaderLength = pucFirstDot - *ppucHeader;
    *ppucPayload = pucFirstDot + 1;
    *pulPayloadLength = pucSecondDot - *ppucPayload;
    *ppucSignature = pucSecondDot + 1;
    *pulSignatureLength = *ppucHeader + ulJWSLength - *ppucSignature;

    return eAzureIoTSuccess;
}

/**
 * @brief Calculate the SHA256 over a buffer of bytes
 *
 * @param pucInput The input buffer over which to calculate the SHA256.
 * @param ulInputLength The length of \p pucInput.
 * @param pucOutput The output buffer into which the SHA256. It must be 32 bytes in length.
 * @return uint32_t The result of the operation.
 * @retval 0 if successful.
 * @retval Non-0 if not successful.
 */
static AzureIoTResult_t prvJWS_SHA256Calculate( const uint8_t * pucInput,
                                                uint32_t ulInputLength,
                                                uint8_t * pucOutput )
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );
    mbedtls_md_update( &ctx, pucInput, ulInputLength );
    mbedtls_md_finish( &ctx, pucOutput );
    mbedtls_md_free( &ctx );

    return eAzureIoTSuccess;
}

/**
 * @brief Verify the manifest via RS256 for the JWS.
 *
 * @param pucInput The input over which the RS256 will be verified.
 * @param ulInputLength The length of \p pucInput.
 * @param pucSignature The encrypted signature which will be decrypted by \p pucN and \p pucE.
 * @param ulSignatureLength The length of \p pucSignature.
 * @param pucN The key's modulus which is used to decrypt \p signature.
 * @param ulNLength The length of \p pucN.
 * @param pucE The exponent used for the key.
 * @param ulELength The length of \p pucE.
 * @param pucBuffer The buffer used as scratch space to make the calculations. It should be at least
 * `jwsRSA3072_SIZE` + `jwsSHA256_SIZE` in size.
 * @param ulBufferLength The length of \p pucBuffer.
 * @return uint32_t The result of the operation.
 * @retval 0 if successful.
 * @retval Non-0 if not successful.
 */
static AzureIoTResult_t prvJWS_RS256Verify( uint8_t * pucInput,
                                            uint32_t ulInputLength,
                                            uint8_t * pucSignature,
                                            uint32_t ulSignatureLength,
                                            uint8_t * pucN,
                                            uint32_t ulNLength,
                                            uint8_t * pucE,
                                            uint32_t ulELength,
                                            uint8_t * pucBuffer,
                                            uint32_t ulBufferLength )
{
    AzureIoTResult_t xResult;
    int32_t lMbedTLSResult;
    uint8_t * pucShaBuffer;
    size_t ulDecryptedLength;
    mbedtls_rsa_context ctx;
    int shaMatchResult;

    if( ulBufferLength < jwsSHA_CALCULATION_SCRATCH_SIZE )
    {
        LogError( ( "[JWS] Buffer Not Large Enough" ) );
        return eAzureIoTErrorOutOfMemory;
    }

    pucShaBuffer = pucBuffer + jwsRSA3072_SIZE;

    /* The signature is encrypted using the input key. We need to decrypt the */
    /* signature which gives us the SHA256 inside a PKCS7 structure. We then compare */
    /* that to the SHA256 of the input. */
    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    lMbedTLSResult = mbedtls_rsa_import_raw( &ctx,
                                             pucN, ulNLength,
                                             NULL, 0,
                                             NULL, 0,
                                             NULL, 0,
                                             pucE, ulELength );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "[JWS] mbedtls_rsa_import_raw res: %i", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "[JWS] mbedtls_rsa_complete res: %i", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "[JWS] mbedtls_rsa_check_pubkey res: %i", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    /* RSA */
    lMbedTLSResult = mbedtls_rsa_pkcs1_decrypt( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, &ulDecryptedLength, pucSignature, pucBuffer, jwsRSA3072_SIZE );

    if( lMbedTLSResult != 0 )
    {
        LogError( ( "[JWS] mbedtls_rsa_pkcs1_decrypt res: %i", lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    mbedtls_rsa_free( &ctx );

    xResult = prvJWS_SHA256Calculate( pucInput, ulInputLength,
                                      pucShaBuffer );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvJWS_SHA256Calculate failed" ) );
        return xResult;
    }

    /* TODO: remove this once we have a valid PKCS7 parser. */
    shaMatchResult = memcmp( pucBuffer + jwsPKCS7_PAYLOAD_OFFSET, pucShaBuffer, jwsSHA256_SIZE );

    if( shaMatchResult != 0 )
    {
        LogError( ( "[JWS] SHA of JWK does NOT match" ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    return xResult;
}

static AzureIoTResult_t prvFindSJWKValue( AzureIoTJSONReader_t * pxPayload,
                                          az_span * pxJWSValue )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    AzureIoTJSONTokenType_t xJSONTokenType;

    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, ( const uint8_t * ) jws_sjwk_json_value, sizeof( jws_sjwk_json_value ) - 1 ) )
        {
            /* Found name, move to value */
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( pxPayload ) );
            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
    }

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] Parse JSK JSON Payload Error: 0x%08x", xResult ) );
        return xResult;
    }

    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_TokenType( pxPayload, &xJSONTokenType ) );

    if( xJSONTokenType != eAzureIoTJSONTokenSTRING )
    {
        LogError( ( "[JWS] JSON token type wrong | type: %08x", xJSONTokenType ) );
        return eAzureIoTErrorFailed;
    }

    *pxJWSValue = pxPayload->_internal.xCoreReader.token.slice;

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvFindRootKeyValue( AzureIoTJSONReader_t * pxPayload,
                                             az_span * pxKIDSpan )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    AzureIoTJSONTokenType_t xJSONTokenType;

    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, ( const uint8_t * ) jws_kid_json_value, sizeof( jws_kid_json_value ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            *pxKIDSpan = pxPayload->_internal.xCoreReader.token.slice;

            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( pxPayload ) );
            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
    }

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] Parse Root Key Error: 0x%08x", xResult ) );
        return xResult;
    }

    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_TokenType( pxPayload, &xJSONTokenType ) );

    if( xJSONTokenType != eAzureIoTJSONTokenSTRING )
    {
        LogError( ( "[JWS] JSON token type wrong | type: %08x", xJSONTokenType ) );
        return eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvFindKeyParts( AzureIoTJSONReader_t * pxPayload,
                                         az_span * pxBase64EncodedNSpan,
                                         az_span * pxBase64EncodedESpan,
                                         az_span * pxAlgSpan )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );

    while( xResult == eAzureIoTSuccess &&
           ( az_span_size( *pxBase64EncodedNSpan ) == 0 ||
             az_span_size( *pxBase64EncodedESpan ) == 0 ||
             az_span_size( *pxAlgSpan ) == 0 ) )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, jws_n_json_value, sizeof( jws_n_json_value ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            *pxBase64EncodedNSpan = pxPayload->_internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, jws_e_json_value, sizeof( jws_e_json_value ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            *pxBase64EncodedESpan = pxPayload->_internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, jws_alg_json_value, sizeof( jws_alg_json_value ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            *pxAlgSpan = pxPayload->_internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( pxPayload ) );
            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
    }

    if( ( xResult != eAzureIoTSuccess ) ||
        ( az_span_size( *pxBase64EncodedNSpan ) == 0 ) ||
        ( az_span_size( *pxBase64EncodedESpan ) == 0 ) ||
        ( az_span_size( *pxAlgSpan ) == 0 ) )
    {
        LogError( ( "[JWS] Parse Signing Key Payload Error: %i",
                    xResult != eAzureIoTSuccess ? xResult : eAzureIoTErrorFailed ) );
        return xResult != eAzureIoTSuccess ? xResult : eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvFindManifestSHA( AzureIoTJSONReader_t * pxPayload,
                                            az_span * pxSHA )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    AzureIoTJSONTokenType_t xJSONTokenType;

    /*Begin object */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
    /*Property Name */
    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( pxPayload, jws_sha256_json_value, sizeof( jws_sha256_json_value ) - 1 ) )
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            break;
        }
        else
        {
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_NextToken( pxPayload ) );
            azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_SkipChildren( pxPayload ) );
            xResult = AzureIoTJSONReader_NextToken( pxPayload );
        }
    }

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] Parse manifest SHA error: 0x%08x", xResult ) );
        return xResult;
    }

    azureiotresultRETURN_IF_FAILED( AzureIoTJSONReader_TokenType( pxPayload, &xJSONTokenType ) );

    if( xJSONTokenType != eAzureIoTJSONTokenSTRING )
    {
        LogError( ( "[JWS] JSON token type wrong | type: %08x", xJSONTokenType ) );
        return eAzureIoTErrorFailed;
    }

    *pxSHA = pxPayload->_internal.xCoreReader.token.slice;

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvBase64DecodeJWK( prvJWSValidationContext_t * pxManifestContext )
{
    az_result xCoreResult;

    xCoreResult = az_base64_url_decode( az_span_create( pxManifestContext->ucJWKHeader, jwsJWK_HEADER_SIZE ),
                                        az_span_create( pxManifestContext->pucJWKBase64EncodedHeader, pxManifestContext->ulJWKBase64EncodedHeaderLength ),
                                        &pxManifestContext->outJWKHeaderLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsJWK_HEADER_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_base64_url_decode( az_span_create( pxManifestContext->ucJWKPayload, jwsJWK_PAYLOAD_SIZE ),
                                        az_span_create( pxManifestContext->pucJWKBase64EncodedPayload, pxManifestContext->ulJWKBase64EncodedPayloadLength ),
                                        &pxManifestContext->outJWKPayloadLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsJWK_PAYLOAD_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_base64_url_decode( az_span_create( pxManifestContext->ucJWKSignature, jwsSIGNATURE_SIZE ),
                                        az_span_create( pxManifestContext->pucJWKBase64EncodedSignature, pxManifestContext->ulJWKBase64EncodedSignatureLength ),
                                        &pxManifestContext->outJWKSignatureLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsSIGNATURE_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvBase64DecodeSigningKey( prvJWSValidationContext_t * pxManifestContext )
{
    az_result xCoreResult;

    xCoreResult = az_base64_decode( az_span_create( pxManifestContext->ucSigningKeyN, jwsRSA3072_SIZE ),
                                    pxManifestContext->xBase64EncodedNSpan,
                                    &pxManifestContext->outSigningKeyNLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsRSA3072_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_base64_decode( az_span_create( pxManifestContext->ucSigningKeyE, jwsSIGNING_KEY_E_SIZE ),
                                    pxManifestContext->xBase64EncodedESpan,
                                    &pxManifestContext->outSigningKeyELength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsSIGNING_KEY_E_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvBase64DecodeJWSHeaderAndPayload( prvJWSValidationContext_t * pxManifestContext )
{
    az_result xCoreResult;

    xCoreResult = az_base64_url_decode( az_span_create( pxManifestContext->ucJWSPayload, jwsJWS_PAYLOAD_SIZE ),
                                        az_span_create( pxManifestContext->pucBase64EncodedPayload, pxManifestContext->ulBase64EncodedPayloadLength ),
                                        &pxManifestContext->outJWSPayloadLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsJWS_PAYLOAD_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_base64_url_decode( az_span_create( pxManifestContext->ucJWSSignature, jwsSIGNATURE_SIZE ),
                                        az_span_create( pxManifestContext->pucBase64EncodedSignature, pxManifestContext->ulBase64SignatureLength ),
                                        &pxManifestContext->outJWSSignatureLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsSIGNATURE_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvValidateRootKey( prvJWSValidationContext_t * pxManifestContext )
{
    AzureIoTJSONReader_t xJSONReader;

    AzureIoTJSONReader_Init( &xJSONReader, pxManifestContext->ucJWKHeader, pxManifestContext->outJWKHeaderLength );

    if( prvFindRootKeyValue( &xJSONReader, &pxManifestContext->kidSpan ) != eAzureIoTSuccess )
    {
        LogError( ( "Could not find kid in JSON" ) );
        return eAzureIoTErrorFailed;
    }

    if( !az_span_is_content_equal( az_span_create( ( uint8_t * ) AzureIoTADURootKeyId, AzureIoTADURootKeyIdSize ), pxManifestContext->kidSpan ) )
    {
        LogError( ( "[JWS] Using the wrong root key" ) );
        return eAzureIoTErrorFailed;
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvVerifySHAMatch( prvJWSValidationContext_t * pxManifestContext,
                                           const uint8_t * pucManifest,
                                           uint32_t ulManifestLength )
{
    AzureIoTJSONReader_t xJSONReader;
    AzureIoTResult_t ulVerificationResult;
    az_result xCoreResult;

    ulVerificationResult = prvJWS_SHA256Calculate( pucManifest,
                                                   ulManifestLength,
                                                   pxManifestContext->ucManifestSHACalculation );

    if( ulVerificationResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] SHA256 Calculation failed" ) );
        return ulVerificationResult;
    }

    AzureIoTJSONReader_Init( &xJSONReader, pxManifestContext->ucJWSPayload, pxManifestContext->outJWSPayloadLength );

    if( prvFindManifestSHA( &xJSONReader, &pxManifestContext->sha256Span ) != eAzureIoTSuccess )
    {
        LogError( ( "Error finding manifest signature SHA" ) );
        return eAzureIoTErrorFailed;
    }

    xCoreResult = az_base64_decode( az_span_create( pxManifestContext->ucParsedManifestSha, jwsSHA256_SIZE ),
                                    pxManifestContext->sha256Span,
                                    &pxManifestContext->outParsedManifestShaSize );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsSHA256_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    if( pxManifestContext->outParsedManifestShaSize != jwsSHA256_SIZE )
    {
        LogError( ( "[JWS] Base64 decoded SHA256 is not the correct length | expected: %i | actual: %i", jwsSHA256_SIZE, pxManifestContext->outParsedManifestShaSize ) );
        return eAzureIoTErrorFailed;
    }

    int32_t lComparisonResult = memcmp( pxManifestContext->ucManifestSHACalculation, pxManifestContext->ucParsedManifestSha, jwsSHA256_SIZE );

    if( lComparisonResult != 0 )
    {
        LogError( ( "[JWS] Calculated manifest SHA does not match SHA in payload" ) );
        return eAzureIoTErrorFailed;
    }
    else
    {
        LogInfo( ( "[JWS] Calculated manifest SHA matches parsed SHA" ) );
    }

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTJWS_ManifestAuthenticate( const uint8_t * pucManifest,
                                           uint32_t ulManifestLength,
                                           uint8_t * pucJWS,
                                           uint32_t ulJWSLength,
                                           uint8_t * pucScratchBuffer,
                                           uint32_t ulScratchBufferLength )
{
    az_result xCoreResult;
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xJSONReader;
    prvJWSValidationContext_t xManifestContext = { 0 };

    /* Break up scratch buffer for reusable and persistant sections */
    uint8_t * ucPersistentScratchSpaceHead = pucScratchBuffer;
    uint8_t * ucReusableScratchSpaceRoot = ucPersistentScratchSpaceHead + jwsJWS_HEADER_SIZE + jwsJWK_PAYLOAD_SIZE;
    uint8_t * ucReusableScratchSpaceHead = ucReusableScratchSpaceRoot;

    /*------------------- Parse and Decode the JWS Header ------------------------*/

    xResult = prvSplitJWS( pucJWS, ulJWSLength,
                           &xManifestContext.pucBase64EncodedHeader, &xManifestContext.ulBase64EncodedHeaderLength,
                           &xManifestContext.pucBase64EncodedPayload, &xManifestContext.ulBase64EncodedPayloadLength,
                           &xManifestContext.pucBase64EncodedSignature, &xManifestContext.ulBase64SignatureLength );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvSplitJWS failed" ) );
        return xResult;
    }

    xManifestContext.ucJWSHeader = ucPersistentScratchSpaceHead;
    ucPersistentScratchSpaceHead += jwsJWS_HEADER_SIZE;
    xCoreResult = az_base64_url_decode( az_span_create( xManifestContext.ucJWSHeader, jwsJWS_HEADER_SIZE ),
                                        az_span_create( xManifestContext.pucBase64EncodedHeader, xManifestContext.ulBase64EncodedHeaderLength ),
                                        &xManifestContext.outJWSHeaderLength );

    if( az_result_failed( xCoreResult ) )
    {
        LogError( ( "[JWS] az_base64_url_decode failed: result %i", xCoreResult ) );

        if( xCoreResult == AZ_ERROR_NOT_ENOUGH_SPACE )
        {
            LogError( ( "[JWS] Decode buffer was too small: %i bytes", jwsJWS_HEADER_SIZE ) );
        }

        return eAzureIoTErrorFailed;
    }

    /*------------------- Parse SJWK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    AzureIoTJSONReader_Init( &xJSONReader, xManifestContext.ucJWSHeader, xManifestContext.outJWSHeaderLength );

    if( prvFindSJWKValue( &xJSONReader, &xManifestContext.xJWKManifestSpan ) != eAzureIoTSuccess )
    {
        LogError( ( "Error finding sjwk value in payload" ) );
        return eAzureIoTErrorFailed;
    }

    /*------------------- Split JWK and Base64 Decode the JWK Payload ------------------------*/

    xResult = prvSplitJWS( az_span_ptr( xManifestContext.xJWKManifestSpan ), az_span_size( xManifestContext.xJWKManifestSpan ),
                           &xManifestContext.pucJWKBase64EncodedHeader, &xManifestContext.ulJWKBase64EncodedHeaderLength,
                           &xManifestContext.pucJWKBase64EncodedPayload, &xManifestContext.ulJWKBase64EncodedPayloadLength,
                           &xManifestContext.pucJWKBase64EncodedSignature, &xManifestContext.ulJWKBase64EncodedSignatureLength );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvSplitJWS failed" ) );
        return xResult;
    }

    xManifestContext.ucJWKHeader = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsJWK_HEADER_SIZE;
    /* Needs to be persisted so we can parse the signing key N and E later */
    xManifestContext.ucJWKPayload = ucPersistentScratchSpaceHead;
    ucPersistentScratchSpaceHead += jwsJWK_PAYLOAD_SIZE;
    xManifestContext.ucJWKSignature = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSIGNATURE_SIZE;

    prvBase64DecodeJWK( &xManifestContext );

    /*------------------- Parse root key id ------------------------*/

    prvValidateRootKey( &xManifestContext );

    /*------------------- Parse necessary pieces for signing key ------------------------*/

    AzureIoTJSONReader_Init( &xJSONReader, xManifestContext.ucJWKPayload, xManifestContext.outJWKPayloadLength );

    if( prvFindKeyParts( &xJSONReader, &xManifestContext.xBase64EncodedNSpan, &xManifestContext.xBase64EncodedESpan, &xManifestContext.xAlgSpan ) != eAzureIoTSuccess )
    {
        LogError( ( "Could not find parts for the signing key" ) );
        return eAzureIoTErrorFailed;
    }

    /*------------------- Verify the signature ------------------------*/

    xManifestContext.ucScratchCalculationBuffer = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSHA_CALCULATION_SCRATCH_SIZE;
    xResult = prvJWS_RS256Verify( xManifestContext.pucJWKBase64EncodedHeader, xManifestContext.ulJWKBase64EncodedHeaderLength + xManifestContext.ulJWKBase64EncodedPayloadLength + 1,
                                  xManifestContext.ucJWKSignature, xManifestContext.outJWKSignatureLength,
                                  ( uint8_t * ) AzureIoTADURootKeyN, AzureIoTADURootKeyNSize,
                                  ( uint8_t * ) AzureIoTADURootKeyE, AzureIoTADURootKeyESize,
                                  xManifestContext.ucScratchCalculationBuffer, jwsSHA_CALCULATION_SCRATCH_SIZE );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvJWS_RS256Verify failed" ) );
        return xResult;
    }

    /*------------------- Reuse Buffer Space ------------------------*/

    /* The JWK verification is now done, so we can reuse the buffers which it used. */
    ucReusableScratchSpaceHead = ucReusableScratchSpaceRoot;

    /*------------------- Decode remaining values from JWS ------------------------*/

    xManifestContext.ucJWSPayload = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsJWS_PAYLOAD_SIZE;
    xManifestContext.ucJWSSignature = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSIGNATURE_SIZE;

    xResult = prvBase64DecodeJWSHeaderAndPayload( &xManifestContext );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvBase64DecodeJWSHeaderAndPayload failed" ) );
        return xResult;
    }

    /*------------------- Base64 decode the signing key ------------------------*/

    xManifestContext.ucSigningKeyN = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsRSA3072_SIZE;
    xManifestContext.ucSigningKeyE = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSIGNING_KEY_E_SIZE;

    xResult = prvBase64DecodeSigningKey( &xManifestContext );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] prvBase64DecodeSigningKey failed" ) );
        return xResult;
    }

    /*------------------- Verify that the signature was signed by signing key ------------------------*/

    if( !az_span_is_content_equal( xManifestContext.xAlgSpan, az_span_create( ( uint8_t * ) jws_alg_rs256, sizeof( jws_alg_rs256 ) - 1 ) ) )
    {
        LogError( ( "[JWS] Algorithm not supported | expected %.*s | actual %.*s",
                    sizeof( jws_alg_rs256 ) - 1, jws_alg_rs256,
                    az_span_size( xManifestContext.xAlgSpan ), az_span_ptr( xManifestContext.xAlgSpan ) ) );
        return eAzureIoTErrorFailed;
    }

    xResult = prvJWS_RS256Verify( xManifestContext.pucBase64EncodedHeader, xManifestContext.ulBase64EncodedHeaderLength + xManifestContext.ulBase64EncodedPayloadLength + 1,
                                  xManifestContext.ucJWSSignature, xManifestContext.outJWSSignatureLength,
                                  xManifestContext.ucSigningKeyN, xManifestContext.outSigningKeyNLength,
                                  xManifestContext.ucSigningKeyE, xManifestContext.outSigningKeyELength,
                                  xManifestContext.ucScratchCalculationBuffer, jwsSHA_CALCULATION_SCRATCH_SIZE );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[JWS] Verification of signed manifest SHA failed" ) );
        return xResult;
    }

    /*------------------- Verify that the SHAs match ------------------------*/

    xManifestContext.ucManifestSHACalculation = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSHA256_SIZE;
    xManifestContext.ucParsedManifestSha = ucReusableScratchSpaceHead;
    ucReusableScratchSpaceHead += jwsSHA256_SIZE;

    return prvVerifySHAMatch( &xManifestContext, pucManifest, ulManifestLength );
}
