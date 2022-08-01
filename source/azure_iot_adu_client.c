/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_adu_client.h"

#include "azure_iot.h"
/* Kernel includes. */
#include "FreeRTOS.h"

#include "azure_iot_private.h"
#include "azure_iot_json_writer.h"
#include "azure_iot_private.h"
#include <azure/iot/az_iot_adu_client.h>

/* Code 406 is "Not Acceptable" */
#define azureiotaduREQUEST_ACCEPTED_CODE    200
#define azureiotaduREQUEST_REJECTED_CODE    406

/* The following key is used to validate the Azure Device Update update manifest signature */
/* For more details, please see https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-security */
const uint8_t AzureIoTADURootKeyId[ 13 ] = "ADU.200702.R";
const uint8_t AzureIoTADURootKeyN[ 385 ] =
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
const uint8_t AzureIoTADURootKeyE[ 3 ] = { 0x01, 0x00, 0x01 };

AzureIoTResult_t AzureIoTADUClient_OptionsInit( AzureIoTADUClientOptions_t * pxADUClientOptions )
{
    AzureIoTResult_t xResult;

    if( pxADUClientOptions == NULL )
    {
        AZLogError( ( "AzureIoTADUClient_OptionsInit failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    }
    else
    {
        memset( pxADUClientOptions, 0, sizeof( AzureIoTADUClientOptions_t ) );

        xResult = eAzureIoTSuccess;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                         AzureIoTADUClientOptions_t * pxADUClientOptions )
{
    az_result xCoreResult;
    az_iot_adu_client_options xADUOptions;

    if( pxAzureIoTADUClient == NULL )
    {
        AZLogError( ( "AzureIoTADUClient_Init failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    memset( ( void * ) pxAzureIoTADUClient, 0, sizeof( AzureIoTADUClient_t ) );

    xADUOptions = az_iot_adu_client_options_default();

    if( pxADUClientOptions )
    {
        if( ( pxADUClientOptions->pucCompatibilityProperties != NULL ) &&
            ( pxADUClientOptions->ulCompatibilityPropertiesLength > 0 ) )
        {
            xADUOptions.device_compatibility_properties = az_span_create(
                ( uint8_t * ) pxADUClientOptions->pucCompatibilityProperties,
                ( int32_t ) pxADUClientOptions->ulCompatibilityPropertiesLength );
        }
    }

    if( az_result_failed( xCoreResult = az_iot_adu_client_init( &pxAzureIoTADUClient->_internal.xADUClient, &xADUOptions ) ) )
    {
        AZLogError( ( "Failed to initialize az_iot_adu_client_init: core error=0x%08x", xCoreResult ) );
        return AzureIoT_TranslateCoreError( xCoreResult );
    }

    return eAzureIoTSuccess;
}

bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                       const uint8_t * pucComponentName,
                                       uint32_t ulComponentNameLength )
{
    if( pxAzureIoTADUClient == NULL )
    {
        AZLogError( ( "AzureIoTADUClient_IsADUComponent failed: invalid argument" ) );
        return false;
    }

    return az_iot_adu_client_is_component_device_update( &pxAzureIoTADUClient->_internal.xADUClient,
                                                         az_span_create( ( uint8_t * ) pucComponentName, ( int32_t ) ulComponentNameLength ) );
}

static void prvCastUpdateRequest( az_iot_adu_client_update_request * pxBaseUpdateRequest,
                                  az_iot_adu_client_update_manifest * pxBaseUpdateManifest,
                                  AzureIoTADUUpdateRequest_t * pxUpdateRequest )
{
    pxUpdateRequest->xWorkflow.pucID = az_span_ptr( pxBaseUpdateRequest->workflow.id );
    pxUpdateRequest->xWorkflow.ulIDLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->workflow.id );
    pxUpdateRequest->xWorkflow.xAction = ( AzureIoTADUAction_t ) pxBaseUpdateRequest->workflow.action;
    pxUpdateRequest->xWorkflow.pucRetryTimestamp = az_span_ptr( pxBaseUpdateRequest->workflow.retry_timestamp );
    pxUpdateRequest->xWorkflow.ulRetryTimestampLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->workflow.retry_timestamp );
    pxUpdateRequest->pucUpdateManifest = az_span_ptr( pxBaseUpdateRequest->update_manifest );
    pxUpdateRequest->ulUpdateManifestLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->update_manifest );
    pxUpdateRequest->pucUpdateManifestSignature = az_span_ptr( pxBaseUpdateRequest->update_manifest_signature );
    pxUpdateRequest->ulUpdateManifestSignatureLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->update_manifest_signature );
    pxUpdateRequest->ulFileUrlCount = pxBaseUpdateRequest->file_urls_count;

    for( uint32_t ulFileUrlIndex = 0; ulFileUrlIndex < pxBaseUpdateRequest->file_urls_count; ulFileUrlIndex++ )
    {
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].pucId = az_span_ptr( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].id );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].ulIdLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].id );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].pucUrl = az_span_ptr( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].url );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].ulUrlLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].url );
    }

    if( pxBaseUpdateManifest != NULL )
    {
        pxUpdateRequest->xUpdateManifest.xUpdateId.pucProvider = az_span_ptr( pxBaseUpdateManifest->update_id.provider );
        pxUpdateRequest->xUpdateManifest.xUpdateId.ulProviderLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.provider );
        pxUpdateRequest->xUpdateManifest.xUpdateId.pucName = az_span_ptr( pxBaseUpdateManifest->update_id.name );
        pxUpdateRequest->xUpdateManifest.xUpdateId.ulNameLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.name );
        pxUpdateRequest->xUpdateManifest.xUpdateId.pucVersion = az_span_ptr( pxBaseUpdateManifest->update_id.version );
        pxUpdateRequest->xUpdateManifest.xUpdateId.ulVersionLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.version );

        pxUpdateRequest->xUpdateManifest.xInstructions.ulStepsCount = pxBaseUpdateManifest->instructions.steps_count;

        for( uint32_t ulStepIndex = 0; ulStepIndex < pxBaseUpdateManifest->instructions.steps_count; ulStepIndex++ )
        {
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pucHandler =
                az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler );
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulHandlerLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler );
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pucInstalledCriteria =
                az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler_properties.installed_criteria );
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulInstalledCriteriaLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler_properties.installed_criteria );
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulFilesCount =
                pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files_count;

            for( uint32_t ulFileIndex = 0; ulFileIndex < pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files_count; ulFileIndex++ )
            {
                pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pxFiles[ ulFileIndex ].pucFileName =
                    az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files[ ulFileIndex ] );
                pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pxFiles[ ulFileIndex ].ulFileNameLength =
                    ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files[ ulFileIndex ] );
            }
        }

        pxUpdateRequest->xUpdateManifest.ulFilesCount = pxBaseUpdateManifest->files_count;

        for( uint32_t ulFileIndex = 0; ulFileIndex < pxBaseUpdateManifest->files_count; ulFileIndex++ )
        {
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pucId =
                az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].id );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulIdLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].id );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pucFileName =
                az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].file_name );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulFileNameLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].file_name );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulSizeInBytes =
                pxBaseUpdateManifest->files[ ulFileIndex ].size_in_bytes;
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulHashesCount =
                pxBaseUpdateManifest->files[ ulFileIndex ].hashes_count;

            for( uint32_t ulFileHashIndex = 0; ulFileHashIndex < pxBaseUpdateManifest->files_count; ulFileHashIndex++ )
            {
                pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].pucId =
                    az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_type );
                pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].ulIdLength =
                    ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_type );
                pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].pucHash =
                    az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_value );
                pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].ulHashLength =
                    ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_value );
            }
        }

        pxUpdateRequest->xUpdateManifest.pucManifestVersion = az_span_ptr( pxBaseUpdateManifest->manifest_version );
        pxUpdateRequest->xUpdateManifest.ulManifestVersionLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->manifest_version );
        pxUpdateRequest->xUpdateManifest.pucCreateDateTime = az_span_ptr( pxBaseUpdateManifest->create_date_time );
        pxUpdateRequest->xUpdateManifest.ulCreateDateTimeLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->create_date_time );
    }
}

