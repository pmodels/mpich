/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is a duplicate of MPIR_Barrier_intra_dissemination, with folowing modification:
 *
 * * Use finite-sized buffer for send/recv instead of a zero-sized NULL buffer.
 *
 * Rationale:
 *   Small but finite-sized msg has more well-defined behaviors -- less suprises.
 *   In particular, used in ofi_finalize by setting max_buffered_send=0 can force all
 *   finite-sized message not to use "inject" only send.
 *
 */
int MPIR_Barrier_intra_fallback(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int size, rank, src, dst, mask, mpi_errno = MPI_SUCCESS;
    char buf[8];
    int mpi_errno_ret = MPI_SUCCESS;

    size = comm_ptr->local_size;
    /* Trivial barriers return immediately */
    if (size == 1)
        goto fn_exit;

    rank = comm_ptr->rank;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        mpi_errno = MPIC_Sendrecv(&buf[0], 4, MPI_BYTE, dst, MPIR_BARRIER_TAG,
                                  &buf[4], 4, MPI_BYTE, src, MPIR_BARRIER_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        mask <<= 1;
    }

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}
