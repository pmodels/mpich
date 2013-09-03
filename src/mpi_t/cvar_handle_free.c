/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_handle_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_handle_free = PMPI_T_cvar_handle_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_handle_free  MPI_T_cvar_handle_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_handle_free as PMPI_T_cvar_handle_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_handle_free
#define MPI_T_cvar_handle_free PMPI_T_cvar_handle_free
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_cvar_handle_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_T_cvar_handle_free - XXX description here

Input/Output Parameters:
. handle - handle to be freed (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);
    MPIR_T_FAIL_IF_UNINITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);

    /* ... body of routine ...  */

    if (handle != NULL) {
        MPIR_T_cvar_handle_t *hnd = *handle;
        MPIU_Free(hnd);
        *handle = MPI_T_CVAR_HANDLE_NULL;
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_CVAR_HANDLE_FREE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_cvar_handle_free", "**mpi_t_cvar_handle_free %p", handle);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
