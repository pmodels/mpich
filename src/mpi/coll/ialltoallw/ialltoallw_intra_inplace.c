/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Inplace Alltoallw
 *
 * We use pair-wise sendrecv_replace in order to conserve memory usage, which
 * is keeping with the spirit of the MPI-2.2 Standard.  But because of this
 * approach all processes must agree on the global schedule of sendrecv_replace
 * operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of time and
 * there will be multiple repeated malloc/free's rather than maintaining a
 * single buffer across the whole loop.  Something like MADRE is probably the
 * best solution for the MPI_IN_PLACE scenario.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_sched_intra_inplace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const int recvcounts[], const int rdispls[],
                                        const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, i, j;
    int dst, rank;
    int recv_extent;
    MPI_Aint true_extent, true_lb;
    int max_size;
    void *tmp_buf = NULL, *adj_tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* The regular MPI_Alltoallw handles MPI_IN_PLACE using pairwise
     * sendrecv_replace calls.  We don't have a sendrecv_replace, so just
     * malloc the maximum of the counts array entries and then perform the
     * pairwise exchanges manually with schedule barriers instead.
     *
     * Because of this approach all processes must agree on the global
     * schedule of "sendrecv_replace" operations to avoid deadlock.
     *
     * This keeps with the spirit of the MPI-2.2 standard, which is to
     * conserve memory when using MPI_IN_PLACE for these routines.
     * Something like MADRE would probably generate a more optimal
     * algorithm. */
    max_size = 0;
    for (i = 0; i < comm_size; ++i) {
        /* only look at recvtypes/recvcounts because the send vectors are
         * ignored when sendbuf==MPI_IN_PLACE */
        MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(recvtypes[i], recv_extent);
        max_size = MPL_MAX(max_size, recvcounts[i] * MPL_MAX(recv_extent, true_extent));
    }
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, max_size, mpi_errno, "Ialltoallw tmp_buf",
                              MPL_MEM_BUFFER);

    for (i = 0; i < comm_size; ++i) {
        /* start inner loop at i to avoid re-exchanging data */
        for (j = i; j < comm_size; ++j) {
            if (rank == i && rank == j) {
                /* no need to "sendrecv_replace" for ourselves */
            } else if (rank == i || rank == j) {
                if (rank == i)
                    dst = j;
                else
                    dst = i;

                MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
                adj_tmp_buf = (void *) ((char *) tmp_buf - true_lb);

                mpi_errno = MPIR_Sched_send(((char *) recvbuf + rdispls[dst]),
                                            recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno =
                    MPIR_Sched_recv(adj_tmp_buf, recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                mpi_errno = MPIR_Sched_copy(adj_tmp_buf, recvcounts[dst], recvtypes[dst],
                                            ((char *) recvbuf + rdispls[dst]),
                                            recvcounts[dst], recvtypes[dst], s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
