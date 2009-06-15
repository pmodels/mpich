/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#include "clog_mem.h"
#include "clog_cache.h"
#include "clog_preamble.h"
#include "clog_commset.h"
#include "clog_util.h"

static int clog_cache_minblocksize; /* constant needed for writing to cache */

CLOG_Cache_t* CLOG_Cache_create( void )
{
    CLOG_Cache_t *cache;

    cache = (CLOG_Cache_t *) MALLOC( sizeof(CLOG_Cache_t) );
    if ( cache == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Cache_create() - MALLOC() fails.\n" );
        fflush( stderr );
        return NULL;
    }

    cache->preamble = CLOG_Preamble_create();
    if ( cache->preamble == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Cache_create() - \n"
                         "\t""CLOG_Preamble_create() returns NULL.\n" );
        fflush( stderr );
        return NULL;
    }

    cache->commset     = CLOG_CommSet_create();
    if ( cache->commset == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Cache_create() - \n"
                         "\t""CLOG_CommSet_create() returns NULL.\n" );
        fflush( stderr );
        return NULL;
    }

    cache->blockdata             = NULL;
    cache->block_size            = -9999;

    cache->is_runtime_bigendian  = CLOG_Util_is_runtime_bigendian();

    cache->local_fd              = CLOG_NULL_FILE;
    strcpy( cache->local_filename, "" );

    cache->local_timediff        = CLOG_MAXTIME;    /* Set an invalid time */

    return cache;
}

void CLOG_Cache_free( CLOG_Cache_t **cache_handle )
{
    CLOG_Cache_t  *cache;

    cache = *cache_handle;

    if ( cache != NULL ) {
        CLOG_CommSet_free( &(cache->commset) );
        CLOG_BlockData_free( &(cache->blockdata) );
        cache->block_size = 0;
        CLOG_Preamble_free( &(cache->preamble) );
        FREE( cache );
    }
    *cache_handle = NULL;
}

/* fill the block with data from the disk */
static void CLOG_Cache_fillblock( CLOG_Cache_t *cache )
{
    int ierr;

    ierr = read( cache->local_fd, cache->blockdata->head, cache->block_size );
    if ( ierr != cache->block_size ) {
        fprintf( stderr, __FILE__":CLOG_Cache_fillblock() - \n"
                         "\t""read(cache->blockdata) fails w/ err=%d.\n",
                         ierr );
        fflush( stderr );
        exit( 1 );
    }
    /* Do byteswapping if necessary then initialize the CLOG_BlockData_t */
    if ( cache->preamble->is_big_endian != cache->is_runtime_bigendian )
        CLOG_BlockData_swap_bytes_first( cache->blockdata );

    /* fixup the local communicator ID in each record */
    if ( cache->preamble->is_finalized != CLOG_BOOL_TRUE )
        CLOG_BlockData_patch_all( cache->blockdata, &(cache->local_timediff),
                                  cache->commset->table );
    else
        CLOG_BlockData_patch_comm( cache->blockdata, cache->commset->table );

    CLOG_BlockData_reset( cache->blockdata );
}

/* flush the block's data to the disk */
static void CLOG_Cache_flushblock( CLOG_Cache_t *cache )
{
    int ierr;

    /* CLOG_Cache_t only writes big-endian file */
    /* if ( cache->is_runtime_bigendian != CLOG_BOOL_TRUE ) */
    if ( cache->preamble->is_big_endian != CLOG_BOOL_TRUE )
        CLOG_BlockData_swap_bytes_last( cache->blockdata );

    ierr = write( cache->local_fd, cache->blockdata->head, cache->block_size );
    if ( ierr != cache->block_size ) {
        fprintf( stderr, __FILE__":CLOG_Cache_flushblock() - \n"
                         "\t""Fail to write to the logfile %s.\n"
                         "\t""Check if the filesystem where the logfile "
                         "resides is full.\n", cache->local_filename );
        fflush( stderr );
        exit( 1 );
    }
    CLOG_BlockData_reset( cache->blockdata );
}

