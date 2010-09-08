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

#if !defined( CLOG_NOMPI )
#include "mpi.h"
#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

#include "clog_const.h"
#include "clog_mem.h"
#include "clog_timer.h"
#include "clog_util.h"
#include "clog_sync.h"

CLOG_Sync_t *CLOG_Sync_create( int world_size, int world_rank )
{
    CLOG_Sync_t  *sync;

    sync = (CLOG_Sync_t *) MALLOC( sizeof(CLOG_Sync_t) );
    if ( sync == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Sync_create() - \n"
                         "\t""MALLOC() fails for CLOG_Sync_t!\n" );
        fflush( stderr );
        return NULL;
    }

    sync->is_ok_to_sync  = CLOG_BOOL_FALSE;
    sync->root           = 0;
    sync->frequency      = CLOG_SYNC_FREQ_COUNT;
    sync->algorithm_ID   = CLOG_SYNC_AGRM_DEFAULT;
    sync->world_size     = world_size;
    sync->world_rank     = world_rank;

    return sync;
}

void CLOG_Sync_free( CLOG_Sync_t **sync_handle )
{
    CLOG_Sync_t  *sync;

    sync = *sync_handle;
    if ( sync != NULL ) {
        FREE( sync );
    }
    *sync_handle  = NULL;
}

