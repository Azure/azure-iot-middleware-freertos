/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>

#include <cmocka.h>
/*-----------------------------------------------------------*/

uint64_t ullLogLineCount = 0;
extern uint32_t ulGetAllTests();
void vLoggingPrintf( const char * pcFormatString,
                     ... );
void vAssertCalled( const char * pcFile,
                    uint32_t ulLine );

/*-----------------------------------------------------------*/

void vAssertCalled( const char * pcFile,
                    uint32_t ulLine )
{
    volatile char * pcFileName = ( volatile char * ) pcFile;
    volatile uint32_t ulLineNumber = ulLine;

    ( void ) pcFileName;
    ( void ) ulLineNumber;

    printf( "vAssertCalled( %s, %u\n", pcFile, ulLine );

    assert_true( 0 );
}

/*-----------------------------------------------------------*/

void vLoggingPrintf( const char * pcFormatString,
                     ... )
{
    va_list arg;

    ullLogLineCount++;
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
/*-----------------------------------------------------------*/
