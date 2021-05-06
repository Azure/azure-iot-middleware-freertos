#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -o errexit # Exit if command failed.
set -o nounset # Exit if variable not set.
set -o pipefail # Exit if pipe failed.

# Pass anything as a first parameter to fix code style problems
FIX=${1:-0}

# Version 0.67 is the source of truth
if ! [ -x "$(command -v uncrustify)" ]; then
    git clone https://github.com/uncrustify/uncrustify.git
    cd uncrustify
    git checkout uncrustify-0.67
    mkdir build
    cd build
    cmake ..
    cmake --build .
    sudo make install
    cd ../..
fi

if [ $FIX -ne 0 ]; then
    uncrustify -c ./uncrustify.cfg --no-backup --replace \
    ./source/*.c                                              \
    ./source/include/*.h                                      \
    ./source/interface/*.h                                    \
    ./ports/coreMQTT/*.c                                      \
    ./ports/coreMQTT/*.h                                      \
    ./tests/config_files/*.h                                  \
    ./tests/e2e/device/*.c                                    \
    ./tests/e2e/device/*.h                                    \
    ./tests/ut/*.h                                            \
    ./tests/ut/*.c
else
    RESULT=$(uncrustify -c ./uncrustify.cfg --check      \
    ./source/*.c                                              \
    ./source/include/*.h                                      \
    ./source/interface/*.h                                    \
    ./ports/coreMQTT/*.c                                      \
    ./ports/coreMQTT/*.h                                      \
    ./tests/config_files/*.h                                  \
    ./tests/e2e/device/*.c                                    \
    ./tests/e2e/device/*.h                                    \
    ./tests/ut/*.h                                            \
    ./tests/ut/*.c)

    if [ $? -ne 0 ]; then
      echo $RESULT | grep "FAIL"
    fi
fi
