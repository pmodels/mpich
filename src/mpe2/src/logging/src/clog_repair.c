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
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "clog_const.h"
#include "clog_record.h"
#include "clog_preamble.h"
#include "clog_commset.h"
#include "clog_cache.h"

int main( int argc, char *argv[] )
{
    CLOG_Cache_t       *cache;
    CLOG_Rec_Header_t  *rec_hdr;
    CLOG_Rec_CommEvt_t *comm_evt;

    if ( argc < 2 ) {
        fprintf( stderr,"usage: %s <logfile>\n", argv[0] );
        exit( -1 );
    }
    CLOG_Rec_sizes_init();

    cache = CLOG_Cache_create();
    if ( cache == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Cache_create() fails!\n" );
        fflush( stderr );
        exit( 1 );
    }
    CLOG_Cache_open4readwrite( cache, argv[1] );
    CLOG_Cache_init4read( cache );
    
    CLOG_Preamble_print( cache->preamble, stdout );

    while ( CLOG_Cache_has_rec( cache ) == CLOG_BOOL_TRUE ) {
        rec_hdr   = CLOG_Cache_get_rec( cache );
        if ( rec_hdr->rectype == CLOG_REC_COMMEVT ) {
            comm_evt  = (CLOG_Rec_CommEvt_t *) rec_hdr->rest;
            CLOG_CommSet_add_GID( cache->commset, comm_evt->gcomm );
        }
    }

    CLOG_CommSet_print( cache->commset, stdout );
    CLOG_Cache_close4readwrite( cache );
    CLOG_Cache_free( &cache );

    return( 0 );
}
