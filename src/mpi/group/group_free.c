/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_free = PMPI_Group_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_free  MPI_Group_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_free as PMPI_Group_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_free(MPI_Group * group) __attribute__ ((weak, alias("PMPI_Group_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_free
#define MPI_Group_free PMPI_Group_free

int MPIR_Group_free_impl(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Do not free MPI_GROUP_EMPTY */
    if (group_ptr->handle != MPI_GROUP_EMPTY) {
        mpi_errno = MPIR_Group_release(group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/*@

MPI_Group_free - Frees a group

Input Parameters:
. group - group to free (handle)

Notes:
On output, group is set to 'MPI_GROUP_NULL'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_GROUP
@*/
int MPI_Group_free(MPI_Group * group)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_FREE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_GROUP(*group, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Group_get_ptr(*group, group_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPIR_Group_valid_ptr(group_ptr, mpi_errno);
            /* If group_ptr is not valid, it will be reset to null */

            /* Cannot free the predefined groups, but allow GROUP_EMPTY
             * because otherwise many tests fail */
            if ((HANDLE_IS_BUILTIN(*group)) && *group != MPI_GROUP_EMPTY) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                 MPI_ERR_GROUP, "**groupperm", 0);
            }
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Group_free_impl(group_ptr);
    *group = MPI_GROUP_NULL;
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_FREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_group_free", "**mpi_group_free %p", group);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
