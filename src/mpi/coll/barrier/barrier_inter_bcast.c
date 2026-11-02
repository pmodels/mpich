/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Bcast-based algorithm
 *
 * rank 0 on each group does an intercommunicator broadcast to the
 * remote group to indicate that all processes in the local group have
 * reached the barrier. We do a 1-byte bcast because a 0-byte bcast
 * will just return without doing anything.
 *
 * first broadcast from left to right group, then from right to left
 * group.
 */

int MPIR_Barrier_inter_bcast(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    int rank, mpi_errno = MPI_SUCCESS, root;
    int i = 0;
    MPIR_Comm *newcomm_ptr = NULL;

    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    /* do a barrier on the local intracommunicator */
    MPIR_COLL_INNER(Barrier, coll_sig, newcomm_ptr);

    if (comm_ptr->is_low_group) {
        /* bcast to right */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_INNER(Bcast, coll_sig, &i, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);

        /* receive bcast from right */
        root = 0;
        MPIR_COLL_INNER(Bcast, coll_sig, &i, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);
    } else {
        /* receive bcast from left */
        root = 0;
        MPIR_COLL_INNER(Bcast, coll_sig, &i, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_INNER(Bcast, coll_sig, &i, 1, MPIR_BYTE_INTERNAL, root, comm_ptr);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
