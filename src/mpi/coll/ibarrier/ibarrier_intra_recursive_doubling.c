/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Recursive Doubling
 *
 * Restrictions: Intracommunicators Only
 *
 * We use the dissemination algorithm described in:
 * Debra Hensgen, Raphael Finkel, and Udi Manber, "Two Algorithms for
 * Barrier Synchronization," International Journal of Parallel
 * Programming, 17(1):1-17, 1988.
 *
 * It uses ceiling(lgp) steps. In step k, 0 <= k <= (ceiling(lgp)-1),
 * process i sends to process (i + 2^k) % p and receives from process
 * (i - 2^k + p) % p.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched_intra_recursive_doubling(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int size, rank, src, dst, mask;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Trivial barriers return immediately */
    if (size == 1)
        goto fn_exit;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;

        mpi_errno = MPIR_Sched_send(NULL, 0, MPI_BYTE, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Sched_recv(NULL, 0, MPI_BYTE, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mask <<= 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
