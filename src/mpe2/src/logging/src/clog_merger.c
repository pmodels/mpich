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
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#if !defined( CLOG_NOMPI )
#include "mpi.h"
#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

#include "clog_const.h"
#include "clog_mem.h"
#include "clog_merger.h"
#include "clog_util.h"

#define CLOG_RANK_NULL   -1

static int clog_merger_minblocksize;

CLOG_Merger_t* CLOG_Merger_create( unsigned int block_size )
{
    CLOG_Merger_t  *merger;

    merger = (CLOG_Merger_t *) MALLOC( sizeof(CLOG_Merger_t) );
    if ( merger == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Merger_create() - \n"
                         "\t""MALLOC() fails for CLOG_Merger_t!\n" );
        fflush( stderr );
        return NULL;
    }


    merger->left_blk  = (CLOG_BlockData_t *)
                        CLOG_BlockData_create( block_size );
    if ( merger->left_blk == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Merger_create() - \n"
                         "\t""CLOG_BlockData_create(%d) fails for left_blk!",
                         block_size );
        fflush( stderr );
        return NULL;
    }

    merger->right_blk  = (CLOG_BlockData_t *)
                         CLOG_BlockData_create( block_size );
    if ( merger->right_blk == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Merger_create() - \n"
                         "\t""CLOG_BlockData_create(%d) fails for right_blk!",
                         block_size );
        fflush( stderr );
        return NULL;
    }

    merger->sorted_blk  = (CLOG_BlockData_t *)
                          CLOG_BlockData_create( block_size );
    if ( merger->sorted_blk == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Merger_create() - \n"
                         "\t""CLOG_BlockData_create(%d) fails for sorted_blk!",
                         block_size );
        fflush( stderr );
        return NULL;
    }

    merger->block_size        = block_size;
    merger->num_active_blks   = 0;
    merger->world_size        = 1;
    merger->local_world_rank  = 0;
    merger->left_world_rank   = 0;
    merger->right_world_rank  = 0;
    merger->parent_world_rank = 0;

    merger->is_big_endian    = CLOG_BOOL_NULL;
    strncpy( merger->out_filename, "mpe_trace."CLOG_FILE_TYPE,
             CLOG_PATH_STRLEN );
    merger->out_fd           = -1;

    return merger;
}

void CLOG_Merger_free( CLOG_Merger_t **merger_handle )
{
    CLOG_Merger_t  *merger;
    merger = *merger_handle;
    if ( merger != NULL ) {
        CLOG_BlockData_free( &(merger->sorted_blk) );
        CLOG_BlockData_free( &(merger->left_blk) );
        CLOG_BlockData_free( &(merger->right_blk) );
        FREE( merger );
    }
    *merger_handle = NULL;
}

/*@
    CLOG_Merger_set_neighbor_ranks - locally determine parent and children
                                     in a binary tree

Input parameters

+  merger->local_world_rank  - calling process''s id
-  merger->world_size        - total number of processes in tree

Output parameters

+  merger->parent_world_rank - parent of local process
                             (or CLOG_RANK_NULL if root)
.  merger->left_world_rank   - left child of local process in binary tree
                             (or CLOG_RANK_NULL if none)
-  merger->right_world_rank  - right child of local process in binary tree
                             (or CLOG_RANK_NULL if none)

@*/
void CLOG_Merger_set_neighbor_ranks( CLOG_Merger_t *merger )
{
    if ( merger->local_world_rank == 0 )
        merger->parent_world_rank = CLOG_RANK_NULL;
    else
        merger->parent_world_rank = (merger->local_world_rank - 1) / 2;

    merger->left_world_rank = (2 * merger->local_world_rank) + 1;
    if ( merger->left_world_rank > merger->world_size - 1 )
        merger->left_world_rank = CLOG_RANK_NULL;

    merger->right_world_rank = (2 * merger->local_world_rank) + 2;
    if ( merger->right_world_rank > merger->world_size - 1 )
        merger->right_world_rank = CLOG_RANK_NULL;
}

