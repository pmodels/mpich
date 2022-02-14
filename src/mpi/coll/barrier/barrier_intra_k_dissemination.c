/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


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
