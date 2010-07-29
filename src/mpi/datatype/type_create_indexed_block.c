/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_indexed_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_indexed_block = PMPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_indexed_block  MPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_indexed_block as PMPI_Type_create_indexed_block
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_indexed_block
#define MPI_Type_create_indexed_block PMPI_Type_create_indexed_block

#undef FUNCNAME
#define FUNCNAME MPIR_Type_create_indexed_block_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Type_create_indexed_block_impl(int count,
                                        int blocklength,
                                        int array_of_displacements[],
                                        MPI_Datatype oldtype,
                                        MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPI_Datatype new_handle;
    MPID_Datatype *new_dtp;
    int i, *ints;

    mpi_errno = MPID_Type_blockindexed(count,
				       blocklength,
				       array_of_displacements,
				       0, /* dispinbytes */
				       oldtype,
				       &new_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 2) * sizeof(int), mpi_errno, "content description");

    ints[0] = count;
    ints[1] = blocklength;

    for (i=0; i < count; i++)
	ints[i+2] = array_of_displacements[i];

    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_INDEXED_BLOCK,
				           count + 2, /* ints */
				           0, /* aints */
				           1, /* types */
				           ints,
				           NULL,
				           &oldtype);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_indexed_block
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
   MPI_Type_create_indexed_block - Create an indexed
     datatype with constant-sized blocks

   Input Parameters:
+ count - length of array of displacements (integer) 
. blocklength - size of block (integer) 
. array_of_displacements - array of displacements (array of integer) 
- oldtype - old datatype (handle) 

    Output Parameter:
. newtype - new datatype (handle) 

Notes:
The indices are displacements, and are based on a zero origin.  A common error
is to do something like the following
.vb
    integer a(100)
    integer blens(10), indices(10)
    do i=1,10
10       indices(i) = 1 + (i-1)*10
    call MPI_TYPE_CREATE_INDEXED_BLOCK(10,1,indices,MPI_INTEGER,newtype,ierr)
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

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_indexed_block(int count,
				  int blocklength,
				  int array_of_displacements[],
				  MPI_Datatype oldtype,
				  MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    
    /* Validate parameters and objects */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(blocklength, "blocklen", mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(array_of_displacements,
				     "indices",
				     mpi_errno);
	    }
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
            if (mpi_errno) goto fn_fail;
	    
	    if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(oldtype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    }

            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Type_create_indexed_block_impl(count, blocklength, array_of_displacements,
                                                    oldtype, newtype);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_create_indexed_block",
	    "**mpi_type_create_indexed_block %d %d %p %D %p", count, blocklength, array_of_displacements, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
