/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_gather_send_op - xfer_gather_send_op

   Parameters:
+  MPID_Request *request_ptr - request
.  const void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
-  int last - last

   Notes:
@*/
int xfer_gather_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last)
{
    return MPI_SUCCESS;
}
