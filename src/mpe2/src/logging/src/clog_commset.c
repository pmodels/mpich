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
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#if !defined( CLOG_NOMPI )

#if !defined( HAVE_PMPI_COMM_CREATE_KEYVAL )
#define PMPI_Comm_create_keyval PMPI_Keyval_create
#endif

#if !defined( HAVE_PMPI_COMM_FREE_KEYVAL )
#define PMPI_Comm_free_keyval PMPI_Keyval_free
#endif

#if !defined( HAVE_PMPI_COMM_SET_ATTR )
#define PMPI_Comm_set_attr PMPI_Attr_put
#endif

#if !defined( HAVE_PMPI_COMM_GET_ATTR )
#define PMPI_Comm_get_attr PMPI_Attr_get
#endif

#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */


#include "clog_const.h"
#include "clog_util.h"
#include "clog_mem.h"
#include "clog_uuid.h"
#include "clog_commset.h"




CLOG_CommSet_t* CLOG_CommSet_create( void )
{
    CLOG_CommSet_t  *commset;
    int              table_size;

    commset = (CLOG_CommSet_t *) MALLOC( sizeof(CLOG_CommSet_t) );
    if ( commset == NULL ) {
        fprintf( stderr, __FILE__":CLOG_CommSet_create() - \n"
                         "\tMALLOC() fails for CLOG_CommSet_t!\n" );
        fflush( stderr );
        return NULL;
    }

    /* LID_key Initialized to 'unallocated' */
    commset->LID_key   = MPI_KEYVAL_INVALID;
    commset->max       = CLOG_COMM_TABLE_INCRE;
    commset->count     = 0;
    table_size         = commset->max * sizeof(CLOG_CommIDs_t);
    commset->table     = (CLOG_CommIDs_t *) MALLOC( table_size );
    if ( commset->table == NULL ) {
        FREE( commset );
        fprintf( stderr, __FILE__":CLOG_CommSet_create() - \n"
                         "\tMALLOC() fails for CLOG_CommSet_t.table[]!\n" );
        fflush( stderr );
        return NULL;
    }
    /*
       memset() to set all alignment space in commset->table.
       This is done to keep valgrind happy on 64bit machine.
    */
    memset( commset->table, 0, table_size );

    /* Initialize */
    commset->IDs4world = &(commset->table[0]);
    commset->IDs4self  = &(commset->table[1]);

    return commset;
}

void CLOG_CommSet_free( CLOG_CommSet_t **comm_handle )
{
    CLOG_CommSet_t  *commset;

    commset = *comm_handle;
    if ( commset != NULL ) {
        if ( commset->table != NULL )
            FREE( commset->table );
        if ( commset->LID_key != MPI_KEYVAL_INVALID )
            PMPI_Comm_free_keyval( &(commset->LID_key) );
        FREE( commset );
    }
    *comm_handle = NULL;
}



static
CLOG_CommIDs_t* CLOG_CommSet_get_new_IDs( CLOG_CommSet_t *commset, int num_IDs )
{
    CLOG_CommIDs_t  *new_table;
    CLOG_CommIDs_t  *new_entry;
    int              new_size;
    int              old_size, old_max;
    int              idx;

    /* Enlarge the CLOG_CommSet_t.table if necessary */
    if ( commset->count + num_IDs > commset->max ) {
        old_max    = commset->max;
        do {
            commset->max += CLOG_COMM_TABLE_INCRE;
        } while ( commset->count + num_IDs > commset->max );
        new_size   = commset->max * sizeof(CLOG_CommIDs_t);
        new_table  = (CLOG_CommIDs_t *) REALLOC( commset->table, new_size );
        if ( new_table == NULL ) {
            fprintf( stderr, __FILE__":CLOG_CommSet_get_next_IDs() - \n"
                             "\t""REALLOC(%p,%d) fails!\n",
                             commset->table, new_size );
            fflush( stderr );
            CLOG_Util_abort( 1 );
        }
        /* memset() newly allocated data to keep valgrind happy on 64bit box */
        old_size   = old_max * sizeof(CLOG_CommIDs_t);
        memset( &(new_table[old_max]), 0, new_size - old_size );

        commset->table     = new_table;
        /* Update commset->IDs4xxxx after successfully REALLOC() */
        commset->IDs4world = &(commset->table[0]);
        commset->IDs4self  = &(commset->table[1]);
    }

    new_entry  = &( commset->table[commset->count] );
    for ( idx = 0; idx < num_IDs; idx++ ) {
        /* Set the local Comm ID temporarily equal to the table entry's index */
        new_entry->local_ID   = commset->count + idx;

        /* set the new entry with the process rank in MPI_COMM_WORLD */
        new_entry->world_rank = commset->world_rank;

        /*
           Since most commIDs requested is intracomms, no related commIDs:
           i.e. set the related CLOG_CommIDs_t* as NULL
        */
        new_entry->next      = NULL;
        
        new_entry++;  /* increment to next CLOG_CommIDs_t in the table[] */
    }

    /* return the leading available table entry in CLOG_CommSet_t */
    new_entry  = &( commset->table[commset->count] );
    /*
       Increment the count to next available slot in the table.
       Also, the count indicates the current used slot in the table.
    */
    commset->count += num_IDs;

    return new_entry;
}

