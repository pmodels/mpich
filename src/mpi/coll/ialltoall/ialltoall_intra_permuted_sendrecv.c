/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Batched Sendrecv
 *
 * Send/recv to permuted destinations/sources, in batches based on Tony Ladd's
 * suggestion.  Permuting the sources helps to avoid overloading a single source
 * or destination all at once.
 *
 * We use this as our medium-sized-message algorithm.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoall_sched_intra_permuted_sendrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoall_sched_intra_permuted_sendrecv(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int rank, comm_size;
    int ii, ss, bblock, dst;
    MPI_Aint sendtype_extent, recvtype_extent;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);       /* we do not handle in-place */
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    for (ii = 0; ii < comm_size; ii += bblock) {
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;
        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            mpi_errno = MPIR_Sched_recv(((char *) recvbuf + dst * recvcount * recvtype_extent),
                                        recvcount, recvtype, dst, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            mpi_errno = MPIR_Sched_send(((char *) sendbuf + dst * sendcount * sendtype_extent),
                                        sendcount, sendtype, dst, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        /* force the (2*ss) sends/recvs above to complete before posting additional ops */
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
