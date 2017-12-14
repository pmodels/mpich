/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_blocked_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_blocked_sched(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                          const int rdispls[], const MPI_Datatype recvtypes[],
                          MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, i;
    int dst, rank;
    int ii, ss, bblock;
    int type_size;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0) bblock = comm_size;

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (ii = 0; ii < comm_size; ii += bblock) {
        ss = comm_size-ii < bblock ? comm_size-ii : bblock;

        /* do the communication -- post ss sends and receives: */
        for (i = 0; i < ss; i++) {
            dst = (rank + i + ii) % comm_size;
            if (recvcounts[dst]) {
                MPIR_Datatype_get_size_macro(recvtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIR_Sched_recv((char *)recvbuf+rdispls[dst],
                            recvcounts[dst], recvtypes[dst],
                            dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }

        for (i=0; i<ss; i++) {
            dst = (rank-i-ii+comm_size) % comm_size;
            if (sendcounts[dst]) {
                MPIR_Datatype_get_size_macro(sendtypes[dst], type_size);
                if (type_size) {
                    mpi_errno = MPIR_Sched_send((char *)sendbuf+sdispls[dst],
                            sendcounts[dst], sendtypes[dst],
                            dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        MPIR_SCHED_BARRIER(s);
    }
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
