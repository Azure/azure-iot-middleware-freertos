#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

TEST_CORES=${1:-2}
TEST_JOB_COUNT=${2:-2} 

echo -e "Building tests"
cmake tests -Bbuild
cmake --build build -- --jobs=$TEST_CORES
cd build

echo -e "Running unit tests"
ctest -j $TEST_JOB_COUNT -C "debug" -VV --output-on-failure --schedule-random -T test

echo -e "Code coverage"
gcovr -r $(pwd) -f ../source/*.c