static
const CLOG_CommIDs_t* CLOG_CommTable_get(       int             table_count,
                                          const CLOG_CommIDs_t *table,
                                          const CLOG_CommGID_t  commgid )
{
    int              idx;

    for ( idx = 0; idx < table_count; idx++ ) {
        if ( CLOG_Uuid_compare( table[idx].global_ID, commgid ) == 0 )
            return &( table[idx] );
    }
    return NULL;
}

static
CLOG_CommIDs_t* CLOG_CommSet_add_new_GID(       CLOG_CommSet_t *commset,
                                          const CLOG_CommGID_t  commgid )
{
    CLOG_CommIDs_t  *commIDs;

    /* Update the next available table entry in CLOG_CommSet_t */
    commIDs              = CLOG_CommSet_get_new_IDs( commset, 1 );
    commIDs->kind        = CLOG_COMM_KIND_UNKNOWN;

    /* Set the global Comm ID */
    CLOG_Uuid_copy( commgid, commIDs->global_ID );

    /* Set the Comm field's */
    commIDs->comm        = MPI_COMM_NULL;
    commIDs->comm_rank   = -1;

    return commIDs;
}

void CLOG_CommSet_add_GID(       CLOG_CommSet_t *commset,
                           const CLOG_CommGID_t  commgid )
{
    const CLOG_CommIDs_t  *commIDs;

    /* If commgid is not in commset, add commgid as a new entry in commset. */
    commIDs = CLOG_CommTable_get( commset->count, commset->table, commgid );
    if ( commIDs == NULL )
        CLOG_CommSet_add_new_GID( commset, commgid );
}

/* Append all child_table[]'s global_IDs to the parent_commset's table[]. */
void CLOG_CommSet_append_GIDs(       CLOG_CommSet_t *parent_commset,
                                     int             child_table_count,
                               const CLOG_CommIDs_t *child_table ) 
{
    int              idx;

    /*
       If child_table[ idx ].global_ID is not in parent_commset,
       add child_table[ idx ].global_ID to parent_commset
    */
    for ( idx = 0; idx < child_table_count; idx++ ) {
        CLOG_CommSet_add_GID( parent_commset, child_table[idx].global_ID );
    }
}

/*
   Synchronize the local_IDs of the parent_commset's table[] and that of
   the child_table[] for the same CLOG_Uuid_t's.  This function does modify
   the content of parent_commset's table[] but does not enlarge the size of
   parent_commset's table[].
*/
CLOG_BOOL_T CLOG_CommSet_sync_IDs(       CLOG_CommSet_t *parent_commset,
                                         int             child_table_count,
                                   const CLOG_CommIDs_t *child_table )
{
    const CLOG_CommIDs_t  *commIDs;
          int              idx;

    /* Update the local_ID in CLOG_CommSet_t's table to globally unique ID */
    for ( idx = 0; idx < parent_commset->count; idx++ ) {
        commIDs  = CLOG_CommTable_get( child_table_count, child_table,
                                       parent_commset->table[idx].global_ID );
        if ( commIDs == NULL ) {
            char uuid_str[CLOG_UUID_STR_SIZE] = {0};
            CLOG_Uuid_sprint( parent_commset->table[idx].global_ID, uuid_str );
            fprintf( stderr, __FILE__":CLOG_CommSet_sync_IDs() - \n"
                             "\t""The parent CLOG_CommSet_t does not contain "
                             "%d-th GID %s in the child_table[]/n",
                             idx, uuid_str );
            fflush( stderr );
            return CLOG_BOOL_FALSE;
        }
        /* if commIDs != NULL */
        parent_commset->table[idx].local_ID = commIDs->local_ID;
    }

    return CLOG_BOOL_TRUE;
}



