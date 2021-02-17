/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_read */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_read = PMPI_T_cvar_read
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_read  MPI_T_cvar_read
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_read as PMPI_T_cvar_read
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
     __attribute__ ((weak, alias("PMPI_T_cvar_read")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_read
#define MPI_T_cvar_read PMPI_T_cvar_read

#endif

/*@
   MPI_T_cvar_read - Read the value of a control variable

Input Parameters:
. handle - handle to the control variable to be read (handle)

Output Parameters:
. buf - initial address of storage location for variable value (choice)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_READ);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_READ);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_CVAR_HANDLE(handle);
            MPIT_ERRTEST_ARGNULL(buf);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_cvar_read_impl(handle, buf);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_READ);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
