/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_free_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_free_keyval = PMPI_Comm_free_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_free_keyval  MPI_Comm_free_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_free_keyval as PMPI_Comm_free_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_free_keyval(int *comm_keyval)  __attribute__ ((weak, alias("PMPI_Comm_free_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_free_keyval
#define MPI_Comm_free_keyval PMPI_Comm_free_keyval

#endif

/*@
   MPI_Comm_free_keyval - Frees an attribute key for communicators

Input/Output Parameters:
. comm_keyval - key value (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_KEYVAL
@*/

int MPI_Comm_free_keyval(int *comm_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *comm_keyval_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_FREE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_FREE_KEYVAL);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(comm_keyval, "comm_keyval", mpi_errno);
            MPIR_ERRTEST_KEYVAL(*comm_keyval, MPIR_COMM, "communicator", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(*comm_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPII_Keyval_get_ptr(*comm_keyval, comm_keyval_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPII_Keyval_valid_ptr(comm_keyval_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_free_keyval(comm_keyval_ptr);
    *comm_keyval = MPI_KEYVAL_INVALID;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_FREE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_comm_free_keyval", "**mpi_comm_free_keyval %p",
                                     comm_keyval);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
