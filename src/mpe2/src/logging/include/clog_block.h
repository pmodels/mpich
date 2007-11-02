/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_BLOCK )
#define _CLOG_BLOCK

#include "clog_timer.h"
#include "clog_commset.h"

#define CLOG_DataUnit_t          char
#define CLOG_DATAUNIT_MPI_TYPE   MPI_CHAR


/*
   Note that CLOG_BlockData_t are actually a little longer than
   CLOG_BLOCK_SIZE, which is the size of CLOG_BlockData_t.
*/

typedef struct {
    CLOG_DataUnit_t    *head;    /* beginning of the data block */
    CLOG_DataUnit_t    *tail;    /* end of the data block */
    CLOG_DataUnit_t    *ptr;     /* next available spot in the data block */
} CLOG_BlockData_t;

CLOG_BlockData_t* CLOG_BlockData_create( unsigned int block_size );

void  CLOG_BlockData_free( CLOG_BlockData_t **data_handle );

void CLOG_BlockData_reset( CLOG_BlockData_t *data );

void CLOG_BlockData_patch_all(       CLOG_BlockData_t *data,
                                     CLOG_Time_t      *local_proc_timediff,
                               const CLOG_CommIDs_t   *commtable );

void CLOG_BlockData_patch_comm(       CLOG_BlockData_t *data,
                                const CLOG_CommIDs_t   *commtable );

void CLOG_BlockData_patch_time( CLOG_BlockData_t *data,
                                CLOG_Time_t      *local_proc_timediff );

void CLOG_BlockData_swap_bytes_last( CLOG_BlockData_t *data );

void CLOG_BlockData_swap_bytes_first( CLOG_BlockData_t *data );

void CLOG_BlockData_print( CLOG_BlockData_t *data, FILE *stream );

typedef struct _CLOG_Block_t {
    CLOG_BlockData_t      *data;
    struct _CLOG_Block_t  *next;   /* next block */
} CLOG_Block_t;

CLOG_Block_t* CLOG_Block_create( unsigned int block_size );

void CLOG_Block_free( CLOG_Block_t **blk_handle );

void CLOG_Block_reset( CLOG_Block_t *block );

#endif /* of _CLOG_BLOCK */
