#!/bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# This script install dependencies of service side of e2e tests and runs e2e test

# ./run.sh <RTOS connectivity interface>
# e.g. ./run.sh veth1

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
    case "$1" in /*)printf "%s\n" "$1";; *)printf "%s\n" "$PWD/$1";; esac;
}

[ -n $1 ] || die "ethernet interface need to be passed; like veth1"

filepath=`abspath $0`
dir=`dirname $filepath`

echo -e "Updating FreeRTOSConfig.h to point to interface $1"
index=`tcpdump --list-interfaces | grep -Ei "([0-9]+).$1" | sed -E 's/^([0-9]+).*/\1/g'`
sed -i "s/#define configNETWORK_INTERFACE_TO_USE.*/#define configNETWORK_INTERFACE_TO_USE ($index)/g" $dir/device/FreeRTOSConfig.h

echo -e "Building Device code"
cd $dir/device; make

echo -e "Running tests"
export DEVICE_TEST_EXE="$dir/device/build/e2e_device_exe"
cd $dir/service; ./mocha_exec.sh alltest
