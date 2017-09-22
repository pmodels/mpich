/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_info.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_info = PMPI_Comm_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_info  MPI_Comm_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_info as PMPI_Comm_get_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info) __attribute__((weak,alias("PMPI_Comm_get_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_info
#define MPI_Comm_get_info PMPI_Comm_get_info

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_get_info_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_get_info_impl(MPIR_Comm * comm_ptr, MPIR_Info ** info_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate an empty info object */
    mpi_errno = MPIR_Info_alloc(info_p_p);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Comm_get_info - Returns a new info object containing the hints
   of the communicator associated with comm. The current setting of
   all hints actually used by the system related to this communicator
   is returned in info_used.  If no such hints exist, a handle to a
   newly created info object is returned that contains no key/value
   pair.  The user is responsible for freeing info_used via
   MPI_INFO_FREE.

Input Parameters:
. comm - communicator object (handle)

Output Parameters:
. info_used - new info argument (handle)

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
@*/
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info * info_used)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_used_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_GET_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_GET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(info_used, "info", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(info_used, "info_used", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_get_info_impl(comm_ptr, &info_used_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    *info_used = info_used_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_GET_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_get_info",
                                 "**mpi_comm_get_info %W %p", comm, info_used);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