void CLOG_Cache_flushlastblock( CLOG_Cache_t *cache )
{
    CLOG_BlockData_t   *blkdata;
    CLOG_Rec_Header_t  *last_hdr;

    blkdata = cache->blockdata;

    /* Write a CLOG_REC_ENDLOG record */
    last_hdr = (CLOG_Rec_Header_t *) blkdata->ptr;
    last_hdr->time    = CLOG_MAXTIME;
    last_hdr->icomm   = 0;                  /* arbitary */
    last_hdr->rank    = 0;                  /* arbitary */
    last_hdr->thread  = 0;                  /* arbitary */
    last_hdr->rectype = CLOG_REC_ENDLOG;

    CLOG_Cache_flushblock( cache );
}

/* CLOG_Cache_open4read() has to be called before CLOG_Cache_init4read() */
void CLOG_Cache_open4read( CLOG_Cache_t *cache, const char *filename )
{
    CLOG_BOOL_T  do_byte_swap;
    int          ierr;

    if ( filename != NULL && strlen(filename) != 0 )
        strcpy( cache->local_filename, filename );
    else {
        fprintf( stderr, __FILE__":CLOG_Cache_open4read() - \n"
                         "\t""input file name is empty.\n" );
        fflush( stderr );
        exit( 1 );
    }

    cache->local_fd = OPEN( cache->local_filename, O_RDONLY, 0 );
    if ( cache->local_fd == -1 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4read() - \n"
                         "\t""Fail to open the file %s for reading.\n",
                         cache->local_filename );
        fflush( stderr );
        exit( 1 );
    }

    CLOG_Preamble_read( cache->preamble, cache->local_fd );
    if ( cache->preamble->commtable_fptr < CLOG_PREAMBLE_SIZE ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4read() - Warning!\n"
                         "\t""Invalid commtable_fptr, "i64fmt", < "
                         "CLOG_PREAMBLE_SIZE, %d.\n"
                         "\t""This logfile could be incomplete. "
                         "clog2_repair may be able to fix it.\n",
                         cache->preamble->commtable_fptr, CLOG_PREAMBLE_SIZE );
        fflush( stderr );
        return;
    }

    lseek( cache->local_fd, cache->preamble->commtable_fptr, SEEK_SET );
    do_byte_swap = (    cache->preamble->is_big_endian
                     != cache->is_runtime_bigendian );
    ierr = CLOG_CommSet_read( cache->commset, cache->local_fd, do_byte_swap );
    if ( ierr <= 0 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4read() - Warning!\n"
                         "\t""CLOG_CommSet_read() return an error code, %d.\n"
                         "\t""This logfile could be incomplete. "
                         "clog2_repair may be able to fix it.\n",
                         ierr );
        fflush( stderr );
    }
}

/*
    CLOG_Cache_t's input interface is separated into 2 functions:
    CLOG_Cache_open4read() and CLOG_Cache_init4read() because the
    synchronization of local commIDs, CLOG_Joiner_sync_commIDs(),
    needs to be inserted between the 2 functions.
*/
/* CLOG_Cache_init4read() has to be called after CLOG_Cache_open4read() */
void CLOG_Cache_init4read( CLOG_Cache_t *cache )
{
    /* Create CLOG_BlockData_t based on CLOG_Preamble_t */
    cache->block_size  = cache->preamble->block_size;
    cache->blockdata   = CLOG_BlockData_create( cache->block_size );

    /* Initialize timediff if CLOG_Cache_t is for a local file */
    if ( cache->preamble->is_finalized != CLOG_BOOL_TRUE )
        cache->local_timediff = 0.0;

    /* Initial update of CLOG_BlockData_t with data from the disk */
    lseek( cache->local_fd, (off_t) CLOG_PREAMBLE_SIZE, SEEK_SET );
    CLOG_Cache_fillblock( cache );
}

/* CLOG_Cache_open4write() has to be called before CLOG_Cache_init4write() */
void CLOG_Cache_open4write( CLOG_Cache_t *cache, const char *filename )
{
    if ( filename != NULL && strlen(filename) != 0 )
        strcpy( cache->local_filename, filename );
    else {
        fprintf( stderr, __FILE__":CLOG_Cache_open4write() - \n"
                         "\t""input file name is empty.\n" );
        fflush( stderr );
        exit( 1 );
    }

    cache->local_fd = OPEN( cache->local_filename, O_CREAT|O_WRONLY|O_TRUNC,
                            0664 );
    if ( cache->local_fd == -1 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4write() - \n"
                         "\t""Fail to open the file %s for writing.\n",
                         cache->local_filename );
        fflush( stderr );
        exit( 1 );
    }

    /*
       Set the OS's native byte ordering to the CLOG_Preamble_t,
       so this is consistent with CLOG_Merger_t.
    */
    cache->preamble->is_big_endian = cache->is_runtime_bigendian;
    CLOG_Preamble_write( cache->preamble, CLOG_BOOL_TRUE, CLOG_BOOL_TRUE,
                         cache->local_fd );
}

