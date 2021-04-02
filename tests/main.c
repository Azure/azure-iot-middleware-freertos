/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

extern uint32_t ulGetAllTests();
void vLoggingPrintf( const char * pcFormatString, ... );

/*-----------------------------------------------------------*/

void vLoggingPrintf( const char * pcFormatString, ... )
{
    va_list arg;

    va_start( arg, pcFormatString );
    vprintf( pcFormatString, arg );
    va_end( arg );
}
/*-----------------------------------------------------------*/

int main( int argc,
          char ** argv )
{
    ( void ) argc;
    ( void ) argv;

    return ( int ) ulGetAllTests();
}
