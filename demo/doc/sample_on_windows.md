# Sample on Windows

## Running the Sample With Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), and update the `democonfigENDPOINT`, `democonfigID_SCOPE`, `democonfigREGISTRATION_ID `. 
1. For authentication you can use `democonfigDEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.

## Running the Sample Without Device Provisioning

1. Open the VS solution [here](./demo/sample_azure_iot_embedded_sdk).
1. Open the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h), comment out `democonfigENABLE_DPS_SAMPLE` and update the `democonfigDEVICE_ID`, `democonfigHOSTNAME`. 
1. For authentication you can use `democonfigDEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM`, and `democonfigCLIENT_PRIVATE_KEY_PEM`. If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.
1. Build the solution and run.
1. If the demo fails due to an unchosen network configuration, choose the appropriate one from the list in the terminal and change the value [here](https://github.com/hihigupt/azure_freertos_middleware/blob/e3bcba92e15d47f7f184ef3648782bf84bb84c7b/demo/sample_azure_iot_embedded_sdk/FreeRTOSConfig.h#L138).

## Generating a Cert

If you need a working x509 certificate to get the samples working please see the following:

```bash
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 30 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=azure-freertos-device"
openssl x509 -noout -text -in device_ec_cert.pem

rm -f device_cert_store.pem
cat device_ec_cert.pem device_ec_key.pem > device_cert_store.pem

openssl x509 -noout -fingerprint -in device_ec_cert.pem | sed 's/://g'| sed 's/\(SHA1 Fingerprint=\)//g' | tee fingerprint.txt
```
