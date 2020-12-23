/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_join */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_join = PMPI_Comm_join
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_join  MPI_Comm_join
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_join as PMPI_Comm_join
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_join(int fd, MPI_Comm * intercomm) __attribute__ ((weak, alias("PMPI_Comm_join")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_join
#define MPI_Comm_join PMPI_Comm_join
#endif

/*@
   MPI_Comm_join - Create a communicator by joining two processes connected by
     a socket.

Input Parameters:
. fd - socket file descriptor

Output Parameters:
. intercomm - new intercommunicator (handle)

 Notes:
  The socket must be quiescent before 'MPI_COMM_JOIN' is called and after
  'MPI_COMM_JOIN' returns. More specifically, on entry to 'MPI_COMM_JOIN', a
  read on the socket will not read any data that was written to the socket
  before the remote process called 'MPI_COMM_JOIN'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_join(int fd, MPI_Comm * intercomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_JOIN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_JOIN);

    /* ... body of routine ...  */

    MPIR_Comm *intercomm_ptr = NULL;
    mpi_errno = MPIR_Comm_join_impl(fd, &intercomm_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }

    if (intercomm_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*intercomm, intercomm_ptr->handle);
    } else {
        *intercomm = MPI_COMM_NULL;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_JOIN);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_join", "**mpi_comm_join %d %p", fd, intercomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
