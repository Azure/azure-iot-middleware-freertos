/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <cmocka.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/threading.h"

#include "threading_alt.h"

#include "azure_iot_jws.h"
/* #include "demo_config.h" */
#include "FreeRTOSConfig.h"

static mbedtls_entropy_context xEntropyContext;
static mbedtls_ctr_drbg_context xCtrDrgbContext;

uint32_t ulGetAllTests();

/* ADU.200702.R */
static uint8_t ucAzureIoTADURootKeyId200702[ 13 ] = "ADU.200702.R";
static uint8_t ucAzureIoTADURootKeyN200702[ 385 ]
    =
    {
    0x00, 0xd5, 0x42, 0x2e, 0xaf, 0x11, 0x54, 0xa3, 0x50, 0x65, 0x87, 0xa2, 0x4d, 0x5b, 0xba,
    0x1a, 0xfb, 0xa9, 0x32, 0xdf, 0xe9, 0x99, 0x5f, 0x05, 0x45, 0xc8, 0xaf, 0xbd, 0x35, 0x1d,
    0x89, 0xe8, 0x27, 0x27, 0x58, 0xa3, 0xa8, 0xee, 0xc5, 0xc5, 0x1e, 0x4f, 0xf7, 0x92, 0xa6,
    0x12, 0x06, 0x7d, 0x3d, 0x7d, 0xb0, 0x07, 0xf6, 0x2c, 0x7f, 0xde, 0x6d, 0x2a, 0xf5, 0xbc,
    0x49, 0xbc, 0x15, 0xef, 0xf0, 0x81, 0xcb, 0x3f, 0x88, 0x4f, 0x27, 0x1d, 0x88, 0x71, 0x28,
    0x60, 0x08, 0xb6, 0x19, 0xd2, 0xd2, 0x39, 0xd0, 0x05, 0x1f, 0x3c, 0x76, 0x86, 0x71, 0xbb,
    0x59, 0x58, 0xbc, 0xb1, 0x88, 0x7b, 0xab, 0x56, 0x28, 0xbf, 0x31, 0x73, 0x44, 0x32, 0x10,
    0xfd, 0x3d, 0xd3, 0x96, 0x5c, 0xff, 0x4e, 0x5c, 0xb3, 0x6b, 0xff, 0x8b, 0x84, 0x9b, 0x8b,
    0x80, 0xb8, 0x49, 0xd0, 0x7d, 0xfa, 0xd6, 0x40, 0x58, 0x76, 0x4d, 0xc0, 0x72, 0x27, 0x75,
    0xcb, 0x9a, 0x2f, 0x9b, 0xb4, 0x9f, 0x0f, 0x25, 0xf1, 0x1c, 0xc5, 0x1b, 0x0b, 0x5a, 0x30,
    0x7d, 0x2f, 0xb8, 0xef, 0xa7, 0x26, 0x58, 0x53, 0xaf, 0xd5, 0x1d, 0x55, 0x01, 0x51, 0x0d,
    0xe9, 0x1b, 0xa2, 0x0f, 0x3f, 0xd7, 0xe9, 0x1d, 0x20, 0x41, 0xa6, 0xe6, 0x14, 0x0a, 0xae,
    0xfe, 0xf2, 0x1c, 0x2a, 0xd6, 0xe4, 0x04, 0x7b, 0xf6, 0x14, 0x7e, 0xec, 0x0f, 0x97, 0x83,
    0xfa, 0x58, 0xfa, 0x81, 0x36, 0x21, 0xb9, 0xa3, 0x2b, 0xfa, 0xd9, 0x61, 0x0b, 0x1a, 0x94,
    0xf7, 0xc1, 0xbe, 0x7f, 0x40, 0x14, 0x4a, 0xc9, 0xfa, 0x35, 0x7f, 0xef, 0x66, 0x70, 0x00,
    0xb1, 0xfd, 0xdb, 0xd7, 0x61, 0x0d, 0x3b, 0x58, 0x74, 0x67, 0x94, 0x89, 0x75, 0x76, 0x96,
    0x7c, 0x91, 0x87, 0xd2, 0x8e, 0x11, 0x97, 0xee, 0x7b, 0x87, 0x6c, 0x9a, 0x2f, 0x45, 0xd8,
    0x65, 0x3f, 0x52, 0x70, 0x98, 0x2a, 0xcb, 0xc8, 0x04, 0x63, 0xf5, 0xc9, 0x47, 0xcf, 0x70,
    0xf4, 0xed, 0x64, 0xa7, 0x74, 0xa5, 0x23, 0x8f, 0xb6, 0xed, 0xf7, 0x1c, 0xd3, 0xb0, 0x1c,
    0x64, 0x57, 0x12, 0x5a, 0xa9, 0x81, 0x84, 0x1f, 0xa0, 0xe7, 0x50, 0x19, 0x96, 0xb4, 0x82,
    0xb1, 0xac, 0x48, 0xe3, 0xe1, 0x32, 0x82, 0xcb, 0x40, 0x1f, 0xac, 0xc4, 0x59, 0xbc, 0x10,
    0x34, 0x51, 0x82, 0xf9, 0x28, 0x8d, 0xa8, 0x1e, 0x9b, 0xf5, 0x79, 0x45, 0x75, 0xb2, 0xdc,
    0x9a, 0x11, 0x43, 0x08, 0xbe, 0x61, 0xcc, 0x9a, 0xc4, 0xcb, 0x77, 0x36, 0xff, 0x83, 0xdd,
    0xa8, 0x71, 0x4f, 0x51, 0x8e, 0x0e, 0x7b, 0x4d, 0xfa, 0x79, 0x98, 0x8d, 0xbe, 0xfc, 0x82,
    0x7e, 0x40, 0x48, 0xa9, 0x12, 0x01, 0xa8, 0xd9, 0x7e, 0xf3, 0xa5, 0x1b, 0xf1, 0xfb, 0x90,
    0x77, 0x3e, 0x40, 0x87, 0x18, 0xc9, 0xab, 0xd9, 0xf7, 0x79
    };
