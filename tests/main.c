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
#include <stdio.h>

#include "FreeRTOSConfig.h"

extern int get_all_tests();

void vLoggingPrintf( const char * pcFormatString, ... )
{
    va_list arg;

    va_start( arg, pcFormatString );
    vprintf( pcFormatString, arg );
    va_end( arg );
}

int main( int argc, char ** argv )
{
    ( void ) argc;
    ( void ) argv;

    return get_all_tests();
}
