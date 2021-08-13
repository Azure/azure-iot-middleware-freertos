# Release History

## 1.0.0-preview.2 (Unreleased)

### Breaking Changes

- Updated name of `AzureIoTHubClient_GetProperties()` API to `AzureIoTHubClient_RequestPropertiesAsync()`.
- Updated `AzureIoTHubMessageType_t` enum value name from `eAzureIoTHubPropertiesGetMessage` to `eAzureIoTHubPropertiesRequestedMessage`.
- Moved `AzureIoT_Base64HMACCalculate()` from `azure_iot.h` to `azure_iot_private.h`

### Bug Fixes

- [[#166]](https://github.com/Azure/azure-iot-middleware-freertos/pull/166) Fix reported property publish topic creation with correct embedded API.

## 1.0.0-preview.1 (2021-07-13)

This release marks the public preview availability for the Azure IoT middleware for FreeRTOS.

- For API documentation, please see the published documentation [here](https://azure.github.io/azure-iot-middleware-freertos/).
- To get started and view samples, view the README on the [IoT middleware for FreeRTOS Samples](https://github.com/Azure-Samples/iot-middleware-freertos-samples) repo.
