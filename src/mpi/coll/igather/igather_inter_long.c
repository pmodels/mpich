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
#undef FUNCNAME
#define FUNCNAME MPIR_Igather_sched_inter_long
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_sched_inter_long(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint remote_size;
    int i;
    MPI_Aint extent;

    remote_size = comm_ptr->remote_size;

    /* long message. use linear algorithm. */
    if (root == MPI_ROOT) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         (recvcount * remote_size * extent));

        for (i = 0; i < remote_size; i++) {
            mpi_errno = MPIR_Sched_recv(((char *) recvbuf + recvcount * i * extent),
                                        recvcount, recvtype, i, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    } else {
        mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
