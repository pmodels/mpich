/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_sched_inter_remote_send_local_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_sched_inter_remote_send_local_scatter(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm_ptr,
                                                        MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        /* root sends all data to rank 0 on remote group and returns */
        mpi_errno = MPIR_Sched_send(sendbuf, sendcount * remote_size, sendtype, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        goto fn_exit;
    } else {
        /* remote group. rank 0 receives data from root. need to
         * allocate temporary buffer to store this data. */
        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

            MPIR_Datatype_get_extent_macro(recvtype, extent);
            MPIR_Ensure_Aint_fits_in_pointer(extent * recvcount * local_size);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                             sendcount * remote_size * extent);

            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *,
                                      recvcount * local_size * (MPL_MAX(extent, true_extent)),
                                      mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - true_lb);

            mpi_errno =
                MPIR_Sched_recv(tmp_buf, recvcount * local_size, recvtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPII_Setup_intercomm_localcomm(comm_ptr);

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual scatter on this intracommunicator */
        mpi_errno =
            MPIR_Iscatter_sched(tmp_buf, recvcount, recvtype, recvbuf, recvcount, recvtype, 0,
                                newcomm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    MPIR_SCHED_CHKPMEM_COMMIT(s);
    return mpi_errno;

  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
