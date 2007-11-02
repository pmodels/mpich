/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_forward_op - xfer_scatter_forward_op

   Parameters:
+  MPID_Request *request_ptr - request
.  int size - size
-  int dest - destination

   Notes:
@*/
int xfer_scatter_forward_op(MPID_Request *request_ptr, int size, int dest)
{
    return MPI_SUCCESS;
}
