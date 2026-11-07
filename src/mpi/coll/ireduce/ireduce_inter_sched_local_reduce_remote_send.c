/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

/* Local reduce remote send
 *
 * Remote group does a local intracommunicator reduce to rank 0. Rank
 * 0 then sends data to root.
 */

int MPIR_Ireduce_inter_sched_local_reduce_remote_send(MPIR_Csel_coll_sig_s * coll_sig,
                                                      MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.reduce.sendbuf;
    void *recvbuf = coll_sig->u.reduce.recvbuf;
    MPI_Aint count = coll_sig->u.reduce.count;
    MPI_Datatype datatype = coll_sig->u.reduce.datatype;
    MPI_Op op = coll_sig->u.reduce.op;
    int root = coll_sig->u.reduce.root;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank;
    void *tmp_buf = NULL;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* remote group. Rank 0 allocates temporary buffer, does
         * local intracommunicator reduce, and then sends the data
         * to root. */
        rank = comm_ptr->rank;

        if (rank == 0) {
            MPI_Aint buf_shift, buf_size;
            MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
            tmp_buf = MPIR_Sched_alloc_state(s, buf_size);
            MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *) ((char *) tmp_buf - buf_shift);
        }

        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }

        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, count, datatype, op, 0,
                              comm_ptr->local_comm);
        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);

        if (rank == 0) {
            mpi_errno = MPIR_Sched_send(tmp_buf, count, datatype, root, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
