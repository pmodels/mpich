/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Allgatherv_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgatherv_ring ( 
    const void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    const int *recvcounts,
    const int *displs,
    MPI_Datatype recvtype,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int        comm_size, rank, i, left, right;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint recvtype_extent;
    int total_count, recvtype_size;
#ifdef MPID_HAS_HETERO
    int tmp_buf_size, nbytes;
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0) goto fn_exit;
    
    MPIR_Datatype_get_extent_macro( recvtype, recvtype_extent );
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

	char * sbuf = NULL, * rbuf = NULL;
        int soffset, roffset;
	int torecv, tosend, min;
	int sendnow, recvnow;
	int sidx, ridx;

    if (sendbuf != MPI_IN_PLACE) {
        /* First, load the "local" version in the recvbuf. */
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, 
                   ((char *)recvbuf + displs[rank]*recvtype_extent),
                                   recvcounts[rank], recvtype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    left  = (comm_size + rank - 1) % comm_size;
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
    while (tosend || torecv) { /* While we have data to send or receive */
        sendnow = ((recvcounts[sidx] - soffset) > min) ? min : (recvcounts[sidx] - soffset);
        recvnow = ((recvcounts[ridx] - roffset) > min) ? min : (recvcounts[ridx] - roffset);
        sbuf = (char *)recvbuf + ((displs[sidx] + soffset) * recvtype_extent);
        rbuf = (char *)recvbuf + ((displs[ridx] + roffset) * recvtype_extent);

        /* Protect against wrap-around of indices */
        if (!tosend)
            sendnow = 0;
        if (!torecv)
            recvnow = 0;

        /* Communicate */
        if (!sendnow && !recvnow) {
        /* Don't do anything. This case is possible if two
         * consecutive processes contribute 0 bytes each. */
        }
	    else if (!sendnow) { /* If there's no data to send, just do a recv call */
            mpi_errno = MPIC_Recv(rbuf, recvnow, recvtype, left, MPIR_ALLGATHERV_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            torecv -= recvnow;
	    }
	    else if (!recvnow) { /* If there's no data to receive, just do a send call */
            mpi_errno = MPIC_Send(sbuf, sendnow, recvtype, right, MPIR_ALLGATHERV_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            tosend -= sendnow;
	    }
	    else { /* There's data to be sent and received */
            mpi_errno = MPIC_Sendrecv(sbuf, sendnow, recvtype, right, MPIR_ALLGATHERV_TAG,
                                             rbuf, recvnow, recvtype, left, MPIR_ALLGATHERV_TAG,
                                             comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
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
