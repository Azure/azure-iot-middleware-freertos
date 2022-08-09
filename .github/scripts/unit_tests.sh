#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_RUN_E2E_TESTS=${1:-1}
TEST_CORES=${2:-2}
TEST_JOB_COUNT=${3:-2}
TEST_RUN_LINE_COVERAGE_THRESHOLD=${4:-80}

echo -e "Using FreeRTOS in libraries/FreeRTOS (`git name-rev --name-only HEAD`)"
TEST_FREERTOS_SRC=`pwd`/libraries/FreeRTOS

echo -e "::group::Building unit tests with Debug"
cmake -Bbuild -DFREERTOS_DIRECTORY=$TEST_FREERTOS_SRC -DMEMORYCHECK_COMMAND_OPTIONS="--error-exitcode=1 --leak-check=full" -DCMAKE_BUILD_TYPE=Debug ./tests/ut
cmake --build build -- --jobs=$TEST_CORES
pushd build

echo -e "::group::Running unit tests"
ctest -j $TEST_JOB_COUNT -C "debug" --output-on-failure --schedule-random -T test

echo -e "::group::Running unit tests with memcheck"
ctest -j $TEST_JOB_COUNT -C "debug" --output-on-failure --schedule-random -T memcheck

echo -e "::group::Code coverage"
gcovr -r $(pwd) -f ../source/.*.c

echo -e "Checking Code coverage for at least ${TEST_RUN_LINE_COVERAGE_THRESHOLD}%"
find ../source/*.c | while read file; \
    do gcovr --fail-under-line ${TEST_RUN_LINE_COVERAGE_THRESHOLD} \
    -r $(pwd) -f $file > /dev/null; done;

popd

echo -e "::group::Building unit tests with Release"
rm -rf build
cmake -Bbuild -DFREERTOS_DIRECTORY=$TEST_FREERTOS_SRC -DCMAKE_BUILD_TYPE=Release ./tests/ut
cmake --build build -- --jobs=$TEST_CORES
