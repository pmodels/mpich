/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Local reduce remote send
 *
 * Remote group does a local intracommunicator reduce to rank 0. Rank
 * 0 then sends data to root.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_sched_inter_local_reduce_remote_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_sched_inter_local_reduce_remote_send(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op, int root,
                                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator reduce, and then sends the data
         * to root. */
        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            MPIR_Datatype_get_extent_macro(datatype, extent);
            /* I think this is the worse case, so we can avoid an assert()
             * inside the for loop */
            /* Should MPIR_SCHED_CHKPMEM_MALLOC do this? */
            MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                                      mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - true_lb);
        }

        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno =
            MPIR_Ireduce_sched(sendbuf, tmp_buf, count, datatype, op, 0, comm_ptr->local_comm, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (rank == 0) {
            mpi_errno = MPIR_Sched_send(tmp_buf, count, datatype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
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
