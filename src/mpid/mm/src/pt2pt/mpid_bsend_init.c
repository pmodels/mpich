/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Bsend_init - mpid_bsend_init

   Parameters:
+  const void * buf - buf
.  int count - count
.  MPI_Datatype datatype - datatype
.  int rank - rank
.  int tag - tag
.  MPID_Comm * comm - comm
.  int context_offset - context offset
-  MPID_Request ** request - request pointer reference

   Notes:
@*/
int MPID_Bsend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset, MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_BSEND_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_BSEND_INIT);

    return mpi_errno;
}
