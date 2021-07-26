#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

echo -e "::group::FreeRTOS Source"

if [ ! -d "libraries/FreeRTOS" ]; then
    git clone https://github.com/FreeRTOS/FreeRTOS.git libraries/FreeRTOS
    pushd libraries/FreeRTOS
    git checkout c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
    git submodule sync
    git submodule update --init --recursive --depth=1
    popd
fi
