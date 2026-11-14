/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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

int MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(MPIR_Csel_coll_sig_s * coll_sig,
                                                           MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.allgatherv.sendbuf;
    MPI_Aint sendcount = coll_sig->u.allgatherv.sendcount;
    MPI_Datatype sendtype = coll_sig->u.allgatherv.sendtype;
    void *recvbuf = coll_sig->u.allgatherv.recvbuf;
    const MPI_Aint *recvcounts = coll_sig->u.allgatherv.recvcounts;
    const MPI_Aint *displs = coll_sig->u.allgatherv.displs;
    MPI_Datatype recvtype = coll_sig->u.allgatherv.recvtype;
    MPIR_Sched_t s = coll_sig->sched;
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
        MPIR_COLL_SCHED_INNER(Igatherv, coll_sig, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, root, comm_ptr);
        /* gatherv to right group */
        root = 0;
        MPIR_COLL_SCHED_INNER(Igatherv, coll_sig, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, root, comm_ptr);
    } else {
        /* gatherv to left group  */
        root = 0;
        MPIR_COLL_SCHED_INNER(Igatherv, coll_sig, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, root, comm_ptr);
        /* gatherv from left group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_SCHED_INNER(Igatherv, coll_sig, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                              displs, recvtype, root, comm_ptr);
    }

    MPIR_SCHED_BARRIER(s);

    /* now do an intracommunicator broadcast within each group. we use
     * a derived datatype to handle the displacements */

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Type_indexed_large_impl(remote_size, recvcounts, displs, recvtype, &newtype);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Type_commit_impl(&newtype);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Sched_add_handle(s, newtype);
    MPIR_Datatype_release_if_not_builtin(newtype);

    MPIR_COLL_SCHED_INNER(Ibcast, coll_sig, recvbuf, 1, newtype, 0, newcomm_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
