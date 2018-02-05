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
    static const char FCNAME[] = "MPI_Waitany";
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    MPID_Progress_state progress_state;
    int i;
    int n_inactive;
    int active_flag;
    int init_req_array;
    int found_nonnull_req;
    int last_disabled_anysource = -1;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WAITANY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPI_WAITANY);

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

    n_inactive = 0;
    init_req_array = TRUE;
    found_nonnull_req = FALSE;

    MPID_Progress_start(&progress_state);
    for (;;) {
        for (i = 0; i < count; i++) {
            if (init_req_array) {
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
                                goto fn_progress_end_fail;
                        }
                        MPID_END_ERROR_CHECKS;
                    }
#endif
                } else {
                    request_ptrs[i] = NULL;
                    ++n_inactive;
                }
            }
            if (request_ptrs[i] == NULL)
                continue;
            /* we found at least one non-null request */
            found_nonnull_req = TRUE;

            if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST &&
                request_ptrs[i]->u.ureq.greq_fns->poll_fn != NULL) {
                /* this is a generalized request; make progress on it */
                mpi_errno =
                    (request_ptrs[i]->u.ureq.greq_fns->poll_fn) (request_ptrs[i]->u.ureq.
                                                                 greq_fns->grequest_extra_state,
                                                                 status);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_progress_end_fail;
            }
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                mpi_errno = MPIR_Request_completion_processing(request_ptrs[i], status,
                                                               &active_flag);
                array_of_requests[i] = request_ptrs[i]->handle;
                if (active_flag) {
                    *indx = i;
                    goto break_l1;
                } else {
                    ++n_inactive;
                    request_ptrs[i] = NULL;

                    if (n_inactive == count) {
                        *indx = MPI_UNDEFINED;
                        /* status is set to empty by MPIR_Request_completion_processing */
                        goto break_l1;
                    }
                }
            } else if (unlikely(MPIR_CVAR_ENABLE_FT &&
                                MPID_Request_is_anysource(request_ptrs[i]) &&
                                !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                last_disabled_anysource = i;
            }
        }
        init_req_array = FALSE;

        if (!found_nonnull_req) {
            /* all requests were NULL */
            *indx = MPI_UNDEFINED;
            if (status != NULL) /* could be null if count=0 */
                MPIR_Status_set_empty(status);
            goto break_l1;
        }

        /* If none of the requests completed, mark the last anysource request
         * as pending failure and break out. */
        if (unlikely(last_disabled_anysource != -1)) {
            MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
            if (status != MPI_STATUS_IGNORE)
                status->MPI_ERROR = mpi_errno;
            goto fn_progress_end_fail;
        }

        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS)
            goto fn_progress_end_fail;
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
  break_l1:
    MPID_Progress_end(&progress_state);

    /* ... end of body of routine ... */

  fn_exit:
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_WAITANY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_progress_end_fail:
    MPID_Progress_end(&progress_state);

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
