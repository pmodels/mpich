/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Short Linear Gather
 *
 * This linear gather algorithm is tuned for short messages. The remote group
 * does a local intracommunicator gather to rank 0. Rank 0 then sends data to
 * root.
 *
 * Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
 */
int MPIR_Igather_sched_inter_short(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_element_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint local_size, remote_size;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno =
            MPIR_Sched_element_recv(recvbuf, recvcount * remote_size, recvtype, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator gather, and then sends the data
         * to root. */
        MPI_Aint sendtype_sz;
        void *tmp_buf = NULL;

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *,
                                      sendcount * local_size * sendtype_sz,
                                      mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
        } else {
            /* silience -Wmaybe-uninitialized due to MPIR_Igather_sched by non-zero ranks */
            sendtype_sz = 0;
        }

        /* all processes in remote group form new intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the a local gather on this intracommunicator */
        mpi_errno = MPIR_Igather_sched(sendbuf, sendcount, sendtype,
                                       tmp_buf, sendcount * sendtype_sz, MPI_BYTE,
                                       0, newcomm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (rank == 0) {
            mpi_errno =
                MPIR_Sched_element_send(tmp_buf, sendcount * local_size * sendtype_sz, MPI_BYTE,
                                        root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
