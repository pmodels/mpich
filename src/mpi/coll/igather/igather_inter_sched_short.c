/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPIR_Igather_inter_sched_short(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.gather.sendbuf;
    MPI_Aint sendcount = coll_sig->u.gather.sendcount;
    MPI_Datatype sendtype = coll_sig->u.gather.sendtype;
    void *recvbuf = coll_sig->u.gather.recvbuf;
    MPI_Aint recvcount = coll_sig->u.gather.recvcount;
    MPI_Datatype recvtype = coll_sig->u.gather.recvtype;
    int root = coll_sig->u.gather.root;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint local_size, remote_size;
    MPIR_Comm *newcomm_ptr = NULL;

    remote_size = comm_ptr->remote_size;
    MPIR_COMM_RANK_SIZE(comm_ptr, rank, local_size);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    } else if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIR_Sched_recv(recvbuf, recvcount * remote_size, recvtype, 0, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator gather, and then sends the data
         * to root. */
        MPI_Aint sendtype_sz;
        void *tmp_buf = NULL;

        if (rank == 0) {
            MPIR_Datatype_get_size_macro(sendtype, sendtype_sz);
            tmp_buf = MPIR_Sched_alloc_state(s, sendcount * local_size * sendtype_sz);
            MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
        } else {
            /* silence -Wmaybe-uninitialized by non-zero ranks */
            sendtype_sz = 0;
        }

        /* all processes in remote group form new intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the a local gather on this intracommunicator */
        MPIR_COLL_SCHED_INNER(Igather, coll_sig, sendbuf, sendcount, sendtype, tmp_buf,
                              sendcount * sendtype_sz, MPIR_BYTE_INTERNAL, 0, newcomm_ptr);

        /* the send below needs the result of the gather */
        MPIR_SCHED_BARRIER(s);

        if (rank == 0) {
            mpi_errno =
                MPIR_Sched_send(tmp_buf, sendcount * local_size * sendtype_sz, MPIR_BYTE_INTERNAL,
                                root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
