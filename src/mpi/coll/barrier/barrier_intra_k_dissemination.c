/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: MPI_Barrier
 *
 * We use the dissemination algorithm described in:
 * Debra Hensgen, Raphael Finkel, and Udi Manbet, "Two Algorithms for
 * Barrier Synchronization," International Journal of Parallel
 * Programming, 17(1):1-17, 1988.
 *
 * It uses ceiling(lgp) steps. In step k, 0 <= k <= (ceiling(lgp)-1),
 * process i sends to process (i + 2^k) % p and receives from process
 * (i - 2^k + p) % p.
 */
int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int size, rank, src, dst, mask, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        mpi_errno = MPIC_Sendrecv(NULL, 0, MPI_BYTE, dst,
                                  MPIR_BARRIER_TAG, NULL, 0, MPI_BYTE,
                                  src, MPIR_BARRIER_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        mask <<= 1;
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}

/* Algorithm: high radix dissemination
 * Similar to dissemination algorithm, but generalized with high radix k
 */
int MPIR_Barrier_intra_k_dissemination(MPIR_Comm * comm, int k, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int i, j, nranks, rank;
    int p_of_k;                 /* minimum power of k that is greater than or equal to number of ranks */
    int shift, to, from;
    int nphases = 0;
    MPIR_Request *sreqs[MAX_RADIX], *rreqs[MAX_RADIX * 2];
    MPIR_Request **send_reqs = NULL, **recv_reqs = NULL;

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    if (nranks == 1)
        goto fn_exit;

    if (nranks < k)
        k = nranks;

    if (k == 2) {
        return MPIR_Barrier_intra_dissemination(comm, errflag);
    }

    /* If k value is greater than the maximum radix defined by MAX_RADIX macro,
     * we allocate memory for requests here. Otherwise we use the requests defined
     * in the communicator for allreduce/barrier recexch */
    if (k > MAX_RADIX) {
        recv_reqs =
            (MPIR_Request **) MPL_malloc((k - 1) * 2 * sizeof(MPIR_Request *), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!recv_reqs, mpi_errno, MPI_ERR_OTHER, "**nomem");
        send_reqs = (MPIR_Request **) MPL_malloc((k - 1) * sizeof(MPIR_Request *), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!send_reqs, mpi_errno, MPI_ERR_OTHER, "**nomem");
    } else {
        send_reqs = sreqs;
        recv_reqs = rreqs;
    }

    p_of_k = 1;
    while (p_of_k < nranks) {
        p_of_k *= k;
        nphases++;
    }

    shift = 1;
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            to = (rank + j * shift) % nranks;
            from = (nranks + (rank - j * shift)) % nranks;
            while (from < 0)
                from += nranks;
            MPIR_Assert(from >= 0 && from < nranks);
            MPIR_Assert(to >= 0 && to < nranks);

            /* recv from (k-1) nbrs */
            mpi_errno =
                MPIC_Irecv(NULL, 0, MPI_BYTE, from, MPIR_BARRIER_TAG, comm,
                           &recv_reqs[(j - 1) + ((k - 1) * (i & 1))]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            /* wait on recvs from prev phase */
            if (i > 0 && j == 1) {
                mpi_errno =
                    MPIC_Waitall(k - 1, &recv_reqs[((k - 1) * ((i - 1) & 1))], MPI_STATUSES_IGNORE,
                                 errflag);
                if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                    MPIR_ERR_POP(mpi_errno);
            }

            mpi_errno =
                MPIC_Isend(NULL, 0, MPI_BYTE, to, MPIR_BARRIER_TAG, comm, &send_reqs[j - 1],
                           errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        }
        mpi_errno = MPIC_Waitall(k - 1, send_reqs, MPI_STATUSES_IGNORE, errflag);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIR_ERR_POP(mpi_errno);
        shift *= k;
    }

    mpi_errno =
        MPIC_Waitall(k - 1, recv_reqs + ((k - 1) * ((nphases - 1) & 1)), MPI_STATUSES_IGNORE,
                     errflag);
    if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (k > MAX_RADIX) {
        MPL_free(recv_reqs);
        MPL_free(send_reqs);
    }
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
