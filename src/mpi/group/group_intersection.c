/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_intersection */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_intersection = PMPI_Group_intersection
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_intersection  MPI_Group_intersection
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_intersection as PMPI_Group_intersection
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) __attribute__((weak,alias("PMPI_Group_intersection")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_intersection
#define MPI_Group_intersection PMPI_Group_intersection

#undef FUNCNAME
#define FUNCNAME MPIR_Group_intersection_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Group_intersection_impl(MPIR_Group *group_ptr1, MPIR_Group *group_ptr2, MPIR_Group **new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size1, i, k, g1_idx, g2_idx, l1_pid, l2_pid, nnew;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_GROUP_INTERSECTION_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_GROUP_INTERSECTION_IMPL);
    /* Return a group consisting of the members of group1 that are 
       in group2 */
    size1 = group_ptr1->size;
    /* Insure that the lpid lists are setup */
    MPIR_Group_setup_lpid_pairs( group_ptr1, group_ptr2 );

    for (i = 0; i < size1; i++) {
        group_ptr1->lrank_to_lpid[i].flag = 0;
    }
    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;
    
    nnew = 0;
    while (g1_idx >= 0 && g2_idx >= 0) {
        l1_pid = group_ptr1->lrank_to_lpid[g1_idx].lpid;
        l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
        if (l1_pid < l2_pid) {
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
        }
        else if (l1_pid > l2_pid) {
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
        }
        else {
            /* Equal */
            group_ptr1->lrank_to_lpid[g1_idx].flag = 1;
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
            nnew ++;
        }
    }
    /* Create the group.  Handle the trivial case first */
    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }
    
    mpi_errno = MPIR_Group_create( nnew, new_group_ptr );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    (*new_group_ptr)->rank = MPI_UNDEFINED;
    (*new_group_ptr)->is_local_dense_monotonic = TRUE;
    k = 0;
    for (i = 0; i < size1; i++) {
        if (group_ptr1->lrank_to_lpid[i].flag) {
            int lpid = group_ptr1->lrank_to_lpid[i].lpid;
            (*new_group_ptr)->lrank_to_lpid[k].lpid = lpid;
            if (i == group_ptr1->rank)
                (*new_group_ptr)->rank = k;
            if (lpid > MPIR_Process.comm_world->local_size ||
                (k > 0 && (*new_group_ptr)->lrank_to_lpid[k-1].lpid != (lpid-1))) {
                (*new_group_ptr)->is_local_dense_monotonic = FALSE;
            }

            k++;
        }
    }

 fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_GROUP_INTERSECTION_IMPL);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_intersection
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@

MPI_Group_intersection - Produces a group as the intersection of two existing
                         groups

Input Parameters:
+ group1 - first group (handle) 
- group2 - second group (handle) 

Output Parameters:
. newgroup - intersection group (handle) 

Notes:
The output group contains those processes that are in both 'group1' and 
'group2'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Group_free
@*/
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr1 = NULL;
    MPIR_Group *group_ptr2 = NULL;
    MPIR_Group *new_group_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_INTERSECTION);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_INTERSECTION);

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
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newgroup, "newgroup", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Group_intersection_impl(group_ptr1, group_ptr2, &new_group_ptr);
    if (mpi_errno) goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newgroup, new_group_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_INTERSECTION);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_group_intersection",
	    "**mpi_group_intersection %G %G %p", group1, group2, newgroup);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

