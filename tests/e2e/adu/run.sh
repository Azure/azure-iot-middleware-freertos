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
UpdateNamePrefix="Linux-E2E-Update"
UpdateName=$UpdateNamePrefix-$RANDOM
export e2eADU_UPDATE_NAME=$UpdateName

[ -n "$test_ethernet_if" ] || die "\$1=ethernet interface need to be passed; like rtosveth1"
[ -n "$test_freertos_src_path" ] || die "\$2=FreeRTOS source path not set"

pushd "$dir"

# Create directory for the update artifacts
if [ ! -d "./e2e_build" ]
then
  mkdir e2e_build
fi

echo -e "Updating FreeRTOSConfig.h to point to interface $test_ethernet_if"
index=`tcpdump --list-interfaces | grep -Ei "([0-9]+).$test_ethernet_if" | sed -E 's/^([0-9]+).*/\1/g'`
sed -i "s/#define configNETWORK_INTERFACE_TO_USE.*/#define configNETWORK_INTERFACE_TO_USE ($index)/g" $test_root_dir/config_files/FreeRTOSConfig.h

# Build and create new update image
echo -e "::group::Build update image"
pushd $dir/device; cmake -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel -DFREERTOS_DIRECTORY=$test_freertos_src_path -DUSE_COREHTTP=ON ../ ; cmake --build build
cp ./build/azure_iot_e2e_adu_tests $dir/e2e_build/azure_iot_e2e_adu_tests_v1.1
popd

echo -e "::group::Clone ADU repo for scripts"
# Clone the device update repo for the scripting
if [ ! -d "./e2e_build/iot-hub-device-update" ]
then
  pushd e2e_build
  git clone https://github.com/Azure/iot-hub-device-update
  popd
fi

# Generate the updated executable
pushd service

echo -e "::group::Create, upload, deploy update manifest and payload"
pwsh -File ./create_and_import_manifest.ps1 -AccountEndpoint "iotsdk-c-production-adu.api.adu.microsoft.com" \
-InstanceId "iotsdk-c-production-adu" \
-UpdateVersion "1.1" \
-AzureAdClientId "321e2686-c228-4c8d-ac77-d19674cb62b5" \
-AzureAdTenantId "72f988bf-86f1-41af-91ab-2d7cd011db47" \
-AzureAdApplicationSecret $ADU_AAD_APPLICATION_SECRET \
-AzureSubscriptionId "db2f889d-dd6c-4269-8d41-a38efcdace46" \
-AzureResourceGroupName "azureiot-euap-e2e" \
-AzureStorageAccountName "euapuploadacct" \
-AzureBlobContainerName "adu-e2e" \
-GroupID "linux-e2e-group" \
-UpdateProvider "ADU-E2E-Tests" \
-UpdateNamePrefix $UpdateNamePrefix \
-UpdateName $UpdateName

popd # service

echo -e "::group::Revert application version | build base image"

# Revert the device version to build v1.0
sed -i "s/#define e2eADU_UPDATE_VERSION .*/#define e2eADU_UPDATE_VERSION \"1.0\"/g" $dir/device/e2e_device_commands.c

# Build and create base image
pushd $dir/device; cmake -Bbuild -DCMAKE_BUILD_TYPE=MinSizeRel -DFREERTOS_DIRECTORY=$test_freertos_src_path -DUSE_COREHTTP=ON ../ ; cmake --build build
cp ./build/azure_iot_e2e_adu_tests ./build/azure_iot_e2e_adu_tests_v1.0
popd # $dir/device

echo -e "::group::Running E2E tests"
export DEVICE_TEST_EXE="$dir/device/build/azure_iot_e2e_adu_tests_v1.0"
chown root $DEVICE_TEST_EXE
chmod u+s $DEVICE_TEST_EXE
cd $dir/service; stdbuf -o0 ./mocha_exec.sh
