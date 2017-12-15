/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of scatter. The algorithm is:
 *
 * Algorithm: MPI_Scatter
 *
 * We use a binomial tree algorithm for both short and long messages. At nodes
 * other than leaf nodes we need to allocate a temporary buffer to store the
 * incoming message. If the root is not rank 0, we reorder the sendbuf in order
 * of relative ranks by copying it into a temporary buffer, so that all the
 * sends from the root are contiguous and in the right order. In the
 * heterogeneous case, we first pack the buffer by using MPI_Pack and then do
 * the scatter.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 * where n is the total size of the data to be scattered from the root.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_inter_generic_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_inter_generic_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    /*  Intercommunicator scatter.
        For short messages, root sends to rank 0 in remote group. rank 0
        does local intracommunicator scatter (binomial tree).
        Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta

        For long messages, we use linear scatter to avoid the extra n.beta.
        Cost: p.alpha + n.beta
    */
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size;
    int i, nbytes, sendtype_size, recvtype_size;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size  = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    }
    else {
        /* remote side */
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        if (root == MPI_ROOT) {
            /* root sends all data to rank 0 on remote group and returns */
            mpi_errno = MPIR_Sched_send(sendbuf, sendcount*remote_size, sendtype, 0, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            goto fn_exit;
        }
        else {
            /* remote group. rank 0 receives data from root. need to
               allocate temporary buffer to store this data. */
            rank = comm_ptr->rank;

            if (rank == 0) {
                MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

                MPIR_Datatype_get_extent_macro(recvtype, extent);
                MPIR_Ensure_Aint_fits_in_pointer(extent*recvcount*local_size);
                MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                        sendcount*remote_size*extent);

                MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvcount*local_size*(MPL_MAX(extent,true_extent)),
                        mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = MPIR_Sched_recv(tmp_buf, recvcount*local_size, recvtype, root, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }

            /* Get the local intracommunicator */
            if (!comm_ptr->local_comm)
                MPII_Setup_intercomm_localcomm(comm_ptr);

            newcomm_ptr = comm_ptr->local_comm;

            /* now do the usual scatter on this intracommunicator */
            mpi_errno = MPID_Iscatter_sched(tmp_buf, recvcount, recvtype,
                    recvbuf, recvcount, recvtype,
                    0, newcomm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }
    else {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT) {
            MPIR_Datatype_get_extent_macro(sendtype, extent);
            for (i = 0; i < remote_size; i++) {
                mpi_errno = MPIR_Sched_send(((char *)sendbuf+sendcount*i*extent),
                        sendcount, sendtype, i, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPIR_SCHED_BARRIER(s);
        }
        else {
            mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }


    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
