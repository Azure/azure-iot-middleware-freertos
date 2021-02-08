#!/bin/bash

set -x # Set trace on
set -o errexit # Exit if command failed
set -o nounset # Exit if variable not set
set -o pipefail # Exit if pipe failed

uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/include/azure_iot.h
uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/include/azure_iot_hub_client.h
uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/include/azure_iot_provisioning_client.h
uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/azure_iot.c
uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/azure_iot_hub_client.c
uncrustify -c ./libraries/FreeRTOS/tools/uncrustify.cfg --no-backup --replace ./source/azure_iot_provisioning_client.c
