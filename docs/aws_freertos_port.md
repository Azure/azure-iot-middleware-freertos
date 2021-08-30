# AWS FreeRTOS Port

## The porting process

Porting from AWS FreeRTOS applications can be done with the following steps. There are several scenarios to cover here.

If starting from a basic CoreMQTT implementation, such as [here](https://github.com/aws/amazon-freertos/blob/main/demos/coreMQTT/mqtt_demo_mutual_auth.c), the porting process is pretty straight forward. The network context of the Azure IoT middleware for FreeRTOS maps directly to the usage of the CoreMQTT library as shown [here](https://github.com/aws/amazon-freertos/blob/3af6570347e4777d43653701bcea6ea8723a63af/demos/coreMQTT/mqtt_demo_mutual_auth.c#L795-L797).

## 