AzureIoTResult_t AzureIoTADUClient_ParseRequest( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTJSONReader_t * pxReader,
                                                 AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                 uint8_t * pucBuffer,
                                                 uint32_t ulBufferSize )
{
    az_iot_adu_client_update_request xBaseUpdateRequest;
    az_iot_adu_client_update_manifest xBaseUpdateManifest;
    az_result xAzResult;
    az_json_reader jr;
    az_span xBufferSpan;

    if( ( pxAzureIoTADUClient == NULL ) || ( pxReader == NULL ) ||
        ( pxAduUpdateRequest == NULL ) || ( pucBuffer == NULL ) || ( ulBufferSize == 0 ) )
    {
        AZLogError( ( "AzureIoTADUClient_ParseRequest failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xBufferSpan = az_span_create( pucBuffer, ( int32_t ) ulBufferSize );

    xAzResult = az_iot_adu_client_parse_service_properties(
        &pxAzureIoTADUClient->_internal.xADUClient,
        &pxReader->_internal.xCoreReader,
        xBufferSpan,
        &xBaseUpdateRequest,
        &xBufferSpan );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_iot_adu_client_parse_service_properties failed" ) );
        return AzureIoT_TranslateCoreError( xAzResult );
    }
    else
    {
        if( az_span_size( xBaseUpdateRequest.update_manifest ) > 0 )
        {
            xAzResult = az_json_reader_init( &jr, xBaseUpdateRequest.update_manifest, NULL );

            if( az_result_failed( xAzResult ) )
            {
                AZLogError( ( "az_json_reader_init failed: 0x%08x", xAzResult ) );
                return AzureIoT_TranslateCoreError( xAzResult );
            }

            xAzResult = az_iot_adu_client_parse_update_manifest(
                &pxAzureIoTADUClient->_internal.xADUClient,
                &jr,
                &xBaseUpdateManifest );

            if( az_result_failed( xAzResult ) )
            {
                AZLogError( ( "az_iot_adu_client_parse_update_manifest failed: 0x%08x", xAzResult ) );
                return AzureIoT_TranslateCoreError( xAzResult );
            }
        }
        else
        {
            /*No manifest to parse */
        }

        prvCastUpdateRequest( &xBaseUpdateRequest,
                              az_span_size( xBaseUpdateRequest.update_manifest ) > 0 ? &xBaseUpdateManifest : NULL,
                              pxAduUpdateRequest );
    }

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTADUClient_SendResponse( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                 AzureIoTADURequestDecision_t xRequestDecision,
                                                 uint32_t ulPropertyVersion,
                                                 uint8_t * pucWritablePropertyResponseBuffer,
                                                 uint32_t ulWritablePropertyResponseBufferSize,
                                                 uint32_t * pulRequestId )
{
    az_result xCoreResult;
    AzureIoTResult_t xResult;
    az_json_writer jw;
    az_span xPayload;

    if( ( pxAzureIoTADUClient == NULL ) || ( pxAzureIoTHubClient == NULL ) ||
        ( pucWritablePropertyResponseBuffer == NULL ) || ( ulWritablePropertyResponseBufferSize == 0 ) )
    {
        AZLogError( ( "AzureIoTADUClient_SendResponse failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    xCoreResult = az_json_writer_init( &jw, az_span_create(
                                           pucWritablePropertyResponseBuffer,
                                           ( int32_t ) ulWritablePropertyResponseBufferSize ), NULL );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "az_json_writer_init failed: 0x%08x", xCoreResult ) );
        return AzureIoT_TranslateCoreError( xCoreResult );
    }

    xCoreResult = az_iot_adu_client_get_service_properties_response(
        &pxAzureIoTADUClient->_internal.xADUClient,
        ( int32_t ) ulPropertyVersion,
        xRequestDecision == eAzureIoTADURequestDecisionAccept ? azureiotaduREQUEST_ACCEPTED_CODE : azureiotaduREQUEST_REJECTED_CODE,
        &jw );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "az_iot_adu_client_get_service_properties_response failed: 0x%08x (%d)", xCoreResult, ulWritablePropertyResponseBufferSize ) );
        return AzureIoT_TranslateCoreError( xCoreResult );
    }

    xPayload = az_json_writer_get_bytes_used_in_destination( &jw );

    xResult = AzureIoTHubClient_SendPropertiesReported(
        pxAzureIoTHubClient,
        az_span_ptr( xPayload ),
        ( uint32_t ) az_span_size( xPayload ),
        pulRequestId );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "[ADU] Failed sending ADU writable properties response: 0x%08x", xResult ) );
        return eAzureIoTErrorPublishFailed;
    }

    return eAzureIoTSuccess;
}

