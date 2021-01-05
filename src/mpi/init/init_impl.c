/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_SUPPRESS_ABORT_MESSAGE
      category    : ERROR_HANDLING
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Disable printing of abort error message.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Global variables */
#ifndef MPIR_SESSION_PREALLOC
#define MPIR_SESSION_PREALLOC 2
#endif

MPIR_Session MPIR_Session_direct[MPIR_SESSION_PREALLOC];

MPIR_Object_alloc_t MPIR_Session_mem = { 0, 0, 0, 0,
    MPIR_SESSION, sizeof(MPIR_Session),
    MPIR_Session_direct, MPIR_SESSION_PREALLOC,
    NULL
};

int MPIR_Abort_impl(MPIR_Comm * comm_ptr, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;

    if (!comm_ptr) {
        /* Use comm world if the communicator is not valid */
        comm_ptr = MPIR_Process.comm_self;
    }

    char abort_str[MPI_MAX_OBJECT_NAME + 100] = "";
    char comm_name[MPI_MAX_OBJECT_NAME];
    int len = MPI_MAX_OBJECT_NAME;
    MPIR_Comm_get_name_impl(comm_ptr, comm_name, &len);
    if (len == 0) {
        MPL_snprintf(comm_name, MPI_MAX_OBJECT_NAME, "comm=0x%X", comm_ptr->handle);
    }
    if (!MPIR_CVAR_SUPPRESS_ABORT_MESSAGE)
        /* FIXME: This is not internationalized */
        MPL_snprintf(abort_str, sizeof(abort_str),
                     "application called MPI_Abort(%s, %d) - process %d", comm_name, errorcode,
                     comm_ptr->rank);
    mpi_errno = MPID_Abort(comm_ptr, mpi_errno, errorcode, abort_str);

    return mpi_errno;
}
