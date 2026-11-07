/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

int MPIR_Ibarrier_inter_sched_bcast(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank, root;
    char *buf = NULL;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* do a barrier on the local intracommunicator */
    if (comm_ptr->local_size != 1) {
        MPIR_COLL_SCHED_INNER(Ibarrier, coll_sig, comm_ptr->local_comm);
        MPIR_SCHED_BARRIER(s);
    }
    /* rank 0 on each group does an intercommunicator broadcast to the
     * remote group to indicate that all processes in the local group
     * have reached the barrier. We do a 1-byte bcast because a 0-byte
     * bcast will just return without doing anything. */

    buf = MPIR_Sched_alloc_state(s, sizeof(char));
    MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    buf[0] = 'D';       /* avoid valgrind warnings */

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buf, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);

        MPIR_SCHED_BARRIER(s);

        /* receive bcast from right */
        root = 0;
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buf, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);
    } else {
        /* receive bcast from left */
        root = 0;
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buf, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);

        MPIR_SCHED_BARRIER(s);

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, buf, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
