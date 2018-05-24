/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPIR_REQUEST_PTR_ARRAY_SIZE)
#define MPIR_REQUEST_PTR_ARRAY_SIZE 16
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Waitany
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Waitany_impl(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    int i;
    int found_nonnull_req;
    int n_inactive;

    MPID_Progress_start(&progress_state);
    for (;;) {
        n_inactive = 0;
        found_nonnull_req = FALSE;

        for (i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test();
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_Progress_end(&progress_state);
                    goto fn_fail;
                }
            }

            if (request_ptrs[i] == NULL) {
                ++n_inactive;
                continue;
            }
            /* we found at least one non-null request */
            found_nonnull_req = TRUE;

            if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                mpi_errno = MPIR_Grequest_poll(request_ptrs[i], status);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                if (MPIR_Request_is_active(request_ptrs[i])) {
                    *indx = i;
                    goto fn_exit;
                } else {
                    ++n_inactive;
                    request_ptrs[i] = NULL;

                    if (n_inactive == count) {
                        *indx = MPI_UNDEFINED;
                        /* status is set to empty by MPIR_Request_completion_processing */
                        goto fn_exit;
                    }
                }
            }
        }

        if (!found_nonnull_req) {
            /* all requests were NULL */
            *indx = MPI_UNDEFINED;
            if (status != NULL) /* could be null if count=0 */
                MPIR_Status_set_empty(status);
            goto fn_exit;
        }

        mpi_errno = MPID_Progress_test();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }

  fn_exit:
    MPID_Progress_end(&progress_state);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* -- Begin Profiling Symbol Block for routine MPI_Waitany */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitany = PMPI_Waitany
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitany  MPI_Waitany
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitany as PMPI_Waitany
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_Waitany")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitany
#define MPI_Waitany PMPI_Waitany

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitany
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Waitany - Waits for any specified MPI Request to complete

Input Parameters:
+ count - list length (integer)
- array_of_requests - array of requests (array of handles)

Output Parameters:
+ indx - index of handle for operation that completed (integer).  In the
range '0' to 'count-1'.  In Fortran, the range is '1' to 'count'.
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

Notes:
If all of the requests are 'MPI_REQUEST_NULL', then 'indx' is returned as
'MPI_UNDEFINED', and 'status' is returned as an empty status.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status * status)
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    int i;
    int active_flag;
    int last_disabled_anysource = -1;
    int first_nonnull = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WAITANY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_WAITANY);

    /* Check the arguments */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COUNT(count, mpi_errno);

            if (count != 0) {
                MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
                /* NOTE: MPI_STATUS_IGNORE != NULL */
                MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(indx, "indx", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPIR_Request **, count * sizeof(MPIR_Request *),
                                   mpi_errno, "request pointers", MPL_MEM_OBJECT);
    }

    *indx = MPI_UNDEFINED;

    for (i = 0; i < count; i++) {
#ifdef HAVE_ERROR_CHECKING
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
#endif /* HAVE_ERROR_CHECKING */
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
            /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
            {
                MPID_BEGIN_ERROR_CHECKS;
                {
                    MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }
                MPID_END_ERROR_CHECKS;
            }
#endif

            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                last_disabled_anysource = i;
            }

            /* Since waitany is likely to return as soon as a
             * completed request is found, the first loop here is
             * taking the longest. We can check for any completed
             * request here, and we can poll from the first entry
             * which is not null. */
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                if (MPIR_Request_is_active(request_ptrs[i])) {
                    *indx = i;
                    break;
                } else {
                    request_ptrs[i] = NULL;
                }
            } else {
                if (!first_nonnull)
                    first_nonnull = i;
            }
        } else {
            request_ptrs[i] = NULL;
        }
    }

    if (*indx == MPI_UNDEFINED) {
        if (unlikely(last_disabled_anysource != -1)) {
            int flag;
            mpi_errno = MPI_Testany(count, array_of_requests, indx, &flag, status);
            goto fn_exit;
        }

        mpi_errno = MPID_Waitany(count - first_nonnull, &request_ptrs[first_nonnull], indx, status);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (*indx != MPI_UNDEFINED) {
            *indx += first_nonnull;
        } else {
            goto fn_exit;
        }
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptrs[*indx], status, &active_flag);
    if (!MPIR_Request_is_persistent(request_ptrs[*indx])) {
        MPIR_Request_free(request_ptrs[*indx]);
        array_of_requests[*indx] = MPI_REQUEST_NULL;
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_WAITANY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                     FCNAME, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_waitany",
                                     "**mpi_waitany %d %p %p %p",
                                     count, array_of_requests, indx, status);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
