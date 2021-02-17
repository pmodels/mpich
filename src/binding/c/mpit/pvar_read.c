/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_read */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_read = PMPI_T_pvar_read
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_read  MPI_T_pvar_read
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_read as PMPI_T_pvar_read
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
     __attribute__ ((weak, alias("PMPI_T_pvar_read")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_read
#define MPI_T_pvar_read PMPI_T_pvar_read

#endif

/*@
   MPI_T_pvar_read - Read the value of a performance variable

Input Parameters:
+ session - identifier of performance experiment session (handle)
- handle - handle of a performance variable (handle)

Output Parameters:
. buf - initial address of storage location for variable value (choice)

Notes:
The MPI_T_pvar_read() call queries the value of the performance variable with the
handle "handle" in the session identified by the parameter session and stores the result
in the buffer identified by the parameter buf. The user is responsible to ensure that the
buffer is of the appropriate size to hold the entire value of the performance variable
(based on the datatype and count returned by the corresponding previous calls to
MPI_T_pvar_get_info() and MPI_T_pvar_handle_alloc(), respectively).

The constant MPI_T_PVAR_ALL_HANDLES cannot be used as an argument for the function
MPI_T_pvar_read().

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_READ);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_READ);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_PVAR_SESSION(session);
            MPIT_ERRTEST_PVAR_HANDLE(handle);
            MPIT_ERRTEST_ARGNULL(buf);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_pvar_read_impl(session, handle, buf);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_READ);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
