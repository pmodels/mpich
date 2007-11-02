/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_init - xfer_init

   Parameters:
+  int tag - tag
.  MPID_Comm *comm_ptr - communicator
-  MPID_Request **request_pptr - request pointer

   Notes:
@*/
int xfer_init(int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr)
{
    MPID_Request *pRequest;
    MPIDI_STATE_DECL(MPID_STATE_XFER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_XFER_INIT);

    pRequest = mm_request_alloc();

    pRequest->comm = comm_ptr;
    pRequest->mm.tag = tag;
    pRequest->mm.op_valid = FALSE;
    pRequest->cc = 0;
    pRequest->cc_ptr = &pRequest->cc;

    *request_pptr = pRequest;

    MPIDI_FUNC_EXIT(MPID_STATE_XFER_INIT);
    return MPI_SUCCESS;
}