/* CLOG_Cache_init4write() has to be called after CLOG_Cache_open4write() */
void CLOG_Cache_init4write( CLOG_Cache_t *cache )
{
    /* Create CLOG_BlockData_t based on CLOG_Preamble_t */
    cache->block_size  = cache->preamble->block_size;
    cache->blockdata   = CLOG_BlockData_create( cache->block_size );

    /*
       Initialize the CLOG_Cache_t's minimum block size: CLOG_REC_ENDBLOCK
    */
    clog_cache_minblocksize = CLOG_Rec_size( CLOG_REC_ENDBLOCK );
}

/* CLOG_Cache_open4readwrite() has to be called before CLOG_Cache_init4read() */
void CLOG_Cache_open4readwrite( CLOG_Cache_t *cache, const char *filename )
{
    CLOG_BOOL_T  do_byte_swap;
    int          ierr;

    if ( filename != NULL && strlen(filename) != 0 )
        strcpy( cache->local_filename, filename );
    else {
        fprintf( stderr, __FILE__":CLOG_Cache_open4readwrite() - \n"
                         "\t""input file name is empty.\n" );
        fflush( stderr );
        exit( 1 );
    }

    cache->local_fd = OPEN( cache->local_filename, O_RDWR, 0664 );
    if ( cache->local_fd == -1 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4readwrite() - \n"
                         "\t""Fail to open the file %s for reading.\n",
                         cache->local_filename );
        fflush( stderr );
        exit( 1 );
    }

    CLOG_Preamble_read( cache->preamble, cache->local_fd );
    if ( cache->preamble->commtable_fptr < CLOG_PREAMBLE_SIZE ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4readwrite() - Warning!\n"
                         "\t""Invalid commtable_fptr, "i64fmt", < "
                         "CLOG_PREAMBLE_SIZE, %d.\n"
                         "\t""This program can fix this incomplete logfile.\n",
                         cache->preamble->commtable_fptr, CLOG_PREAMBLE_SIZE );
        fflush( stderr );
        return;
    }

    lseek( cache->local_fd, cache->preamble->commtable_fptr, SEEK_SET );
    do_byte_swap = (    cache->preamble->is_big_endian
                     != cache->is_runtime_bigendian );
    ierr = CLOG_CommSet_read( cache->commset, cache->local_fd, do_byte_swap );
    if ( ierr <= 0 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_open4readwrite() - Warning!\n"
                         "\t""CLOG_CommSet_read() return an error code, %d.\n"
                         "\t""This program can fix this incomplete logfile.\n",
                         ierr );
        fflush( stderr );
    }
}

/* This is for clog2_print to print the content of clog2 file */
void CLOG_Cache_close4read( CLOG_Cache_t *cache )
{ /*  Empty function for now. */ }

/* This is for clog2_join to write multiple MPI_COMM_WORLD joined clog2 file */
void CLOG_Cache_close4write( CLOG_Cache_t *cache )
{
    CLOG_BOOL_T  do_byte_swap;
    int          ierr;
    
    cache->preamble->commtable_fptr
    = (CLOG_int64_t) lseek( cache->local_fd, 0, SEEK_CUR );
    /* Save CLOG_CommSet_t to BigEndian file */
    do_byte_swap = ( cache->preamble->is_big_endian != CLOG_BOOL_TRUE );
    ierr = CLOG_CommSet_write( cache->commset, cache->local_fd, do_byte_swap );
    if ( ierr < 0 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_close4write() - \n"
                         "\t""CLOG_CommSet_write() fails!\n" );
        fflush( stderr );
        return;
    }
    /* Save updated CLOG_Preamble_t to the BigEndian and Finalized file */
    lseek( cache->local_fd, 0, SEEK_SET );
    CLOG_Preamble_write( cache->preamble, CLOG_BOOL_TRUE, CLOG_BOOL_TRUE,
                         cache->local_fd );
    close( cache->local_fd );
}

