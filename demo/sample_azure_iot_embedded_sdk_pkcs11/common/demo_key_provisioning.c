/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Transport interface implementation include header for TLS. */
#include "core_pkcs11_config.h"
#include "core_pkcs11.h"
#include "using_mbedtls_pkcs11.h"
/* Utilities include. */
#include "core_pki_utils.h"

/* mbedTLS includes. */
#include "mbedtls/pk.h"
#include "mbedtls/oid.h"
/*-----------------------------------------------------------*/

#if !defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif
/*-----------------------------------------------------------*/

/* Length parameters for importing RSA-2048 private keys. */
#define MODULUS_LENGTH        pkcs11RSA_2048_MODULUS_BITS / 8
#define E_LENGTH              3
#define D_LENGTH              pkcs11RSA_2048_MODULUS_BITS / 8
#define PRIME_1_LENGTH        128
#define PRIME_2_LENGTH        128
#define EXPONENT_1_LENGTH     128
#define EXPONENT_2_LENGTH     128
#define COEFFICIENT_LENGTH    128
/*-----------------------------------------------------------*/

/* Adding one to all of the lengths because ASN1 may pad a leading 0 byte
 * to numbers that could be interpreted as negative */
typedef struct RsaParams_t
{
    CK_BYTE modulus[ MODULUS_LENGTH + 1 ];
    CK_BYTE e[ E_LENGTH + 1 ];
    CK_BYTE d[ D_LENGTH + 1 ];
    CK_BYTE prime1[ PRIME_1_LENGTH + 1 ];
    CK_BYTE prime2[ PRIME_2_LENGTH + 1 ];
    CK_BYTE exponent1[ EXPONENT_1_LENGTH + 1 ];
    CK_BYTE exponent2[ EXPONENT_2_LENGTH + 1 ];
    CK_BYTE coefficient[ COEFFICIENT_LENGTH + 1 ];
} RsaParams_t;
/*-----------------------------------------------------------*/

/* This function can be found in libraries/3rdparty/mbedtls_utils/mbedtls_utils.c. */
extern int convert_pem_to_der( const unsigned char * pucInput,
                               size_t xLen,
                               unsigned char * pucOutput,
                               size_t * pxOlen );
/*-----------------------------------------------------------*/