/* Initialize the CLOG_Merger_t */
void CLOG_Merger_init(       CLOG_Merger_t    *merger,
                       const CLOG_Preamble_t  *preamble,
                       const char             *merged_file_prefix )
{
    /* Set up the binary tree MPI ranks ID locally for merging */
    PMPI_Comm_size( MPI_COMM_WORLD, &(merger->world_size) );
    PMPI_Comm_rank( MPI_COMM_WORLD, &(merger->local_world_rank) );
    CLOG_Merger_set_neighbor_ranks( merger );

    merger->is_big_endian  = preamble->is_big_endian;
    /* Open the merged output file at root process */
    if ( merger->parent_world_rank == CLOG_RANK_NULL ) {
        strncpy( merger->out_filename, merged_file_prefix, CLOG_PATH_STRLEN );
        strcat( merger->out_filename, "."CLOG_FILE_TYPE );

        merger->out_fd = OPEN( merger->out_filename, O_CREAT|O_WRONLY|O_TRUNC,
                               0664 );
        if ( merger->out_fd == -1 ) {
            fprintf( stderr, __FILE__":CLOG_Merger_init() - \n"
                             "\t""Could not open file %s for merging!\n",
                             merger->out_filename );
            fflush( stderr );
            CLOG_Util_abort( 1 );
        }
        CLOG_Preamble_write( preamble, CLOG_BOOL_TRUE, CLOG_BOOL_TRUE,
                             merger->out_fd );
    }

    /*
       Initialize the CLOG_Merger's minimum block size: CLOG_REC_ENDBLOCK
    */
    clog_merger_minblocksize = CLOG_Rec_size( CLOG_REC_ENDBLOCK );
}
                                                                                
/* Finalize the CLOG_Merger_t */
void CLOG_Merger_finalize( CLOG_Merger_t *merger, CLOG_Buffer_t *buffer )
{
    CLOG_BOOL_T  do_byte_swap;
    int          ierr;

    if ( merger->out_fd != -1 ) {
        /* Update CLOG_Preamble_t with commtable_fptr */
        buffer->preamble->commtable_fptr
        = (CLOG_int64_t) lseek( merger->out_fd, 0, SEEK_CUR );
        /* Save CLOG_CommSet_t to file */
        do_byte_swap = merger->is_big_endian != CLOG_BOOL_TRUE;
        ierr = CLOG_CommSet_write( buffer->commset, merger->out_fd,
                                   do_byte_swap );
        if ( ierr < 0 ) {
            fprintf( stderr, __FILE__":CLOG_Merger_finalize() - \n"
                             "\t""CLOG_CommSet_write() fails!\n" );
            fflush( stderr );
            return;
        }
        /* Save the updated CLOG_Preamble_t */
        lseek( merger->out_fd, 0, SEEK_SET );
        CLOG_Preamble_write( buffer->preamble, CLOG_BOOL_TRUE, CLOG_BOOL_TRUE,
                             merger->out_fd );
        close( merger->out_fd );
    }
}

