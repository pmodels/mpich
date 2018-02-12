/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Remote Gather Local Bcast
 *
 * This is done differently from the intercommunicator allgather
 * because we don't have all the information to do a local
 * intracommunictor gather (sendcount can be different on each
 * process). Therefore, we do the following: Each group first does an
 * intercommunicator gather to rank 0 and then does an
 * intracommunicator broadcast.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Allgatherv_inter_remote_gather_local_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgatherv_inter_remote_gather_local_bcast(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int
                                                    *displs, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int remote_size, mpi_errno, root, rank;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Comm *newcomm_ptr = NULL;
    MPI_Datatype newtype = MPI_DATATYPE_NULL;

    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* first do an intercommunicator gatherv from left to right group,
     * then from right to left group */
    if (comm_ptr->is_low_group) {
        /* gatherv from right group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        /* gatherv to right group */
        root = 0;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {
        /* gatherv to left group  */
        root = 0;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        /* gatherv from left group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* now do an intracommunicator broadcast within each group. we use
     * a derived datatype to handle the displacements */

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Type_indexed_impl(remote_size, recvcounts, displs, recvtype, &newtype);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Type_commit_impl(&newtype);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Bcast_intra_auto(recvbuf, 1, newtype, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    MPIR_Type_free_impl(&newtype);

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (newtype != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&newtype);

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
