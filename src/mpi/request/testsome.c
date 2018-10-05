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

/* -- Begin Profiling Symbol Block for routine MPI_Testsome */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testsome = PMPI_Testsome
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testsome  MPI_Testsome
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testsome as PMPI_Testsome
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[])
    __attribute__ ((weak, alias("PMPI_Testsome")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Testsome
#define MPI_Testsome PMPI_Testsome

#undef FUNCNAME
#define FUNCNAME MPIR_Testsome_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Testsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int i;
    int n_inactive;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Progress_test();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    /* --END ERROR HANDLING-- */

    n_inactive = 0;
    *outcount = 0;

    for (i = 0; i < incount; i++) {
        if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
            mpi_errno = MPID_Progress_test();
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        if (request_ptrs[i] != NULL && MPIR_Request_has_poll_fn(request_ptrs[i])) {
            mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        if (!MPIR_Request_is_active(request_ptrs[i])) {
            n_inactive += 1;
        } else if (MPIR_Request_is_complete(request_ptrs[i])) {
            array_of_indices[*outcount] = i;
            *outcount += 1;
        }
    }

    if (n_inactive == incount)
        *outcount = MPI_UNDEFINED;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testsome
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Testsome - Tests for some given requests to complete

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

While it is possible to list a request handle more than once in the
'array_of_requests', such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N ThreadSafe

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_IN_STATUS

@*/
int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[])
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    MPI_Status *status_ptr;
    int i;
    int n_inactive;
    int proc_failure = FALSE;
    int active_flag;
    int rc = MPI_SUCCESS;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TESTSOME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_TESTSOME);

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

    /* ... body of routine ... */

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
                    if (mpi_errno)
                        goto fn_fail;
                }
                MPID_END_ERROR_CHECKS;
            }
#endif
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                proc_failure = TRUE;
                MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                if (array_of_statuses != MPI_STATUSES_IGNORE)
                    array_of_statuses[i].MPI_ERROR = rc;
            }
        } else {
            request_ptrs[i] = NULL;
            n_inactive += 1;
        }
    }

    if (n_inactive == incount) {
        *outcount = MPI_UNDEFINED;
        goto fn_exit;
    }

    mpi_errno = MPID_Testsome(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (proc_failure)
        mpi_errno = MPI_ERR_IN_STATUS;

    if (*outcount == MPI_UNDEFINED)
        goto fn_exit;

    for (i = 0; i < *outcount; i++) {
        int idx = array_of_indices[i];
        status_ptr = (array_of_statuses == MPI_STATUSES_IGNORE) ?
            MPI_STATUS_IGNORE : &array_of_statuses[i];
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

    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_TESTSOME);
    MPID_THREAD_CS_EXIT(VNI_GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_testsome", "**mpi_testsome %d %p %p %p %p", incount,
                                 array_of_requests, outcount, array_of_indices, array_of_statuses);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