static uint8_t ucAzureIoTADURootKeyE200702[ 3 ] = { 0x01, 0x00, 0x01 };

/* ADU.200703.R */
static uint8_t ucAzureIoTADURootKeyId200703[ 13 ] = "ADU.200703.R";
static uint8_t ucAzureIoTADURootKeyN200703[ 385 ] =
{
    0x00, 0xb2, 0xa3, 0xb2, 0x74, 0x16, 0xfa, 0xbb, 0x20, 0xf9, 0x52, 0x76, 0xe6, 0x27, 0x3e,
    0x80, 0x41, 0xc6, 0xfe, 0xcf, 0x30, 0xf9, 0xc8, 0x96, 0xf5, 0x59, 0x0a, 0xaa, 0x81, 0xe7,
    0x51, 0x83, 0x8a, 0xc4, 0xf5, 0x17, 0x3a, 0x2f, 0x2a, 0xe6, 0x57, 0xd4, 0x71, 0xce, 0x8a,
    0x3d, 0xef, 0x9a, 0x55, 0x76, 0x3e, 0x99, 0xe2, 0xc2, 0xae, 0x4c, 0xee, 0x2d, 0xb8, 0x78,
    0xf5, 0xa2, 0x4e, 0x28, 0xf2, 0x9c, 0x4e, 0x39, 0x65, 0xbc, 0xec, 0xe4, 0x0d, 0xe5, 0xe3,
    0x38, 0xa8, 0x59, 0xab, 0x08, 0xa4, 0x1b, 0xb4, 0xf4, 0xa0, 0x52, 0xa3, 0x38, 0xb3, 0x46,
    0x21, 0x13, 0xcc, 0x3c, 0x68, 0x06, 0xde, 0xfe, 0x00, 0xa6, 0x92, 0x6e, 0xde, 0x4c, 0x47,
    0x10, 0xd6, 0x1c, 0x9c, 0x24, 0xf5, 0xcd, 0x70, 0xe1, 0xf5, 0x6a, 0x7c, 0x68, 0x13, 0x1d,
    0xe1, 0xc5, 0xf6, 0xa8, 0x4f, 0x21, 0x9f, 0x86, 0x7c, 0x44, 0xc5, 0x8a, 0x99, 0x1c, 0xc5,
    0xd3, 0x06, 0x9b, 0x5a, 0x71, 0x9d, 0x09, 0x1c, 0xc3, 0x64, 0x31, 0x6a, 0xc5, 0x17, 0x95,
    0x1d, 0x5d, 0x2a, 0xf1, 0x55, 0xc7, 0x66, 0xd4, 0xe8, 0xf5, 0xd9, 0xa9, 0x5b, 0x8c, 0xa2,
    0x6c, 0x62, 0x60, 0x05, 0x37, 0xd7, 0x32, 0xb0, 0x73, 0xcb, 0xf7, 0x4b, 0x36, 0x27, 0x24,
    0x21, 0x8c, 0x38, 0x0a, 0xb8, 0x18, 0xfe, 0xf5, 0x15, 0x60, 0x35, 0x8b, 0x35, 0xef, 0x1e,
    0x0f, 0x88, 0xa6, 0x13, 0x8d, 0x7b, 0x7d, 0xef, 0xb3, 0xe7, 0xb0, 0xc9, 0xa6, 0x1c, 0x70,
    0x7b, 0xcc, 0xf2, 0x29, 0x8b, 0x87, 0xf7, 0xbd, 0x9d, 0xb6, 0x88, 0x6f, 0xac, 0x73, 0xff,
    0x72, 0xf2, 0xef, 0x48, 0x27, 0x96, 0x72, 0x86, 0x06, 0xa2, 0x5c, 0xe3, 0x7d, 0xce, 0xb0,
    0x9e, 0xe5, 0xc2, 0xd9, 0x4e, 0xc4, 0xf3, 0x7f, 0x78, 0x07, 0x4b, 0x65, 0x88, 0x45, 0x0c,
    0x11, 0xe5, 0x96, 0x56, 0x34, 0x88, 0x2d, 0x16, 0x0e, 0x59, 0x42, 0xd2, 0xf7, 0xd9, 0xed,
    0x1d, 0xed, 0xc9, 0x37, 0x77, 0x44, 0x7e, 0xe3, 0x84, 0x36, 0x9f, 0x58, 0x13, 0xef, 0x6f,
    0xe4, 0xc3, 0x44, 0xd4, 0x77, 0x06, 0x8a, 0xcf, 0x5b, 0xc8, 0x80, 0x1c, 0xa2, 0x98, 0x65,
    0x0b, 0x35, 0xdc, 0x73, 0xc8, 0x69, 0xd0, 0x5e, 0xe8, 0x25, 0x43, 0x9e, 0xf6, 0xd8, 0xab,
    0x05, 0xaf, 0x51, 0x29, 0x23, 0x55, 0x40, 0x58, 0x10, 0xea, 0xb8, 0xe2, 0xcd, 0x5d, 0x79,
    0xcc, 0xec, 0xdf, 0xb4, 0x5b, 0x98, 0xc7, 0xfa, 0xe3, 0xd2, 0x6c, 0x26, 0xce, 0x2e, 0x2c,
    0x56, 0xe0, 0xcf, 0x8d, 0xee, 0xfd, 0x93, 0x12, 0x2f, 0x00, 0x49, 0x8d, 0x1c, 0x82, 0x38,
    0x56, 0xa6, 0x5d, 0x79, 0x44, 0x4a, 0x1a, 0xf3, 0xdc, 0x16, 0x10, 0xb3, 0xc1, 0x2d, 0x27,
    0x11, 0xfe, 0x1b, 0x98, 0x05, 0xe4, 0xa3, 0x60, 0x31, 0x99
};
static uint8_t ucAzureIoTADURootKeyE200703[ 3 ] = { 0x01, 0x00, 0x01 };

