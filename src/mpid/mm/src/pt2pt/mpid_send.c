/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Send - send

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  int dest - destination
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
.  int mode - communicator mode
-  MPID_Request **request_pptr - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPID_Comm *comm_ptr, int mode, MPID_Request **request_pptr)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEND);

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr, mode, request_pptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEND);
    return mpi_errno;
}
