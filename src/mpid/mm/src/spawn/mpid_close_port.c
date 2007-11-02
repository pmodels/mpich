/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Close_port - close port

   Arguments:
.  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Close_port(char *port_name)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);

    ret_val = mm_close_port(port_name);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return ret_val;
}
