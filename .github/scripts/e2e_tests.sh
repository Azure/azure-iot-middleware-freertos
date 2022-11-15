#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

echo -e "Using FreeRTOS in libraries/FreeRTOS (`git name-rev --name-only HEAD`)"
TEST_FREERTOS_SRC=`pwd`/libraries/FreeRTOS

echo -e "Running IoT Hub and Device Provisioning E2E"
./tests/e2e/iot/run.sh rtosveth1 $TEST_FREERTOS_SRC

echo -e "Running ADU E2E"
./tests/e2e/adu/run.sh rtosveth1 $TEST_FREERTOS_SRC
