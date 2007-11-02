/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_init - xfer_scatter_init

   Parameters:
+  int src - source
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
-  MPID_Request **request_pptr - request pointer

   Notes:
@*/
int xfer_scatter_init(int src, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr)
{
    MPID_Request *pRequest;

    pRequest = mm_request_alloc();

    pRequest->mm.src = src;
    pRequest->mm.tag = tag;
    pRequest->mm.op_valid = FALSE;
    pRequest->comm = comm_ptr;
    pRequest->cc = 0;
    pRequest->cc_ptr = &pRequest->cc;

    *request_pptr = pRequest;

    return MPI_SUCCESS;
}
