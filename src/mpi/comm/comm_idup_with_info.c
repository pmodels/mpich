/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_idup_with_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_idup_with_info = PMPIX_Comm_idup_with_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_idup_with_info  MPIX_Comm_idup_with_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_idup_with_info as PMPIX_Comm_idup_with_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm,
                             MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Comm_idup_with_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_idup_with_info
#define MPIX_Comm_idup_with_info PMPIX_Comm_idup_with_info

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPIX_Comm_idup_with_info - nonblocking communicator duplication

Input Parameters:
. comm - communicator (handle)

Output Parameters:
+ newcomm - copy of comm (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm,
                             MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_Request *dreq = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_IDUP);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_IDUP);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    *request = MPI_REQUEST_NULL;
    *newcomm = MPI_COMM_NULL;

    mpi_errno = MPIR_Comm_idup_impl(comm_ptr, info_ptr, &newcomm_ptr, &dreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOTE: this is a publication for most of the comm, but the context ID
     * won't be valid yet, so we must "republish" relative to the request
     * handle at request completion time. */
    MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    *request = dreq->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_IDUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_idup", "**mpi_comm_idup %C %p %p", comm, newcomm,
                                 request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
