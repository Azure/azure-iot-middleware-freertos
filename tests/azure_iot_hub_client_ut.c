/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_hub_client.h"

static void test_check(void** state)
{
  (void)state;

  printf("test_check\r\n");
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test( test_check )
    };

    return cmocka_run_group_tests_name("azure_iot_hub_client_ut", tests, NULL, NULL);
}
