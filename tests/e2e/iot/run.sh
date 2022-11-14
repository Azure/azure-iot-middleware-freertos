#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# This script install dependencies of service side of e2e tests and runs e2e test

# ./run.sh <RTOS connectivity interface> [FreeRTOS Src path]
# e.g. ./run.sh rtosveth1

# Trace each command by: export DEBUG_SHELL=1
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

abspath () {
  case "$1" in
  /*)
    printf"%s\n" "$1";;
  *)
    printf "%s\n" "$PWD/$1";;
  esac;
}

filepath=`abspath $0`
dir=`dirname $filepath`
test_root_dir=$dir/../..
test_freertos_src_path=${2:-""}
test_ethernet_if=${1:-""}

[ -n "$test_ethernet_if" ] || die "\$1=ethernet interface need to be passed; like rtosveth1"
[ -n "$test_freertos_src_path" ] || die "\$2=FreeRTOS source path not set"

echo -e "Updating FreeRTOSConfig.h to point to interface $test_ethernet_if"
index=`tcpdump --list-interfaces | grep -Ei "([0-9]+).$test_ethernet_if" | sed -E 's/^([0-9]+).*/\1/g'`
sed -i "s/#define configNETWORK_INTERFACE_TO_USE.*/#define configNETWORK_INTERFACE_TO_USE ($index)/g" $test_root_dir/config_files/FreeRTOSConfig.h

echo -e "::group::Building E2E tests"
cd $dir/device; cmake -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel -DFREERTOS_DIRECTORY=$test_freertos_src_path ../ ; cmake --build build

echo -e "::group::E2E tests usage of libaz_iot_middleware_freertos.a"
grep -E  "0x\w{16}\s+0x\w+ .*libaz_iot_middleware_freertos.a" $dir/device/build/azure_iot_e2e_tests.map
echo -e "::group::E2E tests Map file"
cat $dir/device/build/azure_iot_e2e_tests.map

echo -e "::group::Running E2E tests"
export DEVICE_TEST_EXE="$dir/device/build/azure_iot_e2e_tests"
chown root $DEVICE_TEST_EXE
chmod u+s $DEVICE_TEST_EXE
cd $dir/service; stdbuf -o0 ./mocha_exec.sh
