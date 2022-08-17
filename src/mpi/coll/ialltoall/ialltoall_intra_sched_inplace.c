/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Nonblocking all-to-all for sendbuf==MPI_IN_PLACE.
 *
 * Restrictions: Only for MPI_IN_PLACE
 *
 * We use nonblocking equivalent of pair-wise sendrecv_replace in order to
 * conserve memory usage, which is keeping with the spirit of the MPI-2.2
 * Standard.  But because of this approach all processes must agree on the
 * global schedule of "sendrecv_replace" operations to avoid deadlock.
 *
 * Note that this is not an especially efficient algorithm in terms of time.
 * Something like MADRE is probably the best solution for the MPI_IN_PLACE
 * scenario. */
int MPIR_Ialltoall_intra_sched_inplace(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    void *tmp_buf = NULL;
    int i, j;
    int rank, comm_size;
    MPI_Aint nbytes, recvtype_size;
    MPI_Aint recvtype_extent;
    int peer;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    nbytes = recvtype_size * recvcount;

    tmp_buf = MPIR_Sched_alloc_state(s, nbytes);
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (i = 0; i < comm_size; ++i) {
        /* start inner loop at i to avoid re-exchanging data */
        for (j = i; j < comm_size; ++j) {
            if (rank == i && rank == j) {
                /* no need to "sendrecv_replace" for ourselves */
            } else if (rank == i || rank == j) {
                if (rank == i)
                    peer = j;
                else
                    peer = i;

                /* pack to tmp_buf */
                mpi_errno = MPIR_Sched_copy(((char *) recvbuf + peer * recvcount * recvtype_extent),
                                            recvcount, recvtype, tmp_buf, nbytes, MPI_BYTE, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* now simultaneously send from tmp_buf and recv to recvbuf */
                mpi_errno = MPIR_Sched_send(tmp_buf, nbytes, MPI_BYTE, peer, comm_ptr, s);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Sched_recv(((char *) recvbuf + peer * recvcount * recvtype_extent),
                                            recvcount, recvtype, peer, comm_ptr, s);
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
