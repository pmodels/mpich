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

/* -- Begin Profiling Symbol Block for routine MPI_Testall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testall = PMPI_Testall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testall  MPI_Testall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testall as PMPI_Testall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                MPI_Status array_of_statuses[]) __attribute__ ((weak, alias("PMPI_Testall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Testall
#define MPI_Testall PMPI_Testall


#undef FUNCNAME
#define FUNCNAME MPIR_Testall_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Testall_impl(int count, MPIR_Request * request_ptrs[], int *flag,
                      MPI_Status array_of_statuses[], int requests_property)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    int n_completed = 0;

    mpi_errno = MPID_Progress_test();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (requests_property & MPIR_REQUESTS_PROPERTY__NO_GREQUESTS) {
        for (i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test();
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }

            if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
                n_completed++;
            } else {
                break;
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test();
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }

            if (request_ptrs[i] != NULL) {
                if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                    mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                if (MPIR_Request_is_complete(request_ptrs[i])) {
                    n_completed++;
                }
            } else {
                n_completed++;
            }
        }
    }
    *flag = (n_completed == count) ? TRUE : FALSE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Testall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[])
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    int i;
    int n_completed;
    int active_flag;
    int rc = MPI_SUCCESS;
    int proc_failure = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;
    int ignoring_status = (array_of_statuses == MPI_STATUSES_IGNORE);
    MPIR_CHKLMEM_DECL(1);

    int ii, icount, impi_errno;
    n_completed = 0;

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPIR_Request **,
                                   count * sizeof(MPIR_Request *), mpi_errno, "request pointers",
                                   MPL_MEM_OBJECT);
    }

    for (ii = 0; ii < count; ii += MPIR_CVAR_REQUEST_BATCH_SIZE) {
        icount = count - ii > MPIR_CVAR_REQUEST_BATCH_SIZE ?
            MPIR_CVAR_REQUEST_BATCH_SIZE : count - ii;

        requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

        for (i = ii; i < ii + icount; i++) {
            if (array_of_requests[i] != MPI_REQUEST_NULL) {
                MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
                /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
                {
                    MPID_BEGIN_ERROR_CHECKS;
                    {
                        MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                        if (mpi_errno)
                            goto fn_fail;
                    }
                    MPID_END_ERROR_CHECKS;
                }
#endif
                if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                    request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND) {
                    requests_property &= ~MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY;
                    if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST) {
                        requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_GREQUESTS;
                    }
                }
            } else {
                request_ptrs[i] = NULL;
                requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_NULL;
            }
        }

        impi_errno = MPID_Testall(icount, &request_ptrs[ii], flag,
                                  ignoring_status ? MPI_STATUSES_IGNORE : &array_of_statuses[ii],
                                  requests_property);
        if (impi_errno != MPI_SUCCESS) {
            mpi_errno = impi_errno;
            goto fn_fail;
        }

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i]))
                n_completed += 1;
#ifdef HAVE_ERROR_CHECKING
            {
                MPID_BEGIN_ERROR_CHECKS;
                {
                    if (request_ptrs[i] == NULL)
                        continue;
                    if (MPIR_Request_is_complete(request_ptrs[i])) {
                        rc = MPIR_Request_get_error(request_ptrs[i]);
                        if (rc != MPI_SUCCESS) {
                            if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(rc) ||
                                MPIX_ERR_PROC_FAILED_PENDING == MPIR_ERR_GET_CLASS(rc))
                                proc_failure = TRUE;
                            mpi_errno = MPI_ERR_IN_STATUS;
                        }
                    } else if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                        mpi_errno = MPI_ERR_IN_STATUS;
                        MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                        if (!ignoring_status)
                            array_of_statuses[i].MPI_ERROR = rc;
                        proc_failure = TRUE;
                    }
                }
                MPID_END_ERROR_CHECKS;
            }
#endif
        }
    }

    *flag = (n_completed == count) ? TRUE : FALSE;

    /* We only process completion of requests if all are finished, or
     * there is an error. */
    if (!(*flag || mpi_errno == MPI_ERR_IN_STATUS))
        goto fn_exit;

    if (ignoring_status && (requests_property & MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY)) {
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL && MPIR_Request_is_complete(request_ptrs[i]))
                MPIR_Request_completion_processing_fastpath(&array_of_requests[i], request_ptrs[i]);
        }
        goto fn_exit;
    }

    if (ignoring_status) {
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL && MPIR_Request_is_complete(request_ptrs[i])) {
                MPIR_Request_completion_processing(request_ptrs[i],
                                                   MPI_STATUS_IGNORE, &active_flag);
                if (!MPIR_Request_is_persistent(request_ptrs[i])) {
                    MPIR_Request_free(request_ptrs[i]);
                    array_of_requests[i] = MPI_REQUEST_NULL;
                }
            }
        }
        goto fn_exit;
    }

    for (i = 0; i < count; i++) {
        if (request_ptrs[i] != NULL) {
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                rc = MPIR_Request_completion_processing(request_ptrs[i],
                                                        &array_of_statuses[i], &active_flag);
                if (!MPIR_Request_is_persistent(request_ptrs[i])) {
                    MPIR_Request_free(request_ptrs[i]);
                    array_of_requests[i] = MPI_REQUEST_NULL;
                }
                if (mpi_errno == MPI_ERR_IN_STATUS) {
                    if (active_flag) {
                        array_of_statuses[i].MPI_ERROR = rc;
                    } else {
                        array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
                    }
                }
            } else {
                if (mpi_errno == MPI_ERR_IN_STATUS) {
                    if (!proc_failure)
                        array_of_statuses[i].MPI_ERROR = MPI_ERR_PENDING;
                    else
                        array_of_statuses[i].MPI_ERROR = MPIX_ERR_PROC_FAILED_PENDING;
                }
            }
        } else {
            MPIR_Status_set_empty(&array_of_statuses[i]);
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            }
        }
    }

  fn_exit:
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Testall - Tests for the completion of all previously initiated
    requests

Input Parameters:
+ count - lists length (integer)
- array_of_requests - array of requests (array of handles)

Output Parameters:
+ flag - True if all requests have completed; false otherwise (logical)
- array_of_statuses - array of status objects (array of Status).  May be
 'MPI_STATUSES_IGNORE'.

Notes:
  'flag' is true only if all requests have completed.  Otherwise, flag is
  false and neither the 'array_of_requests' nor the 'array_of_statuses' is
  modified.

If one or more of the requests completes with an error, 'MPI_ERR_IN_STATUS' is
returned.  An error value will be present is elements of 'array_of_status'
associated with the requests.  Likewise, the 'MPI_ERROR' field in the status
elements associated with requests that have successfully completed will be
'MPI_SUCCESS'.  Finally, those requests that have not completed will have a
value of 'MPI_ERR_PENDING'.

While it is possible to list a request handle more than once in the
'array_of_requests', such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N ThreadSafe

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_IN_STATUS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TESTALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_TESTALL);

    /* Check the arguments */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int i = 0;

            MPIR_ERRTEST_COUNT(count, mpi_errno);

            if (count != 0) {
                MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
                /* NOTE: MPI_STATUSES_IGNORE != NULL */
                MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);

            for (i = 0; i < count; i++) {
                MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Testall(count, array_of_requests, flag, array_of_statuses);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:

    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_TESTALL);
    MPID_THREAD_CS_EXIT(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_testall", "**mpi_testall %d %p %p %p", count,
                                 array_of_requests, flag, array_of_statuses);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
