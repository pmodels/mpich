/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoall_sched_intra_inplace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoall_sched_intra_inplace(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    void *tmp_buf = NULL;
    int i, j;
    int rank, comm_size;
    int nbytes, recvtype_size;
    MPI_Aint recvtype_extent;
    int peer;
    MPIR_SCHED_CHKPMEM_DECL(1);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf == MPI_IN_PLACE);
#endif

    if (recvcount == 0)
        goto fn_exit;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    nbytes = recvtype_size * recvcount;

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

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
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* now simultaneously send from tmp_buf and recv to recvbuf */
                mpi_errno = MPIR_Sched_send(tmp_buf, nbytes, MPI_BYTE, peer, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_recv(((char *) recvbuf + peer * recvcount * recvtype_extent),
                                            recvcount, recvtype, peer, comm_ptr, s);
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
