/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_hindexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_hindexed  MPI_Type_hindexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_hindexed as PMPI_Type_hindexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_hindexed
#define MPI_Type_hindexed PMPI_Type_hindexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_hindexed

/*@
    MPI_Type_hindexed - Creates an indexed datatype with offsets in bytes

Input Parameters:
+ count - number of blocks -- also number of entries in indices and blocklens
. blocklens - number of elements in each block (array of nonnegative integers) 
. indices - byte displacement of each block (array of MPI_Aint) 
- old_type - old datatype (handle) 

Output Parameter:
. newtype - new datatype (handle) 

.N Deprecated
This routine is replaced by 'MPI_Type_create_hindexed'.

.N ThreadSafe

.N Fortran

The indices are displacements, and are based on a zero origin.  A common error
is to do something like to following
.vb
    integer a(100)
    integer blens(10), indices(10)
    do i=1,10
         blens(i)   = 1
10       indices(i) = (1 + (i-1)*10) * sizeofint
    call MPI_TYPE_HINDEXED(10,blens,indices,MPI_INTEGER,newtype,ierr)
    call MPI_TYPE_COMMIT(newtype,ierr)
    call MPI_SEND(a,1,newtype,...)
.ve
expecting this to send 'a(1),a(11),...' because the indices have values 
'1,11,...'.   Because these are `displacements` from the beginning of 'a',
it actually sends 'a(1+1),a(1+11),...'.

If you wish to consider the displacements as indices into a Fortran array,
consider declaring the Fortran array with a zero origin
.vb
    integer a(0:99)
.ve

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_ARG
@*/
int MPI_Type_hindexed(int count,
		      int blocklens[],
		      MPI_Aint indices[],
		      MPI_Datatype old_type,
		      MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_hindexed";
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPID_Datatype *new_dtp;
    int i, *ints;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_HINDEXED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_HINDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(old_type, "datatype", mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(blocklens, "blocklens", mpi_errno);
		MPIR_ERRTEST_ARGNULL(indices, "indices", mpi_errno);
	    }
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype_get_ptr( old_type, datatype_ptr );
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		}
		/* verify that all blocklengths are >= 0 */
		for (j=0; j < count; j++) {
		    MPIR_ERRTEST_ARGNEG(blocklens[j], "blocklen", mpi_errno);
		}
	    }
	    MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Type_indexed(count,
				  blocklens,
				  indices,
				  1, /* displacements in bytes */
				  old_type,
				  &new_handle);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array");

    /* copy ints into temporary buffer (count and blocklengths) */
    ints[0] = count;
    for (i=0; i < count; i++)
    {
	ints[i+1] = blocklens[i];
    }

    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_HINDEXED,
				           count+1, /* ints */
				           count, /* aints (displs) */
				           1, /* types */
				           ints,
				           indices,
				           &old_type);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_hindexed",
	    "**mpi_type_hindexed %d %p %p %D %p", count, blocklens, indices, old_type, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
