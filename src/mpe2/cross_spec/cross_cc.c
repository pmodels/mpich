#include "cross_conf.h"

#if defined( STD_HEADERS) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

static char* is_runtime_bigendian( void )
{
    union {
        long ll;
        char cc[sizeof(long)];
    } uu;
    uu.ll = 1;
    if ( uu.cc[sizeof(long) - 1] == 1 )
        return "true";
    else
        return "false";
}

int main( int argc, char *argv[] )
{
    FILE     *cross_file;

    cross_file = fopen( CROSS_SPEC_FILE, "a" );
    if ( cross_file == NULL ) {
        fprintf( stderr, "Can't open %s for appending!\n", CROSS_SPEC_FILE );
        return 1;
    }

    fprintf( cross_file, "%s\n", "# C compiler runtime characteristics..." );
    fprintf( cross_file, "CROSS_SIZEOF_CHAR=%d\n", sizeof(char) );
    fprintf( cross_file, "CROSS_SIZEOF_SHORT=%d\n", sizeof(short) );
    fprintf( cross_file, "CROSS_SIZEOF_INT=%d\n", sizeof(int) );
    fprintf( cross_file, "CROSS_SIZEOF_LONG=%d\n", sizeof(long) );
    fprintf( cross_file, "CROSS_SIZEOF_LONG_LONG=%d\n", sizeof(long long) );
    fprintf( cross_file, "CROSS_SIZEOF_VOID_P=%d\n", sizeof(void *) );
    fprintf( cross_file, "CROSS_BIGENDIAN=%s\n", is_runtime_bigendian() );

    fclose( cross_file );
    return 0;
}