static AzureIoTJWS_RootKey_t xADURootKeys[] =
{
    /*Put Root key 03 first since we know 02 is used in these ut (makes sure we cycle through the array) */
    {
        .pucRootKeyId = ucAzureIoTADURootKeyId200703,
        .ulRootKeyIdLength = sizeof( ucAzureIoTADURootKeyId200703 ) - 1,
        .pucRootKeyN = ucAzureIoTADURootKeyN200703,
        .ulRootKeyNLength = sizeof( ucAzureIoTADURootKeyN200703 ),
        .pucRootKeyExponent = ucAzureIoTADURootKeyE200703,
        .ulRootKeyExponentLength = sizeof( ucAzureIoTADURootKeyE200703 )
    },
    {
        .pucRootKeyId = ucAzureIoTADURootKeyId200702,
        .ulRootKeyIdLength = sizeof( ucAzureIoTADURootKeyId200702 ) - 1,
        .pucRootKeyN = ucAzureIoTADURootKeyN200702,
        .ulRootKeyNLength = sizeof( ucAzureIoTADURootKeyN200702 ),
        .pucRootKeyExponent = ucAzureIoTADURootKeyE200702,
        .ulRootKeyExponentLength = sizeof( ucAzureIoTADURootKeyE200702 )
    }
};

static const char ucValidManifest[] = "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"ESPRESSIF\",\"name\":"
                                      "\"ESP32-Azure-IoT-Kit\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":"
                                      "\"ESPRESSIF\",\"deviceModel\":\"ESP32-Azure-IoT-Kit\"}],\"instructions\":{\"steps\":"
                                      "[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"fc3558477982e3235\"],\"handlerProperties\":"
                                      "{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"fc3558477982e3235\":{\"fileName\":\"azure_iot_freertos_esp32.bin\","
                                      "\"sizeInBytes\":866128,\"hashes\":{\"sha256\":\"exKJAqfEo69Ok6C6SWy9+Hhp051JbRsXsnMjGSbbJ6o=\"}}},\"createdDateTime\":"
                                      "\"2022-06-03T00:20:33.8421122Z\"}";
