/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_hindexed(int count, int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements, MPI_Datatype oldtype,
                      MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_hindexed")));
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
+ count - number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
. array_of_blocklengths - number of elements in each block (array of nonnegative integers)
. array_of_displacements - byte displacement of each block (array of MPI_Aint)
- oldtype - old datatype (handle)

Output Parameters:
. newtype - new datatype (handle) 

.N Deprecated
This routine is replaced by 'MPI_Type_create_hindexed'.

.N ThreadSafe

.N Fortran

The array_of_displacements are displacements, and are based on a zero origin.  A common error
is to do something like to following
.vb
    integer a(100)
    integer array_of_blocklengths(10), array_of_displacements(10)
    do i=1,10
         array_of_blocklengths(i)   = 1
10       array_of_displacements(i) = (1 + (i-1)*10) * sizeofint
    call MPI_TYPE_HINDEXED(10,array_of_blocklengths,array_of_displacements,MPI_INTEGER,newtype,ierr)
    call MPI_TYPE_COMMIT(newtype,ierr)
    call MPI_SEND(a,1,newtype,...)
.ve
expecting this to send "a(1),a(11),..." because the array_of_displacements have values
"1,11,...".   Because these are `displacements` from the beginning of "a",
it actually sends "a(1+1),a(1+11),...".

If you wish to consider the displacements as array_of_displacements into a Fortran array,
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
		      int *array_of_blocklengths,
		      MPI_Aint *array_of_displacements,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_hindexed";
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int i, *ints;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_HINDEXED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_HINDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;
	    MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(array_of_blocklengths, "array_of_blocklengths", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_displacements, "array_of_displacements", mpi_errno);
	    }

            if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr( oldtype, datatype_ptr );
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }
            /* verify that all blocklengths are >= 0 */
            for (j=0; j < count; j++) {
                MPIR_ERRTEST_ARGNEG(array_of_blocklengths[j], "blocklength", mpi_errno);
            }

	    MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Type_indexed(count,
				  array_of_blocklengths,
				  array_of_displacements,
				  1, /* displacements in bytes */
				  oldtype,
				  &new_handle);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array");

    /* copy ints into temporary buffer (count and blocklengths) */
    ints[0] = count;
    for (i=0; i < count; i++)
    {
	ints[i+1] = array_of_blocklengths[i];
    }

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_HINDEXED,
				           count+1, /* ints */
				           count, /* aints (displs) */
				           1, /* types */
				           ints,
				           array_of_displacements,
				           &oldtype);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_hindexed",
	    "**mpi_type_hindexed %d %p %p %D %p", count, array_of_blocklengths, array_of_displacements, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
