# AWS FreeRTOS Port

## The Porting Process

Porting from AWS FreeRTOS applications can be done with the following steps. There are several scenarios to cover here.

If starting from a basic CoreMQTT implementation, such as [here](https://github.com/aws/amazon-freertos/blob/main/demos/coreMQTT/mqtt_demo_mutual_auth.c), the porting process is pretty straight forward. The network context of the Azure IoT middleware for FreeRTOS maps similarly to the usage of the CoreMQTT library as shown [here](https://github.com/aws/amazon-freertos/blob/3af6570347e4777d43653701bcea6ea8723a63af/demos/coreMQTT/mqtt_demo_mutual_auth.c#L795-L797).

## Authentication

There are a few options when it comes to authentication with Azure IoT. In addition to the x509 certificates offered on AWS Freertos, you may additionally use SAS keys to authenticate with IoT Hub and the Device Provisioning service. For details on which to use, you can refer to [this document here going over the pros and cons of each](https://azure.microsoft.com/blog/iot-device-authentication-options/).

### x509 Certificates

If using x509 authentication, the integration process happens at the networking layer of the project (outside the scope of the SDK). For an example on how to add them in our samples, please see [this code snippet here](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/98234388ec445de4fa482de54b468fee23d6a1f7/demos/sample_azure_iot/sample_azure_iot.c#L278-L281) which uses a `NetworkContext_t` struct to pass to the chosen TLS stack layer ([in this case mbedTLS](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/98234388ec445de4fa482de54b468fee23d6a1f7/demos/common/transport/transport_tls_socket_using_mbedtls.c#L345-L395)). The equivalent spot to add x509 credentials in AWS FreeRTOS would be [here](https://github.com/aws/amazon-freertos/blob/3af6570347e4777d43653701bcea6ea8723a63af/demos/network_manager/aws_iot_network_manager.c#L887-L890).

For more details on the TLS requirements of Azure IoT (TLS versions, certificate requirements, supported crypto algorithms, etc), [please see this document here](https://docs.microsoft.com/azure/iot-hub/iot-hub-tls-support). To see how your solution certificate requirements might need to change, please see ([here for the AWS specs.](https://docs.aws.amazon.com/iot/latest/developerguide/transport-security.html))

### SAS Keys

Using SAS keys provides another option for your IoT solution. While we recommend x509 certificates over the use of SAS keys, SAS keys can provide their own benefits especially in prototyping. To use SAS keys, you only need to provide the symmetric key via the [`AzureIoTHubClient_SetSymmetricKey()` API listed here](https://github.com/Azure/azure-iot-middleware-freertos/blob/c01460aef798d37a2f5ccf35f1f9274d34bf3d2b/source/include/azure_iot_hub_client.h#L324-L327) and the necessary HMAC function to use in generating the keys. The SAS key will be automatically refreshed when needed if it expires during the connection.

## Platform Features

Direct comparisons between feature sets of AWS and Azure IoT is not the purpose of this document. Application code logic would need to be ported over to use Azure IoT concepts.

- For device-to-cloud features, please see [here](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-d2c-guidance).
- For cloud-to-device features, please see [here](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-c2d-guidance).

Below is a high level explanation of functional features

### Telemetry

Standard MQTT telemetry capability. Send messages with a payload to a service-determined topic. Custom topics are not available at this time. [Details for this feature can be found here](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-d2c).

### Commands

Otherwise known as direct methods, commands/functions which one can invoke from the cloud and run on the device. These are composed of an intended function to run (name) and a possible payload to pass to the function. Any response payload and return value are then sent back to the service. [Details for this feature can be found here](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-direct-methods).

### Device Twin

JSON documents that store device state information including metadata, configurations, and conditions. Azure IoT Hub maintains a device twin for each device that you connect to IoT Hub. [Details for this feature can be found here](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins).
