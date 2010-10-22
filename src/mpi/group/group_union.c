/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_union */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_union = PMPI_Group_union
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_union  MPI_Group_union
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_union as PMPI_Group_union
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_union
#define MPI_Group_union PMPI_Group_union

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_union

/*@

MPI_Group_union - Produces a group by combining two groups

Input Parameters:
+ group1 - first group (handle) 
- group2 - second group (handle) 

Output Parameter:
. newgroup - union group (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Group_free
@*/
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_union";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr1 = NULL;
    MPID_Group *group_ptr2 = NULL;
    MPID_Group *new_group_ptr;
    int g1_idx, g2_idx, nnew, i, k, size1, size2, mylpid;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_UNION);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,); 
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_UNION);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(group1, mpi_errno);
	    MPIR_ERRTEST_GROUP(group2, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Group_get_ptr( group1, group_ptr1 );
    MPID_Group_get_ptr( group2, group_ptr2 );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr1, mpi_errno );
	    MPID_Group_valid_ptr( group_ptr2, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Determine the size of the new group.  The new group consists of all
       members of group1 plus the members of group2 that are not in group1.
    */
    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;

    /* If the lpid list hasn't been created, do it now */
    if (g1_idx < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr1 ); 
	g1_idx = group_ptr1->idx_of_first_lpid;
    }
    if (g2_idx < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr2 ); 
	g2_idx = group_ptr2->idx_of_first_lpid;
    }
    nnew = group_ptr1->size;

    /* Clear the flag bits on the second group.  The flag is set if
       a member of the second group belongs to the union */
    size2 = group_ptr2->size;
    for (i=0; i<size2; i++) {
	group_ptr2->lrank_to_lpid[i].flag = 0;
    }
    /* Loop through the lists that are ordered by lpid (local process
       id) to detect which processes in group 2 are not in group 1
    */
    while (g1_idx >= 0 && g2_idx >= 0) {
	int l1_pid, l2_pid;
	l1_pid = group_ptr1->lrank_to_lpid[g1_idx].lpid;
	l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
	if (l1_pid > l2_pid) {
	    nnew++;
	    group_ptr2->lrank_to_lpid[g2_idx].flag = 1;
	    g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
	}
	else if (l1_pid == l2_pid) {
	    g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
	    g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
	}
	else {
	    /* l1 < l2 */
	    g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
	}
    }
    /* If we hit the end of group1, add the remaining members of group 2 */
    while (g2_idx >= 0) {
	nnew++;
	group_ptr2->lrank_to_lpid[g2_idx].flag = 1;
	g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
    }
    
    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create( nnew, &new_group_ptr );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno) {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    /* If this process is in group1, then we can set the rank now. 
       If we are not in this group, this assignment will set the
       current rank to MPI_UNDEFINED */
    new_group_ptr->rank = group_ptr1->rank;
    
    /* Add group1 */
    size1 = group_ptr1->size;
    for (i=0; i<size1; i++) {
	new_group_ptr->lrank_to_lpid[i].lrank = i;
	new_group_ptr->lrank_to_lpid[i].lpid  = 
	    group_ptr1->lrank_to_lpid[i].lpid;
    }
    
    /* Add members of group2 that are not in group 1 */
    
    if (group_ptr1->rank == MPI_UNDEFINED &&
	group_ptr2->rank >= 0) {
	mylpid = group_ptr2->lrank_to_lpid[group_ptr2->rank].lpid;
    }
    else {
	mylpid = -2;
    }
    k = size1;
    for (i=0; i<size2; i++) {
	if (group_ptr2->lrank_to_lpid[i].flag) {
	    new_group_ptr->lrank_to_lpid[k].lrank = k;
	    new_group_ptr->lrank_to_lpid[k].lpid = 
		group_ptr2->lrank_to_lpid[i].lpid;
	    if (new_group_ptr->rank == MPI_UNDEFINED &&
		group_ptr2->lrank_to_lpid[i].lpid == mylpid)
		new_group_ptr->rank = k;
	    k++;
	}
    }

    /* TODO calculate is_local_dense_monotonic */

    MPIU_OBJ_PUBLISH_HANDLE(*newgroup, new_group_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_UNION);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_group_union",
	    "**mpi_group_union %G %G %p", group1, group2, newgroup);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
