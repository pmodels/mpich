/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

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

    - name        : MPIR_CVAR_COREDUMP_ON_ABORT
      category    : ERROR_HANDLING
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Call libc abort() to generate a corefile

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Initialized_impl(int *flag)
{
    *flag = MPII_world_is_initialized();
    return MPI_SUCCESS;
}

int MPIR_Finalized_impl(int *flag)
{
    *flag = MPII_world_is_finalized();
    return MPI_SUCCESS;
}

int MPIR_Query_thread_impl(int *provided)
{
    *provided = MPIR_ThreadInfo.thread_provided;
    return MPI_SUCCESS;
}

int MPIR_Is_thread_main_impl(int *flag)
{
#if MPICH_THREAD_LEVEL <= MPI_THREAD_FUNNELED || ! defined(MPICH_IS_THREADED)
    {
        *flag = TRUE;
    }
#else
    {
        MPID_Thread_id_t my_thread_id;

        MPID_Thread_self(&my_thread_id);
        MPID_Thread_same(&MPIR_ThreadInfo.main_thread, &my_thread_id, flag);
    }
#endif
    return MPI_SUCCESS;
}

int MPIR_Get_version_impl(int *version, int *subversion)
{
    *version = MPI_VERSION;
    *subversion = MPI_SUBVERSION;

    return MPI_SUCCESS;
}

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
        snprintf(comm_name, MPI_MAX_OBJECT_NAME, "comm=0x%X", comm_ptr->handle);
    }
    if (!MPIR_CVAR_SUPPRESS_ABORT_MESSAGE)
        /* FIXME: This is not internationalized */
        snprintf(abort_str, sizeof(abort_str),
                 "application called MPI_Abort(%s, %d) - process %d", comm_name, errorcode,
                 comm_ptr->rank);
    mpi_errno = MPID_Abort(comm_ptr, mpi_errno, errorcode, abort_str);

    return mpi_errno;
}
