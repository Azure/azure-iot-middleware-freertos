/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_hub_client_properties.c
 * @brief Implementation of the Azure IoT Hub Client properties.
 */

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_hub_client.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

#include "azure_iot_hub_client_properties.h"

#include "azure/iot/az_iot_hub_client_properties.h"

AzureIoTHubClientResult_t AzureIoTHubClientProperties_BuilderBeginComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                             AzureIoTJSONWriter_t * pxJSONWriter,
                                                                             const uint8_t * pucComponentName,
                                                                             uint16_t usComponentNameLength )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_span xComponentSpan;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONWriter == NULL ) || ( pucComponentName == NULL ) || ( usComponentNameLength == 0 ) )
    {
        return eAzureIoTHubClientInvalidArgument;
    }

    xComponentSpan = az_span_create( ( uint8_t * ) pucComponentName, usComponentNameLength );

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_builder_begin_component( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                &pxJSONWriter->_internal.xCoreWriter, xComponentSpan ) ) )
    {
        AZLogError( ( "Could not begin component: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTHubClientProperties_BuilderEndComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                           AzureIoTJSONWriter_t * pxJSONWriter )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONWriter == NULL ) )
    {
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_builder_end_component( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                              &pxJSONWriter->_internal.xCoreWriter ) ) )
    {
        AZLogError( ( "Could not end component: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTHubClientProperties_BuilderBeginResponseStatus( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                  AzureIoTJSONWriter_t * pxJSONWriter,
                                                                                  const uint8_t * pucPropertyName,
                                                                                  uint16_t usPropertyNameLength,
                                                                                  int32_t ilAckCode,
                                                                                  int32_t ilAckVersion,
                                                                                  const uint8_t * pucAckDescription,
                                                                                  uint16_t usAckDescriptionLength )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_span xPropertyName;
    az_span xAckDescription;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONWriter == NULL ) || ( pucPropertyName == NULL ) || ( usPropertyNameLength == 0 ) )
    {
        return eAzureIoTHubClientInvalidArgument;
    }

    xPropertyName = az_span_create( ( uint8_t * ) pucPropertyName, usPropertyNameLength );
    xAckDescription = az_span_create( ( uint8_t * ) pucAckDescription, usAckDescriptionLength );

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_builder_begin_response_status( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                      &pxJSONWriter->_internal.xCoreWriter,
                                                                                      xPropertyName,
                                                                                      ilAckCode, ilAckVersion,
                                                                                      xAckDescription ) ) )
    {
        AZLogError( ( "Could not begin response: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTHubClientProperties_BuilderEndResponseStatus( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                AzureIoTJSONWriter_t * pxJSONWriter )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONWriter == NULL ) )
    {
        return eAzureIoTHubClientInvalidArgument;
    }

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_builder_end_response_status( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                    &pxJSONWriter->_internal.xCoreWriter ) ) )
    {
        AZLogError( ( "Could not end response: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTHubClientProperties_GetPropertiesVersion( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                            AzureIoTJSONReader_t * pxJSONReader,
                                                                            AzureIoTHubMessageType_t xResponseType,
                                                                            uint32_t * pulVersion )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_iot_hub_client_properties_response_type xCoreResponseType;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONReader == NULL ) ||
        ( ( xResponseType != eAzureIoTHubPropertiesGetMessage ) && ( xResponseType != eAzureIoTHubPropertiesWriteablePropertyMessage ) ) ||
        ( pulVersion == NULL ) )
    {
        AZLogError( ( "Invalid parameter: xResponseType must be eAzureIoTHubPropertiesGetMessage or eAzureIoTHubPropertiesWriteablePropertyMessage" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    xCoreResponseType = xResponseType == eAzureIoTHubPropertiesGetMessage ?
                        AZ_IOT_HUB_CLIENT_PROPERTIES_RESPONSE_TYPE_GET : AZ_IOT_HUB_CLIENT_PROPERTIES_RESPONSE_TYPE_DESIRED_PROPERTIES;

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_get_properties_version( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                               &pxJSONReader->_internal.xCoreReader, xCoreResponseType, ( int32_t * ) pulVersion ) ) )
    {
        AZLogError( ( "Could not get property version: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}

AzureIoTHubClientResult_t AzureIoTHubClientProperties_GetNextComponentProperty( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                                                AzureIoTJSONReader_t * pxJSONReader,
                                                                                AzureIoTHubMessageType_t xResponseType,
                                                                                AzureIoTHubClientPropertyType_t xPropertyType,
                                                                                const uint8_t ** ppucComponentName,
                                                                                uint32_t * pusComponentNameLength )
{
    AzureIoTHubClientResult_t xResult;
    az_result xCoreResult;
    az_span xComponentSpan;
    az_iot_hub_client_properties_response_type xCoreResponseType;

    if( ( pxAzureIoTHubClient == NULL ) || ( pxJSONReader == NULL ) ||
        ( ( xResponseType != eAzureIoTHubPropertiesGetMessage ) && ( xResponseType != eAzureIoTHubPropertiesWriteablePropertyMessage ) ) ||
        ( ppucComponentName == NULL ) || ( pusComponentNameLength == NULL ) )
    {
        AZLogError( ( "Invalid parameter: xResponseType must be eAzureIoTHubPropertiesGetMessage or eAzureIoTHubPropertiesWriteablePropertyMessage" ) );
        return eAzureIoTHubClientInvalidArgument;
    }

    xCoreResponseType = xResponseType == eAzureIoTHubPropertiesGetMessage ?
                        AZ_IOT_HUB_CLIENT_PROPERTIES_RESPONSE_TYPE_GET : AZ_IOT_HUB_CLIENT_PROPERTIES_RESPONSE_TYPE_DESIRED_PROPERTIES;

    if( az_result_failed(
            xCoreResult = az_iot_hub_client_properties_get_next_component_property( &pxAzureIoTHubClient->_internal.xAzureIoTHubClientCore,
                                                                                    &pxJSONReader->_internal.xCoreReader,
                                                                                    xCoreResponseType,
                                                                                    ( az_iot_hub_client_property_type ) xPropertyType,
                                                                                    &xComponentSpan ) ) )
    {
        AZLogError( ( "Could not get next component property: core error=0x%08x", xCoreResult ) );
        xResult = eAzureIoTHubClientFailed;
    }
    else
    {
        *ppucComponentName = az_span_ptr( xComponentSpan );
        *pusComponentNameLength = ( uint16_t ) az_span_size( xComponentSpan );
        xResult = eAzureIoTHubClientSuccess;
    }

    return xResult;
}
