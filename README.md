# Azure FreeRTOS Middleware

![Windows Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/MSBuild/badge.svg)

![Linux Build](https://github.com/Azure/azure-iot-middleware-freertos/workflows/C/C++%20CI/badge.svg)

The Azure FreeRTOS Middleware simplifies the connection of devices running FreeRTOS to Azure IoT services. It builds on top of the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c) and adds MQTT client support. Below are key points of this project:

- The Azure FreeRTOS Middleware operates at the MQTT level. Establishing the MQTT connection, subscribing and unsubscribing from topics, sending and receiving of messages, and disconnections are issued by the customer and handled by the SDK.
- Customers control the TLS/TCP connection to the endpoint. This allows for flexibility between software or hardware implementations of either. For porting, please see the [porting](#porting) section below.
- No background threads are created by the Azure FreeRTOS Middleware. Messages are sent and received synchronously.
- Retries with backoff are handled by the customer. FreeRTOS makes use of their own backoff and retry logic which customers are free to use (we demo this in our samples).

## Table of Contents
- [Azure FreeRTOS Middleware](#azure-freertos-middleware)
  - [Table of Contents](#table-of-contents)
  - [Cloning](#cloning)
  - [Repository Structure](#repository-structure)
  - [Current Feature Matrix](#current-feature-matrix)
  - [Porting](#porting)
  - [Samples](#samples)
  - [Known Shortcomings](#known-shortcomings)
  - [APIs May Need to be Added](#apis-may-need-to-be-added)

More docs can be found in the [doc/](doc/) directory.

## Cloning

This repository uses submodules for dependencies. To clone these, you can do one of the following:

To clone this repo and its dependencies in one line, you can use:

```bash
git clone https://github.com/Azure/azure-iot-middleware-freertos.git --recursive
```

If you already have the repository cloned but not the submodules, you can initialize them with the following (in the top level of the repo):

```bash
git submodule update --init --recursive
```

## Repository Structure

The source code for the middleware is located in `source/`.

The following are submoduled in `libraries/` which the middleware takes dependency on:

- `azure-sdk-for-c`
- `coreMQTT`
- `FreeRTOS`

## Current Feature Matrix

| Feature                       | Availability       |
| :---------------------------: | :----------------: |
| Azure IoT Hub                 | :heavy_check_mark: |
| Azure IoT Device Provisioning | :heavy_check_mark: |
| Azure IoT Plug and Play       | Future Release     |

## Porting

To work with the Azure FreeRTOS Middleware, the SDK requires the customer to supply a `send` and `recv` function to the MQTT layer. In depth documentation can be found in the header file [here](source/interface/azure_iot_transport_interface.h).

## Samples

We currently have samples for the following devices. Please click the relevant platform for detailed instructions on getting it working.

- [Linux](./demo/doc/sample_on_linux.md)
- [Windows](./demo/doc/sample_on_windows.md)
- [STML475 Discovery Board](./demo/doc/sample_on_stml475.md)

## Known Shortcomings

- The method response is sent from the method callback which isn't ideal from a threading perspective.

## APIs May Need to be Added

- PnP APIs
  - And with that the APIs for JSON reading and writing (if we are to abstract those or expose the raw JSON APIs)