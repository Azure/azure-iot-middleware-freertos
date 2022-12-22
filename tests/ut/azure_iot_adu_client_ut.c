/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_adu_client.h"
/*-----------------------------------------------------------*/

#define testPROPERTY_CALLBACK_ID              ( 3 )
#define testDEVICE_MANUFACTURER               "Contoso"
#define testDEVICE_MODEL                      "Foobar"
#define testUPDATE_PROVIDER                   testDEVICE_MANUFACTURER
#define testUPDATE_NAME                       testDEVICE_MODEL
#define testUPDATE_VERSION                    "1.0"
#define testDO_VERSION                        "DU;lib/v0.6.0+20211001.174458.c8c4051,DU;agent/v0.6.0+20211001.174418.c8c4051"
#define testDEVICE_CUSTOM_PROPERTY_NAME_1     "abc"
#define testDEVICE_CUSTOM_PROPERTY_VALUE_1    "123"
#define testDEVICE_CUSTOM_PROPERTY_NAME_2     "def"
#define testDEVICE_CUSTOM_PROPERTY_VALUE_2    "456"
#define testUPDATE_ID                         "{\"provider\":\"" testUPDATE_PROVIDER "\",\"name\":\"" testUPDATE_NAME "\",\"version\":\"" testUPDATE_VERSION "\"}"

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static uint8_t ucComponentName[] = "deviceUpdate";
static uint8_t ucSendStatePayload[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\"deviceProperties\":{\"manufacturer\":\"Contoso\",\"model\":\"Foobar\",\"contractModelId\":\"dtmi:azure:iot:deviceUpdateContractModel;2\",\"aduVer\":\"DU;agent/1.0.0\",\"doVer\":\"" testDO_VERSION "\"},\"compatPropertyNames\":\"manufacturer,model\",\"state\":0,\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.0\\\"}\"}}}";
static uint8_t ucSendStateLongPayload[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\
\"deviceProperties\":{\"manufacturer\":\"Contoso\",\"model\":\"Foobar\",\
\"" testDEVICE_CUSTOM_PROPERTY_NAME_1 "\":\"" testDEVICE_CUSTOM_PROPERTY_VALUE_1 "\",\
\"" testDEVICE_CUSTOM_PROPERTY_NAME_2 "\":\"" testDEVICE_CUSTOM_PROPERTY_VALUE_2 "\",\
\"contractModelId\":\"dtmi:azure:iot:deviceUpdateContractModel;2\",\"aduVer\":\"DU;agent/1.0.0\",\"doVer\":\"" testDO_VERSION "\"},\
\"compatPropertyNames\":\"manufacturer,model\",\
\"lastInstallResult\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":\"Ok\",\"step_0\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":\"Ok\"}},\
\"state\":0,\
\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},\
\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.0\\\"}\"}}}";
static uint8_t ucSendStateLongPayloadWithRetry[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\
\"deviceProperties\":{\"manufacturer\":\"Contoso\",\"model\":\"Foobar\",\
\"" testDEVICE_CUSTOM_PROPERTY_NAME_1 "\":\"" testDEVICE_CUSTOM_PROPERTY_VALUE_1 "\",\
\"" testDEVICE_CUSTOM_PROPERTY_NAME_2 "\":\"" testDEVICE_CUSTOM_PROPERTY_VALUE_2 "\",\
\"contractModelId\":\"dtmi:azure:iot:deviceUpdateContractModel;2\",\"aduVer\":\"DU;agent/1.0.0\",\"doVer\":\"" testDO_VERSION "\"},\
\"compatPropertyNames\":\"manufacturer,model\",\
\"lastInstallResult\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":\"Ok\",\"step_0\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":\"Ok\"}},\
\"state\":0,\
\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\",\"retryTimestamp\":\"2022-01-26T11:33:29.9680598Z\"},\
\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.0\\\"}\"}}}";

static uint8_t ucSendResponsePayload[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"service\":{\"ac\":200,\"av\":1,\"value\":{}}}}";
static uint8_t ucHubClientBuffer[ 512 ];
static uint8_t ucRequestPayloadCopy[ 8000 ];
static uint8_t ucPayloadBuffer[ 8000 ];
static uint32_t ulReceivedCallbackFunctionId;
static uint32_t ulExtendedResultCode = 1234;
static uint8_t ucResultDetails[] = "Ok";
static uint8_t ulResultCode = 0;

/*Request Values */
static uint8_t ucADURequestPayload[] = "{\"service\":{\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"5\\\",\\\"updateId\\\":{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.1\\\"},\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"Contoso\\\",\\\"deviceModel\\\":\\\"Foobar\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\",\"updateManifestSignature\":\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9.eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{\"f2f4a804ca17afbae\":\"http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1\"}}}";
static uint8_t ucADURequestPayloadWithRetry[] = "{\"service\":{\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\",\"retryTimestamp\":\"2022-01-26T11:33:29.9680598Z\"},\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"5\\\",\\\"updateId\\\":{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.1\\\"},\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"Contoso\\\",\\\"deviceModel\\\":\\\"Foobar\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\",\"updateManifestSignature\":\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9.eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{\"f2f4a804ca17afbae\":\"http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1\"}}}";
static uint8_t ucADURequestPayloadUnusedFields[] = "{\"service\":{\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"5\\\",\\\"updateId\\\":{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.1\\\"},\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"Contoso\\\",\\\"deviceModel\\\":\\\"Foobar\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\\\"},\\\"mimeType\\\":\\\"application/octet-stream\\\",\\\"relatedFiles\\\":[{\\\"filename\\\":\\\"in1_in2_deltaupdate.dat\\\",\\\"sizeInBytes\\\":\\\"102910752\\\",\\\"hashes\\\":{\\\"sha256\\\":\\\"2MIl...\\\"},\\\"properties\\\":{\\\"microsoft.sourceFileAlgorithm\\\":\\\"sha256\\\",\\\"microsoft.sourceFileHash\\\":\\\"YmFY...\\\"}}],\\\"downloadHandler\\\":{\\\"id\\\":\\\"microsoft/delta:1\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\",\"updateManifestSignature\":\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9.eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{\"f2f4a804ca17afbae\":\"http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1\"}}}";
static uint8_t ucADURequestPayloadNoDeployment[] = "{\"service\":{\"workflow\":{\"action\":255,\"id\":\"nodeployment\"}},\"__t\":\"c\"}";
static uint8_t ucADURequestPayloadNoDeploymentNullManifest[] = "{\"service\":{\"workflow\":{\"action\":255,\"id\":\"nodeployment\"},\"updateManifest\":null,\"updateManifestSignature\":null,\"fileUrls\":null},\"__t\":\"c\"}";
static uint8_t ucADURequestManifest[] = "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Foobar\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"Contoso\",\"deviceModel\":\"Foobar\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"f2f4a804ca17afbae\"],\"handlerProperties\":{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"f2f4a804ca17afbae\":{\"fileName\":\"iot-middleware-sample-adu-v1.1\",\"sizeInBytes\":844976,\"hashes\":{\"sha256\":\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\"}}},\"createdDateTime\":\"2022-07-07T03:02:48.8449038Z\"}";
static uint8_t ucADURequestManifestUnusedFields[] = "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Foobar\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"Contoso\",\"deviceModel\":\"Foobar\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"f2f4a804ca17afbae\"],\"handlerProperties\":{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"f2f4a804ca17afbae\":{\"fileName\":\"iot-middleware-sample-adu-v1.1\",\"sizeInBytes\":844976,\"hashes\":{\"sha256\":\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\"},\"mimeType\":\"application/octet-stream\",\"relatedFiles\":[{\"filename\":\"in1_in2_deltaupdate.dat\",\"sizeInBytes\":\"102910752\",\"hashes\":{\"sha256\":\"2MIl...\"},\"properties\":{\"microsoft.sourceFileAlgorithm\":\"sha256\",\"microsoft.sourceFileHash\":\"YmFY...\"}}],\"downloadHandler\":{\"id\":\"microsoft/delta:1\"}}},\"createdDateTime\":\"2022-07-07T03:02:48.8449038Z\"}";
static uint32_t ulWorkflowAction = 3;
static uint32_t ulWorkflowActionNoDeployment = 255;
static uint8_t ucWorkflowID[] = "51552a54-765e-419f-892a-c822549b6f38";
static uint8_t ucWorkflowIDNoDeployment[] = "nodeployment";
static uint8_t ucWorkflowRetryTimestamp[] = "2022-01-26T11:33:29.9680598Z";
static uint8_t ucManifestVersion[] = "5";
static uint8_t ucUpdateIDProvider[] = "Contoso";
static uint8_t ucUpdateIDName[] = "Foobar";
static uint8_t ucUpdateIDVersion[] = "1.1";
static uint8_t ucInstructionsStepsHandler[] = "microsoft/swupdate:1";
static uint8_t ucInstructionsStepsFile[] = "f2f4a804ca17afbae";
static uint8_t ucInstructionsStepsHandlerPropertiesInstallCriteria[] = "1.0";
static uint8_t ucFilesID[] = "f2f4a804ca17afbae";
static uint8_t ucFilesFilename[] = "iot-middleware-sample-adu-v1.1";
static int64_t llFilesSizeInBytes = 844976;
static uint8_t ucFilesHashID[] = "sha256";
static uint8_t ucFilesHashesSHA[] = "xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=";
static uint8_t ucCreateDateTime[] = "2022-07-07T03:02:48.8449038Z";
static uint8_t ucSignature[] = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9.eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR";
static uint8_t ucFileUrl[] = "http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1";

/* Initialized in setup() below. */
AzureIoTADUClientDeviceProperties_t xADUDeviceProperties;

static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern uint32_t ulDelayReceivePacket;

/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

TickType_t xTaskGetTickCount( void )
{
    return 1;
}
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    return 0xFFFFFFFFFFFFFFFF;
}

/*-----------------------------------------------------------*/

static int setup( void ** state )
{
    ( void ) state;

    assert_int_equal( AzureIoTADUClient_DevicePropertiesInit( &xADUDeviceProperties ), eAzureIoTSuccess );
    xADUDeviceProperties.ucManufacturer = testDEVICE_MANUFACTURER;
    xADUDeviceProperties.ulManufacturerLength = sizeof( testDEVICE_MANUFACTURER ) - 1;
    xADUDeviceProperties.ucModel = testDEVICE_MODEL;
    xADUDeviceProperties.ulModelLength = sizeof( testDEVICE_MODEL ) - 1;
    xADUDeviceProperties.pxCustomProperties = NULL;
    xADUDeviceProperties.ucCurrentUpdateId = testUPDATE_ID;
    xADUDeviceProperties.ulCurrentUpdateIdLength = sizeof( testUPDATE_ID ) - 1;
    xADUDeviceProperties.ucDeliveryOptimizationAgentVersion = testDO_VERSION;
    xADUDeviceProperties.ulDeliveryOptimizationAgentVersionLength = sizeof( testDO_VERSION ) - 1;

    return 0;
}

/*-----------------------------------------------------------*/

static void prvSetupTestIoTHubClient( AzureIoTHubClient_t * pxTestIoTHubClient )
{
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( pxTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucHubClientBuffer,
                                              sizeof( ucHubClientBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTSuccess );
}

/*-----------------------------------------------------------*/

static void prvTestProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                               void * pvContext )
{
    assert_true( pxMessage != NULL );
    assert_true( pvContext == NULL );


    assert_int_equal( pxMessage->xMessageType, eAzureIoTHubPropertiesReportedResponseMessage );

    ulReceivedCallbackFunctionId = testPROPERTY_CALLBACK_ID;
}

/*-----------------------------------------------------------*/

static void testAzureIoTADUClient_OptionsInit_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_OptionsInit( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_OptionsInit_Success( void ** ppvState )
{
    ( void ) ppvState;
    AzureIoTADUClientOptions_t xADUClientOptions;

    assert_int_equal( AzureIoTADUClient_OptionsInit( &xADUClientOptions ), eAzureIoTSuccess );
}

static void testAzureIoTADUClient_Init_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTADUClientOptions_t xADUClientOptions = { 0 };

    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_Init( NULL, &xADUClientOptions ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_Init_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTADUClientOptions_t xADUClientOptions = { 0 };

    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, &xADUClientOptions ), eAzureIoTSuccess );
}

static void testAzureIoTADUClient_IsADUComponent_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_false( AzureIoTADUClient_IsADUComponent( NULL,
                                                    ucComponentName,
                                                    sizeof( ucComponentName ) - 1 ) );
}

static void testAzureIoTADUClient_IsADUComponent_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_true( AzureIoTADUClient_IsADUComponent( &xTestIoTADUClient,
                                                   ucComponentName,
                                                   sizeof( ucComponentName ) - 1 ) );
}

