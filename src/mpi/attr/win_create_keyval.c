/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create_keyval = PMPI_Win_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create_keyval  MPI_Win_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create_keyval as PMPI_Win_create_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_create_keyval(MPI_Win_copy_attr_function * win_copy_attr_fn,
                          MPI_Win_delete_attr_function * win_delete_attr_fn, int *win_keyval,
                          void *extra_state)
    __attribute__ ((weak, alias("PMPI_Win_create_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create_keyval
#define MPI_Win_create_keyval PMPI_Win_create_keyval

#endif

int MPI_Win_create_keyval(MPI_Win_copy_attr_function * win_copy_attr_fn,
                          MPI_Win_delete_attr_function * win_delete_attr_fn,
                          int *win_keyval, void *extra_state)
{
    QMPI_Context context;
    QMPI_Win_create_keyval_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Win_create_keyval(context, 0, win_copy_attr_fn, win_delete_attr_fn, win_keyval,
                                      extra_state);

    fn_ptr = (QMPI_Win_create_keyval_t *) MPIR_QMPI_first_fn_ptrs[MPI_WIN_CREATE_KEYVAL_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_WIN_CREATE_KEYVAL_T], win_copy_attr_fn,
                      win_delete_attr_fn, win_keyval, extra_state);
}

/*@
   MPI_Win_create_keyval - Create an attribute keyval for MPI window objects

Input Parameters:
+ win_copy_attr_fn - copy callback function for win_keyval (function)
. win_delete_attr_fn - delete callback function for win_keyval (function)
- extra_state - extra state for callback functions

Output Parameters:
. win_keyval - key value for future access (integer)

   Notes:
   Default copy and delete functions are available.  These are
+ MPI_WIN_NULL_COPY_FN   - empty copy function
. MPI_WIN_NULL_DELETE_FN - empty delete function
- MPI_WIN_DUP_FN         - simple dup function

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int QMPI_Win_create_keyval(QMPI_Context context, int tool_id,
                           MPI_Win_copy_attr_function * win_copy_attr_fn,
                           MPI_Win_delete_attr_function * win_delete_attr_fn, int *win_keyval,
                           void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_CREATE_KEYVAL);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(win_keyval, "win_keyval", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Win_create_keyval_impl(win_copy_attr_fn, win_delete_attr_fn, win_keyval,
                                            extra_state);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_CREATE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_create_keyval", "**mpi_win_create_keyval %p %p %p %p",
                                 win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
