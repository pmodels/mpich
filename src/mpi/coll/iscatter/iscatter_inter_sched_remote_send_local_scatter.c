/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Remote send local scatter
 *
 * Root sends to rank 0 in remote group.  rank 0 does local
 * intracommunicator scatter.
 *
 * Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
 * where n is the total size of the data to be scattered from the root.
 */

int MPIR_Iscatter_inter_sched_remote_send_local_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        MPI_Aint recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm_ptr,
                                                        MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size;
    MPIR_Comm *newcomm_ptr = NULL;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        /* root sends all data to rank 0 on remote group and returns */
        mpi_errno = MPIR_Sched_send(sendbuf, sendcount * remote_size, sendtype, 0, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        goto fn_exit;
    } else {
        /* remote group. rank 0 receives data from root. need to
         * allocate temporary buffer to store this data. */
        MPI_Aint recvtype_sz;
        void *tmp_buf = NULL;

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);
            tmp_buf = MPIR_Sched_alloc_state(s, recvcount * local_size * recvtype_sz);
            MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

            mpi_errno =
                MPIR_Sched_recv(tmp_buf, recvcount * local_size * recvtype_sz, MPI_BYTE,
                                root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        } else {
            /* silience -Wmaybe-uninitialized due to MPIR_Iscatter_intra_sched_auto by non-zero ranks */
            recvtype_sz = 0;
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPII_Setup_intercomm_localcomm(comm_ptr);

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual scatter on this intracommunicator */
        mpi_errno =
            MPIR_Iscatter_intra_sched_auto(tmp_buf, recvcount * recvtype_sz, MPI_BYTE,
                                           recvbuf, recvcount, recvtype, 0, newcomm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
