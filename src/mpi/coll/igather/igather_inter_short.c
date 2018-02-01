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
#undef FUNCNAME
#define FUNCNAME MPIR_Igather_sched_inter_short
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_sched_inter_short(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint local_size, remote_size;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIR_Sched_recv(recvbuf, recvcount * remote_size, recvtype, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator gather, and then sends the data
         * to root. */

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(sendtype, &true_lb, &true_extent);
            MPIR_Datatype_get_extent_macro(sendtype, extent);

            MPIR_Ensure_Aint_fits_in_pointer(sendcount * local_size *
                                             (MPL_MAX(extent, true_extent)));
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *,
                                      sendcount * local_size * (MPL_MAX(extent, true_extent)),
                                      mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - true_lb);
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
                                       tmp_buf, sendcount, sendtype, 0, newcomm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (rank == 0) {
            mpi_errno =
                MPIR_Sched_send(tmp_buf, sendcount * local_size, sendtype, root, comm_ptr, s);
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
