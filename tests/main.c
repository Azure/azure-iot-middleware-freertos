/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

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
