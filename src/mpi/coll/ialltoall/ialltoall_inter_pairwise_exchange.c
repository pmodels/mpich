/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Pairwise Exchange
 *
 * We use a pairwise exchange algorithm similar to the one used in
 * intracommunicator alltoall for long messages.  Since the local and
 * remote groups can be of different sizes, we first compute the max
 * of local_group_size, remote_group_size.  At step i, 0 <= i <
 * max_size, each process receives from src = (rank - i + max_size) %
 * max_size if src < remote_size, and sends to dst = (rank + i) %
 * max_size if dst < remote_size.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoall_sched_inter_pairwise_exchange
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoall_sched_inter_pairwise_exchange(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int local_size, remote_size, max_size, i;
    MPI_Aint sendtype_extent, recvtype_extent;
    int src, dst, rank;
    char *sendaddr, *recvaddr;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* Do the pairwise exchanges */
    max_size = MPL_MAX(local_size, remote_size);
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     max_size * recvcount * recvtype_extent);
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                     max_size * sendcount * sendtype_extent);
    for (i = 0; i < max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
        } else {
            recvaddr = (char *) recvbuf + src * recvcount * recvtype_extent;
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
        } else {
            sendaddr = (char *) sendbuf + dst * sendcount * sendtype_extent;
        }

        mpi_errno = MPIR_Sched_send(sendaddr, sendcount, sendtype, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_recv(recvaddr, recvcount, recvtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
