/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_idup_with_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_idup_with_info = PMPI_Comm_idup_with_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_idup_with_info  MPI_Comm_idup_with_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_idup_with_info as PMPI_Comm_idup_with_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request)
     __attribute__ ((weak, alias("PMPI_Comm_idup_with_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_idup_with_info
#define MPI_Comm_idup_with_info PMPI_Comm_idup_with_info

#endif

/*@
   MPI_Comm_idup_with_info - nonblocking communicator duplication

Input Parameters:
+ comm - communicator (handle)
- info - info object (handle)

Output Parameters:
+ newcomm - copy of comm (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
.N MPI_ERR_INFO
@*/

int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_IDUP_WITH_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_IDUP_WITH_INFO);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Comm_get_ptr(comm, comm_ptr);
    if (info != MPI_INFO_NULL) {
        MPIR_Info_get_ptr(info, info_ptr);
    }

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno) {
                goto fn_fail;
            }
            if (info != MPI_INFO_NULL) {
                MPIR_Info_valid_ptr(info_ptr, mpi_errno);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Comm *newcomm_ptr = NULL;
    *newcomm = MPI_COMM_NULL;
    MPIR_Request *request_ptr = NULL;
    *request = MPI_REQUEST_NULL;
    mpi_errno = MPIR_Comm_idup_with_info_impl(comm_ptr, info_ptr, &newcomm_ptr, &request_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (newcomm_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    }
    if (request_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_IDUP_WITH_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_comm_idup_with_info",
                                     "**mpi_comm_idup_with_info %C %I %p %p", comm, info, newcomm,
                                     request);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
