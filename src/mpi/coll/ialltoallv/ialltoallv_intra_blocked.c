/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallv_sched_intra_blocked
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallv_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                        const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                        const int recvcounts[], const int rdispls[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    int i;
    int ii, ss, bblock;
    MPI_Aint send_extent, recv_extent, sendtype_size, recvtype_size;
    int dst, rank;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent and size of recvtype, don't look at sendtype for MPI_IN_PLACE */
    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = comm_size;

    /* get size/extent for sendtype */
    MPIR_Datatype_get_extent_macro(sendtype, send_extent);
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < comm_size; ii += bblock) {
        ss = comm_size - ii < bblock ? comm_size - ii : bblock;

        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            if (recvcounts[dst] && recvtype_size) {
                MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                 rdispls[dst] * recv_extent);
                mpi_errno = MPIR_Sched_recv((char *) recvbuf + rdispls[dst] * recv_extent,
                                            recvcounts[dst], recvtype, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
        }

        for (i = 0; i < ss; i++) {
            dst = (rank - i - ii + comm_size) % comm_size;
            if (sendcounts[dst] && sendtype_size) {
                MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                 sdispls[dst] * send_extent);
                mpi_errno = MPIR_Sched_send((char *) sendbuf + sdispls[dst] * send_extent,
                                            sendcounts[dst], sendtype, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        MPIR_SCHED_BARRIER(s);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
