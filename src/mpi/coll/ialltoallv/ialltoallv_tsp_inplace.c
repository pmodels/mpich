/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a recursive exchange based alltoallv */
int MPIR_TSP_Ialltoallv_sched_intra_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const MPI_Aint recvcounts[],
                                            const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    size_t recv_extent;
    MPI_Aint recv_lb, true_extent;
    int nranks, rank, nvtcs;
    int i, dst, send_id, recv_id, dtcopy_id = -1, vtcs[2];
    void *tmp_buf = NULL;
    int tag = 0;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);

    MPI_Aint max_count;
    max_count = 0;
    for (i = 0; i < nranks; ++i) {
        max_count = MPL_MAX(max_count, recvcounts[i]);
    }

    tmp_buf = MPIR_TSP_sched_malloc(max_count * recv_extent, sched);

    for (i = 0; i < nranks; ++i) {
        if (rank != i) {
            dst = i;
            nvtcs = (dtcopy_id == -1) ? 0 : 1;
            vtcs[0] = dtcopy_id;

            mpi_errno = MPIR_TSP_sched_isend((char *) recvbuf + rdispls[dst] * recv_extent,
                                             recvcounts[dst], recvtype, dst, tag, comm,
                                             sched, nvtcs, vtcs, &send_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            mpi_errno =
                MPIR_TSP_sched_irecv(tmp_buf, recvcounts[dst], recvtype, dst, tag, comm,
                                     sched, nvtcs, vtcs, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            mpi_errno = MPIR_TSP_sched_localcopy(tmp_buf, recvcounts[dst], recvtype,
                                                 ((char *) recvbuf +
                                                  rdispls[dst] * recv_extent), recvcounts[dst],
                                                 recvtype, sched, nvtcs, vtcs, &dtcopy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
