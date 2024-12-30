/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule linear algorithm fir neighbor_alltoall */
int MPIR_TSP_Ineighbor_alltoall_sched_allcomm_linear(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k, l;
    int *srcs, *dsts;
    MPI_Aint sendtype_extent, recvtype_extent;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    MPIR_CHKLMEM_DECL();

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

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
        char *sb = ((char *) sendbuf) + k * sendcount * sendtype_extent;
        mpi_errno =
            MPIR_TSP_sched_isend(sb, sendcount, sendtype, dsts[k], tag, comm_ptr, sched, 0, NULL,
                                 &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* need reverse the order to ensure matching when the graph is from MPI_Cart_create and
     * the n-th dimension is periodic and the size is 1 or 2.
     * ref. ineighbor_alltoall_allcomm_sched_linear.c */
    for (l = indegree - 1; l >= 0; l--) {
        char *rb = ((char *) recvbuf) + l * recvcount * recvtype_extent;
        mpi_errno =
            MPIR_TSP_sched_irecv(rb, recvcount, recvtype, srcs[l], tag, comm_ptr, sched, 0, NULL,
                                 &vtx_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
