#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_RUN_E2E_TESTS=${1:-1}
TEST_CORES=${2:-2}
TEST_JOB_COUNT=${3:-2}

echo -e "Building unit tests"
cmake -Dbuild_ut_tests=ON . -Bbuild
cmake --build build -- --jobs=$TEST_CORES
cd build

echo -e "Running unit tests"
ctest -j $TEST_JOB_COUNT -C "debug" --output-on-failure --schedule-random -T test

echo -e "Building e2e tests"
rm -rf build/
cmake -Dbuild_e2e_tests=ON . -Bbuild
cmake --build build -- --jobs=$TEST_CORES
cd build

if [ $TEST_RUN_E2E_TESTS -ne 0 ]; then
    echo -e "Run E2E tests"
    ../tests/e2e/run.sh veth1
else
    echo -e "Skipping E2E tests"
fi

echo -e "Code coverage"
gcovr -r $(pwd) -f ../source/.*.c
