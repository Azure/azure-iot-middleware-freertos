# Azure FreeRTOS Middleware

![Windows Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/MSBuild/badge.svg)

![Linux Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/C/C++%20CI/badge.svg)

More docs can be found in the [doc/](doc/) directory.

## Running Sample on Windows

### Running the Sample With Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), and update the `ENDPOINT`, `ID_SCOPE`, `REGISTRATION_ID `. 
1. For authentication you can use `DEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.

### Running the Sample Without Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), comment out `ENABLE_DPS_SAMPLE` and update the `DEVICE_ID`, `HOSTNAME`. 
1. For authentication you can use `DEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.
1. If the demo fails due to an unchosen network configuration, choose the appropriate one from the list in the terminal and change the value [here](https://github.com/hihigupt/azure_freertos_middleware/blob/e3bcba92e15d47f7f184ef3648782bf84bb84c7b/demo/sample_azure_iot_embedded_sdk/FreeRTOSConfig.h#L138).

## Running Sample on STM32L475 Discovery Board

Please see the doc [here](./demo/sample_azure_iot_embedded_sdk/stm32l475/README.md) to run on this board.

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
- The union [here](https://github.com/hihigupt/azure_freertos_middleware/blob/fd69f8a99289428327adefde33c09b995b19ccb1/source/include/azure_iot_hub_client.h#L51-L56) could be improved especially as it breaks encapsulation of the middleware layer.

## APIs May Need to be Added

- PnP APIs
  - And with that the APIs for JSON reading and writing (if we are to abstract those or expose the raw JSON APIs)
- Feature disable functions

- ## Contributing

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
