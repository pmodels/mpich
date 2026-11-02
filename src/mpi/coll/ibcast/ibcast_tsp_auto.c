/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    /* gentran tree with knomial tree type, radix 2 and pipeline block size 0 */
    mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_TREE_TYPE_KNOMIAL_1, 2, 0, sched);

    return mpi_errno;
}
