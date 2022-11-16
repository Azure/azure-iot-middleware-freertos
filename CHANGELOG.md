# Release History

## 1.2.0-beta.1 (Unreleased)

## 1.1.0 (2022-11-15)

### Features Added

- [[#281]](https://github.com/Azure/azure-iot-middleware-freertos/pull/281) Add Azure Device Update for IoT Hub, enabling Over-the-Air (OTA) updates for embedded devices.

### Other Changes and Improvements

- [[#220]](https://github.com/Azure/azure-iot-middleware-freertos/pull/220) DPS state machine fixes.

## 1.0.0 (2021-09-28)

### Breaking Changes

- [[#201]](https://github.com/Azure/azure-iot-middleware-freertos/pull/201) Move message APIs from `azure_iot.h` to new `azure_iot_message.h`. Rename API prefix from `AzureIoT_Message` to `AzureIoTMessage_`.

### Other Changes and Improvements

- [[#203]](https://github.com/Azure/azure-iot-middleware-freertos/pull/203) Bumped embedded submodule version to `1.2.0`.

## 1.0.0-preview.2 (2021-08-27)

### Breaking Changes

- [[#170]](https://github.com/Azure/azure-iot-middleware-freertos/pull/170) Updated name of `AzureIoTHubClient_GetProperties()` API to `AzureIoTHubClient_RequestPropertiesAsync()`.
- [[#170]](https://github.com/Azure/azure-iot-middleware-freertos/pull/170) Updated `AzureIoTHubMessageType_t` enum value name from `eAzureIoTHubPropertiesGetMessage` to `eAzureIoTHubPropertiesRequestedMessage`.
- [[#171]](https://github.com/Azure/azure-iot-middleware-freertos/pull/171) Moved `AzureIoT_Base64HMACCalculate()` from `azure_iot.h` to `azure_iot_private.h`.

### Bug Fixes

- [[#166]](https://github.com/Azure/azure-iot-middleware-freertos/pull/166) Fix reported property publish topic creation with correct embedded API.

### Other Changes and Improvements

- [[#163]](https://github.com/Azure/azure-iot-middleware-freertos/pull/163) Bumped embedded submodule version to `1.2.0-beta.1`.
- [[#167]](https://github.com/Azure/azure-iot-middleware-freertos/pull/167) Embedded return values are now translated to middleware return values to improve debugability.

## 1.0.0-preview.1 (2021-07-13)

This release marks the public preview availability for the Azure IoT middleware for FreeRTOS.

- For API documentation, please see the published documentation [here](https://azure.github.io/azure-iot-middleware-freertos/).
- To get started and view samples, view the README on the [IoT middleware for FreeRTOS Samples](https://github.com/Azure-Samples/iot-middleware-freertos-samples) repo.
