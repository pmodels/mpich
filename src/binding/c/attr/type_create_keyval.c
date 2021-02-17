/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_keyval = PMPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_keyval  MPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_keyval as PMPI_Type_create_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval,
                           void *extra_state)
                            __attribute__ ((weak, alias("PMPI_Type_create_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_keyval
#define MPI_Type_create_keyval PMPI_Type_create_keyval

#endif

/*@
   MPI_Type_create_keyval - Create an attribute keyval for MPI datatypes

Input Parameters:
+ type_copy_attr_fn - copy callback function for type_keyval (function)
. type_delete_attr_fn - delete callback function for type_keyval (function)
- extra_state - extra state for callback function (None)

Output Parameters:
. type_keyval - key value for future access (integer)

Notes:
Key values are global (available for any and all derived datatypes).

Default copy and delete functions are available.  These are
+ MPI_TYPE_NULL_COPY_FN   - empty copy function
. MPI_TYPE_NULL_DELETE_FN - empty delete function
- MPI_TYPE_DUP_FN         - simple dup function

There are subtle differences between C and Fortran that require that the
copy_fn be written in the same language from which 'MPI_Type_create_keyval'
is called.
This should not be a problem for most users; only programmers using both
Fortran and C in the same program need to be sure that they follow this rule.

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval,
                           void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(type_keyval, "type_keyval", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Type_create_keyval_impl(type_copy_attr_fn, type_delete_attr_fn, type_keyval,
                                             extra_state);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_type_create_keyval", "**mpi_type_create_keyval %p %p %p %p",
                                     type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
