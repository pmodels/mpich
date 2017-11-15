/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

#include "../algorithms/tree/coll.h"

#undef FUNCNAME
#define FUNCNAME COLL_bcast
/* Blocking broadcast */
int COLL_bcast(void *buffer, int count, COLL_dt_t datatype, int root,
                           COLL_comm_t * comm, int *errflag, int tree_type, int k, int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_BCAST);

    int mpi_errno = MPI_SUCCESS;

    /* get the schedule */
    MPIR_TSP_sched_t *sched;
    mpi_errno = COLL_bcast_get_tree_schedule(buffer, count, datatype, root, comm, tree_type, k, segsize, &sched);

    /* execute the schedule */
    COLL_sched_wait(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_BCAST);

    return mpi_errno;
}
