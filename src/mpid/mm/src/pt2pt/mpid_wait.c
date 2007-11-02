/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Wait

/*@
   MPID_Wait - wait

   Arguments:
+  MPID_Request  *request - request
-  MPI_Status   *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Wait(MPID_Request *request, MPI_Status *status)
{
    static const char FCNAME[] = "MPID_Wait";
    MPID_MPI_STATE_DECL(MPID_STATE_MPID_WAIT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_WAIT);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_WAIT);
    return MPI_SUCCESS;
}