static void testAzureIoTADUClient_ParseRequest_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( NULL,
                                                      &xReader,
                                                      &xRequest ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      NULL,
                                                      &xRequest ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      NULL ), eAzureIoTErrorInvalidArgument );
}

static void prvParseRequestSuccess( uint8_t * pucRequestPayload,
                                    int32_t lRequestPayloadLength,
                                    uint8_t * pucExpectedManifest,
                                    int32_t lExpectedManifestLength )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, pucRequestPayload, lRequestPayloadLength ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest ), eAzureIoTSuccess );

    /* Workflow */
    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowAction );

    /* Update Manifest */
    assert_memory_equal( xRequest.pucUpdateManifest, pucExpectedManifest, lExpectedManifestLength );
    assert_int_equal( xRequest.ulUpdateManifestLength, lExpectedManifestLength );
    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowAction );
    assert_memory_equal( xRequest.xWorkflow.pucID, ucWorkflowID, sizeof( ucWorkflowID ) - 1 );
    assert_null( xRequest.xWorkflow.pucRetryTimestamp );
    assert_int_equal( xRequest.xWorkflow.ulRetryTimestampLength, 0 );
    assert_memory_equal( xRequest.xUpdateManifest.pucManifestVersion, ucManifestVersion, sizeof( ucManifestVersion ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.ulManifestVersionLength, sizeof( ucManifestVersion ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucProvider, ucUpdateIDProvider, sizeof( ucUpdateIDProvider ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulProviderLength, sizeof( ucUpdateIDProvider ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucName, ucUpdateIDName, sizeof( ucUpdateIDName ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulNameLength, sizeof( ucUpdateIDName ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucVersion, ucUpdateIDVersion, sizeof( ucUpdateIDVersion ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulVersionLength, sizeof( ucUpdateIDVersion ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pucHandler, ucInstructionsStepsHandler, sizeof( ucInstructionsStepsHandler ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].ulHandlerLength, sizeof( ucInstructionsStepsHandler ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pxFiles[ 0 ].pucFileName, ucInstructionsStepsFile, sizeof( ucInstructionsStepsFile ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pxFiles[ 0 ].ulFileNameLength, sizeof( ucInstructionsStepsFile ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pucInstalledCriteria, ucInstructionsStepsHandlerPropertiesInstallCriteria, sizeof( ucInstructionsStepsHandlerPropertiesInstallCriteria ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].ulInstalledCriteriaLength, sizeof( ucInstructionsStepsHandlerPropertiesInstallCriteria ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pucId, ucFilesID, sizeof( ucFilesID ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].ulIdLength, sizeof( ucFilesID ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pucFileName, ucFilesFilename, sizeof( ucFilesFilename ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].ulFileNameLength, sizeof( ucFilesFilename ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].llSizeInBytes, llFilesSizeInBytes );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucId, ucFilesHashID, sizeof( ucFilesHashID ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulIdLength, sizeof( ucFilesHashID ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucHash, ucFilesHashesSHA, sizeof( ucFilesHashesSHA ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulHashLength, sizeof( ucFilesHashesSHA ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pucCreateDateTime, ucCreateDateTime, sizeof( ucCreateDateTime ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.ulCreateDateTimeLength, sizeof( ucCreateDateTime ) - 1 );

    /* Signature */
    assert_memory_equal( xRequest.pucUpdateManifestSignature, ucSignature, sizeof( ucSignature ) - 1 );
    assert_int_equal( xRequest.ulUpdateManifestSignatureLength, sizeof( ucSignature ) - 1 );

    /* File URLs */
    assert_memory_equal( xRequest.pxFileUrls[ 0 ].pucId, ucFilesID, sizeof( ucFilesID ) - 1 );
    assert_int_equal( xRequest.pxFileUrls[ 0 ].ulIdLength, sizeof( ucFilesID ) - 1 );
    assert_memory_equal( xRequest.pxFileUrls[ 0 ].pucUrl, ucFileUrl, sizeof( ucFileUrl ) - 1 );
    assert_int_equal( xRequest.pxFileUrls[ 0 ].ulUrlLength, sizeof( ucFileUrl ) - 1 );
}

static void testAzureIoTADUClient_ParseRequest_Success( void ** ppvState )
{
    /* We have to copy the expected payload to a scratch buffer since it's unescaped in place */
    memcpy( ucRequestPayloadCopy, ucADURequestPayload, sizeof( ucADURequestPayload ) - 1 );
    prvParseRequestSuccess( ucRequestPayloadCopy, sizeof( ucADURequestPayload ) - 1,
                            ucADURequestManifest, sizeof( ucADURequestManifest ) - 1 );
}

/* Test that a request payload with delta update fields parses without errors (ignoring the fields) */
static void testAzureIoTADUClient_ParseRequest_UnusedFields_Success( void ** ppvState )
{
    /* We have to copy the expected payload to a scratch buffer since it's unescaped in place */
    memcpy( ucRequestPayloadCopy, ucADURequestPayloadUnusedFields, sizeof( ucADURequestPayloadUnusedFields ) - 1 );
    prvParseRequestSuccess( ucRequestPayloadCopy, sizeof( ucADURequestPayloadUnusedFields ) - 1,
                            ucADURequestManifestUnusedFields, sizeof( ucADURequestManifestUnusedFields ) - 1 );
}

static void testAzureIoTADUClient_ParseRequestWithRetry_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest;

    /* We have to copy the expected payload to a scratch buffer since it's unescaped in place */
    memcpy( ucRequestPayloadCopy, ucADURequestPayloadWithRetry, sizeof( ucADURequestPayloadWithRetry ) - 1 );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucRequestPayloadCopy, sizeof( ucADURequestPayloadWithRetry ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest ), eAzureIoTSuccess );

    /* Workflow */
    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowAction );

    /* Update Manifest */
    assert_memory_equal( xRequest.pucUpdateManifest, ucADURequestManifest, sizeof( ucADURequestManifest ) - 1 );
    assert_int_equal( xRequest.ulUpdateManifestLength, sizeof( ucADURequestManifest ) - 1 );
    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowAction );
    assert_memory_equal( xRequest.xWorkflow.pucID, ucWorkflowID, sizeof( ucWorkflowID ) - 1 );
    assert_memory_equal( xRequest.xWorkflow.pucRetryTimestamp, ucWorkflowRetryTimestamp, sizeof( ucWorkflowRetryTimestamp ) - 1 );
    assert_int_equal( xRequest.xWorkflow.ulRetryTimestampLength, sizeof( ucWorkflowRetryTimestamp ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pucManifestVersion, ucManifestVersion, sizeof( ucManifestVersion ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.ulManifestVersionLength, sizeof( ucManifestVersion ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucProvider, ucUpdateIDProvider, sizeof( ucUpdateIDProvider ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulProviderLength, sizeof( ucUpdateIDProvider ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucName, ucUpdateIDName, sizeof( ucUpdateIDName ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulNameLength, sizeof( ucUpdateIDName ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xUpdateId.pucVersion, ucUpdateIDVersion, sizeof( ucUpdateIDVersion ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xUpdateId.ulVersionLength, sizeof( ucUpdateIDVersion ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pucHandler, ucInstructionsStepsHandler, sizeof( ucInstructionsStepsHandler ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].ulHandlerLength, sizeof( ucInstructionsStepsHandler ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pxFiles[ 0 ].pucFileName, ucInstructionsStepsFile, sizeof( ucInstructionsStepsFile ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pxFiles[ 0 ].ulFileNameLength, sizeof( ucInstructionsStepsFile ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].pucInstalledCriteria, ucInstructionsStepsHandlerPropertiesInstallCriteria, sizeof( ucInstructionsStepsHandlerPropertiesInstallCriteria ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.xInstructions.pxSteps[ 0 ].ulInstalledCriteriaLength, sizeof( ucInstructionsStepsHandlerPropertiesInstallCriteria ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pucId, ucFilesID, sizeof( ucFilesID ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].ulIdLength, sizeof( ucFilesID ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pucFileName, ucFilesFilename, sizeof( ucFilesFilename ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].ulFileNameLength, sizeof( ucFilesFilename ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].llSizeInBytes, llFilesSizeInBytes );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucId, ucFilesHashID, sizeof( ucFilesHashID ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulIdLength, sizeof( ucFilesHashID ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucHash, ucFilesHashesSHA, sizeof( ucFilesHashesSHA ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulHashLength, sizeof( ucFilesHashesSHA ) - 1 );
    assert_memory_equal( xRequest.xUpdateManifest.pucCreateDateTime, ucCreateDateTime, sizeof( ucCreateDateTime ) - 1 );
    assert_int_equal( xRequest.xUpdateManifest.ulCreateDateTimeLength, sizeof( ucCreateDateTime ) - 1 );

    /* Signature */
    assert_memory_equal( xRequest.pucUpdateManifestSignature, ucSignature, sizeof( ucSignature ) - 1 );
    assert_int_equal( xRequest.ulUpdateManifestSignatureLength, sizeof( ucSignature ) - 1 );

    /* File URLs */
    assert_memory_equal( xRequest.pxFileUrls[ 0 ].pucId, ucFilesID, sizeof( ucFilesID ) - 1 );
    assert_int_equal( xRequest.pxFileUrls[ 0 ].ulIdLength, sizeof( ucFilesID ) - 1 );
    assert_memory_equal( xRequest.pxFileUrls[ 0 ].pucUrl, ucFileUrl, sizeof( ucFileUrl ) - 1 );
    assert_int_equal( xRequest.pxFileUrls[ 0 ].ulUrlLength, sizeof( ucFileUrl ) - 1 );
}

static void testAzureIoTADUClient_ParseRequest_NoDeployment_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest = { 0 };

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucADURequestPayloadNoDeployment, sizeof( ucADURequestPayloadNoDeployment ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest ), eAzureIoTSuccess );

    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowActionNoDeployment );
    assert_memory_equal( xRequest.xWorkflow.pucID, ucWorkflowIDNoDeployment, sizeof( ucWorkflowIDNoDeployment ) - 1 );
    assert_int_equal( xRequest.xWorkflow.ulIDLength, sizeof( ucWorkflowIDNoDeployment ) - 1 );
    assert_null( xRequest.xWorkflow.pucRetryTimestamp );
    assert_int_equal( xRequest.xWorkflow.ulRetryTimestampLength, 0 );
    assert_null( xRequest.pucUpdateManifest );
    assert_int_equal( xRequest.ulUpdateManifestLength, 0 );
    assert_null( xRequest.pucUpdateManifestSignature );
    assert_int_equal( xRequest.ulUpdateManifestSignatureLength, 0 );
    assert_int_equal( xRequest.ulFileUrlCount, 0 );
}

static void testAzureIoTADUClient_ParseRequest_NoDeploymentNullManifest_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest = { 0 };

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucADURequestPayloadNoDeploymentNullManifest, sizeof( ucADURequestPayloadNoDeploymentNullManifest ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest ), eAzureIoTSuccess );

    assert_int_equal( xRequest.xWorkflow.xAction, ulWorkflowActionNoDeployment );
    assert_memory_equal( xRequest.xWorkflow.pucID, ucWorkflowIDNoDeployment, sizeof( ucWorkflowIDNoDeployment ) - 1 );
    assert_int_equal( xRequest.xWorkflow.ulIDLength, sizeof( ucWorkflowIDNoDeployment ) - 1 );
    assert_null( xRequest.xWorkflow.pucRetryTimestamp );
    assert_int_equal( xRequest.xWorkflow.ulRetryTimestampLength, 0 );
    assert_null( xRequest.pucUpdateManifest );
    assert_int_equal( xRequest.ulUpdateManifestLength, 0 );
    assert_null( xRequest.pucUpdateManifestSignature );
    assert_int_equal( xRequest.ulUpdateManifestSignatureLength, 0 );
    assert_int_equal( xRequest.ulFileUrlCount, 0 );
}

static void testAzureIoTADUClient_SendResponse_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADURequestDecision_t xRequestDecision = eAzureIoTADURequestDecisionAccept;
    AzureIoTADUUpdateRequest_t xRequest;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_SendResponse( NULL,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucPayloadBuffer,
                                                      sizeof( ucPayloadBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      NULL,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucPayloadBuffer,
                                                      sizeof( ucPayloadBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      NULL,
                                                      sizeof( ucPayloadBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucPayloadBuffer,
                                                      0,
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_SendResponse_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADURequestDecision_t xRequestDecision = eAzureIoTADURequestDecisionAccept;
    AzureIoTADUUpdateRequest_t xRequest;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucPayloadBuffer,
                                                      sizeof( ucPayloadBuffer ),
                                                      &ulRequestId ), eAzureIoTSuccess );

    assert_memory_equal( ucPayloadBuffer, ucSendResponsePayload, sizeof( ucSendResponsePayload ) - 1 );
}

static void testAzureIoTADUClient_SendAgentState_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADUClientDeviceProperties_t xTestDeviceProperties;
    AzureIoTADUUpdateRequest_t xRequest;
    AzureIoTADUAgentState_t xState;
    AzureIoTADUClientInstallResult_t xInstallResult;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( NULL,
                                                        &xTestIoTHubClient,
                                                        &xTestDeviceProperties,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        NULL,
                                                        &xTestDeviceProperties,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        NULL,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xTestDeviceProperties,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        NULL,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xTestDeviceProperties,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        0,
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_SendAgentState_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xADUDeviceProperties,
                                                        NULL,
                                                        eAzureIoTADUAgentStateIdle,
                                                        NULL,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTSuccess );

    assert_memory_equal( ucPayloadBuffer, ucSendStatePayload, sizeof( ucSendStatePayload ) - 1 );
}

static void testAzureIoTADUClient_SendAgentState_WithAgentStateAndRequest_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xUpdateRequest;
    AzureIoTADUClientInstallResult_t xInstallResult;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;
    AzureIoTADUDeviceCustomProperties_t xDeviceCustomProperties;

    xDeviceCustomProperties.ulPropertyCount = 2;
    xDeviceCustomProperties.pucPropertyNames[ 0 ] = testDEVICE_CUSTOM_PROPERTY_NAME_1;
    xDeviceCustomProperties.ulPropertyNamesLengths[ 0 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_NAME_1 ) - 1;
    xDeviceCustomProperties.pucPropertyValues[ 0 ] = testDEVICE_CUSTOM_PROPERTY_VALUE_1;
    xDeviceCustomProperties.ulPropertyValuesLengths[ 0 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_VALUE_1 ) - 1;
    xDeviceCustomProperties.pucPropertyNames[ 1 ] = testDEVICE_CUSTOM_PROPERTY_NAME_2;
    xDeviceCustomProperties.ulPropertyNamesLengths[ 1 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_NAME_2 ) - 1;
    xDeviceCustomProperties.pucPropertyValues[ 1 ] = testDEVICE_CUSTOM_PROPERTY_VALUE_2;
    xDeviceCustomProperties.ulPropertyValuesLengths[ 1 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_VALUE_2 ) - 1;
    xADUDeviceProperties.pxCustomProperties = &xDeviceCustomProperties;

    xInstallResult.lResultCode = ulResultCode;
    xInstallResult.lExtendedResultCode = ulExtendedResultCode;
    xInstallResult.pucResultDetails = ucResultDetails;
    xInstallResult.ulResultDetailsLength = sizeof( ucResultDetails ) - 1;
    xInstallResult.ulStepResultsCount = 1;
    xInstallResult.pxStepResults[ 0 ].pucResultDetails = ucResultDetails;
    xInstallResult.pxStepResults[ 0 ].ulResultDetailsLength = sizeof( ucResultDetails ) - 1;
    xInstallResult.pxStepResults[ 0 ].ulExtendedResultCode = ulExtendedResultCode;
    xInstallResult.pxStepResults[ 0 ].ulResultCode = ulResultCode;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucADURequestPayload, sizeof( ucADURequestPayload ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xUpdateRequest ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xADUDeviceProperties,
                                                        &xUpdateRequest,
                                                        eAzureIoTADUAgentStateIdle,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTSuccess );

    assert_memory_equal( ucPayloadBuffer, ucSendStateLongPayload, sizeof( ucSendStateLongPayload ) - 1 );

    xADUDeviceProperties.pxCustomProperties = NULL;
}

static void testAzureIoTADUClient_SendAgentState_WithAgentStateAndRequestWithRetry_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xUpdateRequest;
    AzureIoTADUClientInstallResult_t xInstallResult;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;
    AzureIoTADUDeviceCustomProperties_t xDeviceCustomProperties;

    /* We have to copy the expected payload to a scratch buffer since it's unescaped in place */
    memcpy( ucRequestPayloadCopy, ucADURequestPayloadWithRetry, sizeof( ucADURequestPayloadWithRetry ) - 1 );

    xDeviceCustomProperties.ulPropertyCount = 2;
    xDeviceCustomProperties.pucPropertyNames[ 0 ] = testDEVICE_CUSTOM_PROPERTY_NAME_1;
    xDeviceCustomProperties.ulPropertyNamesLengths[ 0 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_NAME_1 ) - 1;
    xDeviceCustomProperties.pucPropertyValues[ 0 ] = testDEVICE_CUSTOM_PROPERTY_VALUE_1;
    xDeviceCustomProperties.ulPropertyValuesLengths[ 0 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_VALUE_1 ) - 1;
    xDeviceCustomProperties.pucPropertyNames[ 1 ] = testDEVICE_CUSTOM_PROPERTY_NAME_2;
    xDeviceCustomProperties.ulPropertyNamesLengths[ 1 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_NAME_2 ) - 1;
    xDeviceCustomProperties.pucPropertyValues[ 1 ] = testDEVICE_CUSTOM_PROPERTY_VALUE_2;
    xDeviceCustomProperties.ulPropertyValuesLengths[ 1 ] = sizeof( testDEVICE_CUSTOM_PROPERTY_VALUE_2 ) - 1;
    xADUDeviceProperties.pxCustomProperties = &xDeviceCustomProperties;

    xInstallResult.lResultCode = ulResultCode;
    xInstallResult.lExtendedResultCode = ulExtendedResultCode;
    xInstallResult.pucResultDetails = ucResultDetails;
    xInstallResult.ulResultDetailsLength = sizeof( ucResultDetails ) - 1;
    xInstallResult.ulStepResultsCount = 1;
    xInstallResult.pxStepResults[ 0 ].pucResultDetails = ucResultDetails;
    xInstallResult.pxStepResults[ 0 ].ulResultDetailsLength = sizeof( ucResultDetails ) - 1;
    xInstallResult.pxStepResults[ 0 ].ulExtendedResultCode = ulExtendedResultCode;
    xInstallResult.pxStepResults[ 0 ].ulResultCode = ulResultCode;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucRequestPayloadCopy, sizeof( ucADURequestPayloadWithRetry ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xUpdateRequest ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xADUDeviceProperties,
                                                        &xUpdateRequest,
                                                        eAzureIoTADUAgentStateIdle,
                                                        &xInstallResult,
                                                        ucPayloadBuffer,
                                                        sizeof( ucPayloadBuffer ),
                                                        &ulRequestId ), eAzureIoTSuccess );

    assert_memory_equal( ucPayloadBuffer, ucSendStateLongPayloadWithRetry, sizeof( ucSendStateLongPayloadWithRetry ) - 1 );

    xADUDeviceProperties.pxCustomProperties = NULL;
}

uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTADUClient_OptionsInit_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_OptionsInit_Success ),
        cmocka_unit_test( testAzureIoTADUClient_Init_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_Init_Success ),
        cmocka_unit_test( testAzureIoTADUClient_IsADUComponent_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_IsADUComponent_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequestWithRetry_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_UnusedFields_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_NoDeployment_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_NoDeploymentNullManifest_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendResponse_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_SendResponse_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_WithAgentStateAndRequest_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_WithAgentStateAndRequestWithRetry_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_ut", tests, setup, NULL );
}
/*-----------------------------------------------------------*/
