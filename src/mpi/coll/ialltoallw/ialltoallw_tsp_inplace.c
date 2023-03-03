/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule inplace algorithm for  alltoallw */
int MPIR_TSP_Ialltoallw_sched_intra_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[],
                                            const MPI_Datatype sendtypes[], void *recvbuf,
                                            const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                            const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                            MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int tag;
    size_t recv_extent;
    MPI_Aint true_extent, true_lb;
    int nranks, rank, nvtcs;
    int i, dst, send_id, recv_id, dtcopy_id = -1, vtcs[2];
    void *tmp_buf = NULL, *adj_tmp_buf = NULL;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    MPIR_Assert(sendbuf == MPI_IN_PLACE);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* FIXME: Here we allocate tmp_buf using extent and send/recv with datatype directly,
     *        which can be potentially very inefficient. Why don't we use bytes as in
     *        ialltoallw_intra_sched_inplace.c ?
     */
    MPI_Aint max_size;
    max_size = 0;
    for (i = 0; i < nranks; ++i) {
        /* only look at recvtypes/recvcounts because the send vectors are
         * ignored when sendbuf==MPI_IN_PLACE */
        MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(recvtypes[i], recv_extent);
        max_size = MPL_MAX(max_size, recvcounts[i] * MPL_MAX(recv_extent, true_extent));
    }

    tmp_buf = MPIR_TSP_sched_malloc(max_size, sched);
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (i = 0; i < nranks; ++i) {
        if (rank != i) {
            dst = i;
            nvtcs = (dtcopy_id == -1) ? 0 : 1;
            vtcs[0] = dtcopy_id;

            MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
            adj_tmp_buf = (void *) ((char *) tmp_buf - true_lb);

            mpi_errno = MPIR_TSP_sched_isend((char *) recvbuf + rdispls[dst],
                                             recvcounts[dst], recvtypes[dst], dst, tag, comm, sched,
                                             nvtcs, vtcs, &send_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            mpi_errno =
                MPIR_TSP_sched_irecv(adj_tmp_buf, recvcounts[dst], recvtypes[dst], dst, tag, comm,
                                     sched, nvtcs, vtcs, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 2;
            vtcs[0] = send_id;
            vtcs[1] = recv_id;
            mpi_errno = MPIR_TSP_sched_localcopy(adj_tmp_buf, recvcounts[dst], recvtypes[dst],
                                                 ((char *) recvbuf + rdispls[dst]), recvcounts[dst],
                                                 recvtypes[dst], sched, nvtcs, vtcs, &dtcopy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
