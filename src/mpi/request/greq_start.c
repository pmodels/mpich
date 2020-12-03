/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Grequest_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Grequest_start = PMPI_Grequest_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Grequest_start  MPI_Grequest_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Grequest_start as PMPI_Grequest_start
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Grequest_start(MPI_Grequest_query_function * query_fn, MPI_Grequest_free_function * free_fn,
                       MPI_Grequest_cancel_function * cancel_fn, void *extra_state,
                       MPI_Request * request) __attribute__ ((weak, alias("PMPI_Grequest_start")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Grequest_start
#define MPI_Grequest_start PMPI_Grequest_start
#endif

/*@
   MPI_Grequest_start - Create and return a user-defined request

Input Parameters:
+ query_fn - callback function invoked when request status is queried (function)
. free_fn - callback function invoked when request is freed (function)
. cancel_fn - callback function invoked when request is cancelled (function)
- extra_state - Extra state passed to the above functions.

Output Parameters:
.  request - Generalized request (handle)

 Notes on the callback functions:
 The return values from the callback functions must be a valid MPI error code
 or class.  This value may either be the return value from any MPI routine
 (with one exception noted below) or any of the MPI error classes.
 For portable programs, 'MPI_ERR_OTHER' may be used; to provide more
 specific information, create a new MPI error class or code with
 'MPI_Add_error_class' or 'MPI_Add_error_code' and return that value.

 The MPI standard is not clear on the return values from the callback routines.
 However, there are notes in the standard that imply that these are MPI error
 codes.  For example, pages 169 line 46 through page 170, line 1 require that
 the 'free_fn' return an MPI error code that may be used in the MPI completion
 functions when they return 'MPI_ERR_IN_STATUS'.

 The one special case is the error value returned by 'MPI_Comm_dup' when
 the attribute callback routine returns a failure.  The MPI standard is not
 clear on what values may be used to indicate an error return.  Further,
 the Intel MPI test suite made use of non-zero values to indicate failure,
 and expected these values to be returned by the 'MPI_Comm_dup' when the
 attribute routines encountered an error.  Such error values may not be valid
 MPI error codes or classes.  Because of this, it is the user''s responsibility
 to either use valid MPI error codes in return from the attribute callbacks,
 if those error codes are to be returned by a generalized request callback,
 or to detect and convert those error codes to valid MPI error codes (recall
 that MPI error classes are valid error codes).

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Grequest_start(MPI_Grequest_query_function * query_fn,
                       MPI_Grequest_free_function * free_fn,
                       MPI_Grequest_cancel_function * cancel_fn,
                       void *extra_state, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GREQUEST_START);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GREQUEST_START);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, &request_ptr);
    if (mpi_errno)
        goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GREQUEST_START);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_grequest_start", "**mpi_grequest_start %p %p %p %p %p",
                                 query_fn, free_fn, cancel_fn, extra_state, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_class_create*/
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_class_create = PMPIX_Grequest_class_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_class_create MPIX_Grequest_class_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_class_create as PMPIX_Grequest_class_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_class_create(MPI_Grequest_query_function * query_fn,
                               MPI_Grequest_free_function * free_fn,
                               MPI_Grequest_cancel_function * cancel_fn,
                               MPIX_Grequest_poll_function * poll_fn,
                               MPIX_Grequest_wait_function * wait_fn,
                               MPIX_Grequest_class * greq_class)
    __attribute__ ((weak, alias("PMPIX_Grequest_class_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_class_create
#define MPIX_Grequest_class_create PMPIX_Grequest_class_create
#endif

/* extensions for Generalized Request redesign paper */
int MPIX_Grequest_class_create(MPI_Grequest_query_function * query_fn,
                               MPI_Grequest_free_function * free_fn,
                               MPI_Grequest_cancel_function * cancel_fn,
                               MPIX_Grequest_poll_function * poll_fn,
                               MPIX_Grequest_wait_function * wait_fn,
                               MPIX_Grequest_class * greq_class)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* ... body of routine ... */
    mpi_errno = MPIX_Grequest_class_create_impl(query_fn, free_fn, cancel_fn, poll_fn, wait_fn,
                                                greq_class);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */
  fn_exit:
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_grequest_class_create",
                                 "**mpix_grequest_class_create %p %p %p %p %p %p", query_fn,
                                 free_fn, cancel_fn, poll_fn, wait_fn);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_class_allocate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_class_allocate = PMPIX_Grequest_class_allocate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_class_allocate MPIX_Grequest_class_allocate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_class_allocate as PMPIX_Grequest_class_allocate
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, void *extra_state,
                                 MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Grequest_class_allocate")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_class_allocate
#define MPIX_Grequest_class_allocate PMPIX_Grequest_class_allocate
#endif


int MPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class,
                                 void *extra_state, MPI_Request * request)
{
    int mpi_errno;
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* ... body of routine ... */
    mpi_errno = MPIX_Grequest_class_allocate_impl(greq_class, extra_state, request);

    /* ... end of body of routine ... */

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
}

/* -- Begin Profiling Symbol Block for routine MPIX_Grequest_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Grequest_start = PMPIX_Grequest_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Grequest_start MPIX_Grequest_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Grequest_start as PMPIX_Grequest_start
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Grequest_start(MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn,
                        MPIX_Grequest_poll_function * poll_fn,
                        MPIX_Grequest_wait_function * wait_fn,
                        void *extra_state, MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Grequest_start")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Grequest_start
#define MPIX_Grequest_start PMPIX_Grequest_start
#endif


int MPIX_Grequest_start(MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn,
                        MPIX_Grequest_poll_function * poll_fn,
                        MPIX_Grequest_wait_function * wait_fn,
                        void *extra_state, MPI_Request * request)
{
    int mpi_errno;
    MPIR_Request *lrequest_ptr;

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* ... body of routine ... */
    *request = MPI_REQUEST_NULL;
    mpi_errno =
        MPIX_Grequest_start_impl(query_fn, free_fn, cancel_fn, poll_fn, wait_fn, extra_state,
                                 &lrequest_ptr);

    if (mpi_errno == MPI_SUCCESS) {
        *request = lrequest_ptr->handle;
    }

    /* ... end of body of routine ... */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
}
