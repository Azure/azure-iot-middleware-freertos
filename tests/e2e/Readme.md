# Azure IoT Middleware E2E Test Framework

## Overview

The files in this directory implement the device SDK portion of E2E testing of the Embedded SDK for Azure IoT Convenience layer. For example, there are tests to make sure that a C2D is going through an IoTHub for real (NOT UT), and are properly received on a simulated device running the device SDK.

This is complicated by the fact that the Embedded SDK does *not* have a IoTHub service SDK.  So a naive approach of `CreateProcess(e2e_device_exe)` and having said process run the service and device SDK at the same time, checking internal state of each, will not work.

The e2e tests for Azure IoT Convenience layer use a different approach, splitting the service and device portions into distinct units of execution.  Service application code, written in TypeScript using the IoTHub service SDK, will act as a test driver to an executable written in C simulating the device.

## How to run the tests
* Note: Currently these tests are only supported to run on Linux. Windows support will follow.

* The service application test is written in TypeScript.  It uses [Mocha](https://mochajs.org/) as its test framework and the IoTHub service sdks, among other dependencies.

* To run the e2e test framework:

  * [Install software](../../.github/scripts/install_software.sh)

  * [Configure network](../../.github/scripts/init_vm_network.sh) **Note: This creates a veth1 interface for RTOS connectivity.**

  * Send enviornment variables and execute [run.sh](./run.sh) script.
  ``` sh
  sudo bash -c "export IOTHUB_CONNECTION_STRING="<iothub_connection_string>";
   export IOT_PROVISIONING_CONNECTION_STRING="<dps_connection_string>";
   export IOT_PROVISIONING_SCOPE_ID="<dps_id_scope>";
   tests/e2e/run.sh veth1"
  ```
  * Note: if an error pops up stating that it was unable to find .js files, the transpile may have failed, check your npm installs.

## Service application and device interaction

How do the Node service application and the C simulated device interact?

* The [service](./service) directory contains code authored in TypeScript. This code simulates a service application using IOTHUB service SDK's interacting with a IOTHUB. The code:

  * Creates a device on the IoThub, using IoTHub API's. This device is prefixed with the name `azure_services_port_e2e_` followed by a random string for uniqueness.

  * Creates a process built from the [device](./device) directory, passing the device_id, module_id and sas_token as the only command line arguments.

  * Waits a few seconds for the test device to connect.

  * Tests the device, sending various commands and checking the state.

  * On test completion, the test will:

    * Send a special command telling the test to gracefully terminate.

    * Delete the device on IoTHub.

    * Attempt to terminate the test's process, assuming there is some failure where it cannot gracefully exit.

* The [device](./device) directory contains a simulated device code that handle test commands.  It is written in C and is the component under test.

  * On test initiation, the device uses the passed in parameter to connect to IoTHub service.

  * The device will wait for C2D messages from IOTHub.

  * Tests are initiated by command arriving as C2D message payload.