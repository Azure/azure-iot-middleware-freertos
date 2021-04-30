#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_RUN_E2E_TESTS=${1:-1}
TEST_CORES=${2:-2}
TEST_JOB_COUNT=${3:-2}

echo -e "::group::FreeRTOS Source"

if [ ! -d "libraries/FreeRTOS" ]; then
    git clone https://github.com/FreeRTOS/FreeRTOS.git libraries/FreeRTOS
    pushd libraries/FreeRTOS
    git checkout -b c8fa483b68c6c1149c2a7a8bc1e901b38860ec9b
    git submodule sync
    git submodule update --init --recursive --depth=1
    popd
fi

echo -e "Using FreeRTOS in libraries/FreeRTOS (`git name-rev --name-only HEAD`)"
TEST_FREERTOS_SRC=`pwd`/libraries/FreeRTOS

echo -e "::group::Building unit tests"
cmake -Bbuild -Dfreertos_repo_SOURCE_DIR=$TEST_FREERTOS_SRC ./tests/ut
cmake --build build -- --jobs=$TEST_CORES
pushd build

echo -e "::group::Running unit tests"
ctest -j $TEST_JOB_COUNT -C "debug" --output-on-failure --schedule-random -T test

echo -e "::group::Code coverage"
gcovr -r $(pwd) -f ../source/.*.c

popd

if [ $TEST_RUN_E2E_TESTS -ne 0 ]; then
    ./tests/e2e/run.sh veth1 $TEST_FREERTOS_SRC
else
    echo -e "Skipping E2E tests"
fi
