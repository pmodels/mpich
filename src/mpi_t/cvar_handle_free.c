/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_handle_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_handle_free = PMPI_T_cvar_handle_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_handle_free  MPI_T_cvar_handle_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_handle_free as PMPI_T_cvar_handle_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_handle_free(MPI_T_cvar_handle * handle)
    __attribute__ ((weak, alias("PMPI_T_cvar_handle_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_handle_free
#define MPI_T_cvar_handle_free PMPI_T_cvar_handle_free
#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_cvar_handle_free - Free an existing handle for a control variable

Input/Output Parameters:
. handle - handle to be freed (handle)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_HANDLE
@*/
int MPI_T_cvar_handle_free(MPI_T_cvar_handle * handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_cvar_handle_t *hnd;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);

    /* Validate parameters */
    MPIT_ERRTEST_ARGNULL(handle);

    /* ... body of routine ...  */

    hnd = *handle;
    MPL_free(hnd);
    *handle = MPI_T_CVAR_HANDLE_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