/* manifestVersion changed to 5 from 4 */
static const char ucInvalidManifest[] = "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"ESPRESSIF\",\"name\":"
                                        "\"ESP32-Azure-IoT-Kit\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":"
                                        "\"ESPRESSIF\",\"deviceModel\":\"ESP32-Azure-IoT-Kit\"}],\"instructions\":{\"steps\":"
                                        "[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"fc3558477982e3235\"],\"handlerProperties\":"
                                        "{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"fc3558477982e3235\":{\"fileName\":\"azure_iot_freertos_esp32.bin\","
                                        "\"sizeInBytes\":866128,\"hashes\":{\"sha256\":\"exKJAqfEo69Ok6C6SWy9+Hhp051JbRsXsnMjGSbbJ6o=\"}}},\"createdDateTime\":"
                                        "\"2022-06-03T00:20:33.8421122Z\"}";
static char ucValidManifestJWS[] = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDS"
                                   "nVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWS"
                                   "GMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQU"
                                   "zBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jd"
                                   "mRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtd"
                                   "Flla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjV"
                                   "TlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0Y"
                                   "jNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc"
                                   "2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb"
                                   "2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQL"
                                   "URMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3T"
                                   "W5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJe"
                                   "UFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZ"
                                   "jdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTS"
                                   "VFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVIS"
                                   "kVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJMeTlqT1hHc1ZvQ1daM0N1dFhsWWNXQ2"
                                   "VYY2V3YkR4Ri9GbjVqM2srSW1ZPSJ9.Wq4UoXt4dGay_P8uy7jrxM8Iip3KCXkGZvQwnu83704CzDogfVqX4GegT68s47veOi3x2Gf5rjX7vOMzVf9Ck0"
                                   "ylGCfon-vit938hO9MNYM7siA5htYHzotdECD1LfI_BjlLxkwXt0OyLC1PJvMw9N870pb51NtTon0OmaQslEyf6ih6DrEvsNUnyjRcrzSWlIyRo18kqlz"
                                   "eetARTYE7qGQr7oZPh0RWXVP5b5XR3wbJ_IeZ6i85YmjFpbRGJaSPCuzpa7XKvvFzB5rB5lGmbkWsOMyLbVzUriW87BzbB06g-wzs1S-z07s-ZGjTbFdr"
                                   "XrGjkKtv3TaDirjTqHhhJyI2cVLBctr4Wv4XITPyZeJt2KcIQZup-KfCRNbM3c3_PXPgvJtOg5BhmUrUKGMqFTl84EIB44B1QqKmuiTdH3bNQxPKBecpC"
                                   "k-O9g03pB-fk1D_3sL1ju364STs87s77DfGK9e0oHbHgfzp4EdgrwRQBvTCWWKG3iT6ByfSH4N0";
/* Changes the SHA from {"sha256":"Ly9jOXGsVoCWZ3CutXlYcWCeXcewbDxF/Fn5j3k+ImY="} to {"sha256":abcjOXGsVoCWZ3CutXlYcWCeXcewbDxF/Fn5j3k+ImY="} */
/* (First three characters changed) */
static char ucWrongSHAManifestJWS[] = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDS"
                                      "nVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWS"
                                      "GMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQU"
                                      "zBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jd"
                                      "mRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtd"
                                      "Flla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjV"
                                      "TlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0Y"
                                      "jNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc"
                                      "2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb"
                                      "2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQL"
                                      "URMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3T"
                                      "W5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJe"
                                      "UFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZ"
                                      "jdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTS"
                                      "VFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVIS"
                                      "kVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOmFiY2pPWEdzVm9DV1ozQ3V0WGxZY1dDZ"
                                      "VhjZXdiRHhGL0ZuNWozaytJbVk9In0.Wq4UoXt4dGay_P8uy7jrxM8Iip3KCXkGZvQwnu83704CzDogfVqX4GegT68s47veOi3x2Gf5rjX7vOMzVf9Ck0"
                                      "ylGCfon-vit938hO9MNYM7siA5htYHzotdECD1LfI_BjlLxkwXt0OyLC1PJvMw9N870pb51NtTon0OmaQslEyf6ih6DrEvsNUnyjRcrzSWlIyRo18kqlz"
                                      "eetARTYE7qGQr7oZPh0RWXVP5b5XR3wbJ_IeZ6i85YmjFpbRGJaSPCuzpa7XKvvFzB5rB5lGmbkWsOMyLbVzUriW87BzbB06g-wzs1S-z07s-ZGjTbFdr"
                                      "XrGjkKtv3TaDirjTqHhhJyI2cVLBctr4Wv4XITPyZeJt2KcIQZup-KfCRNbM3c3_PXPgvJtOg5BhmUrUKGMqFTl84EIB44B1QqKmuiTdH3bNQxPKBecpC"
                                      "k-O9g03pB-fk1D_3sL1ju364STs87s77DfGK9e0oHbHgfzp4EdgrwRQBvTCWWKG3iT6ByfSH4N0";
