/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Remote gather local bcast
 *
 * This is done differently from the intercommunicator allgather
 * because we don't have all the information to do a local
 * intracommunictor gather (sendcount can be different on each
 * process). Therefore, we do the following: Each group first does an
 * intercommunicator gather to rank 0 and then does an
 * intracommunicator broadcast.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_sched_inter_remote_gather_local_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_sched_inter_remote_gather_local_bcast(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           const int recvcounts[],
                                                           const int displs[],
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int remote_size, root, rank;
    MPIR_Comm *newcomm_ptr = NULL;
    MPI_Datatype newtype = MPI_DATATYPE_NULL;

    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* first do an intercommunicator gatherv from left to right group,
     * then from right to left group */
    if (comm_ptr->is_low_group) {
        /* gatherv from right group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* gatherv to right group */
        root = 0;
        mpi_errno = MPIR_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* gatherv to left group  */
        root = 0;
        mpi_errno = MPIR_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* gatherv from left group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

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

    mpi_errno = MPIR_Ibcast_sched(recvbuf, 1, newtype, 0, newcomm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Type_free_impl(&newtype);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
