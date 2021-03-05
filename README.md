# Azure FreeRTOS Middleware

![Windows Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/MSBuild/badge.svg)

![Linux Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/C/C++%20CI/badge.svg)

The Azure FreeRTOS Middleware simplifies the connection of devices running FreeRTOS to Azure IoT services. It builds on top of the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c) and adds MQTT . Below are key points of this project:

- The Azure FreeRTOS Middleware operates at the MQTT level. Establishing the MQTT connection, subscribing and unsubscribing from topics, sending and receiving of messages, and disconnections are issued by the customer and handled by the SDK.
- Customers control the TLS/TCP connection to the endpoint. The allows for flexibility between software or hardware implementations of either.
- No background threads are created by the Azure FreeRTOS Middleware. Messages are sent and received synchronously.
- Retries with backoff are handled by the customer. FreeRTOS makes use of their own backoff and retry logic which customers are free to use (we demo this in our samples).


More docs can be found in the [doc/](doc/) directory.

## Current Feature Matrix

| Feature                       | Availability       |
| :---------------------------: | :----------------: |
| Azure IoT Hub                 | :heavy_check_mark: |
| Azure IoT Device Provisioning | :heavy_check_mark: |
| Azure IoT Plug and Play       | Future Release     |

## Running Sample on Windows

### Running the Sample With Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), and update the `democonfigENDPOINT`, `democonfigID_SCOPE`, `democonfigREGISTRATION_ID `. 
1. For authentication you can use `democonfigDEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.

### Running the Sample Without Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), comment out `democonfigENABLE_DPS_SAMPLE` and update the `democonfigDEVICE_ID`, `democonfigHOSTNAME`. 
1. For authentication you can use `democonfigDEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.
1. If the demo fails due to an unchosen network configuration, choose the appropriate one from the list in the terminal and change the value [here](https://github.com/hihigupt/azure_freertos_middleware/blob/e3bcba92e15d47f7f184ef3648782bf84bb84c7b/demo/sample_azure_iot_embedded_sdk/FreeRTOSConfig.h#L138).

## Running Sample on STM32L475 Discovery Board

Please see the doc [here](./demo/sample_azure_iot_embedded_sdk/stm32l475/ReadMe.md) to run on this board.

## High Level

The following are submoduled in `/libraries` which the middleware takes dependency on:

- `azure-sdk-for-c`
- `coreMQTT`
- `FreeRTOS`

The spiked middleware is contained in `/source` with the header file found [here](./source/include/azure_iot_hub_client.h).

## Generating a Cert

Just for reference, if you need to generate a cert, here are the commands to run:

```bash
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 30 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=azure-freertos-device"
openssl x509 -noout -text -in device_ec_cert.pem

rm -f device_cert_store.pem
cat device_ec_cert.pem device_ec_key.pem > device_cert_store.pem

openssl x509 -noout -fingerprint -in device_ec_cert.pem | sed 's/://g'| sed 's/\(SHA1 Fingerprint=\)//g' | tee fingerprint.txt
```

## Known Shortcomings

- The method response is sent from the method callback which isn't ideal from a threading perspective.

## APIs May Need to be Added

- PnP APIs
  - And with that the APIs for JSON reading and writing (if we are to abstract those or expose the raw JSON APIs)