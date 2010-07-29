/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_struct */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_struct = PMPI_Type_struct
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_struct  MPI_Type_struct
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_struct as PMPI_Type_struct
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_struct
#define MPI_Type_struct PMPI_Type_struct
#undef FUNCNAME
#define FUNCNAME MPIR_Type_struct_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Type_struct_impl(int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_types[], MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    int i, *ints;
    MPID_Datatype *new_dtp;
    MPIU_CHKLMEM_DECL(1);

    mpi_errno = MPID_Type_struct(count,
				 blocklens,
				 indices,
				 old_types,
				 &new_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);


    MPIU_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array");

    ints[0] = count;
    for (i=0; i < count; i++) {
        ints[i+1] = blocklens[i];
    }
    
    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
                                           MPI_COMBINER_STRUCT,
                                           count+1, /* ints (count, blocklengths) */
                                           count, /* aints (displacements) */
                                           count, /* types */
                                           ints,
                                           indices,
                                           old_types);
    
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
#define FUNCNAME MPI_Type_struct
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
    MPI_Type_struct - Creates a struct datatype

Input Parameters:
+ count - number of blocks (integer) -- also number of 
entries in arrays array_of_types ,
array_of_displacements  and array_of_blocklengths  
. blocklens - number of elements in each block (array)
. indices - byte displacement of each block (array)
- old_types - type of elements in each block (array 
of handles to datatype objects) 

Output Parameter:
. newtype - new datatype (handle) 

.N Deprecated
The replacement for this routine is 'MPI_Type_create_struct'

Notes:
If an upperbound is set explicitly by using the MPI datatype 'MPI_UB', the
corresponding index must be positive.

The MPI standard originally made vague statements about padding and alignment;
this was intended to allow the simple definition of structures that could
be sent with a count greater than one.  For example,
.vb
    struct { int a; char b; } foo;
.ve
may have 'sizeof(foo) > sizeof(int) + sizeof(char)'; for example, 
'sizeof(foo) == 2*sizeof(int)'.  The initial version of the MPI standard
defined the extent of a datatype as including an `epsilon` that would have 
allowed an implementation to make the extent an MPI datatype
for this structure equal to '2*sizeof(int)'.  
However, since different systems might define different paddings, there was 
much discussion by the MPI Forum about what was the correct value of
epsilon, and one suggestion was to define epsilon as zero.
This would have been the best thing to do in MPI 1.0, particularly since 
the 'MPI_UB' type allows the user to easily set the end of the structure.
Unfortunately, this change did not make it into the final document.  
Currently, this routine does not add any padding, since the amount of 
padding needed is determined by the compiler that the user is using to
build their code, not the compiler used to construct the MPI library.
A later version of MPICH may provide for some natural choices of padding
(e.g., multiple of the size of the largest basic member), but users are
advised to never depend on this, even with vendor MPI implementations.
Instead, if you define a structure datatype and wish to send or receive
multiple items, you should explicitly include an 'MPI_UB' entry as the
last member of the structure.  For example, the following code can be used
for the structure foo
.vb
    blen[0] = 1; indices[0] = 0; oldtypes[0] = MPI_INT;
    blen[1] = 1; indices[1] = &foo.b - &foo; oldtypes[1] = MPI_CHAR;
    blen[2] = 1; indices[2] = sizeof(foo); oldtypes[2] = MPI_UB;
    MPI_Type_struct( 3, blen, indices, oldtypes, &newtype );
.ve

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Type_struct(int count,
		    int blocklens[],
		    MPI_Aint indices[],
		    MPI_Datatype old_types[],
		    MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_STRUCT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_STRUCT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr;

	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(blocklens, "blocklens", mpi_errno);
		MPIR_ERRTEST_ARGNULL(indices, "indices", mpi_errno);
		MPIR_ERRTEST_ARGNULL(old_types, "types", mpi_errno);
	    }
	    if (mpi_errno == MPI_SUCCESS) {
		for (i=0; i < count; i++) {
		    MPIR_ERRTEST_ARGNEG(blocklens[i], "blocklen", mpi_errno);
		    MPIR_ERRTEST_DATATYPE(old_types[i], "datatype[i]",
					  mpi_errno);
		    /* stop before we dereference the null type */
		    if (mpi_errno != MPI_SUCCESS) break;

                    if (old_types[i] != MPI_DATATYPE_NULL && HANDLE_GET_KIND(old_types[i]) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(old_types[i], datatype_ptr);
                        MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                    }
		}
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Type_struct_impl(count, blocklens, indices, old_types, newtype);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_STRUCT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_struct",
	    "**mpi_type_struct %d %p %p %p %p", count, blocklens, indices, old_types, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
