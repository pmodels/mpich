/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_write */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_write = PMPI_T_cvar_write
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_write  MPI_T_cvar_write
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_write as PMPI_T_cvar_write
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
     __attribute__ ((weak, alias("PMPI_T_cvar_write")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_write
#define MPI_T_cvar_write PMPI_T_cvar_write

#endif

/*@
   MPI_T_cvar_write - Write a control variable

Input Parameters:
+ handle - handle to the control variable to be written (handle)
- buf - initial address of storage location for variable value (choice)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_CVAR_SET_NEVER
.N MPI_T_ERR_CVAR_SET_NOT_NOW
.N MPI_T_ERR_INVALID
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_WRITE);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_WRITE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_CVAR_HANDLE(handle);
            MPIT_ERRTEST_ARGNULL(buf);
            MPIR_T_cvar_handle_t *hnd = handle;

            if (hnd->scope == MPI_T_SCOPE_CONSTANT) {
                mpi_errno = MPI_T_ERR_CVAR_SET_NEVER;
                goto fn_fail;
            } else if (hnd->scope == MPI_T_SCOPE_READONLY) {
                mpi_errno = MPI_T_ERR_CVAR_SET_NOT_NOW;
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_cvar_write_impl(handle, buf);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_WRITE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
