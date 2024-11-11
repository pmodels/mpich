/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Intercommunicator broadcast
 *
 * Root sends to rank 0 in remote group.  Remote group does local
 * intracommunicator broadcast.
 */

int MPIR_Bcast_inter_remote_send_local_bcast(void *buffer,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm_ptr, int coll_group,
                                             MPIR_Errflag_t errflag)
{
    int rank, mpi_errno;
    MPI_Status status;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_FUNC_ENTER;
    MPIR_Assert(coll_group == MPIR_SUBGROUP_NONE);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    } else if (root == MPI_ROOT) {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno =
            MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm_ptr, coll_group, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* remote group. rank 0 on remote group receives from root */

        rank = comm_ptr->rank;

        if (rank == 0) {
            mpi_errno = MPIC_Recv(buffer, count, datatype, root, MPIR_BCAST_TAG,
                                  comm_ptr, coll_group, &status);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual broadcast on this intracommunicator
         * with rank 0 as root. */
        mpi_errno = MPIR_Bcast_allcomm_auto(buffer, count, datatype, 0, newcomm_ptr,
                                            coll_group, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