/* This is for clog2_repair to fix up incomplete file.  */
void CLOG_Cache_close4readwrite( CLOG_Cache_t *cache )
{
    CLOG_BOOL_T  do_byte_swap;
    int          ierr;
    
    cache->preamble->commtable_fptr
    = (CLOG_int64_t) lseek( cache->local_fd, 0, SEEK_CUR );
    /* Save CLOG_CommSet_t to the file of the Same Endianess */
    do_byte_swap = (    cache->preamble->is_big_endian
                     != cache->is_runtime_bigendian );
    ierr = CLOG_CommSet_write( cache->commset, cache->local_fd, do_byte_swap );
    if ( ierr < 0 ) {
        fprintf( stderr, __FILE__":CLOG_Cache_close4readwrite() - \n"
                         "\t""CLOG_CommSet_write() fails!\n" );
        fflush( stderr );
        return;
    }
    /* Save updated CLOG_Preamble_t to the Same Endianness & Finalized file */
    lseek( cache->local_fd, 0, SEEK_SET );
    CLOG_Preamble_write( cache->preamble, CLOG_BOOL_NULL, CLOG_BOOL_NULL,
                         cache->local_fd );
    close( cache->local_fd );
}



/*
    Iterator interface for CLOG_Cache_t (similar to Java's Iterator interface),
    i.e. CLOG_Cache_has_rec() has to be called before each CLOG_Cache_get_rec()
*/
CLOG_BOOL_T CLOG_Cache_has_rec( CLOG_Cache_t *cache )
{
    CLOG_Rec_Header_t  *hdr;

    hdr = (CLOG_Rec_Header_t *) cache->blockdata->ptr;

    if ( hdr->rectype > CLOG_REC_ENDBLOCK && hdr->rectype < CLOG_REC_NUM )
        return CLOG_BOOL_TRUE;
    else {
        if ( hdr->rectype == CLOG_REC_ENDBLOCK ) {
            CLOG_Cache_fillblock( cache );
            return CLOG_Cache_has_rec( cache );
        }
        if ( hdr->rectype == CLOG_REC_ENDLOG ) {
            return CLOG_BOOL_FALSE;
        }
        fprintf( stderr, __FILE__":CLOG_Cache_has_next() - \n"
                         "\t""Unknown record type "i32fmt".\n", hdr->rectype );
        fflush( stderr );
        exit( 1 );
    }
}

/* Don't advance CLOG_BlockData_t's ptr unless get_rec() is called. */
CLOG_Rec_Header_t* CLOG_Cache_get_rec( CLOG_Cache_t *cache )
{
    CLOG_Rec_Header_t  *hdr;

    hdr = (CLOG_Rec_Header_t *) cache->blockdata->ptr;
    /* Advance CLOG_Cache_t's BlockData pointer by 1 whole record length */
    cache->blockdata->ptr += CLOG_Rec_size( hdr->rectype );
    return hdr;
}

CLOG_Time_t CLOG_Cache_get_time( const CLOG_Cache_t *cache )
{
    CLOG_Rec_Header_t  *hdr;

    hdr = (CLOG_Rec_Header_t *) cache->blockdata->ptr;
    return hdr->time;
}

static int CLOG_Cache_reserved_block_size( CLOG_int32_t rectype )
{
    /* Have CLOG_Rec_size() do the error checking */
    return CLOG_Rec_size( rectype ) + clog_cache_minblocksize;
}

/* CLOG_Cache_put_rec() write to the cache which will be writing to disk */
void CLOG_Cache_put_rec( CLOG_Cache_t *cache, const CLOG_Rec_Header_t *hdr )
{
    CLOG_BlockData_t   *blkdata;
    CLOG_Rec_Header_t  *last_hdr;
    int                 reclen;

    blkdata = cache->blockdata;

    /* Write a CLOG_REC_ENDBLOCK record */
    if (    blkdata->ptr + CLOG_Cache_reserved_block_size( hdr->rectype )
         >= blkdata->tail ) {
        last_hdr = (CLOG_Rec_Header_t *) blkdata->ptr;
        last_hdr->time    = hdr->time;
        last_hdr->icomm   = hdr->icomm;         /* arbitary */ 
        last_hdr->rank    = hdr->rank;          /* arbitary */
        last_hdr->thread  = hdr->thread;        /* arbitary */
        last_hdr->rectype = CLOG_REC_ENDBLOCK;
        CLOG_Cache_flushblock( cache );
    }

    /* Save the CLOG record into the CLOG_BlockData_t */
    reclen = CLOG_Rec_size( hdr->rectype );
    memcpy( blkdata->ptr, hdr, reclen );
    blkdata->ptr += reclen;
}



