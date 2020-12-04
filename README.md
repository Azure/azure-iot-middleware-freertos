# Azure FreeRTOS Middleware

## Running the Sample

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h) and update the `DEVICE_ID`, `HOSTNAME`, `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.
1. If the demo fails due to an unchosen network configuration, choose the appropriate one from the list in the terminal and change the value [here](https://github.com/hihigupt/azure_freertos_middleware/blob/e3bcba92e15d47f7f184ef3648782bf84bb84c7b/demo/sample_azure_iot_embedded_sdk/FreeRTOSConfig.h#L138).

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
