/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file azure_iot_mqtt_port.h
 *
 */

#ifndef AZURE_IOT_MQTT_PORT_H
#define AZURE_IOT_MQTT_PORT_H

#include <stdio.h>

#include "core_mqtt.h"

typedef struct AzureIoTMqtt {
    struct
    {
        struct MQTTContext context;
    } _internal;
} AzureIoTMqtt_t;

#endif // AZURE_IOT_MQTT_PORT_H