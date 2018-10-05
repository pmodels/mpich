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

/* -- Begin Profiling Symbol Block for routine MPI_Waitsome */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitsome = PMPI_Waitsome
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitsome  MPI_Waitsome
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitsome as PMPI_Waitsome
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[])
    __attribute__ ((weak, alias("PMPI_Waitsome")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitsome
#define MPI_Waitsome PMPI_Waitsome

#undef FUNCNAME
#define FUNCNAME MPIR_Waitsome_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Waitsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    MPID_Progress_state progress_state;
    int i;
    int n_active;
    int n_inactive;
    int mpi_errno = MPI_SUCCESS;

    /* Bill Gropp says MPI_Waitsome() is expected to try to make
     * progress even if some requests have already completed;
     * therefore, we kick the pipes once and then fall into a loop
     * checking for completion and waiting for progress. */
    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) {
        /* --BEGIN ERROR HANDLING-- */
        goto fn_fail;
        /* --END ERROR HANDLING-- */
    }
    n_active = 0;

    MPID_Progress_start(&progress_state);
    for (;;) {
        n_inactive = 0;

        for (i = 0; i < incount; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test();
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_Progress_end(&progress_state);
                    goto fn_fail;
                }
            }

            if (request_ptrs[i] != NULL) {
                if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                    mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                if (MPIR_Request_is_complete(request_ptrs[i])) {
                    if (MPIR_Request_is_active(request_ptrs[i])) {
                        array_of_indices[n_active] = i;
                        n_active += 1;
                    } else {
                        request_ptrs[i] = NULL;
                        n_inactive += 1;
                    }
                }
            } else {
                n_inactive += 1;
            }
        }

        if (n_active > 0) {
            *outcount = n_active;
            break;
        } else if (n_inactive == incount) {
            *outcount = MPI_UNDEFINED;
            break;
        }

        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            MPID_Progress_end(&progress_state);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
    MPID_Progress_end(&progress_state);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitsome
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Waitsome - Waits for some given MPI Requests to complete

Input Parameters:
+ incount - length of array_of_requests (integer)
- array_of_requests - array of requests (array of handles)

Output Parameters:
+ outcount - number of completed requests (integer)
. array_of_indices - array of indices of operations that
completed (array of integers)
- array_of_statuses - array of status objects for
    operations that completed (array of Status).  May be 'MPI_STATUSES_IGNORE'.

Notes:
  The array of indicies are in the range '0' to 'incount - 1' for C and
in the range '1' to 'incount' for Fortran.

Null requests are ignored; if all requests are null, then the routine
returns with 'outcount' set to 'MPI_UNDEFINED'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

'MPI_Waitsome' provides an interface much like the Unix 'select' or 'poll'
calls and, in a high qualilty implementation, indicates all of the requests
that have completed when 'MPI_Waitsome' is called.
However, 'MPI_Waitsome' only guarantees that at least one
request has completed; there is no guarantee that `all` completed requests
will be returned, or that the entries in 'array_of_indices' will be in
increasing order. Also, requests that are completed while 'MPI_Waitsome' is
executing may or may not be returned, depending on the timing of the
completion of the message.

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitsome(int incount, MPI_Request array_of_requests[],
                 int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    MPI_Status *status_ptr;
    int i;
    int n_inactive;
    int active_flag;
    int rc = MPI_SUCCESS;
    int disabled_anysource = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WAITSOME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_WAITSOME);

    /* Check the arguments */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COUNT(incount, mpi_errno);

            if (incount != 0) {
                MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
                MPIR_ERRTEST_ARGNULL(array_of_indices, "array_of_indices", mpi_errno);
                /* NOTE: MPI_STATUSES_IGNORE != NULL */
                MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(outcount, "outcount", mpi_errno);

            for (i = 0; i < incount; i++) {
                MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    *outcount = 0;

    /* Convert MPI request handles to a request object pointers */
    if (incount > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPIR_Request **, incount * sizeof(MPIR_Request *),
                                   mpi_errno, "request pointers", MPL_MEM_OBJECT);
    }

    n_inactive = 0;
    for (i = 0; i < incount; i++) {
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
            /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
            {
                MPID_BEGIN_ERROR_CHECKS;
                {
                    MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) {
                        goto fn_fail;
                    }

                }
                MPID_END_ERROR_CHECKS;
            }
#endif

            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                disabled_anysource = TRUE;
            }
        } else {
            n_inactive += 1;
            request_ptrs[i] = NULL;
        }
    }

    if (n_inactive == incount) {
        *outcount = MPI_UNDEFINED;
        goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno =
            MPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
        goto fn_exit;
    }

    mpi_errno = MPID_Waitsome(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
    if (mpi_errno != MPI_SUCCESS) {
        /* --BEGIN ERROR HANDLING-- */
        goto fn_fail;
        /* --END ERROR HANDLING-- */
    }

    if (*outcount == MPI_UNDEFINED)
        goto fn_exit;

    for (i = 0; i < *outcount; i++) {
        int idx = array_of_indices[i];
        status_ptr =
            (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
        rc = MPIR_Request_completion_processing(request_ptrs[idx], status_ptr, &active_flag);
        if (!MPIR_Request_is_persistent(request_ptrs[idx])) {
            MPIR_Request_free(request_ptrs[idx]);
            array_of_requests[idx] = MPI_REQUEST_NULL;
        }
        if (rc == MPI_SUCCESS) {
            request_ptrs[idx] = NULL;
        } else {
            mpi_errno = MPI_ERR_IN_STATUS;
            if (status_ptr != MPI_STATUS_IGNORE) {
                status_ptr->MPI_ERROR = rc;
            }
        }
    }

    if (mpi_errno == MPI_ERR_IN_STATUS) {
        if (array_of_statuses != MPI_STATUSES_IGNORE) {
            for (i = 0; i < *outcount; i++) {
                if (request_ptrs[array_of_indices[i]] == NULL) {
                    array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
                }
            }
        }
    }

    /* ... end of body of routine ... */

  fn_exit:
    if (incount > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_WAITSOME);
    MPID_THREAD_CS_EXIT(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno =
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                             "**mpi_waitsome", "**mpi_waitsome %d %p %p %p %p", incount,
                             array_of_requests, outcount, array_of_indices, array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