/*
   CLOG_CommSet_init() should only be called if MPI is involved.
*/
void CLOG_CommSet_init( CLOG_CommSet_t *commset )
{
    int          *extra_state = NULL; /* unused variable */

    CLOG_Uuid_init();

    /* create LID_Key */
    PMPI_Comm_create_keyval( MPI_COMM_NULL_COPY_FN,
                             MPI_COMM_NULL_DELETE_FN, 
                             &(commset->LID_key), extra_state );

    PMPI_Comm_size( MPI_COMM_WORLD, &(commset->world_size) );
    PMPI_Comm_rank( MPI_COMM_WORLD, &(commset->world_rank) );

    CLOG_CommSet_add_intracomm( commset, MPI_COMM_WORLD );
    CLOG_CommSet_add_intracomm( commset, MPI_COMM_SELF );

    commset->IDs4world = &(commset->table[0]);
    commset->IDs4self  = &(commset->table[1]);
}

/*
    This function should be used on every newly created communicator
    The input argument MPI_Comm is assumed to be not MPI_COMM_NULL
*/
const CLOG_CommIDs_t *CLOG_CommSet_add_intracomm( CLOG_CommSet_t *commset,
                                                  MPI_Comm        intracomm )
{
    CLOG_CommIDs_t  *intracommIDs;

    /* Update the next available table entry in CLOG_CommSet_t */
    intracommIDs         = CLOG_CommSet_get_new_IDs( commset, 1 );
    intracommIDs->kind   = CLOG_COMM_KIND_INTRA;

    /* Set the input MPI_Comm's LID_key attribute with new local CommID's LID */
    /*
       Use CLOG_Pint, sizeof(CLOG_Pint) = sizeof(void *),
       instead of MPI_Aint (MPI_Aint >= CLOG_Pint)
    */
    PMPI_Comm_set_attr( intracomm, commset->LID_key,
                        (void *) (CLOG_Pint) intracommIDs->local_ID );

    /* Set the Comm field */
    intracommIDs->comm   = intracomm;
    PMPI_Comm_rank( intracommIDs->comm, &(intracommIDs->comm_rank) );

    /* Set the global Comm ID */
    if ( intracommIDs->comm_rank == 0 )
        CLOG_Uuid_generate( intracommIDs->global_ID );
    PMPI_Bcast( intracommIDs->global_ID, CLOG_UUID_SIZE, MPI_CHAR,
                0, intracomm );
#if defined( CLOG_COMMSET_PRINT )
    char uuid_str[CLOG_UUID_STR_SIZE] = {0};
    CLOG_Uuid_sprint( intracommIDs->global_ID, uuid_str );
    fprintf( stdout, "comm_rank=%d, comm_global_ID=%s\n",
                     intracommIDs->comm_rank, uuid_str );
#endif

    return intracommIDs;
}

