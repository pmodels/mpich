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
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "clog_const.h"
#include "clog_mem.h"
#include "clog_cache.h"
#include "clog_joiner.h"

CLOG_Joiner_t* CLOG_Joiner_create( int num_files, char *filenames[] )
{
    CLOG_Joiner_t   *joiner;
    CLOG_Cache_t   **in_caches, *out_cache;
    int              idx, ii;

    in_caches = (CLOG_Cache_t **) MALLOC( num_files * sizeof(CLOG_Cache_t *) );
    if ( in_caches == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Joiner_create() - \n"
                         "\t""MALLOC(CLOG_Joiner_t's in_caches[]) fails!\n" );
        fflush( stderr );
        return NULL;
    }

    for ( idx = 0; idx < num_files; idx++ ) {
        in_caches[ idx ] = CLOG_Cache_create();
        if ( in_caches[ idx ] == NULL ) {
            fprintf( stderr, __FILE__":CLOG_Joiner_create() - \n"
                             "\t""CLOG_Cache_create(in_caches[%d]) fails!\n",
                             idx );
            fflush( stderr );
            for ( ii = idx-1; idx >= 0; ii-- ) {
                CLOG_Cache_free( &(in_caches[ idx ]) );
            }
            if ( in_caches != NULL )
                FREE( in_caches );
            return NULL;
        }
        CLOG_Cache_open4read( in_caches[ idx ], filenames[ idx ] );
    }

    out_cache = CLOG_Cache_create();
    if ( out_cache == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Joiner_create() - \n"
                         "\t""CLOG_Cache_create(out_cache) fails!\n" );
        fflush( stderr );
        for ( ii = 0; idx < num_files; ii++ ) {
            CLOG_Cache_free( &(in_caches[ idx ]) );
        }
        if ( in_caches != NULL )
            FREE( in_caches );
        return NULL;
    }

    joiner = (CLOG_Joiner_t *) MALLOC( sizeof(CLOG_Joiner_t) );
    if ( joiner == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Joiner_create() - \n"
                         "\t""MALLOC() of CLOG_Joiner_t fails!\n" );
        fflush( stderr );
        return NULL;
    }

    joiner->num_caches          = num_files;
    joiner->in_caches           = in_caches;
    joiner->out_cache           = out_cache;
    joiner->sorted_caches_head  = NULL;
    joiner->sorted_caches_tail  = NULL;

    return joiner;
}

void CLOG_Joiner_free( CLOG_Joiner_t **joiner_handle )
{
    CLOG_Joiner_t  *joiner;
    int             idx;

    joiner = *joiner_handle;
    if ( joiner != NULL ) {
        for ( idx = 0; idx < joiner->num_caches; idx++ ) {
            CLOG_Cache_free( &(joiner->in_caches[idx]) );
        }
        joiner->num_caches = 0;
        CLOG_Cache_free( &(joiner->out_cache) );
        FREE( joiner );
    }
    *joiner_handle = NULL;
}

/*
    Synchronizes all communicator local_IDs of the input/output caches[].
    The function does not require the output cache be initalized except
    memory allocation for the commset of the output cache.
*/
static void CLOG_Joiner_sync_commIDs( CLOG_Joiner_t *joiner )
{
    int  idx;

    /*
       Update the output cache's commset with that of input caches[].
       So that joiner->out_cache->commset contains all the CLOG_Uuid_t(GID)
       in all the joiner->in_caches[]->commset.
    */
    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
        CLOG_CommSet_append_GIDs( joiner->out_cache->commset,
                                  joiner->in_caches[idx]->commset->count,
                                  joiner->in_caches[idx]->commset->table );
    }

    /*
       Synchronize the local_ID in each of in_caches[]->commset->table[] with
       the local_ID in the out_cache->commset->table[] for the same GID.
       Essentially, the local_IDs in all the in_caches[]'s are made
       to be globally unique integers (GIDs are also globally unique but
       they are much bigger than integers)
    */
    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
