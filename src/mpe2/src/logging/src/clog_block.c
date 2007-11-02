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

#include "clog_mem.h"
#include "clog_block.h"
#include "clog_record.h"
/*
    Input parameter, block_size, is asummed to be divisible by sizoef(double)
*/
CLOG_BlockData_t* CLOG_BlockData_create( unsigned int block_size )
{
    CLOG_BlockData_t *data;
    CLOG_DataUnit_t  *ptr;
    int               data_unit_size;

    if ( block_size <= 0 )
        return NULL;

    data = (CLOG_BlockData_t *) MALLOC( sizeof( CLOG_BlockData_t) );
    if ( data == NULL ) {
        fprintf( stderr, __FILE__":CLOG_BlockData_create() - "
                         "MALLOC() fails for CLOG_BlockData_t!\n" );
        fflush( stderr );
        return NULL;
    }

    data->head = (CLOG_DataUnit_t *) MALLOC( block_size );
    if ( data->head == NULL ) {
        fprintf( stderr, __FILE__":CLOG_BlockData_create() - "
                         "MALLOC(%d) fails!\n", block_size );
        fflush( stderr );
        return NULL;
    }

    data_unit_size = block_size / sizeof(CLOG_DataUnit_t);
    data->tail = data->head + data_unit_size;
    data->ptr  = data->head;

    /* Initialize the CLOG_BlockData_t[] to keep valgrind happy */
    ptr  = data->head;
    while ( ptr < data->tail ) {
        *ptr = 0;
        ptr++;
    }

    return data;
}

void CLOG_BlockData_free( CLOG_BlockData_t **data_handle )
{
    CLOG_BlockData_t *data;
    data = *data_handle;
    if ( data != NULL ) {
        if ( data->head != NULL )
            FREE( data->head );
        FREE( data );
    }
    *data_handle = NULL;
}

void CLOG_BlockData_reset( CLOG_BlockData_t *data )
{
    if ( data != NULL )
        data->ptr  = data->head;
}

