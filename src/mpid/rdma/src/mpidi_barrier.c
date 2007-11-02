/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_Barrier
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Barrier(MPID_Comm * comm)
{
    const int size = comm->remote_size;
    const int rank = comm->rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_BARRIER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_BARRIER);

    if (size > 1)
    {
	int size_pow2;
	int size_extra;
	int p;
	int l;
	MPID_Request * sreq;
	MPID_Request * rreq;
	MPI_Status status;
	MPI_Request request;
	
	size_pow2 = 4;
	while (size_pow2 <= size)
	{
	    size_pow2 <<= 1;
	}
	size_pow2 >>= 1;
	size_extra = size - size_pow2;
    
	if (rank < size_pow2)
	{
	    if(rank < size_extra)
	    {
		/* If I have a partner in the set of extra processes, wait for
                   that process to check in */
		p = size_pow2 + rank;
		MPID_Recv(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
			  MPID_CONTEXT_INTRA_COLL, &status, &rreq);
		if (rreq != NULL)
		{
		    request = rreq->handle;
		    MPI_Wait(&request, &status);
		}
	    }

	    /* perform log-N exchanges among first size_pow2 processes */
	    for (l = 1; l < size_pow2; l <<= 1)
	    {
		p = rank ^ l;

		MPID_Irecv(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
			   MPID_CONTEXT_INTRA_COLL, &rreq);
		MPID_Send(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
			  MPID_CONTEXT_INTRA_COLL, &sreq);
		if (sreq != NULL)
		{
		    request = sreq->handle;
		    MPI_Wait(&request, &status);
		}
		request = rreq->handle;
		MPI_Wait(&request, &status);
	    }

	    /* If I have a partner in the set of extra processes, then notify
               that process that the barrier is complete */
	    /* fanout data to nodes above N2_prev... */
	    if (rank < size_extra)
	    {
		p = size_pow2 + rank;
		MPID_Send(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
				 MPID_CONTEXT_INTRA_COLL, &sreq);
		if (sreq != NULL)
		{
		    request = sreq->handle;
		    MPI_Wait(&request, &status);
		}
	    }
	} 
	else
	{
	    /* If I am an extra process (process greater than the largest power
               of 2 less than size), then perform one exchange with my partner
               in the main set of processes. */
	    p = rank - size_pow2;
	    MPID_Irecv(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
		       MPID_CONTEXT_INTRA_COLL, &rreq);
	    MPID_Send(NULL, 0, MPI_BYTE, p, MPIR_BARRIER_TAG, comm,
		      MPID_CONTEXT_INTRA_COLL, &sreq);
	    if (sreq != NULL)
	    {
		request = sreq->handle;
		MPI_Wait(&request, & status);
	    }
	    request = rreq->handle;
	    MPI_Wait(&request, & status);

	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_BARRIER);
    return MPI_SUCCESS;
}