/* Delete the specified crypto object from storage. */
static CK_RV xDestroyProvidedObjects( CK_SESSION_HANDLE xSession,
                                      CK_BYTE_PTR * ppxPkcsLabels,
                                      CK_OBJECT_CLASS * xClass,
                                      CK_ULONG ulCount )
{
    CK_RV xResult;
    CK_FUNCTION_LIST_PTR pxFunctionList;
    CK_OBJECT_HANDLE xObjectHandle;
    CK_BYTE * pxLabel;
    CK_ULONG uiIndex = 0;

    xResult = C_GetFunctionList( &pxFunctionList );

    for( uiIndex = 0; uiIndex < ulCount; uiIndex++ )
    {
        pxLabel = ppxPkcsLabels[ uiIndex ];

        xResult = xFindObjectWithLabelAndClass( xSession,
                                                ( char * ) pxLabel,
                                                strlen(pxLabel),
                                                xClass[ uiIndex ],
                                                &xObjectHandle );

        while( ( xResult == CKR_OK ) && ( xObjectHandle != CK_INVALID_HANDLE ) )
        {
            xResult = pxFunctionList->C_DestroyObject( xSession, xObjectHandle );

            /* PKCS #11 allows a module to maintain multiple objects with the same
             * label and type. The intent of this loop is to try to delete all of them.
             * However, to avoid getting stuck, we won't try to find another object
             * of the same label/type if the previous delete failed. */
            if( xResult == CKR_OK )
            {
                xResult = xFindObjectWithLabelAndClass( xSession,
                                                        ( char * ) pxLabel,
                                                        strlen(pxLabel),
                                                        xClass[ uiIndex ],
                                                        &xObjectHandle );
            }
            else
            {
                break;
            }
        }

        if( xResult == CKR_FUNCTION_NOT_SUPPORTED )
        {
            break;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/* Import the specified X.509 client certificate into storage. */
static CK_RV xProvisionCertificate( CK_SESSION_HANDLE xSession,
                                    uint8_t * pucCertificate,
                                    size_t xCertificateLength,
                                    uint8_t * pucLabel,
                                    CK_OBJECT_HANDLE_PTR pxObjectHandle )
{
    PKCS11_CertificateTemplate_t xCertificateTemplate;
    CK_OBJECT_CLASS xCertificateClass = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE xCertificateType = CKC_X_509;
    CK_FUNCTION_LIST_PTR pxFunctionList;
    CK_RV xResult;
    uint8_t * pucDerObject = NULL;
    int32_t lConversionReturn = 0;
    size_t xDerLen = 0;
    CK_BBOOL xTokenStorage = CK_TRUE;

    /* TODO: Subject is a required attribute.
     * Currently, this field is not used by FreeRTOS ports,
     * this should be updated so that subject matches proper
     * format for future ports. */
    CK_BYTE xSubject[] = "TestSubject";

    /* Initialize the client certificate template. */
    xCertificateTemplate.xObjectClass.type = CKA_CLASS;
    xCertificateTemplate.xObjectClass.pValue = &xCertificateClass;
    xCertificateTemplate.xObjectClass.ulValueLen = sizeof( xCertificateClass );
    xCertificateTemplate.xSubject.type = CKA_SUBJECT;
    xCertificateTemplate.xSubject.pValue = xSubject;
    xCertificateTemplate.xSubject.ulValueLen = strlen( ( const char * ) xSubject );
    xCertificateTemplate.xValue.type = CKA_VALUE;
    xCertificateTemplate.xValue.pValue = ( CK_VOID_PTR ) pucCertificate;
    xCertificateTemplate.xValue.ulValueLen = ( CK_ULONG ) xCertificateLength;
    xCertificateTemplate.xLabel.type = CKA_LABEL;
    xCertificateTemplate.xLabel.pValue = ( CK_VOID_PTR ) pucLabel;
    xCertificateTemplate.xLabel.ulValueLen = strlen( ( const char * ) pucLabel );
    xCertificateTemplate.xCertificateType.type = CKA_CERTIFICATE_TYPE;
    xCertificateTemplate.xCertificateType.pValue = &xCertificateType;
    xCertificateTemplate.xCertificateType.ulValueLen = sizeof( CK_CERTIFICATE_TYPE );
    xCertificateTemplate.xTokenObject.type = CKA_TOKEN;
    xCertificateTemplate.xTokenObject.pValue = &xTokenStorage;
    xCertificateTemplate.xTokenObject.ulValueLen = sizeof( xTokenStorage );

    xResult = C_GetFunctionList( &pxFunctionList );

    /* Test for a valid certificate: 0x2d is '-', as in ----- BEGIN CERTIFICATE. */
    if( ( pucCertificate == NULL ) || ( pucCertificate[ 0 ] != 0x2d ) )
    {
        xResult = CKR_ATTRIBUTE_VALUE_INVALID;
    }

    if( xResult == CKR_OK )
    {
        /* Convert the certificate to DER format if it was in PEM. The DER key
         * should be about 3/4 the size of the PEM key, so mallocing the PEM key
         * size is sufficient. */
        pucDerObject = pvPortMalloc( xCertificateTemplate.xValue.ulValueLen );
        xDerLen = xCertificateTemplate.xValue.ulValueLen;

        if( pucDerObject != NULL )
        {
            lConversionReturn = convert_pem_to_der( xCertificateTemplate.xValue.pValue,
                                                    xCertificateTemplate.xValue.ulValueLen,
                                                    pucDerObject,
                                                    &xDerLen );

            if( 0 != lConversionReturn )
            {
                xResult = CKR_ARGUMENTS_BAD;
            }
        }
        else
        {
            xResult = CKR_HOST_MEMORY;
        }
    }

    if( xResult == CKR_OK )
    {
        /* Set the template pointers to refer to the DER converted objects. */
        xCertificateTemplate.xValue.pValue = pucDerObject;
        xCertificateTemplate.xValue.ulValueLen = xDerLen;
    }

    /* Best effort clean-up of the existing object, if it exists. */
    if( xResult == CKR_OK )
    {
        xDestroyProvidedObjects( xSession,
                                 &pucLabel,
                                 &xCertificateClass,
                                 1 );
    }

    /* Create an object using the encoded client certificate. */
    if( xResult == CKR_OK )
    {
        configPRINTF( ( "Write certificate...\r\n" ) );

        xResult = pxFunctionList->C_CreateObject( xSession,
                                                  ( CK_ATTRIBUTE_PTR ) &xCertificateTemplate,
                                                  sizeof( xCertificateTemplate ) / sizeof( CK_ATTRIBUTE ),
                                                  pxObjectHandle );
    }

    if( pucDerObject != NULL )
    {
        vPortFree( pucDerObject );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/* Import the specified ECDSA private key into storage. */
static CK_RV prvProvisionPrivateECKey( CK_SESSION_HANDLE xSession,
                                       uint8_t * pucLabel,
                                       CK_OBJECT_HANDLE_PTR pxObjectHandle,
                                       mbedtls_pk_context * pxMbedPkContext )
{
    CK_RV xResult = CKR_OK;
    CK_FUNCTION_LIST_PTR pxFunctionList = NULL;
    CK_BYTE * pxD;               /* Private value D. */
    CK_BYTE * pxEcParams = NULL; /* DER-encoding of an ANSI X9.62 Parameters value */
    int lMbedResult = 0;
    CK_BBOOL xTrue = CK_TRUE;
    CK_KEY_TYPE xPrivateKeyType = CKK_EC;
    CK_OBJECT_CLASS xPrivateKeyClass = CKO_PRIVATE_KEY;
    mbedtls_ecp_keypair * pxKeyPair = ( mbedtls_ecp_keypair * ) pxMbedPkContext->pk_ctx;

    xResult = C_GetFunctionList( &pxFunctionList );

#define EC_PARAMS_LENGTH    10
#define EC_D_LENGTH         32

    pxD = pvPortMalloc( EC_D_LENGTH );

    if( ( pxD == NULL ) )
    {
        xResult = CKR_HOST_MEMORY;
    }

    if( xResult == CKR_OK )
    {
        lMbedResult = mbedtls_mpi_write_binary( &( pxKeyPair->d ), pxD, EC_D_LENGTH );

        if( lMbedResult != 0 )
        {
            configPRINTF( ( "Failed to parse EC private key components. \r\n" ) );
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }
    }

    if( xResult == CKR_OK )
    {
        if( pxKeyPair->grp.id == MBEDTLS_ECP_DP_SECP256R1 )
        {
            pxEcParams = ( CK_BYTE * ) ( "\x06\x08" MBEDTLS_OID_EC_GRP_SECP256R1 );
        }
        else
        {
            xResult = CKR_CURVE_NOT_SUPPORTED;
        }
    }

    if( xResult == CKR_OK )
    {
        CK_ATTRIBUTE xPrivateKeyTemplate[] =
        {
            { CKA_CLASS,     NULL /* &xPrivateKeyClass*/, sizeof( CK_OBJECT_CLASS )                        },
            { CKA_KEY_TYPE,  NULL /* &xPrivateKeyType*/,  sizeof( CK_KEY_TYPE )                            },
            { CKA_LABEL,     pucLabel,                    ( CK_ULONG ) strlen( ( const char * ) pucLabel ) },
            { CKA_TOKEN,     NULL /* &xTrue*/,            sizeof( CK_BBOOL )                               },
            { CKA_SIGN,      NULL /* &xTrue*/,            sizeof( CK_BBOOL )                               },
            { CKA_EC_PARAMS, NULL /* pxEcParams*/,        EC_PARAMS_LENGTH                                 },
            { CKA_VALUE,     NULL /* pxD*/,               EC_D_LENGTH                                      }
        };

        /* Aggregate initializers must not use the address of an automatic variable. */
        /* See MSVC Compiler Warning C4221 */
        xPrivateKeyTemplate[ 0 ].pValue = &xPrivateKeyClass;
        xPrivateKeyTemplate[ 1 ].pValue = &xPrivateKeyType;
        xPrivateKeyTemplate[ 3 ].pValue = &xTrue;
        xPrivateKeyTemplate[ 4 ].pValue = &xTrue;
        xPrivateKeyTemplate[ 5 ].pValue = pxEcParams;
        xPrivateKeyTemplate[ 6 ].pValue = pxD;

        xResult = pxFunctionList->C_CreateObject( xSession,
                                                  ( CK_ATTRIBUTE_PTR ) &xPrivateKeyTemplate,
                                                  sizeof( xPrivateKeyTemplate ) / sizeof( CK_ATTRIBUTE ),
                                                  pxObjectHandle );
    }

    if( pxD != NULL )
    {
        vPortFree( pxD );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/* Import the specified RSA private key into storage. */
static CK_RV prvProvisionPrivateRSAKey( CK_SESSION_HANDLE xSession,
                                        uint8_t * pucLabel,
                                        CK_OBJECT_HANDLE_PTR pxObjectHandle,
                                        mbedtls_pk_context * pxMbedPkContext )
{
    CK_RV xResult = CKR_OK;
    CK_FUNCTION_LIST_PTR pxFunctionList = NULL;
    int lMbedResult = 0;
    CK_KEY_TYPE xPrivateKeyType = CKK_RSA;
    mbedtls_rsa_context * xRsaContext = pxMbedPkContext->pk_ctx;
    CK_OBJECT_CLASS xPrivateKeyClass = CKO_PRIVATE_KEY;
    RsaParams_t * pxRsaParams = NULL;
    CK_BBOOL xTrue = CK_TRUE;

    xResult = C_GetFunctionList( &pxFunctionList );

    pxRsaParams = pvPortMalloc( sizeof( RsaParams_t ) );

    if( pxRsaParams == NULL )
    {
        xResult = CKR_HOST_MEMORY;
    }

    if( xResult == CKR_OK )
    {
        memset( pxRsaParams, 0, sizeof( RsaParams_t ) );

        lMbedResult = mbedtls_rsa_export_raw( xRsaContext,
                                              pxRsaParams->modulus, MODULUS_LENGTH + 1,
                                              pxRsaParams->prime1, PRIME_1_LENGTH + 1,
                                              pxRsaParams->prime2, PRIME_2_LENGTH + 1,
                                              pxRsaParams->d, D_LENGTH + 1,
                                              pxRsaParams->e, E_LENGTH + 1 );

        if( lMbedResult != 0 )
        {
            configPRINTF( ( "Failed to parse RSA private key components. \r\n" ) );
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }

        /* Export Exponent 1, Exponent 2, Coefficient. */
        lMbedResult |= mbedtls_mpi_write_binary( ( mbedtls_mpi const * ) &xRsaContext->DP, pxRsaParams->exponent1, EXPONENT_1_LENGTH + 1 );
        lMbedResult |= mbedtls_mpi_write_binary( ( mbedtls_mpi const * ) &xRsaContext->DQ, pxRsaParams->exponent2, EXPONENT_2_LENGTH + 1 );
        lMbedResult |= mbedtls_mpi_write_binary( ( mbedtls_mpi const * ) &xRsaContext->QP, pxRsaParams->coefficient, COEFFICIENT_LENGTH + 1 );

        if( lMbedResult != 0 )
        {
            configPRINTF( ( "Failed to parse RSA private key Chinese Remainder Theorem variables. \r\n" ) );
            xResult = CKR_ATTRIBUTE_VALUE_INVALID;
        }
    }

    if( xResult == CKR_OK )
    {
        /* When importing the fields, the pointer is incremented by 1
         * to remove the leading 0 padding (if it existed) and the original field length is used */


        CK_ATTRIBUTE xPrivateKeyTemplate[] =
        {
            { CKA_CLASS,            NULL /* &xPrivateKeyClass */, sizeof( CK_OBJECT_CLASS )                        },
            { CKA_KEY_TYPE,         NULL /* &xPrivateKeyType */,  sizeof( CK_KEY_TYPE )                            },
            { CKA_LABEL,            pucLabel,                     ( CK_ULONG ) strlen( ( const char * ) pucLabel ) },
            { CKA_TOKEN,            NULL /* &xTrue */,            sizeof( CK_BBOOL )                               },
            { CKA_SIGN,             NULL /* &xTrue */,            sizeof( CK_BBOOL )                               },
            { CKA_MODULUS,          pxRsaParams->modulus + 1,     MODULUS_LENGTH                                   },
            { CKA_PRIVATE_EXPONENT, pxRsaParams->d + 1,           D_LENGTH                                         },
            { CKA_PUBLIC_EXPONENT,  pxRsaParams->e + 1,           E_LENGTH                                         },
            { CKA_PRIME_1,          pxRsaParams->prime1 + 1,      PRIME_1_LENGTH                                   },
            { CKA_PRIME_2,          pxRsaParams->prime2 + 1,      PRIME_2_LENGTH                                   },
            { CKA_EXPONENT_1,       pxRsaParams->exponent1 + 1,   EXPONENT_1_LENGTH                                },
            { CKA_EXPONENT_2,       pxRsaParams->exponent2 + 1,   EXPONENT_2_LENGTH                                },
            { CKA_COEFFICIENT,      pxRsaParams->coefficient + 1, COEFFICIENT_LENGTH                               }
        };

        /* Aggregate initializers must not use the address of an automatic variable. */
        /* See MSVC Compiler Warning C4221 */
        xPrivateKeyTemplate[ 0 ].pValue = &xPrivateKeyClass;
        xPrivateKeyTemplate[ 1 ].pValue = &xPrivateKeyType;
        xPrivateKeyTemplate[ 3 ].pValue = &xTrue;
        xPrivateKeyTemplate[ 4 ].pValue = &xTrue;

        xResult = pxFunctionList->C_CreateObject( xSession,
                                                  ( CK_ATTRIBUTE_PTR ) &xPrivateKeyTemplate,
                                                  sizeof( xPrivateKeyTemplate ) / sizeof( CK_ATTRIBUTE ),
                                                  pxObjectHandle );
    }

    if( NULL != pxRsaParams )
    {
        vPortFree( pxRsaParams );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/* Import the specified private key into storage. */
static CK_RV xProvisionPrivateKey( CK_SESSION_HANDLE xSession,
                                   uint8_t * pucPrivateKey,
                                   size_t xPrivateKeyLength,
                                   uint8_t * pucLabel,
                                   CK_OBJECT_HANDLE_PTR pxObjectHandle )
{
    CK_RV xResult = CKR_OK;
    mbedtls_pk_type_t xMbedKeyType = MBEDTLS_PK_NONE;
    int lMbedResult = 0;
    mbedtls_pk_context xMbedPkContext = { 0 };

    mbedtls_pk_init( &xMbedPkContext );
    lMbedResult = mbedtls_pk_parse_key( &xMbedPkContext, pucPrivateKey, xPrivateKeyLength, NULL, 0 );

    if( lMbedResult != 0 )
    {
        configPRINTF( ( "Unable to parse private key.\r\n" ) );
        xResult = CKR_ARGUMENTS_BAD;
    }

    /* Determine whether the key to be imported is RSA or EC. */
    if( xResult == CKR_OK )
    {
        xMbedKeyType = mbedtls_pk_get_type( &xMbedPkContext );

        if( xMbedKeyType == MBEDTLS_PK_RSA )
        {
            xResult = prvProvisionPrivateRSAKey( xSession,
                                                 pucLabel,
                                                 pxObjectHandle,
                                                 &xMbedPkContext );
        }
        else if( ( xMbedKeyType == MBEDTLS_PK_ECDSA ) || ( xMbedKeyType == MBEDTLS_PK_ECKEY ) || ( xMbedKeyType == MBEDTLS_PK_ECKEY_DH ) )
        {
            xResult = prvProvisionPrivateECKey( xSession,
                                                pucLabel,
                                                pxObjectHandle,
                                                &xMbedPkContext );
        }
        else
        {
            configPRINTF( ( "Invalid private key type provided.  RSA-2048 and EC P-256 keys are supported.\r\n" ) );
            xResult = CKR_ARGUMENTS_BAD;
        }
    }

    mbedtls_pk_free( &xMbedPkContext );

    return xResult;
}
/*-----------------------------------------------------------*/

/* Delete well-known crypto objects from storage. */
static CK_RV xDestroyDefaultCryptoObjects( CK_SESSION_HANDLE xSession )
{
    CK_RV xResult;
    CK_BYTE * pxPkcsLabels[] =
    {
        ( CK_BYTE * ) pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
        ( CK_BYTE * ) pkcs11configLABEL_CODE_VERIFICATION_KEY,
        ( CK_BYTE * ) pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS,
        ( CK_BYTE * ) pkcs11configLABEL_DEVICE_PUBLIC_KEY_FOR_TLS
    };
    CK_OBJECT_CLASS xClass[] =
    {
        CKO_CERTIFICATE,
        CKO_PUBLIC_KEY,
        CKO_PRIVATE_KEY,
        CKO_PUBLIC_KEY
    };

    xResult = xDestroyProvidedObjects( xSession,
                                       pxPkcsLabels,
                                       xClass,
                                       sizeof( xClass ) / sizeof( CK_OBJECT_CLASS ) );

    return xResult;
}
/*-----------------------------------------------------------*/

static uint32_t prvImportClientCred( CK_SESSION_HANDLE xSession )
{
    CK_RV xResult;
    CK_FUNCTION_LIST_PTR pxFunctionList;
    CK_OBJECT_HANDLE xObject = 0;
    CK_BBOOL xImportedPrivateKey = CK_FALSE;
    CK_BBOOL xKeyPairGenerationMode = CK_FALSE;

#if ( pkcs11configIMPORT_PRIVATE_KEYS_SUPPORTED == 1 )
    xResult = xDestroyDefaultCryptoObjects( xSession );

    if( xResult != CKR_OK )
    {
        configPRINTF( ( "Warning: could not clean-up old crypto objects: error=0x%08x \r\n", xResult ) );
    }
#endif

    xResult = xProvisionCertificate( xSession,
                                     democonfigCLIENT_CERTIFICATE_PEM,
                                     sizeof( democonfigCLIENT_CERTIFICATE_PEM ),
                                     ( uint8_t * ) pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                     &xObject );

    if( ( xResult != CKR_OK ) || ( xObject == CK_INVALID_HANDLE ) )
    {
        configPRINTF( ( "ERROR: Failed to provision device certificate: error=0x%08x \r\n", xResult ) );
        return xResult;
    }

#if ( pkcs11configIMPORT_PRIVATE_KEYS_SUPPORTED == 1 )
    xResult = xProvisionPrivateKey( xSession,
                                    democonfigCLIENT_PRIVATE_KEY_PEM,
                                    sizeof( democonfigCLIENT_PRIVATE_KEY_PEM ),
                                    ( uint8_t * ) pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS,
                                    &xObject );
    if( ( xResult != CKR_OK ) || ( xObject == CK_INVALID_HANDLE ) )
    {
        configPRINTF( ( "ERROR: Failed to provision device private key with status : error=0x%08x\r\n", xResult ) );
        return xResult;
    }
#endif

    return xResult;
}
/*-----------------------------------------------------------*/

uint32_t DemoKeyProvision()
{
    uint32_t xReturn ;
    CK_RV xPKCSResult;
    CK_FUNCTION_LIST_PTR pxFunctionList = NULL;
    CK_SESSION_HANDLE xSession = 0;

    xPKCSResult = C_GetFunctionList( &pxFunctionList );

    /* Initialize the PKCS Module */
    if( ( xPKCSResult = xInitializePkcs11Token() ) != CKR_OK )
    {
        xReturn = -1;
    }
    else if ( ( xPKCSResult = xInitializePkcs11Session( &xSession ) ) != CKR_OK )
    {
        xReturn = -1;
    }
    else
    {
        xReturn = prvImportClientCred( xSession );

        pxFunctionList->C_CloseSession( xSession );
    }

    return xReturn;
}
/*-----------------------------------------------------------*/
