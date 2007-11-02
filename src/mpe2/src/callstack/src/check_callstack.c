#include <stdio.h>
#include <unistd.h>
#include "mpe_callstack.h"

#define MAX_RECURSIVE_COUNT  3

static int dummy_count;

void dummy_subroutine( void );
void dummy_subroutine( void )
{
    MPE_CallStack_t  callstack;

    MPE_CallStack_init( &callstack );

    dummy_count++;
    printf( "dummy_count = %d:\n", dummy_count );
#if defined( SIMPLE_PRINT )
    MPE_CallStack_print( &callstack, STDOUT_FILENO );
#elif defined( FANCY_PRINT )
    MPE_CallStack_fancyprint( &callstack, STDOUT_FILENO,
                              "*****", 1, MPE_CALLSTACK_UNLIMITED );
#else
    MPE_CallStack_iteratorInit( &callstack );
    while ( MPE_CallStack_iteratorHasMore( &callstack ) ) {
        printf( "    %s\n", MPE_CallStack_iteratorFetchNext( &callstack) );
    }
#endif

    if ( dummy_count < MAX_RECURSIVE_COUNT )
        dummy_subroutine();
    /* printf( "hello world!\n" ); */
}

int main( int argc, char *argv[] )
{
    dummy_count = 0;
    dummy_subroutine();
    return 0;
}
