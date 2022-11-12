#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

echo -e "Using FreeRTOS in libraries/FreeRTOS (`git name-rev --name-only HEAD`)"
TEST_FREERTOS_SRC=`pwd`/libraries/FreeRTOS

# Turn off exit on error briefly since we know the docs fail for FreeRTOS
set +e
echo -e "::group::Checking doxygen documentation matches sources"
cmake -Bbuild -Dcheck_docs=ON -DCMAKE_C_COMPILER=clang -DFREERTOS_DIRECTORY=$TEST_FREERTOS_SRC -DFREERTOS_PORT_DIRECTORY=$TEST_FREERTOS_SRC/FreeRTOS/Source/portable/ThirdParty/GCC/Posix -DCONFIG_DIRECTORY=.github/config .
cmake --build build > /dev/null 2> build.log
cat build.log | grep -A 3 -E 'azure_.*\.h'

# If grep "failed" then it means it didn't find any problems. Exit if found something.
if [ $? -eq 0 ]; then
  exit 1
fi
set -e

rm -rf build/

echo -e "::group::Build using clang"
cmake -Bbuild -DCMAKE_C_COMPILER=clang -DFREERTOS_DIRECTORY=$TEST_FREERTOS_SRC -DFREERTOS_PORT_DIRECTORY=$TEST_FREERTOS_SRC/FreeRTOS/Source/portable/ThirdParty/GCC/Posix -DCONFIG_DIRECTORY=.github/config -DUSE_COREHTTP=ON .
cmake --build build

rm -rf build/
