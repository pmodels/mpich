/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_create_endpoints */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_create_endpoints = PMPIX_Comm_create_endpoints
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_create_endpoints  MPIX_Comm_create_endpoints
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_create_endpoints as PMPIX_Comm_create_endpoints
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_create_endpoints(MPI_Comm parent_comm, int my_num_eps, MPI_Info info, MPI_Comm new_comm_hdls[])
    __attribute__ ((weak, alias("PMPIX_Comm_create_endpoints")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_create_endpoints
#define MPIX_Comm_create_endpoints PMPIX_Comm_create_endpoints

#endif /* !defined(MPICH_MPI_FROM_PMPI) */

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_create_endpoints
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPIX_Comm_create_endpoints - Creates a new endpoints communicator

Input Parameters:
+ parent_comm - parent communicator (handle)
. my_num_ep - number of endpoints to be created at this process
. info - info object (handle)

Output Parameters:
. new_comm_hdls - array of handles to new communicator (array of handles)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

.seealso: MPI_Comm_Create, MPI_Comm_dup
@*/
int MPIX_Comm_create_endpoints(MPI_Comm parent_comm, int my_num_eps, MPI_Info info, MPI_Comm new_comm_hdls[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL, **newcomm_ptrs;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_ENDPOINTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Validate parameters, and convert MPI object handles to object pointers */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(parent_comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPIR_Comm_get_ptr(parent_comm, comm_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* If comm_ptr is not valid, it will be reset to null */
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPIR_Comm_get_ptr(parent_comm, comm_ptr);
    }
#endif


    /* ... body of routine ...  */
    mpi_errno = MPID_Comm_create_endpoints(comm_ptr, my_num_eps, info, &newcomm_ptrs);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (newcomm_ptrs != NULL) {
        int i;
        for (i = 0; i < my_num_eps; i++)
            MPIR_OBJ_PUBLISH_HANDLE(new_comm_hdls[i], newcomm_ptrs[i]->handle);
    } else {
        int i;
        for (i = 0; i < my_num_eps; i++)
            new_comm_hdls[i] = MPI_COMM_NULL;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_CREATE_ENDPOINTS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_comm_create_endpoints", "**mpix_comm_create_endpoints %C %d %I %p", parent_comm, my_num_eps, info, new_comm_hdls);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
