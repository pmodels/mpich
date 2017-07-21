/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2,
                              int ranks2[]) __attribute__((weak,alias("PMPI_Group_translate_ranks")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_translate_ranks
#define MPI_Group_translate_ranks PMPI_Group_translate_ranks

#undef FUNCNAME
#define FUNCNAME MPIR_Group_translate_ranks_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Group_translate_ranks_impl(MPIR_Group *gp1, int n, const int ranks1[],
                                    MPIR_Group *gp2, int ranks2[])
{
    int mpi_errno = MPI_SUCCESS;
    int i, g2_idx, l1_pid, l2_pid;

    MPL_DBG_MSG_S(MPIR_DBG_OTHER,VERBOSE,"gp2->is_local_dense_monotonic=%s", (gp2->is_local_dense_monotonic ? "TRUE" : "FALSE"));

    /* Initialize the output ranks */
    for (i=0; i<n; i++)
        ranks2[i] = MPI_UNDEFINED;

    if (gp2->size > 0 && gp2->is_local_dense_monotonic) {
        /* g2 probably == group_of(MPI_COMM_WORLD); use fast, constant-time lookup */
        int lpid_offset = gp2->lrank_to_lpid[0].lpid;

        MPIR_Assert(lpid_offset >= 0);
        for (i = 0; i < n; ++i) {
            int g1_lpid;

            if (ranks1[i] == MPI_PROC_NULL) {
                ranks2[i] = MPI_PROC_NULL;
                continue;
            }
            /* "adjusted" lpid from g1 */
            g1_lpid = gp1->lrank_to_lpid[ranks1[i]].lpid - lpid_offset;
            if ((g1_lpid >= 0) && (g1_lpid < gp2->size)) {
                ranks2[i] = g1_lpid;
            }
            /* else leave UNDEFINED */
        }
    }
    else {
        /* general, slow path; lookup time is dependent on the user-provided rank values! */
        g2_idx = gp2->idx_of_first_lpid;
        if (g2_idx < 0) {
            MPII_Group_setup_lpid_list( gp2 );
            g2_idx = gp2->idx_of_first_lpid;
        }
        if (g2_idx >= 0) {
            /* g2_idx can be < 0 if the g2 group is empty */
            l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
            for (i=0; i<n; i++) {
                if (ranks1[i] == MPI_PROC_NULL) {
                    ranks2[i] = MPI_PROC_NULL;
                    continue;
                }
                l1_pid = gp1->lrank_to_lpid[ranks1[i]].lpid;
                /* Search for this l1_pid in group2.  Use the following
                   optimization: start from the last position in the lpid list
                   if possible.  A more sophisticated version could use a 
                   tree based or even hashed search to speed the translation. */
                if (l1_pid < l2_pid || g2_idx < 0) {
                    /* Start over from the beginning */
                    g2_idx = gp2->idx_of_first_lpid;
                    l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
                }
                while (g2_idx >= 0 && l1_pid > l2_pid) {
                    g2_idx = gp2->lrank_to_lpid[g2_idx].next_lpid;
                    if (g2_idx >= 0)
                        l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
                    else
                        l2_pid = -1;
                }
                if (l1_pid == l2_pid)
                    ranks2[i] = g2_idx;
            }
        }
    }
    return mpi_errno;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_translate_ranks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
 MPI_Group_translate_ranks - Translates the ranks of processes in one group to 
                             those in another group

Input Parameters:
+ group1 - group1 (handle) 
. n - number of ranks in  'ranks1' and 'ranks2'  arrays (integer) 
. ranks1 - array of zero or more valid ranks in 'group1' 
- group2 - group2 (handle) 

Output Parameters:
. ranks2 - array of corresponding ranks in group2,  'MPI_UNDEFINED'  when no 
  correspondence exists. 

  As a special case (see the MPI-2 errata), if the input rank is 
  'MPI_PROC_NULL', 'MPI_PROC_NULL' is given as the output rank.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[],
			      MPI_Group group2, int ranks2[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr1 = NULL;
    MPIR_Group *group_ptr2 = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* The routines that setup the group data structures must be executed
       within a mutex.  As most of the group routines are not performance
       critical, we simple run these routines within the SINGLE_CS */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(group1, mpi_errno);
	    MPIR_ERRTEST_GROUP(group2, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPIR_Group_get_ptr( group1, group_ptr1 );
    MPIR_Group_get_ptr( group2, group_ptr2 );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPIR_Group_valid_ptr( group_ptr1, mpi_errno );
            MPIR_Group_valid_ptr( group_ptr2, mpi_errno );
	    /* If either group_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNEG(n,"n",mpi_errno);
	    if (group_ptr1) {
		/* Check that the rank entries are valid */
		int size1 = group_ptr1->size;
                int i;
		for (i=0; i<n; i++) {
		    if ( (ranks1[i] < 0 && ranks1[i] != MPI_PROC_NULL) || 
			ranks1[i] >= size1) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                              MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
							  MPI_ERR_RANK,
						  "**rank", "**rank %d %d", 
						  ranks1[i], size1 );
                        goto fn_fail;
		    }
		}
	    }
	    MPIR_ERRTEST_ARGNULL(ranks2, "ranks2", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr1, n, ranks1, group_ptr2, ranks2);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_TRANSLATE_RANKS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
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
    /* --END ERROR HANDLING-- */
}