/*
    local_proc_timediff is modifiable

    This routine updates the CLOG_Rec_Header_t's time and icomm.
    i.e., it updates the header's time based on the timeshift and
    updates the icomm(internal comm ID) to globally(unique) comm ID
    since the commtable[] is assumed to have been merged/sychronized.
*/
void CLOG_BlockData_patch_all(       CLOG_BlockData_t *data,
                                     CLOG_Time_t      *local_proc_timediff,
                               const CLOG_CommIDs_t   *commtable )
{
    CLOG_Rec_Header_t     *hdr;
    CLOG_Rec_Timeshift_t  *tshift;
    CLOG_Rec_CommEvt_t    *comm;
    CLOG_Rec_MsgEvt_t     *msg;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype  = hdr->rectype;
        /*
           For intercomm,
           commtable[ hdr->icomm   ] =        intercommIDs
           commtable[ hdr->icomm+1 ] =  local_intracommIDs
           commtable[ hdr->icomm+2 ] = remote_intracommIDs
        */
        switch (rectype) {
            case CLOG_REC_BAREEVT:
            case CLOG_REC_CARGOEVT:
                /* For intercomm, replace intercommID by local_intracommID */
                if ( commtable[ hdr->icomm ].kind == CLOG_COMM_KIND_INTER )
                    hdr->icomm  = commtable[ hdr->icomm+1 ].local_ID;
                else
                    hdr->icomm  = commtable[ hdr->icomm ].local_ID;
                break;
            case CLOG_REC_MSGEVT:
                msg         = (CLOG_Rec_MsgEvt_t *) hdr->rest;
                /*
                   Since hdr->icomm is the index that points to the
                   correct CLOG_CommIDs_t in commtable[], so fix
                   hdr->icomm last. Otherwise, the location of
                   the correct CLOG_CommIDs_t will be lost.
                */
                if ( commtable[ hdr->icomm ].kind == CLOG_COMM_KIND_INTER ) {
                    msg->icomm  = commtable[ hdr->icomm+2 ].local_ID;
                    hdr->icomm  = commtable[ hdr->icomm+1 ].local_ID;
                }
                else {
                    msg->icomm  = commtable[ hdr->icomm ].local_ID;
                    hdr->icomm  = msg->icomm;
                }
                break;
            case CLOG_REC_COMMEVT:
                comm                  = (CLOG_Rec_CommEvt_t *) hdr->rest;
                /* if icomm == CLOG_COMM_LID_NULL, don't change icomm */
                if ( comm->icomm != CLOG_COMM_LID_NULL )
                    comm->icomm       = commtable[ comm->icomm ].local_ID;
                hdr->icomm   = commtable[ hdr->icomm ].local_ID;
                break;
            case CLOG_REC_TIMESHIFT:
                tshift                = (CLOG_Rec_Timeshift_t *) hdr->rest;
                *local_proc_timediff  = tshift->timeshift;
                tshift->timeshift    *= -1;
                hdr->icomm            = commtable[ hdr->icomm ].local_ID;
                break;
            default:
                hdr->icomm            = commtable[ hdr->icomm ].local_ID;
        }   /* Endof switch (rectype) */
        hdr->time   += *local_proc_timediff;
        hdr          = (CLOG_Rec_Header_t *)
                       ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
    This routine updates the CLOG_Rec_Header_t's icomm (communicator local_ID).
    i.e., it updates the icomm(internal comm ID) to globally(unique) comm ID
    since the commtable[] is assumed to have been merged/sychronized.
*/
void CLOG_BlockData_patch_comm(       CLOG_BlockData_t *data,
                                const CLOG_CommIDs_t   *commtable )
{
    CLOG_Rec_Header_t     *hdr;
    CLOG_Rec_Timeshift_t  *tshift;
    CLOG_Rec_CommEvt_t    *comm;
    CLOG_Rec_MsgEvt_t     *msg;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype  = hdr->rectype;
        /*
           For intercomm,
           commtable[ hdr->icomm   ] =        intercommIDs
           commtable[ hdr->icomm+1 ] =  local_intracommIDs
           commtable[ hdr->icomm+2 ] = remote_intracommIDs
        */
        switch (rectype) {
            case CLOG_REC_BAREEVT:
            case CLOG_REC_CARGOEVT:
                /* For intercomm, replace intercommID by local_intracommID */
                if ( commtable[ hdr->icomm ].kind == CLOG_COMM_KIND_INTER )
                    hdr->icomm  = commtable[ hdr->icomm+1 ].local_ID;
                else
                    hdr->icomm  = commtable[ hdr->icomm ].local_ID;
                break;
            case CLOG_REC_MSGEVT:
                msg         = (CLOG_Rec_MsgEvt_t *) hdr->rest;
                /*
                   Since hdr->icomm is the index that points to the
                   correct CLOG_CommIDs_t in commtable[], so fix
                   hdr->icomm last. Otherwise, the location of
                   the correct CLOG_CommIDs_t will be lost.
                */
                if ( commtable[ hdr->icomm ].kind == CLOG_COMM_KIND_INTER ) {
                    msg->icomm  = commtable[ hdr->icomm+2 ].local_ID;
                    hdr->icomm  = commtable[ hdr->icomm+1 ].local_ID;
                }
                else {
                    msg->icomm  = commtable[ hdr->icomm ].local_ID;
                    hdr->icomm  = msg->icomm;
                }
                break;
            case CLOG_REC_COMMEVT:
                comm                  = (CLOG_Rec_CommEvt_t *) hdr->rest;
                /* if icomm == CLOG_COMM_LID_NULL, don't change icomm */
                if ( comm->icomm != CLOG_COMM_LID_NULL )
                    comm->icomm       = commtable[ comm->icomm ].local_ID;
                hdr->icomm   = commtable[ hdr->icomm ].local_ID;
                break;
            case CLOG_REC_TIMESHIFT:
                tshift                = (CLOG_Rec_Timeshift_t *) hdr->rest;
                hdr->icomm            = commtable[ hdr->icomm ].local_ID;
                break;
            default:
                hdr->icomm            = commtable[ hdr->icomm ].local_ID;
        }   /* Endof switch (rectype) */
        hdr          = (CLOG_Rec_Header_t *)
                       ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

void CLOG_BlockData_patch_time( CLOG_BlockData_t *data,
                                CLOG_Time_t      *local_proc_timediff )
{
    CLOG_Rec_Header_t     *hdr;
    CLOG_Rec_Timeshift_t  *tshift;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype  = hdr->rectype;
        if ( rectype == CLOG_REC_TIMESHIFT ) {
            tshift                = (CLOG_Rec_Timeshift_t *) hdr->rest;
            *local_proc_timediff  = tshift->timeshift;
            tshift->timeshift    *= -1;
        }
        hdr->time   += *local_proc_timediff;
        hdr          = (CLOG_Rec_Header_t *)
                       ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}


/*
   Assume readable(understandable) byte ordering before byteswapping
*/
void CLOG_BlockData_swap_bytes_last( CLOG_BlockData_t *data )
{
    CLOG_Rec_Header_t  *hdr;
    int                 rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype  = hdr->rectype;
        CLOG_Rec_swap_bytes_last( hdr );
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
   Assume non-readable byte ordering before byteswapping
*/
void CLOG_BlockData_swap_bytes_first( CLOG_BlockData_t *data )
{
    CLOG_Rec_Header_t  *hdr;
    int                 rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        CLOG_Rec_swap_bytes_first( hdr );
        rectype  = hdr->rectype;
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
   Assume CLOG_BlockData_t is understandable, no byteswapping is needed.
*/
void CLOG_BlockData_print( CLOG_BlockData_t *data, FILE *stream )
{
    CLOG_Rec_Header_t     *hdr;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        CLOG_Rec_print( hdr, stream );
        rectype  = hdr->rectype;
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}




/*
    Input parameter, block_size, is asummed to be divisible by sizoef(double)
*/
CLOG_Block_t* CLOG_Block_create( unsigned int block_size )
{
    CLOG_Block_t *blk;

    if ( block_size <= 0 )
        return NULL;

    blk = (CLOG_Block_t *) MALLOC( sizeof( CLOG_Block_t ) );
    if ( blk == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Block_create() - "
                         "MALLOC() fails for CLOG_Block_t!\n" );
        fflush( stderr );
        return NULL;
    }

    blk->data = CLOG_BlockData_create( block_size );
    if ( blk->data == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Block_create() - "
                         "CLOG_BlockData_create(%d) fails!",
                         block_size );
        fflush( stderr );
        return NULL;
    }
    blk->next      = NULL;

    return blk;
}

void CLOG_Block_free( CLOG_Block_t **blk_handle )
{
    CLOG_Block_t *blk;
    blk = *blk_handle;
    if ( blk != NULL ) {
        CLOG_BlockData_free( &(blk->data) );
        FREE( blk );
    }
    *blk_handle = NULL;
}

void CLOG_Block_reset( CLOG_Block_t *block )
{
    if ( block != NULL )
        CLOG_BlockData_reset( block->data );
}
