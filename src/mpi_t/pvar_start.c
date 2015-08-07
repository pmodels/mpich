/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h" /* For MPL_DL_FOREACH */

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_start = PMPI_T_pvar_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_start  MPI_T_pvar_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_start as PMPI_T_pvar_start
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle) __attribute__((weak,alias("PMPI_T_pvar_start")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_start
#define MPI_T_pvar_start PMPI_T_pvar_start

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_start_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_start_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_watermark_t *mark;

    if (MPIR_T_pvar_is_sum(handle)) {
        /* To start SUM, we only need to cache its current value into offset.
         * If it has ever been started, accum holds correct value. Otherwise,
         * accum is zero since handle allocation.
         */
        if (handle->get_value == NULL) {
            MPIU_Memcpy(handle->offset, handle->addr, handle->bytes * handle->count);
        } else {
            handle->get_value(handle->addr, handle->obj_handle,
                              handle->count, handle->offset);
        }
    } else if (MPIR_T_pvar_is_watermark(handle)) {
        /* To start WATERMARK, if it has not ever been started,
         * we need to set its starting value properly.
         */
        mark = (MPIR_T_pvar_watermark_t *)handle->addr;

        if (MPIR_T_pvar_is_first(handle)) {
            MPIU_Assert(mark->first_used);
            mark->first_started = TRUE;
            if (!MPIR_T_pvar_is_oncestarted(handle))
                mark->watermark = mark->current;
        } else {
            if (!MPIR_T_pvar_is_oncestarted(handle))
                handle->watermark = mark->current;
        }
    }

    /* OK, let's get started */
    MPIR_T_pvar_set_started(handle);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_pvar_start - Start a performance variable

Input Parameters:
+ session - identifier of performance experiment session (handle)
- handle - handle of a performance variable (handle)

Notes:
If the constant MPI_T_PVAR_ALL_HANDLES is passed in handle, the MPI implementation
attempts to start all variables within the session identified by the parameter session for
which handles have been allocated. In this case, the routine returns MPI_SUCCESS if all
variables are started successfully, otherwise MPI_T_ERR_PVAR_NO_STARTSTOP is returned.
Continuous variables and variables that are already started are ignored when
MPI_T_PVAR_ALL_HANDLES is specified.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_PVAR_NO_STARTSTOP
@*/
int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_START);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_START);

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

    if (handle == MPI_T_PVAR_ALL_HANDLES) {
        MPIR_T_pvar_handle_t *hnd;
        MPL_DL_FOREACH(session->hlist, hnd) {
            if (!MPIR_T_pvar_is_continuous(hnd) && !MPIR_T_pvar_is_started(hnd))
                mpi_errno = MPIR_T_pvar_start_impl(session, hnd);
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

        if (!MPIR_T_pvar_is_started(handle)) {
            mpi_errno = MPIR_T_pvar_start_impl(session, handle);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_START);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_start", "**mpi_t_pvar_start %p %p", session, handle);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