static void prvFillBaseAduDeviceProperties( AzureIoTADUClientDeviceProperties_t * pxDeviceProperties,
                                            az_iot_adu_client_device_properties * pxBaseAduDeviceProperties )
{
    pxBaseAduDeviceProperties->manufacturer = az_span_create(
        ( uint8_t * ) pxDeviceProperties->ucManufacturer,
        ( int32_t ) pxDeviceProperties->ulManufacturerLength );
    pxBaseAduDeviceProperties->model = az_span_create(
        ( uint8_t * ) pxDeviceProperties->ucModel,
        ( int32_t ) pxDeviceProperties->ulModelLength );

    if( pxDeviceProperties->pxCustomProperties == NULL )
    {
        pxBaseAduDeviceProperties->custom_properties = NULL;
    }
    else
    {
        pxDeviceProperties->pxCustomProperties->_internal.xCustomProperties.count =
            ( int32_t ) pxDeviceProperties->pxCustomProperties->ulPropertyCount;

        for( int32_t lPropertyIndex = 0;
             lPropertyIndex < ( int32_t ) pxDeviceProperties->pxCustomProperties->ulPropertyCount;
             lPropertyIndex++ )
        {
            pxDeviceProperties->pxCustomProperties->_internal.xCustomProperties.names[ lPropertyIndex ] =
                az_span_create(
                    pxDeviceProperties->pxCustomProperties->pucPropertyNames[ lPropertyIndex ],
                    ( int32_t ) pxDeviceProperties->pxCustomProperties->ulPropertyNamesLengths[ lPropertyIndex ] );
            pxDeviceProperties->pxCustomProperties->_internal.xCustomProperties.values[ lPropertyIndex ] =
                az_span_create(
                    pxDeviceProperties->pxCustomProperties->pucPropertyValues[ lPropertyIndex ],
                    ( int32_t ) pxDeviceProperties->pxCustomProperties->ulPropertyValuesLengths[ lPropertyIndex ] );
        }

        pxBaseAduDeviceProperties->custom_properties =
            &pxDeviceProperties->pxCustomProperties->_internal.xCustomProperties;
    }

    pxBaseAduDeviceProperties->update_id.name = az_span_create(
        ( uint8_t * ) pxDeviceProperties->xCurrentUpdateId.ucName,
        ( int32_t ) pxDeviceProperties->xCurrentUpdateId.ulNameLength );
    pxBaseAduDeviceProperties->update_id.provider = az_span_create(
        ( uint8_t * ) pxDeviceProperties->xCurrentUpdateId.ucProvider,
        ( int32_t ) pxDeviceProperties->xCurrentUpdateId.ulProviderLength );
    pxBaseAduDeviceProperties->update_id.version = az_span_create(
        ( uint8_t * ) pxDeviceProperties->xCurrentUpdateId.ucVersion,
        ( int32_t ) pxDeviceProperties->xCurrentUpdateId.ulVersionLength );
    pxBaseAduDeviceProperties->adu_version = AZ_SPAN_FROM_STR( AZ_IOT_ADU_CLIENT_AGENT_VERSION );
    pxBaseAduDeviceProperties->delivery_optimization_agent_version = az_span_create(
        ( uint8_t * ) pxDeviceProperties->ucDeliveryOptimizationAgentVersion,
        ( int32_t ) pxDeviceProperties->ulDeliveryOptimizationAgentVersionLength );
}

