/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_provisioning_client.h"

static void test_check(void** state)
{
  (void)state;

  printf("test check");
}

int get_all_tests()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test( test_check )
    };

    return cmocka_run_group_tests_name("azure_iot_provisioning_client_ut", tests, NULL, NULL);
}
