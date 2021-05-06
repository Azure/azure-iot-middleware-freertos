#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

uncrustify -c ../../uncrustify.cfg --check       \
../../source/*.c                                 \
../../source/include/*.h                         \
../../source/interface/*.h                       \
../../ports/coreMQTT/*.c                         \
../../ports/coreMQTT/*.h                         \
../../tests/config_files/*.h                     \
../../tests/e2e/device/*.c                       \
../../tests/e2e/device/*.h                       \
../../tests/ut/*.h                               \
../../tests/ut/*.c                               \
|                                                \
grep "FAIL"
