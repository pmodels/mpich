/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>
#include "queue.h"

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

#include "coll_progress_util.h"
#include "coll_schedule_tree.h"
#include "coll_args_generic.h"

/* Initialize collective algorithm */
MPL_STATIC_INLINE_PREFIX int COLL_init()
{
    return 0;
}

/* Initialize algorithm specific communicator */
MPL_STATIC_INLINE_PREFIX int COLL_comm_init(COLL_comm_t * comm, int *tag_ptr, int rank, int comm_size)
{
    int k;

    /* initialize transport communicator */
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));

    /* generate a knomial tree (tree_type=0) by default */
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    k = 2; /* Use 2 by default */

    COLL_tree_init(rank, comm_size, 0, k, 0, &mycomm->tree);

    comm->tree_comm.curTag = tag_ptr;
    return 0;
}

/* Initialize algorithm specific communicator data to NULL */
MPL_STATIC_INLINE_PREFIX int COLL_comm_init_null(COLL_comm_t * comm)
{
    /* initialize transport communicator */
    TSP_comm_init_null(&comm->tsp_comm, COLL_COMM_BASE(comm));

    comm->tree_comm.tree.max_children = 0;

    return 0;
}

/* Cleanup algorithm communicator */
MPL_STATIC_INLINE_PREFIX int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    COLL_tree_free (&comm->tree_comm.tree);

    return 0;
}


#undef FUNCNAME
#define FUNCNAME COLL_bcast_get_tree_schedule
/* Internal function that returns broadcast schedule. This is used by COLL_bcast and COLL_ibcast
 * to get the broadcast schedule */
MPL_STATIC_INLINE_PREFIX int COLL_bcast_get_tree_schedule(void *buffer, int count, COLL_dt_t datatype,
                                                      int root, COLL_comm_t * comm, int tree_type,
                                                      int k, int segsize, TSP_sched_t ** sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);

    int mpi_errno = MPI_SUCCESS;

    /* generate key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = BCAST,
                             .args = {.bcast = {.bcast = 
                                                 {.buf = buffer,
                                                  .count = count,
                                                  .dt_id = (int) datatype,
                                                  .root = root,
                                                  .pad = 0
                                                 },
                                                .tree_type = tree_type,
                                                .k = k,
                                                .segsize = segsize}
                                    }
                            };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    int key_size = COLL_get_key_size (&coll_args);
    *sched =
        TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, tag, &is_new);

    if (is_new) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Schedule does not exist\n"));
        /* generate the schedule */
        mpi_errno = COLL_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm,
                                                    tree_type, k, segsize, *sched, 1);
        /* store the schedule (it is optional for the transport to store the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, key_size, (void *) *sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);

    return mpi_errno;
}
