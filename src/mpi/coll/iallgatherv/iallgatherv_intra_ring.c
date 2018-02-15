/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_sched_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_sched_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, total_count;
    MPI_Aint recvtype_extent;
    int rank, comm_size;
    int left, right;
    char *sbuf = NULL;
    char *rbuf = NULL;
    int soffset, roffset;
    int torecv, tosend, min;
    int sendnow, recvnow;
    int sidx, ridx;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if (sendbuf != MPI_IN_PLACE) {
        /* First, load the "local" version in the recvbuf. */
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    ((char *) recvbuf + displs[rank] * recvtype_extent),
                                    recvcounts[rank], recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    left = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    torecv = total_count - recvcounts[rank];
    tosend = total_count - recvcounts[right];

    min = recvcounts[0];
    for (i = 1; i < comm_size; i++)
        if (min > recvcounts[i])
            min = recvcounts[i];
    if (min * recvtype_extent < MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE)
        min = MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE / recvtype_extent;
    /* Handle the case where the datatype extent is larger than
     * the pipeline size. */
    if (!min)
        min = 1;

    sidx = rank;
    ridx = left;
    soffset = 0;
    roffset = 0;
    while (tosend || torecv) {  /* While we have data to send or receive */
        sendnow = ((recvcounts[sidx] - soffset) > min) ? min : (recvcounts[sidx] - soffset);
        recvnow = ((recvcounts[ridx] - roffset) > min) ? min : (recvcounts[ridx] - roffset);
        sbuf = (char *) recvbuf + ((displs[sidx] + soffset) * recvtype_extent);
        rbuf = (char *) recvbuf + ((displs[ridx] + roffset) * recvtype_extent);

        /* Protect against wrap-around of indices */
        if (!tosend)
            sendnow = 0;
        if (!torecv)
            recvnow = 0;

        /* Communicate */
        if (recvnow) {  /* If there's no data to send, just do a recv call */
            mpi_errno = MPIR_Sched_recv(rbuf, recvnow, recvtype, left, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            torecv -= recvnow;
        }
        if (sendnow) {  /* If there's no data to receive, just do a send call */
            mpi_errno = MPIR_Sched_send(sbuf, sendnow, recvtype, right, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            tosend -= sendnow;
        }
        MPIR_SCHED_BARRIER(s);

        soffset += sendnow;
        roffset += recvnow;
        if (soffset == recvcounts[sidx]) {
            soffset = 0;
            sidx = (sidx + comm_size - 1) % comm_size;
        }
        if (roffset == recvcounts[ridx]) {
            roffset = 0;
            ridx = (ridx + comm_size - 1) % comm_size;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
