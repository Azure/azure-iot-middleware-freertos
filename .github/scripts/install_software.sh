#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

sudo dpkg --add-architecture i386
sudo apt update
sudo apt install -y gcc-multilib g++-multilib ninja-build gcovr libpcap-dev:i386 ethtool isc-dhcp-server libcmocka-dev:i386 unifdef gcc-arm-none-eabi dos2unix valgrind npm
sudo apt install -y gcc-multilib g++-multilib ninja-build libcmocka-dev
# az cli for adu
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
az upgrade

# pwsh for adu
# Update the list of packages
sudo apt-get update
# Install pre-requisite packages.
sudo apt-get install -y wget apt-transport-https software-properties-common
# Download the Microsoft repository GPG keys
wget -q "https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb"
# Register the Microsoft repository GPG keys
sudo dpkg -i packages-microsoft-prod.deb
# Update the list of packages after we added packages.microsoft.com
sudo apt-get update
# Install PowerShell
sudo apt-get install -y powershell
