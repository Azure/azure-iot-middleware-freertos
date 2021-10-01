# Porting from Azure IoT C SDK to Middleware

This document gives some guidance on porting applications from the [Azure IoT C SDK](https://github.com/Azure/azure-iot-sdk-c) to this library (the Azure IoT middleware for FreeRTOS). Both SDKs were built with different intentions and concentrations, leading to different application approaches written by users. This document will go over the differences in architectures and what that means to use each SDK.

## Architectures

Below shows the high level differences in architectures between the two SDKs. Notice that all **green** boxes are handled and supported by Microsoft, while all **blue** boxes are handled by the user.

![img](./resources/tall-v1-arch.png) ![img](./resources/tall-mid-arch.png)

From the above pictures, a few key points should be kept in mind.

| Azure IoT C SDK | Azure IoT middleware for FreeRTOS |
| --------------- | --------------------------------- |
| Developed to be a vertically integrated stack (transport, TLS, TCP/IP) and handling Azure IoT features | Developed to handle only only MQTT and Azure IoT features on top of MQTT |
| Developed to support different operating systems | Developed to support one platform/OS (FreeRTOS) |
| Supports three transport layers (MQTT, AMQP, and HTTP) | Supports only MQTT |

