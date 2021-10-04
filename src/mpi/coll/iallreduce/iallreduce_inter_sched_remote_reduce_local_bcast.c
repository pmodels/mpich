/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Intercommunicator Allreduce
 *
 * We first do an intercommunicator reduce to rank 0 on left group,
 * then an intercommunicator reduce to rank 0 on right group, followed
 * by local intracommunicator broadcasts in each group.
 *
 * We don't do local reduces first and then intercommunicator
 * broadcasts because it would require allocation of a temporary
 * buffer.
 */

int MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast(const void *sendbuf, void *recvbuf,
                                                          MPI_Aint count, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, root;
    MPIR_Comm *lcomm_ptr = NULL;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
     * then from left group to rank 0 in right group */
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno =
            MPIR_Ireduce_inter_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno =
            MPIR_Ireduce_inter_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno =
            MPIR_Ireduce_inter_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno =
            MPIR_Ireduce_inter_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* don't bcast until the reductions have finished */
    mpi_errno = MPIR_Sched_barrier(s);
    MPIR_ERR_CHECK(mpi_errno);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }
    lcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Ibcast_intra_sched_auto(recvbuf, count, datatype, 0, lcomm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
