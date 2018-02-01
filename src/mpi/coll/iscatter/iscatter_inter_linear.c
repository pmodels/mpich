/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Linear
 *
 * Linear algorithm where the root sends to each process individually.
 *
 * Cost: p.alpha + n.beta
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_sched_inter_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_sched_inter_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int remote_size;
    int i;
    MPI_Aint extent;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_extent_macro(sendtype, extent);
        for (i = 0; i < remote_size; i++) {
            mpi_errno =
                MPIR_Sched_send(((char *) sendbuf + sendcount * i * extent), sendcount, sendtype, i,
                                comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