#if defined( CLOG_JOINER_PRINT )
        fprintf( stdout, "BEFORE SYNC: in_caches[%d]:", idx );
        CLOG_CommSet_print( joiner->in_caches[idx]->commset, stdout );
#endif
        if (    CLOG_CommSet_sync_IDs( joiner->in_caches[idx]->commset,
                                       joiner->out_cache->commset->count,
                                       joiner->out_cache->commset->table )
             != CLOG_BOOL_TRUE ) {
            fprintf( stderr, __FILE__":CLOG_Joiner_init() - \n"
                             "\t""CLOG_CommSet_sync_IDs(%s,out) fails!\n",
                             joiner->in_caches[idx]->local_filename );
            fflush( stderr );
            exit( 1 );
        }
#if defined( CLOG_JOINER_PRINT )
        fprintf( stdout, "AFTER SYNC: in_caches[%d]:", idx );
        CLOG_CommSet_print( joiner->in_caches[idx]->commset, stdout );
        fprintf( stdout, "\n" );
#endif
    }
}

static void CLOG_Joiner_sync_preambles( CLOG_Joiner_t *joiner )
{
    CLOG_Preamble_t  *in_preamble, *out_preamble;
    int               num_links;
    int               idx;

    out_preamble  = joiner->out_cache->preamble;
    num_links     = 0;
    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
        if ( CLOG_Cache_has_rec( joiner->in_caches[idx] ) == CLOG_BOOL_TRUE ) {
            in_preamble  = joiner->in_caches[idx]->preamble;
            if ( num_links == 0 )
                CLOG_Preamble_copy( in_preamble, out_preamble );
            else
                CLOG_Preamble_sync( out_preamble, in_preamble );
            num_links++;
        }
    }
}

void CLOG_Joiner_init( CLOG_Joiner_t *joiner, const char *out_filename )
{
    CLOG_CacheLink_t   *head_link, *tail_link;
    CLOG_CacheLink_t   *curr_link, *earliest_link;
    CLOG_Time_t         curr_time, earliest_time;
    int                 num_links;
    int                 idx;

    /*
       Sync all local commIDs to be globally unique integers in
       both input/output caches.
    */
    CLOG_Joiner_sync_commIDs( joiner );

    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
        CLOG_Cache_init4read( joiner->in_caches[idx] );
    }

    /* Setup a list of CLOG_Joiner_t->in_caches[] pointed by head_link. */
    head_link      = NULL;
    tail_link      = NULL;
    num_links      = 0;
    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
        if ( CLOG_Cache_has_rec( joiner->in_caches[idx] ) == CLOG_BOOL_TRUE ) {
            /* Setup initialize binary linked list of CLOG_CacheLink_t */
            curr_link  = CLOG_CacheLink_create( joiner->in_caches[idx] );
            CLOG_CacheLink_insert( &head_link, &tail_link, NULL, curr_link );
            num_links++;
        }
    }

    /* Initialize the output cache's preamble with in_cache[0]'s preambles. */
    CLOG_Joiner_sync_preambles( joiner );

    CLOG_Cache_open4write( joiner->out_cache, out_filename );
    CLOG_Cache_init4write( joiner->out_cache );

    /* Do a N^2 sort, slow but only need to do the N^2 sort once */
    while ( num_links > 0 ) {
        /* Locate the earliest CLOG_CacheLink_t in head_link */
        earliest_link  = head_link;
        earliest_time  = CLOG_Cache_get_time( earliest_link->cache );
        for ( curr_link  = head_link;
              curr_link != NULL;
              curr_link  = curr_link->next ) {
            curr_time = CLOG_Cache_get_time( curr_link->cache );
            if ( curr_time < earliest_time ) {
                earliest_link = curr_link;
                earliest_time = curr_time;
            }
        }
        /* Detach the earliest_link from the List of head_link & tail_link */
        CLOG_CacheLink_detach( &head_link, &tail_link,
                               earliest_link );
        /* Append the earliest_link to the joiner->sorted_caches */
        CLOG_CacheLink_insert( &(joiner->sorted_caches_head),
                               &(joiner->sorted_caches_tail),
                               NULL, earliest_link );
        num_links--;
    }
}