void CLOG_Sync_init( CLOG_Sync_t *sync )
{
    char         *env_sync_agrm;
    char         *env_sync_freq;
    int           local_is_ok_to_sync;

    if ( CLOG_Util_is_MPIWtime_synchronized() == CLOG_BOOL_TRUE )
        local_is_ok_to_sync = CLOG_BOOL_FALSE;
    else
        local_is_ok_to_sync = CLOG_BOOL_TRUE;

    /* If MPE_CLOCKS_SYNC is not set, use the value of MPI_WTIME_IS_GLOBAL  */
    local_is_ok_to_sync = CLOG_Util_getenvbool( "MPE_CLOCKS_SYNC",
                                                local_is_ok_to_sync );
    PMPI_Allreduce( &local_is_ok_to_sync, &(sync->is_ok_to_sync),
                    1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

    env_sync_freq = (char *) getenv( "MPE_SYNC_FREQUENCY" );
    if ( env_sync_freq != NULL ) {
        sync->frequency = atoi( env_sync_freq );
    }
    PMPI_Bcast( &(sync->frequency), 1, MPI_INT,
                sync->root, MPI_COMM_WORLD );

    env_sync_agrm = (char *) getenv( "MPE_SYNC_ALGORITHM" );
    if ( env_sync_agrm != NULL ) {
        if (  strcmp( env_sync_agrm, "DEFAULT" ) == 0
           || strcmp( env_sync_agrm, "default" ) == 0 )
            sync->algorithm_ID = CLOG_SYNC_AGRM_DEFAULT; 
        if (  strcmp( env_sync_agrm, "SEQ" ) == 0
           || strcmp( env_sync_agrm, "seq" ) == 0 )
            sync->algorithm_ID = CLOG_SYNC_AGRM_SEQ; 
        if (  strcmp( env_sync_agrm, "BITREE" ) == 0
           || strcmp( env_sync_agrm, "bitree" ) == 0 )
            sync->algorithm_ID = CLOG_SYNC_AGRM_BITREE; 
        if (  strcmp( env_sync_agrm, "ALTNGBR" ) == 0
           || strcmp( env_sync_agrm, "altngbr" ) == 0 )
            sync->algorithm_ID = CLOG_SYNC_AGRM_ALTNGBR; 
    }
    PMPI_Bcast( &(sync->algorithm_ID), 1, MPI_INT,
                sync->root, MPI_COMM_WORLD );
}



static int CLOG_Sync_ring_rank( int world_size, int root, int adjusting_rank )
{
    if ( adjusting_rank >= root )
        return adjusting_rank - root;
    else
        return adjusting_rank - root + world_size;
}

/*
    A Sequential and non-scalable, O(N), algorithm.

    The root process serially synchronizes with each slave, using the
    first algorithm similar to what quoted in Gropp's "Scalable clock
    synchronization on distributed processors without a common clock".
    The array is calculated on the root and the elements are then scattered
    back to each processes based on rank, local_time_offset.
*/
static CLOG_Time_t CLOG_Sync_run_seq( CLOG_Sync_t *sync )
{
    int           ii, idx;
    CLOG_Time_t   dummytime;
    CLOG_Time_t  *gpofst_pairs;
    CLOG_Time_t   sync_time_start, sync_time_final, sync_time_esti;
    CLOG_Time_t   bestgap, bestshift;
    MPI_Request  *sync_reqs;
    MPI_Status    status;

    dummytime    = 0.0;
    sync_reqs    = NULL;
    gpofst_pairs = NULL;
    if ( sync->world_rank == sync->root ) {
        sync_reqs    = (MPI_Request *) MALLOC( sync->world_size
                                             * sizeof(MPI_Request) );
        gpofst_pairs = (CLOG_Time_t *) MALLOC( 2 * sync->world_size
                                             * sizeof(CLOG_Time_t) );
    }

    PMPI_Barrier( MPI_COMM_WORLD );
    PMPI_Barrier( MPI_COMM_WORLD ); /* approximate common starting point */

    if ( sync->world_rank == sync->root ) {
        /* I am the root node, but not necessarily rank 0 */
        for ( ii = 0; ii < sync->world_size; ii++ ) {
            if ( ii != sync->root ) {
                bestgap    = CLOG_SYNC_LARGEST_GAP;
                bestshift  = 0.0;
                PMPI_Send( &dummytime, 0, CLOG_TIME_MPI_TYPE, ii,
                           CLOG_SYNC_MASTER_READY, MPI_COMM_WORLD );
                PMPI_Recv( &dummytime, 0, CLOG_TIME_MPI_TYPE, ii,
                           CLOG_SYNC_SLAVE_READY, MPI_COMM_WORLD,
                           &status );
                for ( idx = 0; idx < sync->frequency; idx++ ) {
                    sync_time_start = CLOG_Timer_get();
                    PMPI_Send( &dummytime, 1, CLOG_TIME_MPI_TYPE, ii,
                               CLOG_SYNC_TIME_QUERY, MPI_COMM_WORLD );
                    PMPI_Recv( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE, ii,
                               CLOG_SYNC_TIME_ANSWER, MPI_COMM_WORLD,
                               &status );
                    sync_time_final = CLOG_Timer_get();
                    if ( (sync_time_final - sync_time_start) < bestgap ) {
                        bestgap   = sync_time_final - sync_time_start;
                        bestshift = 0.5 * (sync_time_final + sync_time_start)
                                  - sync_time_esti;
                    }
                }
                gpofst_pairs[2*ii]   = bestgap;
                gpofst_pairs[2*ii+1] = bestshift;
            }
            else {   /* ii = root */
                gpofst_pairs[2*ii]   = 0.0;
                gpofst_pairs[2*ii+1] = 0.0;
            }
        }
    }
    else {                        /* not the root */
        PMPI_Recv( &dummytime, 0, CLOG_TIME_MPI_TYPE, sync->root,
                   CLOG_SYNC_MASTER_READY, MPI_COMM_WORLD, &status );
        PMPI_Send( &dummytime, 0, CLOG_TIME_MPI_TYPE, sync->root,
                   CLOG_SYNC_SLAVE_READY, MPI_COMM_WORLD );
        for ( idx = 0; idx < sync->frequency; idx++ ) {
            PMPI_Recv( &dummytime, 1, CLOG_TIME_MPI_TYPE, sync->root,
                       CLOG_SYNC_TIME_QUERY, MPI_COMM_WORLD, &status );
            sync_time_esti = CLOG_Timer_get();
            PMPI_Send( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE, sync->root,
                       CLOG_SYNC_TIME_ANSWER, MPI_COMM_WORLD );
        }
    }

    PMPI_Scatter( gpofst_pairs, 2, CLOG_TIME_MPI_TYPE,
                  sync->best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                  sync->root, MPI_COMM_WORLD );

    if ( sync->world_rank == sync->root ) {
        FREE( gpofst_pairs );
        FREE( sync_reqs );
    }

    return sync->best_gpofst[1];
}

/*
    A scalable, O(log2(N)), algorithm.

    A Binomial-Tree algorithm that does O(log2(N)) steps.
    A scalable but not perfectly parallel algorithm but produces
    reasonable accuracy compared to O(N) algorithm.
*/
static CLOG_Time_t CLOG_Sync_run_bitree( CLOG_Sync_t *sync )
{
    int          prev_adj_rank, next_adj_rank, local_adj_rank;
    int          rank_sep, rank_quot, rank_rem;
    int          gpofst_size, gpofst_idx, idx, ii;
    int          send_cnt, recv_cnt, max_cnt;
    CLOG_Time_t *gpofst_pairs, *gpofst_ptr;
    CLOG_Time_t  best_gp, best_ofst, dummytime;
    CLOG_Time_t  sync_time_start, sync_time_final, sync_time_esti;
    MPI_Status   status;

    PMPI_Barrier( MPI_COMM_WORLD );
    PMPI_Barrier( MPI_COMM_WORLD ); /* approximate common starting point */
    local_adj_rank  = CLOG_Sync_ring_rank( sync->world_size, sync->root,
                                           sync->world_rank );

    dummytime     = 0.0;
    gpofst_size   = 0;
    gpofst_pairs  = NULL;
    if ( local_adj_rank % 2 == 0 ) {
        if ( local_adj_rank == 0 ) /* if ( sync->world_rank == sync->root ) */
            gpofst_size  = 2 * sync->world_size;
        else
            gpofst_size  = sync->world_size;
        gpofst_pairs = (CLOG_Time_t *) MALLOC( gpofst_size
                                             * sizeof(CLOG_Time_t) );
    }

    /* Initialize all the gap variables in gpofst_pairs[ even index ] */
    for ( ii = 0; ii < gpofst_size ; ii+=2 ) {
        gpofst_pairs[ ii ] = CLOG_SYNC_LARGEST_GAP;
    }
    for ( idx = 0; idx < sync->frequency; idx++ ) {
        /* Setup variable for Binomial Tree algorithm */
        rank_sep   = 1;
        rank_quot  = local_adj_rank >> 1; /* rank_quot  = local_adj_rank / 2; */        rank_rem   = local_adj_rank &  1; /* rank_rem   = local_adj_rank % 2; */        gpofst_idx = 0;  /* point at next available index in gpofst_pairs[] */
        while ( rank_sep < sync->world_size ) {
            if ( rank_rem == 0 ) {
                next_adj_rank  = CLOG_Sync_ring_rank( sync->world_size,
                                                      sync->root,
                                                      sync->world_rank
                                                    + rank_sep );
                if ( next_adj_rank < sync->world_size ) {
                    /* Send&Recv ready message */
                    PMPI_Send( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                               next_adj_rank, CLOG_SYNC_MASTER_READY,
                               MPI_COMM_WORLD );
                       /*
                          Set max_cnt to maximum possible receive count
                          as if sync->world_size == power_of_2.
                          - gpofst_pairs[ gpofst_idx .. gpofst_idx+1 ] are for
                            DIRECT gpofst exchange with the remote process.
                          - gpofst_pairs[ gpofst_idx+2 ..
                                          gpofst_idx+2 + recv_cnt-1 ] are for
                            INDIRECT gpofst exchanges with the remote process,
                            i.e. other gpofst exchanges between the remote and
                            other processes(not the local and remote processes).                       */
                    max_cnt  = 2 * ( rank_sep-1 );
                    if ( max_cnt > 0 )
                        gpofst_ptr = &( gpofst_pairs[gpofst_idx+2] );
                    else
                        gpofst_ptr = &dummytime;
                    PMPI_Recv( gpofst_ptr, max_cnt, CLOG_TIME_MPI_TYPE,
                               next_adj_rank, CLOG_SYNC_SLAVE_READY,
                               MPI_COMM_WORLD, &status );
                        /* Set recv_cnt to actual received count of double */
                    PMPI_Get_count( &status, CLOG_TIME_MPI_TYPE, &recv_cnt );
                    /* Send&Recv timestamp  */
                    sync_time_start  = CLOG_Timer_get();
                    PMPI_Send( &dummytime, 1, CLOG_TIME_MPI_TYPE,
                               next_adj_rank, CLOG_SYNC_TIME_QUERY,
                               MPI_COMM_WORLD );
                    PMPI_Recv( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE,
                               next_adj_rank, CLOG_SYNC_TIME_ANSWER,
                               MPI_COMM_WORLD, &status );
                    sync_time_final  = CLOG_Timer_get();
                    if (   (sync_time_final - sync_time_start)
                         < gpofst_pairs[ gpofst_idx ] ) {
                        gpofst_pairs[ gpofst_idx ]   = sync_time_final
                                                     - sync_time_start;
                        gpofst_pairs[ gpofst_idx+1 ] = 0.5 * ( sync_time_final
                                                             + sync_time_start )                                                     - sync_time_esti;
                    }
                    best_gp     = gpofst_pairs[ gpofst_idx ];
                    best_ofst   = gpofst_pairs[ gpofst_idx+1 ];
                    gpofst_idx += 2;
                    /* increment to include the recent exchanges */
                    for ( ii = 0; ii < recv_cnt; ii += 2 ) {
                        gpofst_pairs[ gpofst_idx+ii ]    += best_gp;
                        gpofst_pairs[ gpofst_idx+ii+1 ]  += best_ofst;
                    }
                    gpofst_idx += recv_cnt; /* total increment = recv_cnt + 2 */                }
            }
            else { /* if ( rank_rem != 0 ) */
                prev_adj_rank  = CLOG_Sync_ring_rank( sync->world_size,
                                                      sync->root,
                                                      sync->world_rank
                                                    - rank_sep );
                if ( prev_adj_rank >= 0 ) {
                    /* Recv&Send ready message */
                    PMPI_Recv( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                               prev_adj_rank, CLOG_SYNC_MASTER_READY,
                               MPI_COMM_WORLD, &status );
                        /* Send gpofst_pairs[ 0 ... gpofst_idx-1 ] */
                    send_cnt    = gpofst_idx;
                    gpofst_ptr  = gpofst_pairs;
                    PMPI_Send( gpofst_ptr, send_cnt, CLOG_TIME_MPI_TYPE,
                               prev_adj_rank, CLOG_SYNC_SLAVE_READY,
                               MPI_COMM_WORLD );
                    /* Recv&Send timestamp  */
                    PMPI_Recv( &dummytime, 1, CLOG_TIME_MPI_TYPE,
                               prev_adj_rank, CLOG_SYNC_TIME_QUERY,
                               MPI_COMM_WORLD, &status );
                    sync_time_esti  = CLOG_Timer_get();
                    PMPI_Send( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE,
                               prev_adj_rank, CLOG_SYNC_TIME_ANSWER,
                               MPI_COMM_WORLD );
                    break;
                }
            }   /* endof if ( rank_rem == 0 ) */

            rank_rem    = rank_quot & 1; /* rank_rem   = rank_quot % 2; */
            rank_quot >>= 1;             /* rank_quot /= 2; */
            rank_sep  <<= 1;             /* rank_sep  *= 2; */
        }   /* endof while ( rank_sep < sync->world_size ) */
    }   /* endof for ( idx < sync->frequency ) */

    /* Compute the global time offset based on relative time offset. */
    if ( sync->world_rank == sync->root ) {
        for ( ii = 2*(sync->world_size-1); ii >= 2; ii -= 2 ) {
            gpofst_pairs[ ii   ] = gpofst_pairs[ ii-2 ]; /* gap */
            gpofst_pairs[ ii+1 ] = gpofst_pairs[ ii-1 ]; /* offset */
        }
        gpofst_pairs[ 0 ] = 0.0;
        gpofst_pairs[ 1 ] = 0.0;
    }
    PMPI_Scatter( gpofst_pairs, 2, CLOG_TIME_MPI_TYPE,
                  sync->best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                  sync->root, MPI_COMM_WORLD );

    if ( gpofst_pairs != NULL )
        FREE( gpofst_pairs );

    return sync->best_gpofst[1];
}

/*
    A scalable O(1), perfectly parallel, algorithm.

    An alterating neighbor in a ring algorithm that does constant steps.
    The algorithm is least accurate but perfectly parallel.
*/
static CLOG_Time_t CLOG_Sync_run_altngbr( CLOG_Sync_t *sync )
{
    int          prev_world_rank, next_world_rank;
    int          ii, idx;
    int          iorder, is_odd_rank;
    CLOG_Time_t  dummytime;
    CLOG_Time_t  sync_time_start, sync_time_final, sync_time_esti;
    CLOG_Time_t  bestgap, bestshift;
    CLOG_Time_t *gpofst_pairs, best_gpofst[2];
    CLOG_Time_t  tmp_gp, tmp_ofst, sum_gp, sum_ofst;
    MPI_Status   status;

    dummytime    = 0.0;
    bestshift    = 0.0;

    /* Set neighbors' world rank */
    prev_world_rank = sync->world_rank - 1;
    if ( prev_world_rank < 0 )
        prev_world_rank = sync->world_size - 1;
    next_world_rank = sync->world_rank + 1;
    if ( next_world_rank >= sync->world_size )
        next_world_rank = 0;

    PMPI_Barrier( MPI_COMM_WORLD );
    PMPI_Barrier( MPI_COMM_WORLD ); /* approximate common starting point */

    bestgap = CLOG_SYNC_LARGEST_GAP;
    for ( idx = 0; idx < sync->frequency; idx++ ) {
        for ( iorder  = sync->world_rank;
              iorder <= sync->world_rank+1;
              iorder++ ) {
            is_odd_rank = iorder % 2;
            if ( ! is_odd_rank ) {
                /* Send&Recv ready message */
                PMPI_Send( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                           next_world_rank, CLOG_SYNC_MASTER_READY,
                           MPI_COMM_WORLD );
                PMPI_Recv( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                           next_world_rank, CLOG_SYNC_SLAVE_READY,
                           MPI_COMM_WORLD, &status );
                /* Send&Recv timestamp  */
                sync_time_start  = CLOG_Timer_get();
                PMPI_Send( &dummytime, 1, CLOG_TIME_MPI_TYPE,
                           next_world_rank, CLOG_SYNC_TIME_QUERY,
                           MPI_COMM_WORLD );
                PMPI_Recv( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE,
                           next_world_rank, CLOG_SYNC_TIME_ANSWER,
                           MPI_COMM_WORLD, &status );
                sync_time_final  = CLOG_Timer_get();
                if ( (sync_time_final - sync_time_start) < bestgap ) {
                    bestgap   = sync_time_final - sync_time_start;
                    bestshift = 0.5 * (sync_time_final + sync_time_start)
                              - sync_time_esti;
                }
            }
            else {
                /* Recv&Send ready message */
                PMPI_Recv( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                           prev_world_rank, CLOG_SYNC_MASTER_READY,
                           MPI_COMM_WORLD, &status );
                PMPI_Send( &dummytime, 0, CLOG_TIME_MPI_TYPE,
                           prev_world_rank, CLOG_SYNC_SLAVE_READY,
                           MPI_COMM_WORLD );
                /* Recv&Send timestamp  */
                PMPI_Recv( &dummytime, 1, CLOG_TIME_MPI_TYPE,
                           prev_world_rank, CLOG_SYNC_TIME_QUERY,
                           MPI_COMM_WORLD, &status );
                sync_time_esti  = CLOG_Timer_get();
                PMPI_Send( &sync_time_esti, 1, CLOG_TIME_MPI_TYPE,
                           prev_world_rank, CLOG_SYNC_TIME_ANSWER,
                           MPI_COMM_WORLD );
            }
        }
    }   /* endof for ( idx < sync->frequency ) */

    best_gpofst[0]  = bestgap;
    best_gpofst[1]  = bestshift;
    if ( sync->root != 0 ) {
        /* Poor man implementation of MPI_Scan when root =\= 0 */
        gpofst_pairs = NULL;
        if ( sync->world_rank == sync->root )
            gpofst_pairs = (CLOG_Time_t *) MALLOC( 2 * sync->world_size
                                                 * sizeof(CLOG_Time_t) );
        PMPI_Gather( best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                     gpofst_pairs, 2, CLOG_TIME_MPI_TYPE,
                     sync->root, MPI_COMM_WORLD );
        if ( sync->world_rank == sync->root ) {
            sum_gp    = gpofst_pairs[2*sync->root];
            sum_ofst  = gpofst_pairs[2*sync->root+1];
            gpofst_pairs[2*sync->root]    = 0.0;
            gpofst_pairs[2*sync->root+1]  = 0.0;
            for ( ii = sync->root+1; ii < sync->world_size; ii++ ) {
                tmp_gp                = gpofst_pairs[2*ii];
                tmp_ofst              = gpofst_pairs[2*ii+1];
                gpofst_pairs[2*ii]    = sum_gp;
                gpofst_pairs[2*ii+1]  = sum_ofst;
                sum_gp               += tmp_gp;
                sum_ofst             += tmp_ofst;
            }
            for ( ii = 0; ii < sync->root; ii++ ) {
                tmp_gp                = gpofst_pairs[2*ii];
                tmp_ofst              = gpofst_pairs[2*ii+1];
                gpofst_pairs[2*ii]    = sum_gp;
                gpofst_pairs[2*ii+1]  = sum_ofst;
                sum_gp               += tmp_gp;
                sum_ofst             += tmp_ofst;
            }
        }
        PMPI_Scatter( gpofst_pairs, 2, CLOG_TIME_MPI_TYPE,
                      sync->best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                      sync->root, MPI_COMM_WORLD );
        if ( sync->world_rank == sync->root )
            FREE( gpofst_pairs );
    }
    else {  /* root == 0 */
        PMPI_Send( best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                   next_world_rank, CLOG_SYNC_TIME_QUERY,
                   MPI_COMM_WORLD );
        PMPI_Recv( best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                   prev_world_rank, CLOG_SYNC_TIME_QUERY,
                   MPI_COMM_WORLD, &status );
        if ( sync->world_rank == 0 ) {
            best_gpofst[0] = 0.0;
            best_gpofst[1] = 0.0;
        }
        PMPI_Scan( best_gpofst, sync->best_gpofst, 2, CLOG_TIME_MPI_TYPE,
                   MPI_SUM, MPI_COMM_WORLD );
    }

    return sync->best_gpofst[1];
}

/*@
      CLOG_Sync_run - synchronize clocks with selected algorithm through
                      MPE_SYNC_ALGORITHM

Inout Parameters:
+ sync            - CLOG_Sync_t contains local best_gpofst[2] to be filled in

Return value:
   local time offset after synchronization
@*/
CLOG_Time_t CLOG_Sync_run( CLOG_Sync_t *sync )
{
    switch ( sync->algorithm_ID ) {
        case CLOG_SYNC_AGRM_DEFAULT:
            if ( sync->world_size > 16 )
                return CLOG_Sync_run_bitree( sync );
            else
                return CLOG_Sync_run_seq( sync );
        case CLOG_SYNC_AGRM_SEQ:
            return CLOG_Sync_run_seq( sync );
        case CLOG_SYNC_AGRM_BITREE:
            return CLOG_Sync_run_bitree( sync );
        case CLOG_SYNC_AGRM_ALTNGBR:
            return CLOG_Sync_run_altngbr( sync );
        default:
            if ( sync->world_rank == 0 ) {
                fprintf( stderr, __FILE__":CLOG_Sync_run() - \n"
                                 "Unknown MPE_SYNC_ALGORITHM ID = %d."
                                 "Assume default synchronization mechanism.\n",
                                 sync->algorithm_ID );
                fflush( stderr );
            }
            if ( sync->world_size > 16 )
                return CLOG_Sync_run_bitree( sync );
            else
                return CLOG_Sync_run_seq( sync );
    }
}

const char *CLOG_Sync_print_type( const CLOG_Sync_t *sync )
{
    switch ( sync->algorithm_ID ) {
        case CLOG_SYNC_AGRM_DEFAULT:
            return "Default";
        case CLOG_SYNC_AGRM_SEQ:
            return "Seq";
        case CLOG_SYNC_AGRM_BITREE:
            return "BiTree";
        case CLOG_SYNC_AGRM_ALTNGBR:
            return "AltNgbr";
        default:
            return "Unknown(assume Default)";
    }
}