static void prvFillBaseAduWorkflow( AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                    az_iot_adu_client_workflow * pxBaseWorkflow )
{
    if( pxAduUpdateRequest != NULL )
    {
        pxBaseWorkflow->action = ( int32_t ) pxAduUpdateRequest->xWorkflow.xAction;
        pxBaseWorkflow->id = az_span_create(
            ( uint8_t * ) pxAduUpdateRequest->xWorkflow.pucID,
            ( int32_t ) pxAduUpdateRequest->xWorkflow.ulIDLength );
        pxBaseWorkflow->retry_timestamp = az_span_create(
            ( uint8_t * ) pxAduUpdateRequest->xWorkflow.pucRetryTimestamp,
            ( int32_t ) pxAduUpdateRequest->xWorkflow.ulRetryTimestampLength );
    }
}

static void prvFillBaseAduInstallResults( AzureIoTADUClientInstallResult_t * pxUpdateResults,
                                          az_iot_adu_client_install_result * pxBaseInstallResults )
{
    memset( pxBaseInstallResults, 0, sizeof( *pxBaseInstallResults ) );

    if( pxUpdateResults != NULL )
    {
        pxBaseInstallResults->result_code = pxUpdateResults->lResultCode;
        pxBaseInstallResults->extended_result_code = pxUpdateResults->lExtendedResultCode;

        if( ( pxUpdateResults->pucResultDetails != NULL ) &&
            ( pxUpdateResults->ulResultDetailsLength > 0 ) )
        {
            pxBaseInstallResults->result_details = az_span_create(
                ( uint8_t * ) pxUpdateResults->pucResultDetails,
                ( int32_t ) pxUpdateResults->ulResultDetailsLength );
        }
        else
        {
            pxBaseInstallResults->result_details = AZ_SPAN_EMPTY;
        }

        pxBaseInstallResults->step_results_count = ( int32_t ) pxUpdateResults->ulStepResultsCount;

        for( int lIndex = 0; lIndex < ( int32_t ) pxUpdateResults->ulStepResultsCount; lIndex++ )
        {
            pxBaseInstallResults->step_results[ lIndex ].result_code =
                ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulResultCode;
            pxBaseInstallResults->step_results[ lIndex ].extended_result_code =
                ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulExtendedResultCode;

            if( ( pxUpdateResults->pxStepResults[ lIndex ].pucResultDetails != NULL ) &&
                ( pxUpdateResults->pxStepResults[ lIndex ].ulResultDetailsLength > 0 ) )
            {
                pxBaseInstallResults->step_results[ lIndex ].result_details = az_span_create(
                    ( uint8_t * ) pxUpdateResults->pxStepResults[ lIndex ].pucResultDetails,
                    ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulResultDetailsLength
                    );
            }
            else
            {
                pxBaseInstallResults->step_results[ lIndex ].result_details = AZ_SPAN_EMPTY;
            }
        }
    }
}

