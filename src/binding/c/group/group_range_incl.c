/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_range_incl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_range_incl  MPI_Group_range_incl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_range_incl as PMPI_Group_range_incl
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
     __attribute__ ((weak, alias("PMPI_Group_range_incl")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_range_incl
#define MPI_Group_range_incl PMPI_Group_range_incl

#endif

/*@
   MPI_Group_range_incl - Creates a new group from ranges of ranks in an existing group

Input Parameters:
+ group - group (handle)
. n - number of triplets in array ranges (integer)
- ranges - a one-dimensional array of integer triplets, of the form (first rank, last rank, stride) indicating ranks in group of processes to be included in newgroup (integer)

Output Parameters:
. newgroup - new group derived from above, in the order defined by ranges (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_GROUP
.seealso: MPI_Group_free
@*/

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_RANGE_INCL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_RANGE_INCL);

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
            	mpi_errno = MPIR_Group_check_valid_ranges(group_ptr, ranges, n);
            }
            if (mpi_errno) {
            	goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Group *newgroup_ptr = NULL;
    *newgroup = MPI_GROUP_NULL;
    mpi_errno = MPIR_Group_range_incl_impl(group_ptr, n, ranges, &newgroup_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (newgroup_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*newgroup, newgroup_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_group_range_incl", "**mpi_group_range_incl %G %d %p %p",
                                     group, n, ranges, newgroup);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
