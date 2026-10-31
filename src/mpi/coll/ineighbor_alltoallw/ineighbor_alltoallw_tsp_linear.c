/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule linear algorithm for neighbor_alltoallw */
int MPIR_TSP_Ineighbor_alltoallw_sched_allcomm_linear(const void *sendbuf,
                                                      const MPI_Aint sendcounts[],
                                                      const MPI_Aint sdispls[],
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const MPI_Aint recvcounts[],
                                                      const MPI_Aint rdispls[],
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, k, l;
    bool is_cartesian;
    int *srcs, *dsts;
    int tag, vtx_id;
    MPIR_FUNC_ENTER;

    MPIR_CHKLMEM_DECL();

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, indegree * sizeof(int));
    MPIR_CHKLMEM_MALLOC(dsts, outdegree * sizeof(int));
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    MPIR_ERR_CHECK(mpi_errno);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb;

        sb = ((char *) sendbuf) + sdispls[k];
        /* the type of an empty entry is not held by the schedule and may not be valid anymore */
        MPI_Datatype dt = (sendcounts[k] > 0) ? sendtypes[k] : MPIR_BYTE_INTERNAL;
        mpi_errno =
            MPIR_TSP_sched_isend(sb, sendcounts[k], dt, dsts[k], tag, comm_ptr, sched, 0,
                                 NULL, &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* See ineighbor_alltoall_allcomm_sched_linear.c for why the receives are
     * swapped on a Cartesian topology. */
    is_cartesian = MPIR_Topo_is_cartesian(comm_ptr);
    MPIR_Assert(!is_cartesian || indegree % 2 == 0);
    for (i = 0; i < indegree; ++i) {
        if (is_cartesian) {
            l = i ^ 1;
        } else {
            l = i;
        }
        char *rb = ((char *) recvbuf) + rdispls[l];
        MPI_Datatype dt = (recvcounts[l] > 0) ? recvtypes[l] : MPIR_BYTE_INTERNAL;
        mpi_errno =
            MPIR_TSP_sched_irecv(rb, recvcounts[l], dt, srcs[l], tag, comm_ptr, sched, 0,
                                 NULL, &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
