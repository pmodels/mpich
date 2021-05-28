/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPIR_Ibarrier_intra_sched_recursive_doubling(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int size, rank, src, dst, mask;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;

        mpi_errno = MPIR_Sched_send(NULL, 0, MPI_BYTE, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Sched_recv(NULL, 0, MPI_BYTE, src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
