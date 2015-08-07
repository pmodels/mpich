/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_reset */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_reset = PMPI_T_pvar_reset
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_reset  MPI_T_pvar_reset
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_reset as PMPI_T_pvar_reset
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle) __attribute__((weak,alias("PMPI_T_pvar_reset")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_reset
#define MPI_T_pvar_reset PMPI_T_pvar_reset

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_reset_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_reset_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    if (MPIR_T_pvar_is_sum(handle)) {
        /* Use zero as starting value */
        memset(handle->accum, 0, handle->bytes * handle->count);

        /* Record current value as offset when pvar is running (i.e., started) */
        if (MPIR_T_pvar_is_started(handle)) {
            if (handle->get_value == NULL) {
                MPIU_Memcpy(handle->offset, handle->addr, handle->bytes * handle->count);
            } else {
                handle->get_value(handle->addr, handle->obj_handle,
                                  handle->count, handle->offset);
            }
        }
    } else if (MPIR_T_pvar_is_watermark(handle)) {
        if (MPIR_T_pvar_is_started(handle)) {
            /* Use the current value as starting value when pvar is running */
            mark = (MPIR_T_pvar_watermark_t *)handle->addr;
            if (MPIR_T_pvar_is_first(handle)) {
                MPIU_Assert(mark->first_used);
                mark->watermark = mark->current;
            } else {
                handle->watermark = mark->current;
            }
        } else {
            /* If pvar is stopped, clear the oncestarted flag
             * so that when it is re-started, it looks new.
             */
            MPIR_T_pvar_unset_oncestarted(handle);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_reset
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_PVAR_NO_WRITE
@*/
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_handle_t *hnd;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_RESET);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_RESET);

     /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_PVAR_SESSION(session, mpi_errno);
            MPIR_ERRTEST_PVAR_HANDLE(handle, mpi_errno);
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* If handle is MPI_T_PVAR_ALL_HANDLES, dispatch the call.
     * Otherwise, do correctness check, then go to impl.
     */
    if (handle == MPI_T_PVAR_ALL_HANDLES) {
        MPL_DL_FOREACH(session->hlist, hnd) {
            if (!MPIR_T_pvar_is_readonly(hnd)) {
                mpi_errno = MPIR_T_pvar_reset_impl(session, hnd);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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
        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_RESET);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_reset", "**mpi_t_pvar_reset %p %p", session, handle);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
