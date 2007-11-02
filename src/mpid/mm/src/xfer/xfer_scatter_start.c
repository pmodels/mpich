/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_start - xfer_scatter_start

   Parameters:
+  MPID_Request *request_ptr - request

   Notes:
@*/
int xfer_scatter_start(MPID_Request *request_ptr)
{
    int mpi_errno;
    MPID_Request *pRequest;

    /* choose the buffers scheme to complete this operation */
    pRequest = request_ptr;
    while (pRequest)
    {
	mpi_errno = mm_choose_buffer(pRequest);
	if (mpi_errno != MPI_SUCCESS)
	{
	    return mpi_errno;
	}
	pRequest = pRequest->mm.next_ptr;
    }

    /* enqueue the cars */

    return MPI_SUCCESS;
}
