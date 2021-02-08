# Run Demo on STM32L475-Discovery board

1. Update demo_config.h to point to your IoTHub or DPS endpoint.
```
/**
 * @brief Provisioning service endpoint.
 *
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#service-operations-endpoint
 * 
 */
#define ENDPOINT                            "<YOUR DPS ENDPOINT HERE>"

/**
 * @brief Id scope of provisioning service.
 * 
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#id-scope
 * 
 */
#define ID_SCOPE                            "<YOUR ID SCOPE HERE>"

/**
 * @brief Registration Id of provisioning service
 *
 *  @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#registration-id
 */
#define REGISTRATION_ID                     "<YOUR REGISTRATION ID HERE>"

#endif // ENABLE_DPS_SAMPLE

/**
 * @brief IoTHub device Id.
 *
 */
#define DEVICE_ID                           "<YOUR DEVICE ID HERE>"

/**
 * @brief IoTHub module Id.
 *
 */
#define MODULE_ID                           ""
/**
 * @brief IoTHub hostname.
 *
 */
#define HOSTNAME                            "<YOUR IOT HUB HOSTNAME HERE>"

/**
 * @brief Device symmetric key
 *
 */
#define DEVICE_SYMMETRIC_KEY                "<Symmetric key>"

```

2. Compile the code with following macro to set the SSID and password of the AP.
```
-DWIFI_SSID="higupt-dev3"
-DWIFI_PASSWORD="welcome1
```

3. Run the demo.
