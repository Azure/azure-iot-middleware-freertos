# Azure FreeRTOS Middleware

**THIS REPO IS IN ACTIVE DEVELOPMENT AND NOT GA READY. APIS ARE SUBJECT TO CHANGE WITHOUT NOTICE UNTIL AN OFFICIAL RELEASE HAS BEEN PUBLISHED.**

The Azure FreeRTOS Middleware simplifies the connection of devices running FreeRTOS to Azure IoT services. It builds on top of the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c) and adds MQTT client support. Below are key points of this project:

- The Azure FreeRTOS Middleware operates at the MQTT level. Establishing the MQTT connection, subscribing and unsubscribing from topics, sending and receiving of messages, and disconnections are issued by the customer and handled by the SDK.
- Customers control the TLS/TCP connection to the endpoint. This allows for flexibility between software or hardware implementations of either. For porting, please see the [porting](#porting) section below.
- No background threads are created by the Azure FreeRTOS Middleware. Messages are sent and received synchronously.
- Retries with backoff are handled by the customer. FreeRTOS makes use of their own backoff and retry logic which customers are free to use (we demo this in our samples).

## Repo Structure

As this repo is under development, the `main` branch is limited until tested and thoroughly reviewed code has been merged. For the most cutting edge developments, please see the `sdk_spike` branch [here](https://github.com/Azure/azure-iot-middleware-freertos/tree/sdk_spike). There we have samples for several devices including:

- STM32L475
- STM32L4+
- STM32H745
- NXP RT1060
- Linux
- Windows

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
