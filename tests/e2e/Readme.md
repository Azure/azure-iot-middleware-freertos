# Azure IoT Middleware E2E Test Framework

## Overview

The files in this directory implement the device SDK portion of E2E testing of the Azure IoT middleware for FreeRTOS. For example, there are tests to make sure that a C2D is going through an IoT Hub for real (NOT UT), and are properly received on a simulated device running the device SDK.

This is complicated by the fact that the Embedded SDK does *not* have an IoT Hub service SDK. So a naive approach of `CreateProcess(e2e_device_exe)` and having said process run the service and device SDK at the same time, checking internal state of each, will not work.

The e2e tests for Azure IoT Convenience layer use a different approach, splitting the service and device portions into distinct units of execution. Service application code, written in TypeScript using the IoT Hub service SDK, will act as a test driver to an executable written in C simulating the device.

## How to run the tests

* Note: Currently these tests are only supported to run on Linux. Windows support will follow.

* The service application test is written in TypeScript. It uses [Mocha](https://mochajs.org/) as its test framework and the IoT Hub service sdks, among other dependencies.

* To run the e2e test framework:

  * [Install software](../../.github/scripts/install_software.sh)

  * [Configure network](../../.github/scripts/init_linux_port_vm_network.sh) **Note: This creates a rtosveth1 interface for RTOS connectivity.**

  * [Fetch FreeRTOS](../../.github/scripts/fetch_freertos.sh): You will have to set the full path to this directory in the script later.

  * Send environment variables and execute [run.sh](./iot/run.sh) script.

  ``` sh
  sudo bash -c 'export IOTHUB_CONNECTION_STRING="<iothub_connection_string>";
   export IOT_PROVISIONING_CONNECTION_STRING="<dps_connection_string>";
   export IOT_PROVISIONING_SCOPE_ID="<dps_id_scope>";
   tests/e2e/iot/run.sh rtosveth1 <PATH TO FREERTOS>'
  ```

  * Note: if an error pops up stating that it was unable to find .js files, the transpile may have failed, check your npm installs.

## Service application and device interaction

How do the Node service application and the C simulated device interact?

* The [service](./iot/service) directory contains code authored in TypeScript. This code simulates a service application using IoT Hub service SDK's interacting with a IoT Hub. The code:

  * Creates a device on the IoT Hub, using IoT Hub API's. This device is prefixed with the name `azure_mid_freertos_e2e_` followed by a random string for uniqueness.

  * Creates a process built from the [device](./iot/device) directory, passing the device_id, module_id and sas_token as the only command line arguments.

  * Waits a few seconds for the test device to connect.

  * Tests the device, sending various commands which the device will execute and then return some status.

  * On test completion, the test will:

    * Send a special command telling the test to gracefully terminate.

    * Delete the device on IoT Hub.

    * Attempt to terminate the test's process, assuming there is some failure where it cannot gracefully exit.

* The [device](./iot/device) directory contains code for a simulated device that handles test commands. It is written in C and is the component under test.

  * On test initiation, the device uses the passed in parameter to connect to IoT Hub service.

  * The device will send a specific telemetry message to the service to signal that it is connected and ready to perform the required tests.

  * The device will wait for C2D messages from IoT Hub.

  * Tests are initiated by command arriving as C2D messages.

## In Depth Look

Okay, specifically, how do the tests work?

### Brief Typescript (Javascript) Overview

Off the start, [Typescript](https://www.typescriptlang.org/) (Javascript but with types which gets transpiled to Javascript) is an event driven programming language. Most of what is happening is based on callbacks or async "Promises". You call some function and give it a callback to call once the function is done executing or it has received the result you were waiting for. This is run in the "NodeJS" Javascript engine, which is Google's V8 engine from Chrome. This just means it's an engine which helps manage the execution of the program (gives some network I/O, allows asynchronous programming, etc).

For example, [let's look here at the section which waits for the device to send its "connected" telemetry](./iot/service/common/e2e_test_core.ts#L445-L450). We call `verifyTelemetryMessage()` which returns a "Promise" object ([a promise is essentially encapsulating work into an object and exposing a `resolve()` and `reject()` state once the work is done or error'd](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)). This allows execution of a function to be suspended and frees the Javascript engine to work on other things while that function "awaits" a result. Once the result settles (maybe an network event came in and gave us the PUBLISH we were waiting for), the promise can be `resolve()`'d and the execution of the function can continue.

### Flow of the Test Setup

* [Get the arguments (if applicable)](./iot/service/iothub_client/e2e_iot_hub_client_test.ts#L60-L87)
* [Get the connection string (either from the command line or env var)](./iot/service/iothub_client/e2e_iot_hub_client_test.ts#L90)
* [Call the "before" hook for the Mocha test framework](./iot/service/iothub_client/e2e_iot_hub_client_test.ts#L96). This calls `testSetup` which essentially uses the IoT Hub connection string to create a device and receive the device credentials ([by calling createTestDeviceAndTestProcess](./iot/service/common/e2e_test_core.ts#L426-L453)). Once the credentials have been received, it executes the "test process" (aka the device executable for the middleware), and passes the device info and credentials via command line arguments. Once the device executable has been spawned, the service code will wait for a specific telemetry message which signals that the device has connected and is ready for commands to be received.
* Once the service and device are ready, they both enter their respective testing framework sections ([for device code at ulE2EDeviceProcessCommands](./iot/device/e2e_device_commands.c#L1186) | [for service code at mainTest](./iot/service/iothub_client/e2e_iot_hub_client_test.ts#L116)).
* The service creates a class for a given test, extended from the parent class `CommandTestData` ([link here](./iot/service/common/e2e_test_commands.ts#L26)). Each one states what the operation is the device should execute and optionally what the expected payload is from the device.
* The service invokes the specified test to run (maybe fetching the twin data), the device receives the message, parses it, and runs what was requested. If the service expects a payload as a response, the device will send it and the service will compare to ensure it is correct ([for example, here it checks the twin values](./iot/service/iothub_client/e2e_iot_hub_client_test.ts#L228-233)) with `assert.ok(twinObject.reported.temperature);`.
