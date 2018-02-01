/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Local gather remote bcast
 *
 * Each group does a gather to local root with the local
 * intracommunicator, and then does an intercommunicator broadcast.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_sched_inter_local_gather_remote_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_sched_inter_local_gather_remote_bcast(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size, root;
    MPI_Aint true_extent, true_lb = 0, extent, send_extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if ((rank == 0) && (sendcount != 0)) {
        /* In each group, rank 0 allocates temp. buffer for local
         * gather */
        MPIR_Type_get_true_extent_impl(sendtype, &true_lb, &true_extent);

        MPIR_Datatype_get_extent_macro(sendtype, send_extent);
        extent = MPL_MAX(send_extent, true_extent);

        MPIR_Ensure_Aint_fits_in_pointer(extent * sendcount * local_size);
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, extent * sendcount * local_size, mpi_errno,
                                  "tmp_buf", MPL_MEM_BUFFER);

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        mpi_errno = MPIR_Igather_sched(sendbuf, sendcount, sendtype,
                                       tmp_buf, sendcount, sendtype, 0, newcomm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Ibcast_sched(tmp_buf, sendcount * local_size,
                                          sendtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Ibcast_sched(recvbuf, recvcount * remote_size,
                                          recvtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPIR_Ibcast_sched(recvbuf, recvcount * remote_size,
                                          recvtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPIR_Ibcast_sched(tmp_buf, sendcount * local_size,
                                          sendtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
