/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Routine to schedule a disdem based barrier with radix k */
int MPIR_TSP_Ibarrier_sched_intra_k_dissemination(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j, nranks, rank;
    int p_of_k, shift, to, from;        /* minimum power of k that is greater than or equal to number of ranks */
    int nphases = 0;
    int tag, vtx_id;
    int *recv_ids = NULL;
    MPIR_CHKLMEM_DECL();

    MPIR_FUNC_ENTER;

    MPIR_COMM_RANK_SIZE(comm, rank, nranks);

    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    p_of_k = 1;
    while (p_of_k < nranks) {
        p_of_k *= k;
        nphases++;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "dissem barrier - number of phases = %d\n", nphases));

    MPIR_CHKLMEM_MALLOC(recv_ids, sizeof(int) * nphases * (k - 1));
    shift = 1;
    for (i = 0; i < nphases; i++) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "dissem barrier - start scheduling phase %d\n", i));
        for (j = 1; j < k; j++) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "shift = %d \n", shift));
            to = (rank + j * shift) % nranks;
            from = (rank - j * shift) % nranks;
            if (from < 0)
                from += nranks;

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "dissem barrier - scheduling recv from %d\n", from));
            mpi_errno =
                MPIR_TSP_sched_irecv(NULL, 0, MPIR_BYTE_INTERNAL, from, tag, comm, sched, 0, NULL,
                                     &recv_ids[i * (k - 1) + j - 1]);
            MPIR_ERR_CHECK(mpi_errno);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "dissem barrier - scheduling send to %d\n", to));
            mpi_errno =
                MPIR_TSP_sched_isend(NULL, 0, MPIR_BYTE_INTERNAL, to, tag, comm, sched, i * (k - 1),
                                     recv_ids, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "dissem barrier - scheduled phase %d\n", i));
        }
        shift *= k;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
