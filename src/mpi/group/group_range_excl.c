/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_range_excl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_range_excl = PMPI_Group_range_excl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_range_excl  MPI_Group_range_excl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_range_excl as PMPI_Group_range_excl
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) __attribute__((weak,alias("PMPI_Group_range_excl")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_range_excl
#define MPI_Group_range_excl PMPI_Group_range_excl
#undef FUNCNAME
#define FUNCNAME MPIR_Group_range_excl_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Group_range_excl_impl(MPIR_Group *group_ptr, int n, int ranges[][3], MPIR_Group **new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size, i, j, k, nnew, first, last, stride;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_GROUP_RANGE_EXCL_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_GROUP_RANGE_EXCL_IMPL);
    /* Compute size, assuming that included ranks are valid (and distinct) */
    size = group_ptr->size;
    nnew = 0;
    for (i = 0; i < n; i++) {
        first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
        /* works for stride of either sign.  Error checking above
           has already guaranteed stride != 0 */
        nnew += 1 + (last - first) / stride;
    }
    nnew = size - nnew;

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create( nnew, new_group_ptr );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    (*new_group_ptr)->rank = MPI_UNDEFINED;

    /* Group members are taken in rank order from the original group,
       with the specified members removed. Use the flag array for that
       purpose.  If this was a critical routine, we could use the
       flag values set in the error checking part, if the error checking
       was enabled *and* we are not MPI_THREAD_MULTIPLE, but since this
       is a low-usage routine, we haven't taken that optimization.  */

    /* First, mark the members to exclude */
    for (i = 0; i < size; i++)
        group_ptr->lrank_to_lpid[i].flag = 0;
    
    for (i = 0; i < n; i++) {
        first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
        if (stride > 0) {
            for (j = first; j <= last; j += stride) {
                group_ptr->lrank_to_lpid[j].flag = 1;
            }
        }
        else {
            for (j = first; j >= last; j += stride) {
                group_ptr->lrank_to_lpid[j].flag = 1;
            }
        }
    }
    /* Now, run through the group and pick up the members that were
       not excluded */
    k = 0;
    for (i = 0; i < size; i++) {
        if (!group_ptr->lrank_to_lpid[i].flag) {
            (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr->lrank_to_lpid[i].lpid;
            if (group_ptr->rank == i) {
                (*new_group_ptr)->rank = k;
            }
            k++;
        }
    }

    /* TODO calculate is_local_dense_monotonic */


 fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_GROUP_RANGE_EXCL_IMPL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_range_excl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@

MPI_Group_range_excl - Produces a group by excluding ranges of processes from
       an existing group

Input Parameters:
+ group - group (handle) 
. n - number of elements in array 'ranks' (integer) 
- ranges - a one-dimensional array of integer triplets of the
form (first rank, last rank, stride), indicating the ranks in
'group'  of processes to be excluded from the output group 'newgroup' .  

Output Parameters:
. newgroup - new group derived from above, preserving the 
order in 'group'  (handle) 

Note:  
The MPI standard requires that each of the ranks to be excluded must be
a valid rank in the group and all elements must be distinct or the
function is erroneous.  

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_RANK
.N MPI_ERR_ARG

.seealso: MPI_Group_free
@*/
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
                         MPI_Group *newgroup)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr = NULL, *new_group_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_RANGE_EXCL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_RANGE_EXCL);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(group, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPIR_Group_get_ptr( group, group_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPIR_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */

	    /* Check the exclusion array.  Ensure that all ranges are
	       valid and that the specified exclusions are unique */
	    if (group_ptr) {
		mpi_errno = MPIR_Group_check_valid_ranges( group_ptr, 
							   ranges, n );
	    }
            if (mpi_errno) goto fn_fail;
	     MPIR_ERRTEST_ARGNULL(newgroup, "newgroup", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Group_range_excl_impl(group_ptr, n, ranges, &new_group_ptr);
    if (mpi_errno) goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newgroup, new_group_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_RANGE_EXCL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
    mpi_errno = MPIR_Err_create_code(
	mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_group_range_excl",
	"**mpi_group_range_excl %G %d %p %p", group, n, ranges, newgroup);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

