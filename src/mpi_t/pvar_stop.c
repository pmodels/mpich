/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_stop */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_stop = PMPI_T_pvar_stop
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_stop  MPI_T_pvar_stop
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_stop as PMPI_T_pvar_stop
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle) __attribute__((weak,alias("PMPI_T_pvar_stop")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_stop
#define MPI_T_pvar_stop PMPI_T_pvar_stop

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_stop_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_stop_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    MPIR_T_pvar_unset_started(handle);

    /* Side-effect when pvar is SUM or WATERMARK */
    if (MPIR_T_pvar_is_sum(handle)) {
        /* Read the current value first */
        if (handle->get_value == NULL) {
            MPIU_Memcpy(handle->current, handle->addr, handle->bytes * handle->count);
        } else {
            handle->get_value(handle->addr, handle->obj_handle,
                              handle->count, handle->current);
        }

        /* Substract offset from current, and accumulate the result to accum */
        switch (handle->datatype) {
        case MPI_UNSIGNED_LONG_LONG:
            for (i = 0; i < handle->count; i++) {
                ((unsigned long long *)handle->accum)[i] +=
                    ((unsigned long long *)handle->current)[i]
                    - ((unsigned long long *)handle->offset)[i];
            }
            break;
        case MPI_DOUBLE:
            for (i = 0; i < handle->count; i++) {
                ((double *)handle->accum)[i] +=
                    ((double *)handle->current)[i]
                    - ((double *)handle->offset)[i];
            }
            break;
        case MPI_UNSIGNED:
            for (i = 0; i < handle->count; i++) {
                ((unsigned *)handle->accum)[i] +=
                    ((unsigned *)handle->current)[i]
                    - ((unsigned *)handle->offset)[i];
            }
            break;
        case MPI_UNSIGNED_LONG:
            for (i = 0; i < handle->count; i++) {
                ((unsigned long *)handle->accum)[i] +=
                    ((unsigned long *)handle->current)[i]
                    - ((unsigned long *)handle->offset)[i];
            }
            break;
        default:
            /* Code should never come here */
            mpi_errno = MPI_ERR_INTERN; goto fn_fail;
            break;
        }

    } else if (MPIR_T_pvar_is_watermark(handle)) {
        /* When handle is first, clear the flag in pvar too */
        if (MPIR_T_pvar_is_first(handle)) {
            mark = (MPIR_T_pvar_watermark_t *)handle->addr;
            MPIU_Assert(mark->first_used);
            mark->first_started = FALSE;
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_stop
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_pvar_stop - Stop a performance variable

Input Parameters:
+ session - identifier of performance experiment session (handle)
- handle - handle of a performance variable (handle)

Notes:
This functions stops the performance variable with the handle identified by the parameter
handle in the session identified by the parameter session.

If the constant MPI_T_PVAR_ALL_HANDLES is passed in handle, the MPI implementation
attempts to stop all variables within the session identified by the parameter session for
which handles have been allocated. In this case, the routine returns MPI_SUCCESS if all
variables are stopped successfully, otherwise MPI_T_ERR_PVAR_NO_STARTSTOP is returned.
Continuous variables and variables that are already stopped are ignored when
MPI_T_PVAR_ALL_HANDLES is specified.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_PVAR_NO_STARTSTOP
@*/
int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_STOP);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_STOP);

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
        MPIR_T_pvar_handle_t *hnd;
        MPL_DL_FOREACH(session->hlist, hnd) {
            if (!MPIR_T_pvar_is_continuous(hnd) && MPIR_T_pvar_is_started(hnd)) {
                mpi_errno = MPIR_T_pvar_stop_impl(session, hnd);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }
        }
    } else {
        if (handle->session != session) {
            mpi_errno = MPI_T_ERR_INVALID_HANDLE;
            goto fn_fail;
        }
        if (MPIR_T_pvar_is_continuous(handle)) {
            mpi_errno = MPI_T_ERR_PVAR_NO_STARTSTOP;
            goto fn_fail;
        }

        if (MPIR_T_pvar_is_started(handle)) {
            mpi_errno = MPIR_T_pvar_stop_impl(session, handle);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_STOP);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_stop", "**mpi_t_pvar_stop %p %p", session, handle);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
