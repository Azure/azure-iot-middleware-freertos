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

[ -v ADU_IOTHUB_CONNECTION_STRING ] || die "ADU_IOTHUB_CONNECTION_STRING is not set!"
[ -v DEVICE_TEST_EXE ] || die "DEVICE_TEST_EXE is not set!. Please set it to device side binary of e2e tests"

echo -e "Installing node dependencies using npm"

npm install

echo -e "Done installing node dependencies"

echo -e "Compiling typescripts to js"
npm run build
echo -e "Done compiling typescripts to js"

echo -e "Cleaning expired e2e resources"
ADU_IOTHUB_CONNECTION_STRING=$ADU_IOTHUB_CONNECTION_STRING npm run cleanup
echo -e "Done Cleaning expired e2e resources"

echo -e "Running tests"
ADU_IOTHUB_CONNECTION_STRING=$ADU_IOTHUB_CONNECTION_STRING \
DEVICE_TEST_EXE=$DEVICE_TEST_EXE \
npm run adu