/*
    Flush the sorted buffer to the output logfile.
*/
void CLOG_Merger_flush( CLOG_Merger_t *merger )
{
    int  ierr;

    if ( merger->is_big_endian != CLOG_BOOL_TRUE )
        CLOG_BlockData_swap_bytes_last( merger->sorted_blk );

    ierr = write( merger->out_fd, merger->sorted_blk->head,
                  merger->block_size );
    if ( ierr != merger->block_size ) {
        fprintf( stderr, __FILE__":CLOG_Merger_flush() - \n"
                         "\t""Fail to write to the logfile %s, ierr = %d.\n",
                         merger->out_filename, ierr );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

/*
    clog_merger_minblocksize is defined in CLOG_Merger_init()
*/
static int CLOG_Merger_reserved_block_size( CLOG_int32_t rectype )
{
    /* Have CLOG_Rec_size() do the error checking */
    return CLOG_Rec_size( rectype ) + clog_merger_minblocksize;
}

/*
   Save the CLOG record marked by CLOG_Rec_Header_t to the output file.
*/
void CLOG_Merger_save_rec( CLOG_Merger_t *merger, const CLOG_Rec_Header_t *hdr )
{
    CLOG_BlockData_t   *sorted_blk;
    CLOG_Rec_Header_t  *sorted_hdr;
    int                 reclen;

    sorted_blk = merger->sorted_blk;


    /*
        If the sorted_blk is full, send sorted_blk to the process's parent
        if the parent exists.  If the parent process does not exist, it must
        be the root.  Then write the sorted_blk to the merged logfile,
        reinitialize the sorted_blk.
    */
    if (    sorted_blk->ptr + CLOG_Merger_reserved_block_size( hdr->rectype )
         >= sorted_blk->tail ) {
        sorted_hdr = (CLOG_Rec_Header_t *) sorted_blk->ptr;
        sorted_hdr->time       = hdr->time;     /* use prev record's time */
        sorted_hdr->icomm      = 0;
        sorted_hdr->rank       = merger->local_world_rank;
        sorted_hdr->thread     = 0;
        sorted_hdr->rectype    = CLOG_REC_ENDBLOCK;

        if ( merger->parent_world_rank != CLOG_RANK_NULL ) {
            PMPI_Ssend( sorted_blk->head, merger->block_size, 
                        CLOG_DATAUNIT_MPI_TYPE, merger->parent_world_rank,
                        CLOG_MERGE_LOGBUFTYPE, MPI_COMM_WORLD );
        }
        else { /* if parent_world_rank does not exist, must be the root */
            CLOG_Merger_flush( merger );
        }
        sorted_blk->ptr = sorted_blk->head;
    }

    /* CLOG_Rec_print( hdr, stdout ); */
    /* Save the CLOG record into the sorted buffer */
    reclen = CLOG_Rec_size( hdr->rectype );
    memcpy( sorted_blk->ptr, hdr, reclen );
    sorted_blk->ptr += reclen;
}


/* Used internally by CLOG_Merger_sort() */
/*
   Here blockdata can be either left_blk or right_blk defined in CLOG_Merger_t
*/
void CLOG_Merger_refill_sideblock( CLOG_BlockData_t  *blockdata,
                                   int block_world_rank, int block_size )
{
    MPI_Status         status;

    PMPI_Recv( blockdata->head, block_size, CLOG_DATAUNIT_MPI_TYPE,
               block_world_rank, CLOG_MERGE_LOGBUFTYPE, MPI_COMM_WORLD,
               &status );
    CLOG_BlockData_reset( blockdata );
}

/* Used internally by CLOG_Merger_sort() */
void CLOG_Merger_refill_localblock( CLOG_BlockData_t *blockdata,
                                    CLOG_Buffer_t    *buffer,
                                    CLOG_Time_t      *timediff_handle )
{
    /* refill the whole CLOG_Buffer_t from disk */
    if ( buffer->curr_block == NULL || buffer->num_used_blocks == 0 )
        CLOG_Buffer_localIO_read( buffer );

    if ( buffer->num_used_blocks > 0 ) {
        blockdata->head = buffer->curr_block->data->head;
        CLOG_BlockData_patch_all( blockdata, timediff_handle,
                                  buffer->commset->table );
        CLOG_BlockData_reset( blockdata );
        buffer->curr_block = buffer->curr_block->next;
        buffer->num_used_blocks--;
    }
    else /* if ( buffer->num_used_blocks == 0 ) */
        blockdata->ptr += CLOG_Rec_size( CLOG_REC_ENDBLOCK );
}

/* Used internally by CLOG_Merger_sort() */
/*
   Here blockdata can be either left_blk or right_blk defined in CLOG_Merger_t
*/
CLOG_Rec_Header_t *
CLOG_Merger_next_sideblock_hdr( CLOG_BlockData_t   *blockdata,
                                CLOG_Rec_Header_t  *hdr,
                                CLOG_Merger_t      *merger,
                                int                 block_world_rank,
                                int                 block_size )
{
    if ( hdr->rectype == CLOG_REC_ENDLOG ) {
        hdr->time      = CLOG_MAXTIME;
        (merger->num_active_blks)--;
    }
    else {
        CLOG_Merger_save_rec( merger, hdr );
        blockdata->ptr += CLOG_Rec_size( hdr->rectype );
        hdr = (CLOG_Rec_Header_t *) blockdata->ptr;
        if ( hdr->rectype == CLOG_REC_ENDBLOCK ) {
            CLOG_Merger_refill_sideblock( blockdata, block_world_rank,
                                          block_size );
            hdr = (CLOG_Rec_Header_t *) blockdata->ptr;
        }
    }
    return hdr;
}

/* Used internally by CLOG_Merger_sort() */
CLOG_Rec_Header_t *
CLOG_Merger_next_localblock_hdr( CLOG_BlockData_t   *blockdata,
                                 CLOG_Rec_Header_t  *hdr,
                                 CLOG_Merger_t      *merger,
                                 CLOG_Buffer_t      *buffer,
                                 CLOG_Time_t        *timediff_handle ) 
{
    if ( hdr->rectype == CLOG_REC_ENDLOG ) {
        hdr->time      = CLOG_MAXTIME;
        (merger->num_active_blks)--;
    }
    else {
        CLOG_Merger_save_rec( merger, hdr );
        blockdata->ptr += CLOG_Rec_size( hdr->rectype );
        hdr = (CLOG_Rec_Header_t *) blockdata->ptr;
        if ( hdr->rectype == CLOG_REC_ENDBLOCK ) {
            CLOG_Merger_refill_localblock( blockdata, buffer,
                                           timediff_handle );
            hdr = (CLOG_Rec_Header_t *) blockdata->ptr;
        }
    }
    return hdr;
}


/*
*/
void CLOG_Merger_sort( CLOG_Merger_t *merger, CLOG_Buffer_t *buffer )
{
    CLOG_Rec_Header_t  *left_hdr, *right_hdr, *local_hdr;
    CLOG_BlockData_t   *left_blk, *right_blk, *local_blk;
    CLOG_BlockData_t    local_shadow_blockdata ;
    CLOG_Time_t         local_timediff;
    int                 left_world_rank, right_world_rank;
    int                 block_size;

    /* May want to move this to CLOG_Merger_init() or CLOG_Converge_init() */
    /* Merge/Synchronize all the CLOG_CommSet_t at all the processes */
    CLOG_CommSet_merge( buffer->commset );
    /*
        reinitialization of CLOG_Buffer_t so that
        buffer->curr_block == buffer->head->block
    */
    CLOG_Buffer_localIO_reinit4read( buffer );

    merger->num_active_blks = 0;
    block_size              = merger->block_size;

    /* local_timediff's value is modifiable by CLOG_BlockData_patch_all() */
    local_timediff          = 0.0;
    left_world_rank         = merger->left_world_rank;
    right_world_rank        = merger->right_world_rank;
    left_blk                = merger->left_blk;
    right_blk               = merger->right_blk;

    local_blk  = &local_shadow_blockdata;
    /* Initialize the local_blk from CLOG_Buffer_t.curr_block */
    if ( buffer->curr_block != NULL && buffer->num_used_blocks > 0 ) {
        merger->num_active_blks++;
        CLOG_Merger_refill_localblock( local_blk, buffer, &local_timediff );
    }

    /* Initialize CLOG_Merger_t.left_blk from its left neighbor */
    if ( left_world_rank != CLOG_RANK_NULL ) {
        merger->num_active_blks++;
        CLOG_Merger_refill_sideblock( left_blk, left_world_rank, block_size );
    }
    else {
        left_hdr            = (CLOG_Rec_Header_t *) left_blk->head;
        left_hdr->time      = CLOG_MAXTIME;
    }

    /* Initialize CLOG_Merger_t.right_blk its right neighbor */
    if ( right_world_rank != CLOG_RANK_NULL ) {
        merger->num_active_blks++;
        CLOG_Merger_refill_sideblock( right_blk, right_world_rank, block_size );
    }
    else {
        right_hdr            = (CLOG_Rec_Header_t *) right_blk->head;
        right_hdr->time      = CLOG_MAXTIME;
    }

    left_hdr  = (CLOG_Rec_Header_t *) left_blk->ptr;
    right_hdr = (CLOG_Rec_Header_t *) right_blk->ptr;
    local_hdr = (CLOG_Rec_Header_t *) local_blk->ptr;
    while ( merger->num_active_blks > 0 ) {
        if ( left_hdr->time <= right_hdr->time ) {
            if ( left_hdr->time <= local_hdr->time )
                left_hdr  = CLOG_Merger_next_sideblock_hdr( left_blk,
                                                            left_hdr,
                                                            merger,
                                                            left_world_rank,
                                                            block_size );
            else
                local_hdr = CLOG_Merger_next_localblock_hdr( local_blk,
                                                             local_hdr,
                                                             merger,
                                                             buffer,
                                                             &local_timediff );
        }
        else {
            if ( right_hdr->time <= local_hdr->time )
                right_hdr = CLOG_Merger_next_sideblock_hdr( right_blk,
                                                            right_hdr,
                                                            merger,
                                                            right_world_rank,
                                                            block_size );
            else
                local_hdr = CLOG_Merger_next_localblock_hdr( local_blk,
                                                             local_hdr,
                                                             merger,
                                                             buffer,
                                                             &local_timediff );
        }
    }   /* Endof while ( merger->num_active_blks > 0 ) */
}

void CLOG_Merger_last_flush( CLOG_Merger_t *merger )
{
    CLOG_BlockData_t  *sorted_blk;
    CLOG_Rec_Header_t *sorted_hdr;

    sorted_blk = merger->sorted_blk;

    /* Write the CLOG_REC_ENDLOG as the very last record in the sorted buffer */
    sorted_hdr = (CLOG_Rec_Header_t *) sorted_blk->ptr;
    sorted_hdr->time       = CLOG_MAXTIME;    /* use prev record's time */
    sorted_hdr->icomm      = 0;
    sorted_hdr->rank       = merger->local_world_rank;
    sorted_hdr->thread     = 0;
    sorted_hdr->rectype    = CLOG_REC_ENDLOG;

    if ( merger->parent_world_rank != CLOG_RANK_NULL ) {
        PMPI_Ssend( sorted_blk->head, merger->block_size,
                    CLOG_DATAUNIT_MPI_TYPE, merger->parent_world_rank,
                    CLOG_MERGE_LOGBUFTYPE, MPI_COMM_WORLD );
    }
    else { /* if parent_world_rank does not exist, must be the root */
        CLOG_Merger_flush( merger );
    }
}
