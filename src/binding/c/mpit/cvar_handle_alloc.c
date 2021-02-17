/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_handle_alloc */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_handle_alloc = PMPI_T_cvar_handle_alloc
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_handle_alloc  MPI_T_cvar_handle_alloc
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_handle_alloc as PMPI_T_cvar_handle_alloc
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle,
                            int *count)  __attribute__ ((weak, alias("PMPI_T_cvar_handle_alloc")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_handle_alloc
#define MPI_T_cvar_handle_alloc PMPI_T_cvar_handle_alloc

#endif

/*@
   MPI_T_cvar_handle_alloc - Allocate a handle for a control variable

Input Parameters:
+ cvar_index - index of control variable for which handle is to be allocated (index)
- obj_handle - reference to a handle of the mpi object to which this variable is supposed to be bound (pointer)

Output Parameters:
+ handle - allocated handle (handle)
- count - number of elements used to represent this variable (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID
.N MPI_T_ERR_INVALID_INDEX
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_OUT_OF_HANDLES
@*/

int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle,
                            int *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_CVAR_INDEX(cvar_index);
            MPIT_ERRTEST_ARGNULL(handle);
            MPIT_ERRTEST_ARGNULL(count);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_cvar_handle_alloc_impl(cvar_index, obj_handle, handle, count);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
