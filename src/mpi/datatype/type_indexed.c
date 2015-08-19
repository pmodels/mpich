/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_indexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_indexed = PMPI_Type_indexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_indexed  MPI_Type_indexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_indexed as PMPI_Type_indexed
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_indexed(int count, const int *array_of_blocklengths,
                     const int *array_of_displacements, MPI_Datatype oldtype,
                     MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_indexed")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_indexed
#define MPI_Type_indexed PMPI_Type_indexed

#undef FUNCNAME
#define FUNCNAME MPIR_Type_indexed_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_indexed_impl(int count, const int *array_of_blocklengths,
                           const int *array_of_displacements,
                           MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPID_Datatype *new_dtp;
    int i, *ints;
    MPIU_CHKLMEM_DECL(1);

    mpi_errno = MPID_Type_indexed(count,
				  array_of_blocklengths,
				  array_of_displacements,
				  0, /* displacements not in bytes */
				  oldtype,
				  &new_handle);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    /* copy all integer values into a temporary buffer; this
     * includes the count, the blocklengths, and the displacements.
     */
    MPIU_CHKLMEM_MALLOC(ints, int *, (2 * count + 1) * sizeof(int), mpi_errno, "contents integer array");

    ints[0] = count;

    for (i=0; i < count; i++) {
	ints[i+1] = array_of_blocklengths[i];
    }
    for (i=0; i < count; i++) {
	ints[i + count + 1] = array_of_displacements[i];
    }
    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
					   MPI_COMBINER_INDEXED,
					   2*count + 1, /* ints */
					   0, /* aints  */
					   1, /* types */
					   ints,
					   NULL,
					   &oldtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    MPID_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_indexed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Type_indexed - Creates an indexed datatype

Input Parameters:
+ count - number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
. array_of_blocklengths - number of elements in each block (array of nonnegative integers)
. array_of_displacements - displacement of each block in multiples of oldtype (array of
  integers)
- oldtype - old datatype (handle)

Output Parameters:
. newtype - new datatype (handle) 

.N ThreadSafe

.N Fortran

The array_of_displacements are displacements, and are based on a zero origin.  A common error
is to do something like to following
.vb
    integer a(100)
    integer array_of_blocklengths(10), array_of_displacements(10)
    do i=1,10
         array_of_blocklengths(i)   = 1
10       array_of_displacements(i) = 1 + (i-1)*10
    call MPI_TYPE_INDEXED(10,array_of_blocklengths,array_of_displacements,MPI_INTEGER,newtype,ierr)
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
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Type_indexed(int count,
		     const int *array_of_blocklengths,
		     const int *array_of_displacements,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_INDEXED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_INDEXED);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(array_of_blocklengths, "array_of_blocklengths", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_displacements, "array_of_displacements", mpi_errno);
	    }
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);

            if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr( oldtype, datatype_ptr );
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
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

    mpi_errno = MPIR_Type_indexed_impl(count, array_of_blocklengths, array_of_displacements, oldtype, newtype);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_INDEXED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_indexed",
	    "**mpi_type_indexed %d %p %p %D %p", count,array_of_blocklengths, array_of_displacements, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
