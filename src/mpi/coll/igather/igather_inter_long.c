/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Long Linear Gather
 *
 * This linear gather algorithm is tuned for long messages. It avoids an extra
 * O(n) communications over the short message algorithm.
 *
 * Cost: p.alpha + n.beta
 */
int MPIR_Igather_sched_inter_long(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_element_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint remote_size;
    int i;
    MPI_Aint extent;

    remote_size = comm_ptr->remote_size;

    /* long message. use linear algorithm. */
    if (root == MPI_ROOT) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);

        for (i = 0; i < remote_size; i++) {
            mpi_errno = MPIR_Sched_element_recv(((char *) recvbuf + recvcount * i * extent),
                                                recvcount, recvtype, i, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    } else {
        mpi_errno = MPIR_Sched_element_send(sendbuf, sendcount, sendtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
