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
                                             int count,
                                             MPI_Datatype datatype,
                                             int root,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int rank, mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_BCAST_INTER);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_BCAST_INTER);


    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    } else if (root == MPI_ROOT) {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno = MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm_ptr, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
    } else {
        /* remote group. rank 0 on remote group receives from root */

        rank = comm_ptr->rank;

        if (rank == 0) {
            mpi_errno = MPIC_Recv(buffer, count, datatype, root,
                                  MPIR_BCAST_TAG, comm_ptr, &status, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual broadcast on this intracommunicator
         * with rank 0 as root. */
        mpi_errno = MPIR_Bcast_allcomm_auto(buffer, count, datatype, 0, newcomm_ptr, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);
    }

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_BCAST_INTER);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}