#if defined( SIMPLE_JOINER_SORT )
/*
   A SIMPLIFIED version of CLOG_Joiner_sort() that may have done too many
   detach/insert of leading CLOG_CacheLink_t.
*/
void CLOG_Joiner_sort( CLOG_Joiner_t *joiner )
{
    CLOG_Rec_Header_t *earliest_hdr;
    CLOG_CacheLink_t  *head_link, *tail_link;
    CLOG_CacheLink_t  *detach_link, *loc_link;
    CLOG_Time_t        detach_time;

#if defined( CLOG_JOINER_PRINT )
    int                count;
    count  = 0;
#endif

    /* Set local head and tail CLOG_CacheLink_t's to speed up access */
    head_link = joiner->sorted_caches_head;
    tail_link = joiner->sorted_caches_tail;

    /* When head CLOG_CacheLink_t is NULL, there is no more record. */
    while (    head_link != NULL
            && CLOG_Cache_has_rec( head_link->cache ) == CLOG_BOOL_TRUE ) {
        earliest_hdr = CLOG_Cache_get_rec( head_link->cache );
#if defined( CLOG_JOINER_PRINT )
        fprintf( stdout, "%d: ", ++count );
        CLOG_Rec_print( earliest_hdr, stdout );
        fflush( stdout );
#endif
        CLOG_Cache_put_rec( joiner->out_cache, earliest_hdr );

        /* Detach the head CLOG_CacheLink_t from the sorted_caches */
        detach_link = head_link;
        CLOG_CacheLink_detach( &head_link, &tail_link, detach_link );

        /*
           Attach the detached CLOG_CacheLink_t if it is non-empty
           If the CLOG_CacheLink_t has empty cache, throw it away,
           i.e not attaching back to the binary list.
        */
        if ( CLOG_Cache_has_rec( detach_link->cache ) == CLOG_BOOL_TRUE ) {
            detach_time = CLOG_Cache_get_time( detach_link->cache );
            /* Locate the CLOG_CacheLink_t whose time >= detach time */
            loc_link = head_link;
            while (    loc_link != NULL
                    && CLOG_Cache_get_time( loc_link->cache ) < detach_time ) {
                loc_link = loc_link->next;
            }
            CLOG_CacheLink_insert( &head_link, &tail_link,
                                   loc_link, detach_link );
        }
    }
    /* Set CLOG_Joiner_t's head & tail CLOG_CacheLink_ts to the local values */
    joiner->sorted_caches_head = head_link;
    joiner->sorted_caches_tail = tail_link;

    /* Flush whatever data in out_cache to disk */
    CLOG_Cache_flushlastblock( joiner->out_cache );
}
#else
/* 
   An OPTIMIZED version of CLOG_Joiner_sort() that eliminates
   unnecessary detach/insert of leading CLOG_CacheLink_t.
*/
void CLOG_Joiner_sort( CLOG_Joiner_t *joiner )
{
    CLOG_Rec_Header_t *earliest_hdr;
    CLOG_CacheLink_t  *head_link, *tail_link; 
    CLOG_CacheLink_t  *detach_link, *loc_link; 
    CLOG_Time_t        detach_time, next_head_time;

#if defined( CLOG_JOINER_PRINT )
    int                count;
    count  = 0;
#endif

    /* Set local head and tail CLOG_CacheLink_t's to speed up access */
    head_link = joiner->sorted_caches_head;
    tail_link = joiner->sorted_caches_tail;

    /* When head CLOG_CacheLink_t is NULL, there is no more record. */
    while ( head_link != NULL ) {
        if ( head_link->next != NULL ) {
        /*  When there are TWO or MORE non-empty CLOG_CacheLink_t's.  */
            if (    CLOG_Cache_has_rec( head_link->next->cache )
                 == CLOG_BOOL_TRUE ) {
                next_head_time = CLOG_Cache_get_time( head_link->next->cache );
                while (    CLOG_Cache_has_rec( head_link->cache )
                        == CLOG_BOOL_TRUE
                        && CLOG_Cache_get_time( head_link->cache )
                        <= next_head_time ) {
                     earliest_hdr = CLOG_Cache_get_rec( head_link->cache );
                     CLOG_Cache_put_rec( joiner->out_cache, earliest_hdr );
#if defined( CLOG_JOINER_PRINT )
                     fprintf( stdout, "%d(A): ", ++count );
                     CLOG_Rec_print( earliest_hdr, stdout );
                     fflush( stdout );
#endif
                }
#if defined( CLOG_JOINER_PRINT )
                fprintf( stdout, "---------------\n" );
                fflush( stdout );
#endif

                /*
                   Either CLOG_Cache_has_rec( head_link->cache ) == FALSE
                    Or    curr_time > next_time.
                   => Detach the head_link first.
                   => If the leading CLOG_CacheLink_t is empty, leave the link
                      un-detached. i.e. throw it away.
                   => If the leading CLOG_Cache_t is non-empty, reattach the
                      link back to the appropriate place in the linked list.
                */
                detach_link = head_link;
                CLOG_CacheLink_detach( &head_link, &tail_link, detach_link );

                if (    CLOG_Cache_has_rec( detach_link->cache )
                     == CLOG_BOOL_TRUE ) {
                /* if ( CLOG_Cache_get_time( head->cache ) > next_time ). */
                    detach_time = CLOG_Cache_get_time( detach_link->cache );
                    /* Locate the CLOG_CacheLink_t whose time > detach's time */
                    loc_link = head_link;
                    while (    loc_link != NULL
                            && CLOG_Cache_get_time( loc_link->cache )
                            <= detach_time ) {
                        loc_link = loc_link->next;
                    }
                    CLOG_CacheLink_insert( &head_link, &tail_link,
                                           loc_link, detach_link );
                }
            }
            else { /* CLOG_Cache_has_rec( head_link->next->cache ) == FALSE */
                /* This scenario should have never occured. If occurs, warns. */
                fprintf( stderr, __FILE__":CLOG_Joiner_sort() - Warning! "
                                 "This scenario should have never occured!\n"
                                 "\t""head_link->next != NULL && "
                                 "but head_link->next->cache is empty!\n"
                                 "Detaching head_link->next....! " );
                fflush( stderr );
                detach_link = head_link->next;
                CLOG_CacheLink_detach( &head_link, &tail_link, detach_link );
            }
        }
        else { /*  When there is only ONE non-empty CLOG_CacheLink_t.  */
            while ( CLOG_Cache_has_rec( head_link->cache ) ) {
                earliest_hdr = CLOG_Cache_get_rec( head_link->cache );
                CLOG_Cache_put_rec( joiner->out_cache, earliest_hdr );
#if defined( CLOG_JOINER_PRINT )
                fprintf( stdout, "%d(B): ", ++count );
                 CLOG_Rec_print( earliest_hdr, stdout );
                 fflush( stdout );
#endif
            }
            detach_link = head_link;
            CLOG_CacheLink_detach( &head_link, &tail_link, detach_link );
        }

    }
    /* Set CLOG_Joiner_t's head & tail CLOG_CacheLink_ts to the local values */
    joiner->sorted_caches_head = head_link;
    joiner->sorted_caches_tail = tail_link;

    /* Flush whatever data in out_cache to disk */
    CLOG_Cache_flushlastblock( joiner->out_cache );
}
#endif

void CLOG_Joiner_finalize( CLOG_Joiner_t *joiner )
{
    int idx;

    /* Update the CLOG_Premable_t in joiner->out_cache's file */
    CLOG_Cache_close4write( joiner->out_cache );

    for ( idx = 0; idx < joiner->num_caches; idx++ ) {
        /* Empty functions */
        CLOG_Cache_close4read( joiner->in_caches[idx] );
    }
}
