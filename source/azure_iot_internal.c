/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "internal/azure_iot_internal.h"

#include "azure_iot_result.h"
#include "azure/az_core.h"

AzureIoTResult_t _AzureIoT_TranslateCoreError(az_result xCoreError)
{
  AzureIoTResult_t xResult = eAzureIoTErrorFailed;

  switch(xCoreError)
  {
    case AZ_OK:
      xResult = eAzureIoTSuccess;
      break;
    case AZ_ERROR_IOT_TOPIC_NO_MATCH:
      xResult = eAzureIoTErrorTopicNoMatch;
      break;
    case AZ_ERROR_IOT_END_OF_PROPERTIES:
      xResult = eAzureIoTErrorEndOfProperties;
      break;
    case AZ_ERROR_ARG:
      xResult = eAzureIoTErrorInvalidArgument;
      break;
    case AZ_ERROR_ITEM_NOT_FOUND:
      xResult = eAzureIoTErrorItemNotFound;
      break;
  }

  return xResult;
}
