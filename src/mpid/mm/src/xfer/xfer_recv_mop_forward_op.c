/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_recv_mop_forward_op - xfer_recv_mop_forward_op

   Parameters:
+  MPID_Request *request_ptr - request
.  void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
.  int last - last
.  int src - source
-  int dest - destination

   Notes:
@*/
int xfer_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src, int dest)
{
    MPIDI_STATE_DECL(MPID_STATE_XFER_RECV_MOP_FORWARD_OP);
    MPIDI_FUNC_ENTER(MPID_STATE_XFER_RECV_MOP_FORWARD_OP);
    MPIDI_FUNC_EXIT(MPID_STATE_XFER_RECV_MOP_FORWARD_OP);
    return MPI_SUCCESS;
}