AzureIoTResult_t AzureIoTADUClient_SendAgentState( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                   AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   AzureIoTADUClientDeviceProperties_t * pxDeviceProperties,
                                                   AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                   AzureIoTADUAgentState_t xAgentState,
                                                   AzureIoTADUClientInstallResult_t * pxUpdateResults,
                                                   uint8_t * pucBuffer,
                                                   uint32_t ulBufferSize,
                                                   uint32_t * pulRequestId )
{
    az_result xAzResult;
    az_iot_adu_client_device_properties xBaseADUDeviceProperties;
    az_iot_adu_client_workflow xBaseWorkflow;
    az_iot_adu_client_install_result xInstallResult;
    az_json_writer jw;
    az_span xPropertiesPayload;

    if( ( pxAzureIoTADUClient == NULL ) || ( pxAzureIoTHubClient == NULL ) ||
        ( pxDeviceProperties == NULL ) || ( pucBuffer == NULL ) || ( ulBufferSize == 0 ) )
    {
        AZLogError( ( "AzureIoTADUClient_SendAgentState failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    prvFillBaseAduDeviceProperties( pxDeviceProperties, &xBaseADUDeviceProperties );
    prvFillBaseAduWorkflow( pxAduUpdateRequest, &xBaseWorkflow );
    prvFillBaseAduInstallResults( pxUpdateResults, &xInstallResult );

    xAzResult = az_json_writer_init( &jw, az_span_create( pucBuffer, ( int32_t ) ulBufferSize ), NULL );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_json_writer_init failed: 0x%08x", xAzResult ) );
        return AzureIoT_TranslateCoreError( xAzResult );
    }

    xAzResult = az_iot_adu_client_get_agent_state_payload(
        &pxAzureIoTADUClient->_internal.xADUClient,
        &xBaseADUDeviceProperties,
        ( int32_t ) xAgentState,
        pxAduUpdateRequest != NULL ? &xBaseWorkflow : NULL,
        pxUpdateResults != NULL ? &xInstallResult : NULL,
        &jw );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_iot_adu_client_get_agent_state_payload failed: 0x%08x", xAzResult ) );
        return AzureIoT_TranslateCoreError( xAzResult );
    }

    xPropertiesPayload = az_json_writer_get_bytes_used_in_destination( &jw );

    if( AzureIoTHubClient_SendPropertiesReported(
            pxAzureIoTHubClient,
            az_span_ptr( xPropertiesPayload ),
            ( uint32_t ) az_span_size( xPropertiesPayload ),
            pulRequestId ) != eAzureIoTSuccess )
    {
        AZLogError( ( "[ADU] Failed sending ADU agent state." ) );
        return eAzureIoTErrorPublishFailed;
    }

    return eAzureIoTSuccess;
}
