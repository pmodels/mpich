/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_translate_ranks */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_translate_ranks = PMPI_Group_translate_ranks
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_translate_ranks  MPI_Group_translate_ranks
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_translate_ranks as PMPI_Group_translate_ranks
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_translate_ranks
#define MPI_Group_translate_ranks PMPI_Group_translate_ranks

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_translate_ranks

/*@
 MPI_Group_translate_ranks - Translates the ranks of processes in one group to 
                             those in another group

  Input Parameters:
+ group1 - group1 (handle) 
. n - number of ranks in  'ranks1' and 'ranks2'  arrays (integer) 
. ranks1 - array of zero or more valid ranks in 'group1' 
- group2 - group2 (handle) 

  Output Parameter:
. ranks2 - array of corresponding ranks in group2,  'MPI_UNDEFINED'  when no 
  correspondence exists. 

  As a special case (see the MPI-2 errata), if the input rank is 
  'MPI_PROC_NULL', 'MPI_PROC_NULL' is given as the output rank.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, 
			      MPI_Group group2, int *ranks2)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Group_translate_ranks";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr1 = NULL;
    MPID_Group *group_ptr2 = NULL;
    int i, g2_idx, l1_pid, l2_pid;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* The routines that setup the group data structures must be executed
       within a mutex.  As most of the group routines are not performance
       critical, we simple run these routines within the SINGLE_CS */
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);

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
	    /* If either group_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNEG(n,"n",mpi_errno);
	    if (group_ptr1) {
		/* Check that the rank entries are valid */
		int size1 = group_ptr1->size;
		for (i=0; i<n; i++) {
		    if ( (ranks1[i] < 0 && ranks1[i] != MPI_PROC_NULL) || 
			ranks1[i] >= size1) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                              MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
							  MPI_ERR_RANK,
						  "**rank", "**rank %d %d", 
						  ranks1[i], size1 );
			break;
		    }
		}
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Initialize the output ranks */
    for (i=0; i<n; i++) {
	ranks2[i] = MPI_UNDEFINED;
    }

    /* We may want to optimize for the special case of group2 is 
       a dup of MPI_COMM_WORLD, or more generally, has rank == lpid 
       for everything within the size of group2.  NOT DONE YET */
    g2_idx = group_ptr2->idx_of_first_lpid;
    if (g2_idx < 0) {
	MPIR_Group_setup_lpid_list( group_ptr2 );
	g2_idx = group_ptr2->idx_of_first_lpid;
    }
    if (g2_idx >= 0) {
	/* g2_idx can be < 0 if the g2 group is empty */
	l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
	for (i=0; i<n; i++) {
	    if (ranks1[i] == MPI_PROC_NULL) {
		ranks2[i] = MPI_PROC_NULL;
		continue;
	    }
	    l1_pid = group_ptr1->lrank_to_lpid[ranks1[i]].lpid;
	    /* Search for this l1_pid in group2.  Use the following
	       optimization: start from the last position in the lpid list
	       if possible.  A more sophisticated version could use a 
	       tree based or even hashed search to speed the translation. */
	    if (l1_pid < l2_pid || g2_idx < 0) {
		/* Start over from the beginning */
		g2_idx = group_ptr2->idx_of_first_lpid;
		l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
	    }
	    while (g2_idx >= 0 && l1_pid > l2_pid) {
		g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
		if (g2_idx >= 0) {
		    l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
		}
		else 
		    l2_pid = -1;
	    }
	    if (l1_pid == l2_pid) {
		ranks2[i] = group_ptr2->lrank_to_lpid[g2_idx].lrank;
	    }
	}
    }
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_group_translate_ranks",
	    "**mpi_group_translate_ranks %G %d %p %G %p", 
	    group1, n, ranks1, group2, ranks2);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

