/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "utlist.h"
#include "mpir_info.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_info = PMPI_Comm_set_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_info  MPI_Comm_set_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_info as PMPI_Comm_set_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
    __attribute__ ((weak, alias("PMPI_Comm_set_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_info
#define MPI_Comm_set_info PMPI_Comm_set_info

int MPIR_Comm_set_info_impl(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    mpi_errno = MPII_Comm_set_hints(comm_ptr, info_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
   MPI_Comm_set_info - Set new values for the hints of the
   communicator associated with comm.  The call is collective on the
   group of comm.  The info object may be different on each process,
   but any info entries that an implementation requires to be the same
   on all processes must appear with the same value in each process''
   info object.

Input Parameters:
+ comm - communicator object (handle)
- info - info argument (handle)

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
@*/
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_SET_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_SET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
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
            /* Validate pointers */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_set_info_impl(comm_ptr, info_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_SET_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_set_info",
                                 "**mpi_comm_set_info %W %p", comm, info);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
