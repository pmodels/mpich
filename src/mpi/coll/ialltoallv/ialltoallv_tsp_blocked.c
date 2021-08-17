/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a recursive exchange based alltoallv */
int MPIR_TSP_Ialltoallv_sched_intra_blocked(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const MPI_Aint recvcounts[],
                                            const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, int bblock, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    size_t recv_extent, send_extent, sendtype_size, recvtype_size;
    MPI_Aint recv_lb, send_lb, true_extent;
    int nranks, rank;
    int i, j, comm_block, dst;
    int tag = 0, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    MPIR_Assert(sendbuf != MPI_IN_PLACE);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    MPIR_Datatype_get_extent_macro(sendtype, send_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &send_lb, &true_extent);
    send_extent = MPL_MAX(send_extent, true_extent);
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);

    if (bblock == 0)
        bblock = nranks;

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (i = 0; i < nranks; i += bblock) {
        comm_block = nranks - i < bblock ? nranks - i : bblock;

        /* do the communication -- post ss sends and receives: */
        for (j = 0; j < comm_block; j++) {
            dst = (rank + j + i) % nranks;
            if (recvcounts[dst] && recvtype_size) {
                mpi_errno = MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[dst] * recv_extent,
                                                 recvcounts[dst], recvtype, dst, tag, comm, sched,
                                                 0, NULL, &vtx_id);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }
        }

        for (j = 0; j < comm_block; j++) {
            dst = (rank - j - i + nranks) % nranks;
            if (sendcounts[dst] && sendtype_size) {
                mpi_errno = MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst] * send_extent,
                                                 sendcounts[dst], sendtype, dst, tag, comm, sched,
                                                 0, NULL, &vtx_id);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        mpi_errno = MPIR_TSP_sched_fence(sched);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
