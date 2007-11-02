/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif

#include "clog_const.h"
#include "clog_joiner.h"

int main( int argc, char *argv[] )
{
    CLOG_Joiner_t   *joiner;
    char           **filenames;
    int              num_logfiles;

    if ( argc < 3 ) {
        fprintf( stderr, "usage: %s <logfile1> <logfile2> ...\n", argv[0] );
        exit( -1 );
    }
    num_logfiles = argc - 1;
    filenames    = &argv[1];

    CLOG_Rec_sizes_init();

    joiner = CLOG_Joiner_create( num_logfiles, filenames );
    if ( joiner == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Joiner_create() fails \n" );
        fflush( stderr );
        exit( -1 );
    }

    CLOG_Joiner_init( joiner, "merged.clog2" );
    CLOG_Joiner_sort( joiner );
    CLOG_Joiner_finalize( joiner );

    CLOG_Joiner_free( &joiner );
    return( 0 );
}
