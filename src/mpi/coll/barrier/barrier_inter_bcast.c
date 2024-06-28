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

int MPIR_Barrier_inter_bcast(MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int rank, mpi_errno = MPI_SUCCESS, root;
    int mpi_errno_ret = MPI_SUCCESS;
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
    mpi_errno = MPIR_Barrier(newcomm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm_ptr->is_low_group) {
        /* bcast to right */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);

        /* receive bcast from right */
        root = 0;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }
  fn_exit:
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}
