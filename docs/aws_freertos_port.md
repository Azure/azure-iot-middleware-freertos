# AWS FreeRTOS Port

## The Porting Process

Porting from AWS FreeRTOS applications can be done with the following steps. There are several scenarios to cover here.

If starting from a basic CoreMQTT implementation, such as [here](https://github.com/aws/amazon-freertos/blob/main/demos/coreMQTT/mqtt_demo_mutual_auth.c), the porting process is pretty straight forward. The network context of the Azure IoT middleware for FreeRTOS maps similarly to the usage of the CoreMQTT library as shown [here](https://github.com/aws/amazon-freertos/blob/3af6570347e4777d43653701bcea6ea8723a63af/demos/coreMQTT/mqtt_demo_mutual_auth.c#L795-L797).

## Authentication

There are a few options when it comes to authentication with Azure IoT. In addition to the x509 certificates offered on AWS Freertos, you may additionally use SAS keys to authenticate with IoT Hub and the Device Provisioning service. For details on which to use, you can refer to [this document here going over the pros and cons of each](https://azure.microsoft.com/blog/iot-device-authentication-options/).

### x509 Certificates

If using x509 authentication, the integration process happens at the networking layer of the project (outside the scope of the SDK). For an example on how to add them in our samples, please see [this code snippet here](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/98234388ec445de4fa482de54b468fee23d6a1f7/demos/sample_azure_iot/sample_azure_iot.c#L278-L281) which uses a `NetworkContext_t` struct to pass to the chosen TLS stack layer ([in this case mbedTLS](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/98234388ec445de4fa482de54b468fee23d6a1f7/demos/common/transport/transport_tls_socket_using_mbedtls.c#L345-L395)). The equivalent spot to add x509 credentials in AWS FreeRTOS would be [here](https://github.com/aws/amazon-freertos/blob/3af6570347e4777d43653701bcea6ea8723a63af/demos/network_manager/aws_iot_network_manager.c#L887-L890).

### SAS Keys

Using SAS keys provides another option for your IoT solution. While we recommend x509 certificates over the use of SAS keys, SAS keys can provide their own benefits especially in prototyping. To use SAS keys, you only need to provide the symmetric key via the [`AzureIoTHubClient_SetSymmetricKey()` API listed here](https://github.com/Azure/azure-iot-middleware-freertos/blob/c01460aef798d37a2f5ccf35f1f9274d34bf3d2b/source/include/azure_iot_hub_client.h#L324-L327).
