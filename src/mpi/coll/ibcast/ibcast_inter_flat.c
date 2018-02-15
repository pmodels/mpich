/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "ibcast.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_sched_intra_flat
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_sched_inter_flat(void *buffer, int count, MPI_Datatype datatype,
                                 int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    /* Intercommunicator broadcast.
     * Root sends to rank 0 in remote group. Remote group does local
     * intracommunicator broadcast. */
    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    } else if (root == MPI_ROOT) {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* remote group. rank 0 on remote group receives from root */
        if (comm_ptr->rank == 0) {
            mpi_errno = MPIR_Sched_recv(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        if (comm_ptr->local_comm == NULL) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        /* now do the usual broadcast on this intracommunicator
         * with rank 0 as root. */
        mpi_errno = MPIR_Ibcast_sched(buffer, count, datatype, root, comm_ptr->local_comm, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
