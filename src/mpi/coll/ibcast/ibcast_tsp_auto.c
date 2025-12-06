/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"
#include "coll_csel.h"

/* Provides a "flat" broadcast that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */
/* Remove this function when gentran algos are in json file */
static int MPIR_Ibcast_sched_intra_tsp_flat_auto(void *buffer, MPI_Aint count,
                                                 MPI_Datatype datatype, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint type_size, nbytes;
    int tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    int radix = 2, scatterv_k = 2, allgatherv_k = 2, block_size = 0;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        /* gentran tree with knomial tree type, radix 2 and pipeline block size 0 */
        mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                     tree_type, radix, block_size, sched);
    } else {
        /* gentran scatterv recexch allgather with radix 2 */
        mpi_errno =
            MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root,
                                                            comm_ptr, 0,
                                                            scatterv_k, allgatherv_k, sched);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast_sched_intra_tsp_flat_auto(buffer, count, datatype, root,
                                                      comm_ptr, sched);

    return mpi_errno;
}
