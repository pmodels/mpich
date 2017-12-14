/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"

/* Pairwise exchanges for long messages. If comm_size is a power-of-two, do a
 * pairwise exchange using exclusive-or to create pairs. Else send to rank+i,
 * receive from rank-i. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoall_intra_pairwise_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoall_intra_pairwise_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int src, dst, is_pof2;
    int rank, comm_size;
    MPI_Aint sendtype_extent, recvtype_extent;

    MPIR_Assert(sendbuf != MPI_IN_PLACE); /* we do not handle in-place */

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* Make local copy first */
    mpi_errno = MPIR_Sched_copy(((char *)sendbuf + rank*sendcount*sendtype_extent),
                                sendcount, sendtype,
                                ((char *)recvbuf + rank*recvcount*recvtype_extent),
                                recvcount, recvtype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    is_pof2 = MPIU_is_pof2(comm_size, NULL);

    /* Do the pairwise exchanges */
    for (i = 1; i < comm_size; i++) {
        if (is_pof2 == 1) {
            /* use exclusive-or algorithm */
            src = dst = rank ^ i;
        }
        else {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
        }

        mpi_errno = MPIR_Sched_send(((char *)sendbuf + dst*sendcount*sendtype_extent),
                                    sendcount, sendtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_recv(((char *)recvbuf + src*recvcount*recvtype_extent),
                                    recvcount, recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

