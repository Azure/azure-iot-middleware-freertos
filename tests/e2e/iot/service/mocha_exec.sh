#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# This script install dependencies of service side of e2e tests and runs e2e test

if [[ ! -z ${DEBUG_SHELL} ]]
then
  set -x # Activate the expand mode if DEBUG is anything but empty.
fi

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

die() {
    echo "${1:-"Unknown Error"}" 1>&2
    exit 1
}

[ -v IOTHUB_CONNECTION_STRING ] || die "IOTHUB_CONNECTION_STRING is not set!"
[ -v DEVICE_TEST_EXE ] || die "DEVICE_TEST_EXE is not set!. Please set it to device side binary of e2e tests"

echo -e "Installing node dependencies using npm"

npm install

echo -e "Done installing node dependencies"

echo -e "Compiling typescripts to js"
npm run build
echo -e "Done compiling typescripts to js"

echo -e "Cleaning expired e2e resources"
IOTHUB_CONNECTION_STRING=$IOTHUB_CONNECTION_STRING npm run cleanup
echo -e "Done Cleaning expired e2e resources"

# Github Ubuntu 20.04 runners have issues running `npm run` inside an `npm run` call
# so we don't use `npm run alltest` here. Call each one individually.
echo -e "Running tests"
IOTHUB_CONNECTION_STRING=$IOTHUB_CONNECTION_STRING DEVICE_TEST_EXE=$DEVICE_TEST_EXE \
IOT_PROVISIONING_CONNECTION_STRING=${IOT_PROVISIONING_CONNECTION_STRING:-""} \
IOT_PROVISIONING_SCOPE_ID=${IOT_PROVISIONING_SCOPE_ID:-""} \
npm run iothub_e2e_test
npm run iothub_e2e_test_using_dm
npm run iot_provisioning_e2e_test
npm run iot_provisioning_e2e_test_using_dm
