/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_excl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_excl = PMPI_Group_excl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_excl  MPI_Group_excl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_excl as PMPI_Group_excl
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_excl
#define MPI_Group_excl PMPI_Group_excl

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_excl

/*@

MPI_Group_excl - Produces a group by reordering an existing group and taking
        only unlisted members

Input Parameters:
+ group - group (handle) 
. n - number of elements in array 'ranks' (integer) 
- ranks - array of integer ranks in 'group' not to appear in 'newgroup' 

Output Parameter:
. newgroup - new group derived from above, preserving the order defined by 
 'group' (handle) 

Note:  
The MPI standard requires that each of the ranks to excluded must be
a valid rank in the group and all elements must be distinct or the
function is erroneous.  

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_ARG
.N MPI_ERR_RANK

.seealso: MPI_Group_free
@*/
int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_excl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL, *new_group_ptr;
    int size, i, newi;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_EXCL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_EXCL);
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(group, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(n,"n",mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Group_get_ptr( group, group_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */
	    if (group_ptr) {
		mpi_errno = MPIR_Group_check_valid_ranks( group_ptr, 
							  ranks, n );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Allocate a new group and lrank_to_lpid array */
    size = group_ptr->size;
    if (size == n) {
	*newgroup = MPI_GROUP_EMPTY;
	goto fn_exit;
    }
    mpi_errno = MPIR_Group_create( size - n, &new_group_ptr );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    new_group_ptr->rank = MPI_UNDEFINED;
    /* Use flag fields to mark the members to *exclude* . */

    for (i=0; i<size; i++) {
	group_ptr->lrank_to_lpid[i].flag = 0;
    }
    for (i=0; i<n; i++) {
	group_ptr->lrank_to_lpid[ranks[i]].flag = 1;
    }
    
    newi = 0;
    for (i=0; i<size; i++) {
	if (group_ptr->lrank_to_lpid[i].flag == 0) {
	    new_group_ptr->lrank_to_lpid[newi].lrank = newi;
	    new_group_ptr->lrank_to_lpid[newi].lpid = 
		group_ptr->lrank_to_lpid[i].lpid;
	    if (group_ptr->rank == i) new_group_ptr->rank = newi;
	    newi++;
	}
    }

    new_group_ptr->size = size - n;
    new_group_ptr->idx_of_first_lpid = -1;
    /* TODO calculate is_local_dense_monotonic */

    MPIU_OBJ_PUBLISH_HANDLE(*newgroup, new_group_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_EXCL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_group_excl",
	    "**mpi_group_excl %G %d %p %p", group, n, ranks, newgroup);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
