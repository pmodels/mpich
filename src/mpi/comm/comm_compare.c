/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_compare */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_compare = PMPI_Comm_compare
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_compare  MPI_Comm_compare
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_compare as PMPI_Comm_compare
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_compare
#define MPI_Comm_compare PMPI_Comm_compare

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_compare
#undef FCNAME
#define FCNAME "MPI_Comm_compare"

/*@

MPI_Comm_compare - Compares two communicators

Input Parameters:
+ comm1 - comm1 (handle) 
- comm2 - comm2 (handle) 

Output Parameter:
. result - integer which is 'MPI_IDENT' if the contexts and groups are the
same, 'MPI_CONGRUENT' if different contexts but identical groups, 'MPI_SIMILAR'
if different contexts but similar groups, and 'MPI_UNEQUAL' otherwise

Using 'MPI_COMM_NULL' with 'MPI_Comm_compare':

It is an error to use 'MPI_COMM_NULL' as one of the arguments to
'MPI_Comm_compare'.  The relevant sections of the MPI standard are 

$(2.4.1 Opaque Objects)
A null handle argument is an erroneous 'IN' argument in MPI calls, unless an
exception is explicitly stated in the text that defines the function.

$(5.4.1. Communicator Accessors)
where there is no text in 'MPI_COMM_COMPARE' allowing a null handle.

.N ThreadSafe
(To perform the communicator comparisions, this routine may need to
allocate some memory.  Memory allocation is not interrupt-safe, and hence
this routine is only thread-safe.)

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr1 = NULL;
    MPID_Comm *comm_ptr2 = NULL;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_COMPARE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_COMPARE);

    MPIU_THREADPRIV_GET;
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm1, mpi_errno);
	    MPIR_ERRTEST_COMM(comm2, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm1, comm_ptr1 );
    MPID_Comm_get_ptr( comm2, comm_ptr2 );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr1, mpi_errno );
            MPID_Comm_valid_ptr( comm_ptr2, mpi_errno );
	    MPIR_ERRTEST_ARGNULL( result, "result", mpi_errno );
	    /* If comm_ptr1 or 2 is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (comm_ptr1->comm_kind != comm_ptr2->comm_kind) {
	*result = MPI_UNEQUAL;
    }
    else if (comm1 == comm2) {
	*result = MPI_IDENT;
    }
    else if (comm_ptr1->comm_kind == MPID_INTRACOMM) {
	MPI_Group group1, group2;

	MPIR_Nest_incr();
	NMPI_Comm_group( comm1, &group1 );
	NMPI_Comm_group( comm2, &group2 );
	NMPI_Group_compare( group1, group2, result );
	/* If the groups are the same but the contexts are different, then
	   the communicators are congruent */
	if (*result == MPI_IDENT) 
	    *result = MPI_CONGRUENT;
	NMPI_Group_free( &group1 );
	NMPI_Group_free( &group2 );
	MPIR_Nest_decr();
    }
    else { 
	/* INTER_COMM */
	int       lresult, rresult;
	MPI_Group group1, group2;
	MPI_Group rgroup1, rgroup2;
	
	/* Get the groups and see what their relationship is */
	MPIR_Nest_incr();
	NMPI_Comm_group (comm1, &group1);
	NMPI_Comm_group (comm2, &group2);
	NMPI_Group_compare ( group1, group2, &lresult );

	NMPI_Comm_remote_group (comm1, &rgroup1);
	NMPI_Comm_remote_group (comm2, &rgroup2);
	NMPI_Group_compare ( rgroup1, rgroup2, &rresult );

	/* Choose the result that is "least" strong. This works 
	   due to the ordering of result types in mpi.h */
	(*result) = (rresult > lresult) ? rresult : lresult;

	/* They can't be identical since they're not the same
	   handle, they are congruent instead */
	if ((*result) == MPI_IDENT)
	  (*result) = MPI_CONGRUENT;

	/* Free the groups */
	NMPI_Group_free (&group1);
	NMPI_Group_free (&group2);
	NMPI_Group_free (&rgroup1);
	NMPI_Group_free (&rgroup2);
	MPIR_Nest_decr();
    }
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_COMPARE);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_comm_compare",
	    "**mpi_comm_compare %C %C %p", comm1, comm2, result);
    }
    /* Use whichever communicator is non-null if possible */
    mpi_errno = MPIR_Err_return_comm( comm_ptr1 ? comm_ptr1 : comm_ptr2, 
				      FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