/*
    Assume that "intracomm" be the LOCAL communicator of the "intercomm".
*/
const CLOG_CommIDs_t*
CLOG_CommSet_add_intercomm(       CLOG_CommSet_t *commset,
                                  MPI_Comm        intercomm,
                            const CLOG_CommIDs_t *intracommIDs )
{
    CLOG_CommIDs_t  *intercommIDs;
    CLOG_CommIDs_t  *local_intracommIDs, *remote_intracommIDs;
    CLOG_CommIDs_t  *orig_intracommIDs, intracommIDs_val;
    MPI_Status       status;
    MPI_Request      request;
    int              is_intercomm;

    /* Confirm if input intercomm is really an intercommunicator */
    PMPI_Comm_test_inter( intercomm, &is_intercomm );
    if ( !is_intercomm )
        return CLOG_CommSet_add_intracomm( commset, intercomm );

    /*
       Since CLOG_CommSet_get_new_IDs() may call realloc()
       which may invalidate any CLOG_CommIDs_t pointer,
       copy the content of input intracommIDs pointer to a local buffer.
    */
    orig_intracommIDs = &intracommIDs_val;
    memcpy( orig_intracommIDs, intracommIDs, sizeof(CLOG_CommIDs_t) );

    /* Set the next available table entry in CLOG_CommSet_t with intercomm */
    intercommIDs         = CLOG_CommSet_get_new_IDs( commset, 3 );
    intercommIDs->kind   = CLOG_COMM_KIND_INTER;

    /* Set the input MPI_Comm's LID_key attribute with new local CommID */
    /*
       Use CLOG_Pint, sizeof(CLOG_Pint) = sizeof(void *),
       instead of MPI_Aint (MPI_Aint >= CLOG_Pint)
    */
    PMPI_Comm_set_attr( intercomm, commset->LID_key,
                        (void *) (CLOG_Pint) intercommIDs->local_ID );

    /* Set the Comm field with the intercomm's info */
    intercommIDs->comm   = intercomm;
    PMPI_Comm_rank( intercommIDs->comm, &(intercommIDs->comm_rank) );

    /* Set the global Comm ID based on intercomm's local group rank */
    if ( intercommIDs->comm_rank == 0 )
        CLOG_Uuid_generate( intercommIDs->global_ID );

    /*
       Broadcast the (local side of) intercomm ID within local intracomm,
       i.e. orig_intracommIDs->comm
    */
    PMPI_Bcast( intercommIDs->global_ID, CLOG_UUID_SIZE, MPI_CHAR,
                0, orig_intracommIDs->comm );

#if defined( CLOG_COMMSET_PRINT )
    char uuid_str[CLOG_UUID_STR_SIZE] = {0};
    CLOG_Uuid_sprint( intercommIDs->global_ID, uuid_str );
    fprintf( stdout, "comm_rank=%d, comm_global_ID=%s\n",
                     intercommIDs->comm_rank, uuid_str );
#endif

    /* Set the next available table entry with the LOCAL intracomm's info */
    local_intracommIDs            = intercommIDs + 1;
    local_intracommIDs->kind      = CLOG_COMM_KIND_LOCAL;
    local_intracommIDs->local_ID  = orig_intracommIDs->local_ID;
    CLOG_Uuid_copy( orig_intracommIDs->global_ID,
                    local_intracommIDs->global_ID );
    local_intracommIDs->comm      = orig_intracommIDs->comm;
    local_intracommIDs->comm_rank = orig_intracommIDs->comm_rank;
    /* NOTE: LOCAL intracommIDs->comm_rank == intercommIDs->comm_rank */

    /* Set the next available table entry with the REMOTE intracomm's info */
    remote_intracommIDs           = intercommIDs + 2;
    remote_intracommIDs->kind     = CLOG_COMM_KIND_REMOTE;
    /*
       Broadcast local_intracommIDs's GID to everyone in remote_intracomm, i.e.
       Send local_intracommIDs' GID from the root of local intracomms to the
       root of the remote intracomms and save it as remote_intracommIDs' GID.
       Then broadcast the received GID to the rest of intracomm.
    */
    if ( intercommIDs->comm_rank == 0 ) {
       PMPI_Irecv( remote_intracommIDs->global_ID, CLOG_UUID_SIZE, MPI_CHAR,
                   0, 9999, intercomm, &request );            
       PMPI_Send( local_intracommIDs->global_ID, CLOG_UUID_SIZE, MPI_CHAR,
                  0, 9999, intercomm );
       PMPI_Wait( &request, &status );
    }
    PMPI_Bcast( remote_intracommIDs->global_ID, CLOG_UUID_SIZE, MPI_CHAR,
                0, orig_intracommIDs->comm );
    /*
       Since REMOTE intracomm is NOT known or undefinable in LOCAL intracomm,
       (that is why we have intercomm).  Set comm and comm_rank to NULL
    */
    remote_intracommIDs->comm       = MPI_COMM_NULL;
    remote_intracommIDs->comm_rank  = -1;

    /* Set the related CLOG_CommIDs_t* to the local and remote intracommIDs */
    intercommIDs->next              = local_intracommIDs;
    local_intracommIDs->next        = remote_intracommIDs;

    return intercommIDs;
}



