/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_read */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_read = PMPI_T_pvar_read
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_read  MPI_T_pvar_read
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_read as PMPI_T_pvar_read
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) __attribute__((weak,alias("PMPI_T_pvar_read")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_read
#define MPI_T_pvar_read PMPI_T_pvar_read

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_read_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_read_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *restrict buf)
{
    int i, mpi_errno = MPI_SUCCESS;

    /* Reading a never started pvar, or a stopped and then reset wartermark,
     * will run into this nasty situation. MPI-3.0 did not define what error
     * code should be returned. We returned a generic MPI error code. With MPI-3.1
     * approved, we changed it to MPI_T_ERR_INVALID.
     */
    if (!MPIR_T_pvar_is_oncestarted(handle)) {
        mpi_errno = MPI_T_ERR_INVALID;
        goto fn_fail;
    }

    /* For SUM pvars, return accum value + current value - offset value */
    if (MPIR_T_pvar_is_sum(handle) && MPIR_T_pvar_is_started(handle)) {
        if (handle->get_value == NULL) {
            /* A running SUM without callback. Read its current value directly */
            switch (handle->datatype) {
            case MPI_UNSIGNED_LONG_LONG:
                /* Put long long and double at front, since they are common */
                for (i = 0; i < handle->count; i++) {
                    ((unsigned long long *)buf)[i] =
                        ((unsigned long long *)handle->accum)[i]
                        + ((unsigned long long *)handle->addr)[i]
                        - ((unsigned long long *)handle->offset)[i];
                }
                break;
            case MPI_DOUBLE:
                for (i = 0; i < handle->count; i++) {
                    ((double *)buf)[i] =
                        ((double *)handle->accum)[i]
                        + ((double *)handle->addr)[i]
                        - ((double *)handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned *)buf)[i] =
                        ((unsigned *)handle->accum)[i]
                        + ((unsigned *)handle->addr)[i]
                        - ((unsigned *)handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED_LONG:
                for (i = 0; i < handle->count; i++) {
                   ((unsigned long *)buf)[i] =
                        ((unsigned long *)handle->accum)[i]
                        + ((unsigned long *)handle->addr)[i]
                        - ((unsigned long *)handle->offset)[i];
                }
                break;
            default:
                /* Code should never come here */
                mpi_errno = MPI_ERR_INTERN; goto fn_fail;
                break;
            }
        } else {
            /* A running SUM with callback. Read its current value into handle */
            handle->get_value(handle->addr, handle->obj_handle,
                              handle->count, handle->current);

            switch (handle->datatype) {
            case MPI_UNSIGNED_LONG_LONG:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned long long *)buf)[i] =
                        ((unsigned long long *)handle->accum)[i]
                        + ((unsigned long *)handle->current)[i]
                        - ((unsigned long long *)handle->offset)[i];
                }
                break;
            case MPI_DOUBLE:
                for (i = 0; i < handle->count; i++) {
                    ((double *)buf)[i] =
                        ((double *)handle->accum)[i]
                        + ((double *)handle->current)[i]
                        - ((double *)handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned *)buf)[i] =
                        ((unsigned *)handle->accum)[i]
                        + ((unsigned *)handle->current)[i]
                        - ((unsigned *)handle->offset)[i];
                }
                break;
            case MPI_UNSIGNED_LONG:
                for (i = 0; i < handle->count; i++) {
                    ((unsigned long *)buf)[i] =
                        ((unsigned long *)handle->accum)[i]
                        + ((unsigned long *)handle->current)[i]
                        - ((unsigned long *)handle->offset)[i];
                }
                break;
            default:
                /* Code should never come here */
                mpi_errno = MPI_ERR_INTERN; goto fn_fail;
                break;
            }
        }
    }
    else if (MPIR_T_pvar_is_sum(handle) && !MPIR_T_pvar_is_started(handle)) {
        /* A SUM is stopped. Return accum directly */
        MPIU_Memcpy(buf, handle->accum, handle->bytes * handle->count);
    }
    else if (MPIR_T_pvar_is_watermark(handle)) {
        /* Callback and array are not allowed for watermarks, since they
         * can not gurantee correct semantics of watermarks.
         */
        MPIU_Assert(handle->get_value == NULL && handle->count == 1);

        if (MPIR_T_pvar_is_first(handle)) {
            /* Current value of the first handle of a watermark is stored at
             * a special location nearby the watermark itself.
             */
            switch (handle->datatype) {
            case MPI_UNSIGNED_LONG_LONG:
                 *(unsigned long long *)buf =
                    ((MPIR_T_pvar_watermark_t *)handle->addr)->watermark.ull;
                 break;
            case MPI_DOUBLE:
                 *(double *)buf =
                    ((MPIR_T_pvar_watermark_t *)handle->addr)->watermark.f;
                 break;
            case MPI_UNSIGNED:
                 *(unsigned *)buf =
                    ((MPIR_T_pvar_watermark_t *)handle->addr)->watermark.u;
                 break;
            case MPI_UNSIGNED_LONG:
                 *(unsigned long *)buf =
                    ((MPIR_T_pvar_watermark_t *)handle->addr)->watermark.ul;
                 break;
            default:
                /* Code should never come here */
                 mpi_errno = MPI_ERR_INTERN; goto fn_fail;
                 break;
            }
        } else {
            /* For remaining handles, their current value are in the handle */
            switch (handle->datatype) {
            case MPI_UNSIGNED_LONG_LONG:
                 *(unsigned long long *)buf = handle->watermark.ull;
                 break;
            case MPI_DOUBLE:
                 *(double *)buf = handle->watermark.f;
                 break;
            case MPI_UNSIGNED:
                 *(unsigned *)buf = handle->watermark.u;
                 break;
            case MPI_UNSIGNED_LONG:
                 *(unsigned long *)buf = handle->watermark.ul;
                 break;
            default:
                /* Code should never come here */
                 mpi_errno = MPI_ERR_INTERN; goto fn_fail;
                 break;
            }
        }
    } else {
        /* For STATE, LEVEL, SIZE, PERCENTAGE, no caching is needed */
        if (handle->get_value == NULL)
            MPIU_Memcpy(buf, handle->addr, handle->bytes * handle->count);
        else
            handle->get_value(handle->addr, handle->obj_handle,
                              handle->count, buf);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_HANDLE
@*/
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_READ);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_READ);

    /* Validate parameters */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_PVAR_SESSION(session, mpi_errno);
            MPIR_ERRTEST_PVAR_HANDLE(handle, mpi_errno);
            MPIR_ERRTEST_ARGNULL(buf, "buf", mpi_errno);
            if (handle == MPI_T_PVAR_ALL_HANDLES  || session != handle->session
                || !MPIR_T_pvar_is_oncestarted(handle))
            {
                mpi_errno = MPI_T_ERR_INVALID_HANDLE;
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_read_impl(session, handle, buf);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_READ);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_read", "**mpi_t_pvar_read %p %p %p", session, handle, buf);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
