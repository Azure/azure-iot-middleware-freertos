#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo dpkg --add-architecture i386
sudo apt update
sudo apt install -y gcc-multilib g++-multilib ninja-build gcovr libpcap-dev:i386 ethtool isc-dhcp-server libcmocka-dev:i386 unifdef gcc-arm-none-eabi dos2unix
sudo apt install -y gcc-multilib g++-multilib ninja-build libcmocka-dev