static char ucScratchBuffer[ azureiotjwsSCRATCH_BUFFER_SIZE ];

static int prvInitMbedTLS( mbedtls_entropy_context * pxEntropyContext,
                           mbedtls_ctr_drbg_context * pxCtrDrgbContext )
{
    int32_t lMbedtlsError = 0;

    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );

    /* Initialize contexts for random number generation. */
    mbedtls_entropy_init( pxEntropyContext );
    mbedtls_ctr_drbg_init( pxCtrDrgbContext );

    /* Add a strong entropy source. At least one is required. */
    lMbedtlsError = mbedtls_entropy_add_source( pxEntropyContext,
                                                mbedtls_platform_entropy_poll,
                                                NULL,
                                                32,
                                                MBEDTLS_ENTROPY_SOURCE_STRONG );

    if( lMbedtlsError != 0 )
    {
        printf( "mbedtls_entropy_add_source failed: %i\n", lMbedtlsError );
        return lMbedtlsError;
    }

    /* Seed the random number generator. */
    lMbedtlsError = mbedtls_ctr_drbg_seed( pxCtrDrgbContext,
                                           mbedtls_entropy_func,
                                           pxEntropyContext,
                                           "jws_ut",
                                           strlen( "jws_ut" ) );

    if( lMbedtlsError != 0 )
    {
        printf( "mbedtls_ctr_drbg_seed failed: %i\n", lMbedtlsError );
    }

    return lMbedtlsError;
}

static int setup( void ** state )
{
    memset( ucScratchBuffer, 0, sizeof( ucScratchBuffer ) );
    return 0;
}

static void testAzureIoTJWS_ManifestAuthenticate_Success( void ** ppvState )
{
    assert_int_equal( AzureIoTJWS_ManifestAuthenticate( ucValidManifest, strlen( ucValidManifest ),
                                                        ucValidManifestJWS, strlen( ucValidManifestJWS ),
                                                        &xADURootKeys[ 0 ], sizeof( xADURootKeys ) / sizeof( xADURootKeys[ 0 ] ),
                                                        ucScratchBuffer, sizeof( ucScratchBuffer ) ), eAzureIoTSuccess );
}

static void testAzureIoTJWS_ManifestAuthenticate_Failure( void ** ppvState )
{
    assert_int_equal( AzureIoTJWS_ManifestAuthenticate( ucInvalidManifest, strlen( ucInvalidManifest ),
                                                        ucValidManifestJWS, strlen( ucValidManifestJWS ),
                                                        &xADURootKeys[ 0 ], sizeof( xADURootKeys ) / sizeof( xADURootKeys[ 0 ] ),
                                                        ucScratchBuffer, sizeof( ucScratchBuffer ) ), eAzureIoTErrorFailed );
}

static void testAzureIoTJWS_ManifestAuthenticate_WrongSha_Failure( void ** ppvState )
{
    assert_int_equal( AzureIoTJWS_ManifestAuthenticate( ucValidManifest, strlen( ucValidManifest ),
                                                        ucWrongSHAManifestJWS, strlen( ucWrongSHAManifestJWS ),
                                                        &xADURootKeys[ 0 ], sizeof( xADURootKeys ) / sizeof( xADURootKeys[ 0 ] ),
                                                        ucScratchBuffer, sizeof( ucScratchBuffer ) ), eAzureIoTErrorFailed );
}

uint32_t ulGetAllTests()
{
    if( prvInitMbedTLS( &xEntropyContext, &xCtrDrgbContext ) != 0 )
    {
        printf( "mbedtls init failed\n" );
        return 1;
    }

    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test_setup( testAzureIoTJWS_ManifestAuthenticate_Success,          setup ),
        cmocka_unit_test_setup( testAzureIoTJWS_ManifestAuthenticate_Failure,          setup ),
        cmocka_unit_test_setup( testAzureIoTJWS_ManifestAuthenticate_WrongSha_Failure, setup ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_jws_ut", tests, NULL, NULL );
}
