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

/* -- Begin Profiling Symbol Block for routine MPI_Startall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Startall = PMPI_Startall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Startall  MPI_Startall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Startall as PMPI_Startall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Startall(int count, MPI_Request array_of_requests[])
    __attribute__ ((weak, alias("PMPI_Startall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Startall
#define MPI_Startall PMPI_Startall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPI_Startall - Starts a collection of persistent requests

Input Parameters:
+ count - list length (integer)
- array_of_requests - array of requests (array of handle)

   Notes:

   Unlike 'MPI_Waitall', 'MPI_Startall' does not provide a mechanism for
   returning multiple errors nor pinpointing the request(s) involved.
   Furthermore, the behavior of 'MPI_Startall' after an error occurs is not
   defined by the MPI standard.  If well-defined error reporting and behavior
   are required, multiple calls to 'MPI_Start' should be used instead.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_REQUEST
@*/
int MPI_Startall(int count, MPI_Request array_of_requests[])
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_STARTALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_STARTALL);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);

            for (i = 0; i < count; i++) {
                MPIR_ERRTEST_REQUEST(array_of_requests[i], mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPIR_Request **, count * sizeof(MPIR_Request *),
                                   mpi_errno, "request pointers", MPL_MEM_OBJECT);
    }

    for (i = 0; i < count; i++) {
        MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
    }

    /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            for (i = 0; i < count; i++) {
                MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }
            for (i = 0; i < count; i++) {
                MPIR_ERRTEST_PERSISTENT(request_ptrs[i], mpi_errno);
                MPIR_ERRTEST_PERSISTENT_ACTIVE(request_ptrs[i], mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPID_Startall(count, request_ptrs);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_STARTALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_startall", "**mpi_startall %d %p", count,
                                 array_of_requests);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
