/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

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

/*@
  MPIR_Type_indexed - create an indexed datatype

Input Parameters:
+ count - number of blocks in type
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes (the
  displacement_array is an array of ints), otherwise they in terms of
  extent of oldtype (the displacement_array is an array of MPI_Aints)
- oldtype - type (using handle) of datatype on which new type is based

Output Parameters:
. newtype - handle of new indexed datatype

  Return Value:
  0 on success, -1 on failure.
@*/

int MPIR_Type_indexed(int count,
		      const int *blocklength_array,
		      const void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    int i;
    MPI_Aint contig_count;
    MPI_Aint el_sz, el_ct, old_ct, old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;
    MPI_Datatype el_type;

    MPIR_Datatype *new_dtp;

    if (count == 0) return MPII_Type_zerolen(newtype);

    /* sanity check that blocklens are all non-negative */
    for (i = 0; i < count; ++i) {
        DLOOP_Assert(blocklength_array[i] >= 0);
    }

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPIR_Type_indexed",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin)
    {
	/* builtins are handled differently than user-defined types because
	 * they have no associated dataloop or datatype structure.
	 */
	el_sz      = MPIR_Datatype_get_basic_size(oldtype);
	old_sz     = el_sz;
	el_ct      = 1;
	el_type    = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = (MPI_Aint) el_sz;
	old_true_ub   = (MPI_Aint) el_sz;
	old_extent    = (MPI_Aint) el_sz;
	old_is_contig = 1;

	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

        MPIR_Assign_trunc(new_dtp->alignsize, el_sz, MPI_Aint);
	new_dtp->builtin_element_size = el_sz;
	new_dtp->basic_type       = el_type;

	new_dtp->max_contig_blocks = count;
    }
    else
    {
	/* user-defined base type (oldtype) */
	MPIR_Datatype *old_dtp;

	MPIR_Datatype_get_ptr(oldtype, old_dtp);

	/* Ensure that "builtin_element_size" fits into an int datatype. */
	MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

	el_sz   = old_dtp->builtin_element_size;
	old_sz  = old_dtp->size;
	el_ct   = old_dtp->n_builtin_elements;
	el_type = old_dtp->basic_type;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	MPIR_Datatype_is_contig(oldtype, &old_is_contig);

	new_dtp->has_sticky_lb = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub = old_dtp->has_sticky_ub;
	new_dtp->builtin_element_size  = (MPI_Aint) el_sz;
	new_dtp->basic_type        = el_type;

        new_dtp->max_contig_blocks = 0;
        for(i=0; i<count; i++)
            new_dtp->max_contig_blocks 
                += old_dtp->max_contig_blocks
                    * ((MPI_Aint ) blocklength_array[i]);
    }

    /* find the first nonzero blocklength element */
    i = 0;
    while (i < count && blocklength_array[i] == 0) i++;

    if (i == count) {
	MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
	return MPII_Type_zerolen(newtype);
    }

    /* priming for loop */
    old_ct = blocklength_array[i];
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	(((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);

    MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);

    /* determine min lb, max ub, and count of old types in remaining
     * nonzero size blocks
     */
    for (i++; i < count; i++)
    {
	MPI_Aint tmp_lb, tmp_ub;
	
	if (blocklength_array[i] > 0) {
	    old_ct += blocklength_array[i]; /* add more oldtypes */
	
	    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
		(((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
	
	    /* calculate ub and lb for this block */
	    MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint)(blocklength_array[i]),
				      eff_disp,
				      old_lb,
				      old_ub,
				      old_extent,
				      tmp_lb,
				      tmp_ub);
	
	    if (tmp_lb < min_lb) min_lb = tmp_lb;
	    if (tmp_ub > max_ub) max_ub = tmp_ub;
	}
    }

    new_dtp->size = old_ct * old_sz;

    new_dtp->lb      = min_lb;
    new_dtp->ub      = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent  = max_ub - min_lb;

    new_dtp->n_builtin_elements = old_ct * el_ct;

    /* new type is only contig for N types if it's all one big
     * block, its size and extent are the same, and the old type
     * was also contiguous.
     */
    new_dtp->is_contig = 0;
    if(old_is_contig)
    {
	MPI_Aint *blklens = MPL_malloc(count *sizeof(MPI_Aint));
	for (i=0; i<count; i++)
		blklens[i] = blocklength_array[i];
        contig_count = MPIR_Type_indexed_count_contig(count,
						  blklens,
						  displacement_array,
						  dispinbytes,
						  old_extent);
        new_dtp->max_contig_blocks = contig_count;
        if( (contig_count == 1) &&
            ((MPI_Aint) new_dtp->size == new_dtp->extent))
        {
            new_dtp->is_contig = 1;
        }
	MPL_free(blklens);
    }

    *newtype = new_dtp->handle;
    return mpi_errno;
}

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
    MPIR_Datatype *new_dtp;
    int i, *ints;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_indexed(count,
				  array_of_blocklengths,
				  array_of_displacements,
				  0, /* displacements not in bytes */
				  oldtype,
				  &new_handle);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    /* copy all integer values into a temporary buffer; this
     * includes the count, the blocklengths, and the displacements.
     */
    MPIR_CHKLMEM_MALLOC(ints, int *, (2 * count + 1) * sizeof(int), mpi_errno, "contents integer array");

    ints[0] = count;

    for (i=0; i < count; i++) {
	ints[i+1] = array_of_blocklengths[i];
    }
    for (i=0; i < count; i++) {
	ints[i + count + 1] = array_of_displacements[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp,
					   MPI_COMBINER_INDEXED,
					   2*count + 1, /* ints */
					   0, /* aints  */
					   1, /* types */
					   ints,
					   NULL,
					   &oldtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

 fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_INDEXED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_INDEXED);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int j;
	    MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(array_of_blocklengths, "array_of_blocklengths", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_displacements, "array_of_displacements", mpi_errno);
	    }
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);

            if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr( oldtype, datatype_ptr );
                MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_INDEXED);
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
