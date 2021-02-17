/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_excl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_excl = PMPI_Group_excl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_excl  MPI_Group_excl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_excl as PMPI_Group_excl
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup)
     __attribute__ ((weak, alias("PMPI_Group_excl")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_excl
#define MPI_Group_excl PMPI_Group_excl

#endif

/*@
   MPI_Group_excl - Produces a group by reordering an existing group and taking only unlisted members

Input Parameters:
+ group - group (handle)
. n - number of elements in array ranks (integer)
- ranks - array of integer ranks of processes in group not to appear in newgroup (integer)

Output Parameters:
. newgroup - new group derived from above, preserving the order defined by group (handle)

Note:
The MPI standard requires that each of the ranks to excluded must be
a valid rank in the group and all elements must be distinct or the
function is erroneous.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_GROUP
.seealso: MPI_Group_free
@*/

int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_EXCL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_EXCL);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_GROUP(group, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Group_get_ptr(group, group_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Group_valid_ptr(group_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNEG(n, "n", mpi_errno);
            MPIR_ERRTEST_ARGNULL(newgroup, "newgroup", mpi_errno);
            if (group_ptr) {
                mpi_errno = MPIR_Group_check_valid_ranks(group_ptr, ranks, n);
                if (mpi_errno) {
                    goto fn_fail;
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    if (group_ptr->size == n) {
        *newgroup = MPI_GROUP_EMPTY;
        goto fn_exit;
    }

    /* ... body of routine ... */
    MPIR_Group *newgroup_ptr = NULL;
    *newgroup = MPI_GROUP_NULL;
    mpi_errno = MPIR_Group_excl_impl(group_ptr, n, ranks, &newgroup_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (newgroup_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*newgroup, newgroup_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_EXCL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_group_excl", "**mpi_group_excl %G %d %p %p", group, n,
                                     ranks, newgroup);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
