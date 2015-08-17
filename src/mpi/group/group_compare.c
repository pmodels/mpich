/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_compare */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_compare = PMPI_Group_compare
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_compare  MPI_Group_compare
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_compare as PMPI_Group_compare
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result) __attribute__((weak,alias("PMPI_Group_compare")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_compare
#define MPI_Group_compare PMPI_Group_compare

int MPIR_Group_compare_impl(MPID_Group *group_ptr1, MPID_Group *group_ptr2, int *result)
{
    int mpi_errno = MPI_SUCCESS;
    int g1_idx, g2_idx, size, i;

    /* See if their sizes are equal */
    if (group_ptr1->size != group_ptr2->size) {
	*result = MPI_UNEQUAL;
	goto fn_exit;
    }

    /* Run through the lrank to lpid lists of each group in lpid order
       to see if the same processes are involved */
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
    while (g1_idx >= 0 && g2_idx >= 0) {
	if (group_ptr1->lrank_to_lpid[g1_idx].lpid !=
	    group_ptr2->lrank_to_lpid[g2_idx].lpid) {
	    *result = MPI_UNEQUAL;
	    goto fn_exit;
	}
	g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
	g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
    }

    /* See if the processes are in the same order by rank */
    size = group_ptr1->size;
    for (i=0; i<size; i++) {
	if (group_ptr1->lrank_to_lpid[i].lpid !=
	    group_ptr2->lrank_to_lpid[i].lpid) {
	    *result = MPI_SIMILAR;
	    goto fn_exit;
	}
    }
	
    /* If we reach here, the groups are identical */
    *result = MPI_IDENT;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_compare
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Group_compare - Compares two groups

Input Parameters:
+ group1 - group1 (handle) 
- group2 - group2 (handle) 

Output Parameters:
. result - integer which is 'MPI_IDENT' if the order and members of
the two groups are the same, 'MPI_SIMILAR' if only the members are the same,
and 'MPI_UNEQUAL' otherwise

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_ARG
@*/
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr1 = NULL;
    MPID_Group *group_ptr2 = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_COMPARE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* The routines that setup the group data structures must be executed
       within a mutex.  As most of the group routines are not performance
       critical, we simple run these routines within the SINGLE_CS */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_COMPARE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL( result, "result", mpi_errno );
	    MPIR_ERRTEST_GROUP(group1, mpi_errno);
	    MPIR_ERRTEST_GROUP(group2, mpi_errno);
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

    mpi_errno = MPIR_Group_compare_impl(group_ptr1, group_ptr2, result);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_group_compare",
	    "**mpi_group_compare %G %G %p", group1, group2, result);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