/*
    The input argument MPI_Comm is assumed not to be MPI_COMM_NULL
*/
CLOG_CommLID_t CLOG_CommSet_get_LID( CLOG_CommSet_t *commset, MPI_Comm comm )
{
    CLOG_Pint  ptrlen_value;
    int        istatus;

    PMPI_Comm_get_attr( comm, commset->LID_key, &ptrlen_value, &istatus );
    if ( !istatus ) {
        fprintf( stderr, __FILE__":CLOG_CommSet_get_LID() - \n"
                         "\t""PMPI_Comm_get_attr() fails!\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
    return (CLOG_CommLID_t) ptrlen_value;
}

const CLOG_CommIDs_t* CLOG_CommSet_get_IDs( CLOG_CommSet_t *commset,
                                            MPI_Comm comm )
{
    CLOG_Pint  ptrlen_value;
    int        istatus;

    PMPI_Comm_get_attr( comm, commset->LID_key, &ptrlen_value, &istatus );
    if ( !istatus ) {
        fprintf( stderr, __FILE__":CLOG_CommSet_get_IDs() - \n"
                         "\t""PMPI_Comm_get_attr() fails!\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
    return &( commset->table[(CLOG_CommLID_t) ptrlen_value] );
}


void CLOG_CommSet_merge( CLOG_CommSet_t *commset )
{
    int              comm_world_size, comm_world_rank;
    int              rank_sep, rank_quot, rank_rem;
    int              rank_src, rank_dst;
    int              recv_table_count, recv_table_size;
    CLOG_CommIDs_t  *recv_table;
    MPI_Status       status;

    comm_world_rank  = commset->world_rank;
    comm_world_size  = commset->world_size;

    /* Collect CLOG_CommIDs_t table through Recursive Doubling algorithm */
    rank_sep   = 1;
    rank_quot  = comm_world_rank >> 1; /* rank_quot  = comm_world_rank / 2; */
    rank_rem   = comm_world_rank &  1; /* rank_rem   = comm_world_rank % 2; */

    while ( rank_sep < comm_world_size ) {
        if ( rank_rem == 0 ) {
            rank_src = comm_world_rank + rank_sep;
            if ( rank_src < comm_world_size ) {
                /* Recv from rank_src */
                PMPI_Recv( &recv_table_count, 1, MPI_INT, rank_src,
                           CLOG_COMM_TAG_START, MPI_COMM_WORLD, &status );
                recv_table_size  = recv_table_count * sizeof(CLOG_CommIDs_t);
                recv_table  = (CLOG_CommIDs_t *) MALLOC( recv_table_size );
                if ( recv_table == NULL ) {
                    fprintf( stderr, __FILE__":CLOG_CommSet_merge() - \n"
                                     "\t""MALLOC(%d) fails!\n",
                                     recv_table_size );
                    fflush( stderr );
                    CLOG_Util_abort( 1 );
                }
                /*
                   For simplicity, receive commset's whole table and uses only
                   CLOG_CommGID_t column from the table.  The other columns
                   are relevant only to the sending process.
                */
                PMPI_Recv( recv_table, recv_table_size, MPI_CHAR, rank_src,
                           CLOG_COMM_TAG_START+1, MPI_COMM_WORLD, &status );

                /* Append all global_IDs in the recv_table to commset's */
                CLOG_CommSet_append_GIDs( commset,
                                          recv_table_count, recv_table );
                if ( recv_table != NULL ) {
                    FREE( recv_table );
                    recv_table = NULL;
                }
            }
        }
        else /* if ( rank_rem != 0 ) */ {
            /* After sending CLOG_CommIDs_t table, the process does a barrier */
            rank_dst = comm_world_rank - rank_sep;
            if ( rank_dst >= 0 ) {
                recv_table_count  = commset->count;
                /* Send from rank_dst */
                PMPI_Send( &recv_table_count, 1, MPI_INT, rank_dst,
                           CLOG_COMM_TAG_START, MPI_COMM_WORLD );
                /*
                   For simplicity, send commset's whole table including
                   useless things even though only CLOG_CommGID_t column
                   will be used from the table in the receiving process.
                */
                recv_table_size  = recv_table_count * sizeof(CLOG_CommIDs_t);
                PMPI_Send( commset->table, recv_table_size, MPI_CHAR, rank_dst,
                           CLOG_COMM_TAG_START+1, MPI_COMM_WORLD );
                break;  /* get out of the while loop */
            }
        }

        rank_rem    = rank_quot & 1; /* rank_rem   = rank_quot % 2; */
        rank_quot >>= 1;             /* rank_quot /= 2; */
        rank_sep  <<= 1;             /* rank_sep  *= 2; */
    }   /* endof while ( rank_sep < comm_world_size ) */

    /*
       Synchronize everybody in MPI_COMM_WORLD
       before broadcasting result back to everybody.
    */
    PMPI_Barrier( MPI_COMM_WORLD );

    if ( comm_world_rank == 0 )
        recv_table_count  = commset->count;
    else
        recv_table_count  = 0;
    PMPI_Bcast( &recv_table_count, 1, MPI_INT, 0, MPI_COMM_WORLD );

    /* Allocate a buffer for root process's commset->table */
    recv_table_size  = recv_table_count * sizeof(CLOG_CommIDs_t);
    recv_table  = (CLOG_CommIDs_t *) MALLOC( recv_table_size );
    if ( recv_table == NULL ) {
        fprintf( stderr, __FILE__":CLOG_CommSet_merge() - \n"
                         "\t""MALLOC(%d) fails!\n", recv_table_size );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
    /*
       memset() to set all alignment space in commset->table.
       This is done to keep valgrind happy on 64bit machine.
       memset( recv_table, 0, recv_table_size );
    */

    if ( comm_world_rank == 0 )
        memcpy( recv_table, commset->table, recv_table_size );
    PMPI_Bcast( recv_table, recv_table_size, MPI_CHAR, 0, MPI_COMM_WORLD );

    /* Make local_IDs in CLOG_CommSet_t's table[] to globally unique integers */
    if (    CLOG_CommSet_sync_IDs( commset, recv_table_count, recv_table )
         != CLOG_BOOL_TRUE ) {
        fprintf( stderr, __FILE__":CLOG_CommSet_merge() - \n"
                         "\t""CLOG_CommSet_sync_IDs() fails!\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
    if ( recv_table != NULL ) {
        FREE( recv_table );
        recv_table  = NULL;
    }

    PMPI_Barrier( MPI_COMM_WORLD );
}

static void CLOG_CommRec_swap_bytes( char *commrec )
{
    char  *ptr;

    ptr  = commrec;
    CLOG_Uuid_swap_bytes( ptr );
    ptr += CLOG_UUID_SIZE;
    CLOG_Util_swap_bytes( ptr, sizeof(CLOG_CommLID_t), 1 );
    ptr += sizeof(CLOG_CommLID_t);
    CLOG_Util_swap_bytes( ptr, sizeof(CLOG_int32_t), 1 );
    ptr += sizeof(CLOG_int32_t);
}

static void CLOG_CommRec_print_kind( CLOG_int32_t comm_kind, FILE *stream )
{
    switch (comm_kind) {
        case CLOG_COMM_KIND_INTER:
            fprintf( stream, "InterComm " );
            break;
        case CLOG_COMM_KIND_INTRA:
            fprintf( stream, "IntraComm " );
            break;
        case CLOG_COMM_KIND_LOCAL:
            fprintf( stream, "LocalComm " );
            break;
        case CLOG_COMM_KIND_REMOTE:
            fprintf( stream, "RemoteComm" );
            break;
        default:
            fprintf( stream, "Unknown("i32fmt")", comm_kind );
    }
}

/*
   If succeeds, returns the number of CLOG_CommIDs_t in CLOG_CommSet_t, ie >= 0.
   If fails, returns -1;
*/
int CLOG_CommSet_write( const CLOG_CommSet_t *commset, int fd,
                              CLOG_BOOL_T     do_byte_swap )
{
    char            *local_buffer, *ptr;
    CLOG_int32_t     count_buffer;
    CLOG_CommIDs_t  *commIDs;
    int              commrec_size, buffer_size;
    int              ierr, idx;

    count_buffer   = commset->count;
    /* Swap bytes of the "count" buffer BEFORE writting to disk */
    if ( do_byte_swap == CLOG_BOOL_TRUE )
        CLOG_Util_swap_bytes( &count_buffer, sizeof(CLOG_int32_t), 1 );

    ierr = write( fd, &count_buffer, sizeof(CLOG_int32_t) );
    if ( ierr != sizeof(CLOG_int32_t) )
        return -1;

    commrec_size = CLOG_UUID_SIZE + sizeof(CLOG_CommLID_t)
                 + sizeof(CLOG_int32_t);
    buffer_size  = commrec_size * commset->count;
    local_buffer = (char *) MALLOC( buffer_size );

    ptr  = local_buffer;
    /* Save the CLOG_CommIDs_t[] in the order the entries were created */
    for ( idx = 0; idx < commset->count; idx++ ) {
        commIDs = &( commset->table[idx] );
        memcpy( ptr, commIDs->global_ID, CLOG_UUID_SIZE );
        ptr += CLOG_UUID_SIZE;
        memcpy( ptr, &(commIDs->local_ID), sizeof(CLOG_CommLID_t) );
        ptr += sizeof(CLOG_CommLID_t);
        memcpy( ptr, &(commIDs->kind), sizeof(CLOG_int32_t) );
        ptr += sizeof(CLOG_int32_t);
    }

    /* Swap bytes of the content buffer BEFORE writting to disk */
    if ( do_byte_swap == CLOG_BOOL_TRUE ) {
        ptr  = local_buffer;
        for ( idx = 0; idx < commset->count; idx++ ) {
            CLOG_CommRec_swap_bytes( ptr );
            ptr += commrec_size;
        }
    }

    ierr = write( fd, local_buffer, buffer_size );
    if ( ierr != buffer_size )
        return -1;

    if ( local_buffer != NULL )
        FREE( local_buffer );

    return (int) commset->count;
}

/*
   If succeeds, returns the number of CLOG_CommIDs_t in CLOG_CommSet_t, ie >= 0.
   If fails, returns -1;
*/
int CLOG_CommSet_read( CLOG_CommSet_t *commset, int fd,
                       CLOG_BOOL_T     do_byte_swap )
{
    char            *local_buffer, *ptr;
    CLOG_int32_t     count_buffer;
    CLOG_CommIDs_t  *commIDs;
    int              commrec_size, buffer_size;
    int              ierr, idx;

    ierr = read( fd, &count_buffer, sizeof(CLOG_int32_t) );
    if ( ierr != sizeof(CLOG_int32_t) )
        return -1;
    /* Swap bytes of the "count" buffer AFTER reading from disk */
    if ( do_byte_swap == CLOG_BOOL_TRUE )
        CLOG_Util_swap_bytes( &count_buffer, sizeof(CLOG_int32_t), 1 );

    commrec_size = CLOG_UUID_SIZE + sizeof(CLOG_CommLID_t)
                 + sizeof(CLOG_int32_t);
    buffer_size  = count_buffer * commrec_size;
    local_buffer = (char *) MALLOC( buffer_size );

    ierr = read( fd, local_buffer, buffer_size );
    if ( ierr != buffer_size )
        return -1;

    /* Swap bytes before reading into memory */
    if ( do_byte_swap == CLOG_BOOL_TRUE ) {
        ptr  = local_buffer;
        for ( idx = 0; idx < count_buffer; idx++ ) {
            CLOG_CommRec_swap_bytes( ptr );
            ptr += commrec_size;
        }
    }

    ptr  = local_buffer;
    /* Resurrect the CLOG_CommIDs_t[] in the order the entries were created */
    for ( idx = 0; idx < count_buffer; idx++ ) {
        commIDs = CLOG_CommSet_add_new_GID( commset, ptr );
        ptr += CLOG_UUID_SIZE;
        commIDs->local_ID = *( (CLOG_CommLID_t *) ptr );
        ptr += sizeof(CLOG_CommLID_t);
        commIDs->kind     = *( (CLOG_int32_t *) ptr );
        ptr += sizeof(CLOG_int32_t);
    }

    if ( local_buffer != NULL )
        FREE( local_buffer );

    return count_buffer;
}

void CLOG_CommSet_print( CLOG_CommSet_t *commset, FILE *stream )
{
    CLOG_CommIDs_t *commIDs;
    char            uuid_str[CLOG_UUID_STR_SIZE] = {0};
    int             idx;

    for ( idx = 0; idx < commset->count; idx++ ) {
        commIDs = &( commset->table[idx] );
        CLOG_Uuid_sprint( commIDs->global_ID, uuid_str );
        fprintf( stream, "GID=%s ", uuid_str );
        fprintf( stream, "LID="i32fmt" ", commIDs->local_ID );
        fprintf( stream, "kind=" );
        CLOG_CommRec_print_kind( commIDs->kind, stream );
        fprintf( stream, "\n" );
    }
}