CLOG_CacheLink_t* CLOG_CacheLink_create( CLOG_Cache_t* cache )
{
    CLOG_CacheLink_t  *cachelink;

    cachelink = (CLOG_CacheLink_t *) MALLOC( sizeof(CLOG_CacheLink_t) );
    if ( cachelink == NULL ) {
        fprintf( stderr, __FILE__":CLOG_CacheLink_create() - "
                         "MALLOC() fails for CLOG_CacheLink_t!\n" );
        fflush( stderr );
        return NULL;
    }

    cachelink->cache  = cache;
    cachelink->prev   = NULL;
    cachelink->next   = NULL;

    return cachelink;
}

void CLOG_CacheLink_free( CLOG_CacheLink_t **cachelink_handle )
{
    CLOG_CacheLink_t  *cachelink;
    cachelink  = *cachelink_handle;
    if ( cachelink != NULL ) {
        FREE( cachelink );
    }
    *cachelink_handle = NULL;
}

/*
   Detach "detach" from the List pointed by "*head_handle" & "*tail_handle"
   "detach" is assumed to be non-NULL.
*/
void CLOG_CacheLink_detach( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *detach )
{
    if ( detach->prev != NULL ) {
        detach->prev->next = detach->next;
        /*
        detach->prev       = NULL;
        Cannot set detach->prev to NULL yet as it may be needed later.
        */
    }
    else { /* (detach->prev == NULL) <=> (detach == *head_handle) */
        if ( detach->next != NULL ) {
            (*head_handle)        = detach->next;
            (*head_handle)->prev  = NULL;
        }
        else
            (*head_handle)        = NULL;
    }

    if ( detach->next != NULL ) {
        detach->next->prev = detach->prev;
        /*
        detach->next       = NULL;
        Don't set detach->next to NULL here to be in parallel to the code above.
        */
    }
    else { /* (detach->next == NULL) <=> (detach == *tail_handle) */
        if ( detach->prev != NULL ) {
            (*tail_handle)        = detach->prev;
            (*tail_handle)->next  = NULL;
        }
        else
            (*tail_handle)        = NULL;
    }

    /* Make sure that the detach is really detached to any links */
    detach->prev  = NULL;
    detach->next  = NULL;
}

/* Append "curr" after "*tail_handle", then Advance "*tail_handle" */
void CLOG_CacheLink_append( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *curr )
{
    /* CLOG_CacheLink_t *tail; tail = *tail_handle */
    curr->prev            = *tail_handle;
    curr->next            = NULL;
    if ( *tail_handle != NULL ) /* *head_handle != *tail_handle != NULL */
        (*tail_handle)->next  = curr;
    else  /* *head_handle == *tail_handle == NULL */
        (*head_handle)    = curr;
    (*tail_handle)        = curr;
}

/*
    location is
    1) either between *head_handle and *tail_handle
       *head_handle <= location <= *tail_handle
    2) or NULL, i.e. after *tail_handle
       *head_handle <= *tail_handle < location(=NULL)

    Therefore    insert <= location , i.e. insert comes before location.

    CLOG_CacheLink_insert( &head, &tail, NULL, insert ) is functionally
    the same as CLOG_Cache_append( &head, &tail, insert );
*/
void CLOG_CacheLink_insert( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *location,
                            CLOG_CacheLink_t  *insert )
{
     /*  before <= insert <= after */
     CLOG_CacheLink_t  *before, *after;

     /* connect the "insert" with "after" */
     after  = location;
     if ( after != NULL ) {
         before        = after->prev;
         after->prev   = insert;
     }
     else {
         before        = *tail_handle;
         *tail_handle  = insert;
     }
     insert->next  = after;

     /* connect the "insert" with "before" */
     if ( before != NULL )
         before->next  = insert;
     else
         *head_handle  = insert;
     insert->prev  = before;
}
