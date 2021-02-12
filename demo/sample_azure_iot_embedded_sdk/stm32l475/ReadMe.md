# Run Demo on STM32L475-Discovery board

## Update Device Credentials

In [demo_config.h](../common/demo_config.h), update the following values for either DPS or IoT Hub:

### DPS

- `ENDPOINT`
- `ID_SCOPE`
- `REGISTRATION_ID`
- `DEVICE_SYMMETRIC_KEY` OR `democonfigCLIENT_CERTIFICATE_PEM` and `democonfigCLIENT_PRIVATE_KEY_PEM`

### IoT Hub

- `DEVICE_ID`
- `HOSTNAME`
- `DEVICE_SYMMETRIC_KEY` OR `democonfigCLIENT_CERTIFICATE_PEM` and `democonfigCLIENT_PRIVATE_KEY_PEM`

## Update Device WiFi Credentials

Update the WiFi SSID and password in [main.c](../../Common/stm32l475/main.c)

- `WIFI_SSID`
- `WIFI_PASSWORD`
