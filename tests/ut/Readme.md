# Azure IoT Middleware Unit Test Framework

## Overview

The files in this directory implement the device SDK portion of unit testing of the Azure IoT middleware for FreeRTOS.

## Prerequisites

The Unit Tests are using the [CMocka](https://cmocka.org/) framework.
On Ubuntu, this can be installed by running:

`sudo apt install libcmocka-dev libcmocka0`

## How to run the unit tests
* Note: Currently these tests are only supported to run on Linux.

1. Make sure the middleware repository was cloned and has up-to-date submodules: `git submodule update`.
1. Follow the [Building Guide](../../README.md#building) to set up a FreeRTOS directory outside of this repository making sure the FreeRTOS repository has its submodules up-to-date. You can use [this script](../../.github/scripts/fetch_freertos.sh) to make sure you have the correct version.
1. Configure and build the unit tests:

```bash
cd tests/ut
mkdir build
cd build
cmake -DFREERTOS_DIRECTORY='<path_to_FreeRTOS repo>' ..
cmake --build . -j
ctest
```
