/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_reset */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_reset = PMPI_T_pvar_reset
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_reset  MPI_T_pvar_reset
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_reset as PMPI_T_pvar_reset
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
     __attribute__ ((weak, alias("PMPI_T_pvar_reset")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_reset
#define MPI_T_pvar_reset PMPI_T_pvar_reset

#endif

/*@
   MPI_T_pvar_reset - Reset a performance variable

Input Parameters:
+ session - identifier of performance experiment session (handle)
- handle - handle of a performance variable (handle)

Notes:
The MPI_T_pvar_reset() call sets the performance variable with the handle identified
by the parameter handle to its starting value. If it is not possible
to change the variable, the function returns MPI_T_ERR_PVAR_NO_WRITE.
If the constant MPI_T_PVAR_ALL_HANDLES is passed in handle, the MPI implementation
attempts to reset all variables within the session identified by the parameter session for
which handles have been allocated. In this case, the routine returns MPI_SUCCESS if all
variables are reset successfully, otherwise MPI_T_ERR_PVAR_NO_WRITE is returned. Readonly
variables are ignored when MPI_T_PVAR_ALL_HANDLES is specified.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_PVAR_NO_WRITE
@*/

int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_RESET);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_RESET);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_PVAR_SESSION(session);
            MPIT_ERRTEST_PVAR_HANDLE(handle);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_T_pvar_handle_t *hnd;
    /* If handle is MPI_T_PVAR_ALL_HANDLES, dispatch the call.
     * Otherwise, do correctness check, then go to impl.
     */
    if (handle == MPI_T_PVAR_ALL_HANDLES) {
        DL_FOREACH(session->hlist, hnd) {
            if (!MPIR_T_pvar_is_readonly(hnd)) {
                mpi_errno = MPIR_T_pvar_reset_impl(session, hnd);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
        }
    } else {
        if (handle->session != session) {
            mpi_errno = MPI_T_ERR_INVALID_HANDLE;
            goto fn_fail;
        }

        if (MPIR_T_pvar_is_readonly(handle)) {
            mpi_errno = MPI_T_ERR_PVAR_NO_WRITE;
            goto fn_fail;
        }

        mpi_errno = MPIR_T_pvar_reset_impl(session, handle);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_RESET);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
