/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Ring Algorithm:
 *
 * In the first step, each process i sends its contribution to process
 * i+1 and receives the contribution from process i-1 (with
 * wrap-around).  From the second step onwards, each process i
 * forwards to process i+1 the data it received from process i-1 in
 * the previous step.  This takes a total of p-1 steps.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta
 *
 * This algorithm is preferred to recursive doubling for long messages
 * because we find that this communication pattern (nearest neighbor)
 * performs twice as fast as recursive doubling for long messages (on
 * Myrinet and IBM SP).
 */

int MPIR_Allgatherv_intra_ring(const void *sendbuf,
                               MPI_Aint sendcount,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               const MPI_Aint * recvcounts,
                               const MPI_Aint * displs,
                               MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank, i, left, right;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint recvtype_extent;
    int total_count;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    if (sendbuf != MPI_IN_PLACE) {
        /* First, load the "local" version in the recvbuf. */
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   ((char *) recvbuf + displs[rank] * recvtype_extent),
                                   recvcounts[rank], recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    left = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    MPI_Aint torecv, tosend, max, chunk_count;
    torecv = total_count - recvcounts[rank];
    tosend = total_count - recvcounts[right];

    chunk_count = 0;
    max = recvcounts[0];
    for (i = 1; i < comm_size; i++)
        if (max < recvcounts[i])
            max = recvcounts[i];
    if (MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE > 0 &&
        max * recvtype_extent > MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE) {
        chunk_count = MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE / recvtype_extent;
        /* Handle the case where the datatype extent is larger than
         * the pipeline size. */
        if (!chunk_count)
            chunk_count = 1;
    }
    /* pipeline is disabled */
    if (!chunk_count)
        chunk_count = max;

    int soffset, roffset;
    int sidx, ridx;
    sidx = rank;
    ridx = left;
    soffset = 0;
    roffset = 0;
    while (tosend || torecv) {  /* While we have data to send or receive */
        MPI_Aint sendnow, recvnow;
        sendnow = ((recvcounts[sidx] - soffset) >
                   chunk_count) ? chunk_count : (recvcounts[sidx] - soffset);
        recvnow = ((recvcounts[ridx] - roffset) >
                   chunk_count) ? chunk_count : (recvcounts[ridx] - roffset);

        char *sbuf, *rbuf;
        sbuf = (char *) recvbuf + ((displs[sidx] + soffset) * recvtype_extent);
        rbuf = (char *) recvbuf + ((displs[ridx] + roffset) * recvtype_extent);

        /* Protect against wrap-around of indices */
        if (!tosend)
            sendnow = 0;
        if (!torecv)
            recvnow = 0;

        /* Communicate */
        if (!sendnow && !recvnow) {
            /* Don't do anything. This case is possible if two
             * consecutive processes contribute 0 bytes each. */
        } else if (!sendnow) {  /* If there's no data to send, just do a recv call */
            mpi_errno =
                MPIC_Recv(rbuf, recvnow, recvtype, left, MPIR_ALLGATHERV_TAG, comm_ptr, &status,
                          errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            torecv -= recvnow;
        } else if (!recvnow) {  /* If there's no data to receive, just do a send call */
            mpi_errno =
                MPIC_Send(sbuf, sendnow, recvtype, right, MPIR_ALLGATHERV_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            tosend -= sendnow;
        } else {        /* There's data to be sent and received */
            mpi_errno = MPIC_Sendrecv(sbuf, sendnow, recvtype, right, MPIR_ALLGATHERV_TAG,
                                      rbuf, recvnow, recvtype, left, MPIR_ALLGATHERV_TAG,
                                      comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            tosend -= sendnow;
            torecv -= recvnow;
        }

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
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
