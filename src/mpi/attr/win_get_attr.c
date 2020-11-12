/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_attr  MPI_Win_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_attr as PMPI_Win_get_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag)
    __attribute__ ((weak, alias("PMPI_Win_get_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_attr
#define MPI_Win_get_attr PMPI_Win_get_attr

int MPII_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag,
                      MPIR_Attr_type attr_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPII_WIN_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPII_WIN_GET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_WIN(win, mpi_errno);
            MPIR_ERRTEST_KEYVAL(win_keyval, MPIR_WIN, "window", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Win_get_ptr(win, win_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPIR_Win_valid_ptr(win_ptr, mpi_errno);
            /* If win_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(attribute_val, "attribute_val", mpi_errno);
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Win_get_attr_impl(win_ptr, win_keyval, attribute_val, flag, attr_type);
    if (mpi_errno)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPII_WIN_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_win_get_attr", "**mpi_win_get_attr %W %d %p %p", win,
                                 win_keyval, attribute_val, flag);
    }
    MPIR_Win_get_ptr(win, win_ptr);
    mpi_errno = MPIR_Err_return_win(win_ptr, __func__, mpi_errno);
#endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif

int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag)
{
    QMPI_Context context;
    QMPI_Win_get_attr_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Win_get_attr(context, 0, win, win_keyval, attribute_val, flag);

    fn_ptr = (QMPI_Win_get_attr_t *) MPIR_QMPI_first_fn_ptrs[MPI_WIN_GET_ATTR_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_WIN_GET_ATTR_T], win, win_keyval,
                      attribute_val, flag);
}

/*@
   MPI_Win_get_attr - Get attribute cached on an MPI window object

Input Parameters:
+ win - window to which the attribute is attached (handle)
- win_keyval - key value (integer)

Output Parameters:
+ attribute_val - attribute value, unless flag is false
- flag - false if no attribute is associated with the key (logical)

   Notes:
   The following attributes are predefined for all MPI Window objects\:

+ MPI_WIN_BASE - window base address.
. MPI_WIN_SIZE - window size, in bytes.
- MPI_WIN_DISP_UNIT - displacement unit associated with the window.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int QMPI_Win_get_attr(QMPI_Context context, int tool_id, MPI_Win win, int win_keyval,
                      void *attribute_val, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WIN_GET_ATTR);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WIN_GET_ATTR);

    mpi_errno = MPII_Win_get_attr(win, win_keyval, attribute_val, flag, MPIR_ATTR_PTR);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
    return mpi_errno;
}
