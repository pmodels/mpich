/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Test

/*@
   MPID_Test - test

   Arguments:
+  MPID_Request *request - request
.  int *flag - flag
-  MPI_Status *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Test(MPID_Request *request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPID_Test";
    MPID_MPI_STATE_DECL(MPID_STATE_MPID_TEST);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_TEST);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_TEST);
    return MPI_SUCCESS;
}
