/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Startall - mpid_startall

   Parameters:
+  int count - count
-  MPID_Request * requests[] - requests

   Notes:
@*/
int MPID_Startall(int count, MPID_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_STARTALL);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_STARTALL);

    return mpi_errno;
}
