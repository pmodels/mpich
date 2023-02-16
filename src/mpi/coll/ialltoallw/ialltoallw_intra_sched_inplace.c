/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPIR_Ialltoallw_intra_sched_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, i, j;
    int dst, rank;
    MPI_Aint recvtype_sz;
    void *tmp_buf = NULL;

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
    MPI_Aint max_size = 0;
    for (i = 0; i < comm_size; ++i) {
        /* only look at recvtypes/recvcounts because the send vectors are
         * ignored when sendbuf==MPI_IN_PLACE */
        MPIR_Datatype_get_size_macro(recvtypes[i], recvtype_sz);
        max_size = MPL_MAX(max_size, recvcounts[i] * recvtype_sz);
    }
    tmp_buf = MPIR_Sched_alloc_state(s, max_size);
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

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

                MPIR_Datatype_get_size_macro(recvtypes[dst], recvtype_sz);
                mpi_errno = MPIR_Sched_send(((char *) recvbuf + rdispls[dst]),
                                            recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_recv(tmp_buf, recvcounts[dst] * recvtype_sz, MPI_BYTE,
                                            dst, comm_ptr, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                mpi_errno = MPIR_Sched_copy(tmp_buf, recvcounts[dst] * recvtype_sz, MPI_BYTE,
                                            ((char *) recvbuf + rdispls[dst]),
                                            recvcounts[dst], recvtypes[dst], s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
