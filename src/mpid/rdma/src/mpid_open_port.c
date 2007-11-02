/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Open_port - short description

   Input Arguments:
+  MPI_Info info - info
-  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);

    mpi_errno = MPIDI_CH3_Open_port(port_name);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);

    return mpi_errno;
}
