/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_darray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_darray = PMPI_Type_create_darray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_darray  MPI_Type_create_darray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_darray as PMPI_Type_create_darray
#endif
/* -- End Profiling Symbol Block */

#ifndef MIN
#define MIN(__a, __b) (((__a) < (__b)) ? (__a) : (__b))
#endif

PMPI_LOCAL int MPIR_Type_block(int *array_of_gsizes,
			       int dim,
			       int ndims,
			       int nprocs,
			       int rank,
			       int darg,
			       int order,
			       MPI_Aint orig_extent,
			       MPI_Datatype type_old,
			       MPI_Datatype *type_new,
			       MPI_Aint *st_offset);
PMPI_LOCAL int MPIR_Type_cyclic(int *array_of_gsizes,
				int dim,
				int ndims,
				int nprocs,
				int rank,
				int darg,
				int order,
				MPI_Aint orig_extent,
				MPI_Datatype type_old,
				MPI_Datatype *type_new,
				MPI_Aint *st_offset);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_darray
#define MPI_Type_create_darray PMPI_Type_create_darray



PMPI_LOCAL int MPIR_Type_block(int *array_of_gsizes,
			       int dim,
			       int ndims,
			       int nprocs,
			       int rank,
			       int darg,
			       int order,
			       MPI_Aint orig_extent,
			       MPI_Datatype type_old,
			       MPI_Datatype *type_new,
			       MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    static const char FCNAME[] = "MPIR_Type_block";
    int mpi_errno, blksize, global_size, mysize, i, j;
    MPI_Aint stride;

    global_size = array_of_gsizes[dim];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
	blksize = (global_size + nprocs - 1)/nprocs;
    else {
	blksize = darg;

#ifdef HAVE_ERROR_CHECKING
	if (blksize <= 0) {
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					     MPIR_ERR_RECOVERABLE,
					     FCNAME,
					     __LINE__,
					     MPI_ERR_ARG,
					     "**darrayblock",
					     "**darrayblock %d",
					     blksize);
	    return mpi_errno;
	}
	if (blksize * nprocs < global_size) {
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					     MPIR_ERR_RECOVERABLE,
					     FCNAME,
					     __LINE__,
					     MPI_ERR_ARG,
					     "**darrayblock2",
					     "**darrayblock2 %d %d",
					     blksize*nprocs,
					     global_size);
	    return mpi_errno;
	}
#endif
    }

    j = global_size - blksize*rank;
    mysize = MIN(blksize, j);
    if (mysize < 0) mysize = 0;

    stride = orig_extent;
    if (order == MPI_ORDER_FORTRAN) {
	if (dim == 0) {
	    mpi_errno = MPID_Type_contiguous(mysize,
					     type_old,
					     type_new);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else {
	    for (i=0; i<dim; i++) stride *= (MPI_Aint)(array_of_gsizes[i]);
	    mpi_errno = MPID_Type_vector(mysize,
					 1,
					 stride,
					 1, /* stride in bytes */
					 type_old,
					 type_new);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	}
    }
    else {
	if (dim == ndims-1) {
	    mpi_errno = MPID_Type_contiguous(mysize,
					     type_old,
					     type_new);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else {
	    for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)(array_of_gsizes[i]);
	    mpi_errno = MPID_Type_vector(mysize,
					 1,
					 stride,
					 1, /* stride in bytes */
					 type_old,
					 type_new);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	}
    }

    *st_offset = (MPI_Aint) blksize * (MPI_Aint) rank;
     /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0) *st_offset = 0;

    return MPI_SUCCESS;
}


PMPI_LOCAL int MPIR_Type_cyclic(int *array_of_gsizes,
				int dim,
				int ndims,
				int nprocs,
				int rank,
				int darg,
				int order,
				MPI_Aint orig_extent,
				MPI_Datatype type_old,
				MPI_Datatype *type_new,
				MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    static const char FCNAME[] = "MPIR_Type_cyclic";
    int mpi_errno,blksize, i, blklens[3], st_index, end_index,
	local_size, rem, count;
    MPI_Aint stride, disps[3];
    MPI_Datatype type_tmp, types[3];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG) blksize = 1;
    else blksize = darg;

#ifdef HAVE_ERROR_CHECKING
    if (blksize <= 0) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 FCNAME,
					 __LINE__,
					 MPI_ERR_ARG,
					 "**darraycyclic",
					 "**darraycyclic %d",
					 blksize);
	return mpi_errno;
    }
#endif

    st_index = rank*blksize;
    end_index = array_of_gsizes[dim] - 1;

    if (end_index < st_index) local_size = 0;
    else {
	local_size = ((end_index - st_index + 1)/(nprocs*blksize))*blksize;
	rem = (end_index - st_index + 1) % (nprocs*blksize);
	local_size += MIN(rem, blksize);
    }

    count = local_size/blksize;
    rem = local_size % blksize;

    stride = (MPI_Aint) nprocs * (MPI_Aint) blksize * orig_extent;
    if (order == MPI_ORDER_FORTRAN)
	for (i=0; i<dim; i++) stride *= (MPI_Aint)(array_of_gsizes[i]);
    else for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)(array_of_gsizes[i]);

    mpi_errno = MPID_Type_vector(count,
				 blksize,
				 stride,
				 1, /* stride in bytes */
				 type_old,
				 type_new);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (rem) {
	/* if the last block is of size less than blksize, include
	   it separately using MPI_Type_struct */

	types[0] = *type_new;
	types[1] = type_old;
	disps[0] = 0;
	disps[1] = (MPI_Aint) count * stride;
	blklens[0] = 1;
	blklens[1] = rem;

	mpi_errno = MPID_Type_struct(2,
				     blklens,
				     disps,
				     types,
				     &type_tmp);
	MPIR_Type_free_impl(type_new);
	*type_new = type_tmp;

	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
    }

    /* In the first iteration, we need to set the displacement in that
       dimension correctly. */
    if (((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
	((order == MPI_ORDER_C) && (dim == ndims-1)))
    {
        types[0] = MPI_LB;
        disps[0] = 0;
        types[1] = *type_new;
        disps[1] = (MPI_Aint) rank * (MPI_Aint) blksize * orig_extent;
        types[2] = MPI_UB;
        disps[2] = orig_extent * (MPI_Aint)(array_of_gsizes[dim]);
        blklens[0] = blklens[1] = blklens[2] = 1;
        mpi_errno = MPID_Type_struct(3,
				     blklens,
				     disps,
				     types,
				     &type_tmp);
        MPIR_Type_free_impl(type_new);
        *type_new = type_tmp;

	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */

        *st_offset = 0;  /* set it to 0 because it is taken care of in
                            the struct above */
    }
    else {
        *st_offset = (MPI_Aint) rank * (MPI_Aint) blksize;
        /* st_offset is in terms of no. of elements of type oldtype in
         * this dimension */
    }

    if (local_size == 0) *st_offset = 0;

    return MPI_SUCCESS;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_darray


/*@
   MPI_Type_create_darray - Create a datatype representing a distributed array

   Input Parameters:
+ size - size of process group (positive integer)
. rank - rank in process group (nonnegative integer)
. ndims - number of array dimensions as well as process grid dimensions (positive integer)
. array_of_gsizes - number of elements of type oldtype in each dimension of global array (array of positive integers)
. array_of_distribs - distribution of array in each dimension (array of state)
. array_of_dargs - distribution argument in each dimension (array of positive integers)
. array_of_psizes - size of process grid in each dimension (array of positive integers)
. order - array storage order flag (state)
- oldtype - old datatype (handle)

    Output Parameter:
. newtype - new datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_darray(int size,
			   int rank,
			   int ndims,
			   int array_of_gsizes[],
			   int array_of_distribs[],
			   int array_of_dargs[],
			   int array_of_psizes[],
			   int order,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_darray";
    int mpi_errno = MPI_SUCCESS, i;
    MPI_Datatype new_handle;

    int procs, tmp_rank, tmp_size, blklens[3], *coords;
    MPI_Aint *st_offsets, orig_extent, disps[3];
    MPI_Datatype type_old, type_new = MPI_DATATYPE_NULL, types[3];

#   ifdef HAVE_ERROR_CHECKING
    MPI_Aint   size_with_aint;
    MPI_Offset size_with_offset;
#   endif

    int *ints;
    MPID_Datatype *datatype_ptr = NULL;
    MPIU_CHKLMEM_DECL(3);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_DARRAY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_DARRAY);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr(oldtype, datatype_ptr);
    MPID_Datatype_get_extent_macro(oldtype, orig_extent);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Check parameters */
	    MPIR_ERRTEST_ARGNEG(rank, "rank", mpi_errno);
	    MPIR_ERRTEST_ARGNONPOS(size, "size", mpi_errno);
	    MPIR_ERRTEST_ARGNONPOS(ndims, "ndims", mpi_errno);

	    MPIR_ERRTEST_ARGNULL(array_of_gsizes, "array_of_gsizes", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_distribs, "array_of_distribs", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_dargs, "array_of_dargs", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_psizes, "array_of_psizes", mpi_errno);
	    if (order != MPI_ORDER_C && order != MPI_ORDER_FORTRAN) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME,
						 __LINE__,
						 MPI_ERR_ARG,
						 "**arg",
						 "**arg %s",
						 "order");
	    }

	    for (i=0; mpi_errno == MPI_SUCCESS && i < ndims; i++) {
		MPIR_ERRTEST_ARGNONPOS(array_of_gsizes[i], "gsize", mpi_errno);
		MPIR_ERRTEST_ARGNONPOS(array_of_psizes[i], "psize", mpi_errno);

		if ((array_of_distribs[i] != MPI_DISTRIBUTE_NONE) &&
		    (array_of_distribs[i] != MPI_DISTRIBUTE_BLOCK) &&
		    (array_of_distribs[i] != MPI_DISTRIBUTE_CYCLIC))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						     MPIR_ERR_RECOVERABLE,
						     FCNAME,
						     __LINE__,
						     MPI_ERR_ARG,
						     "**darrayunknown",
						     0);
		}

		if ((array_of_dargs[i] != MPI_DISTRIBUTE_DFLT_DARG) &&
		    (array_of_dargs[i] <= 0))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						     MPIR_ERR_RECOVERABLE,
						     FCNAME,
						     __LINE__,
						     MPI_ERR_ARG,
						     "**arg",
						     "**arg %s",
						     "array_of_dargs");
		}

		if ((array_of_distribs[i] == MPI_DISTRIBUTE_NONE) &&
		    (array_of_psizes[i] != 1))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						     MPIR_ERR_RECOVERABLE,
						     FCNAME,
						     __LINE__,
						     MPI_ERR_ARG,
						     "**darraydist",
						     "**darraydist %d %d",
						     i, array_of_psizes[i]);
		}
	    }

	    /* TODO: GET THIS CHECK IN ALSO */

	    /* check if MPI_Aint is large enough for size of global array.
	       if not, complain. */

	    size_with_aint = orig_extent;
	    for (i=0; i<ndims; i++) size_with_aint *= array_of_gsizes[i];
	    size_with_offset = orig_extent;
	    for (i=0; i<ndims; i++) size_with_offset *= array_of_gsizes[i];
	    if (size_with_aint != size_with_offset) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_FATAL,
						 FCNAME,
						 __LINE__,
						 MPI_ERR_ARG,
						 "**darrayoverflow",
						 "**darrayoverflow %L",
						 size_with_offset);
	    }

            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If datatype_ptr is not valid, it will be reset to null */
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno) goto fn_fail;
	    /* --END ERROR HANDLING-- */
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

/* calculate position in Cartesian grid as MPI would (row-major
   ordering) */
    MPIU_CHKLMEM_MALLOC_ORJUMP(coords, int *, ndims * sizeof(int), mpi_errno, "position is Cartesian grid");

    procs = size;
    tmp_rank = rank;
    for (i=0; i<ndims; i++) {
	procs = procs/array_of_psizes[i];
	coords[i] = tmp_rank/procs;
	tmp_rank = tmp_rank % procs;
    }

    MPIU_CHKLMEM_MALLOC_ORJUMP(st_offsets, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno, "st_offsets");

    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
      /* dimension 0 changes fastest */
	for (i=0; i<ndims; i++) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		mpi_errno = MPIR_Type_block(array_of_gsizes,
					    i,
					    ndims,
					    array_of_psizes[i],
					    coords[i],
					    array_of_dargs[i],
					    order,
					    orig_extent,
					    type_old,
					    &type_new,
					    st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		mpi_errno = MPIR_Type_cyclic(array_of_gsizes,
					     i,
					     ndims,
					     array_of_psizes[i],
					     coords[i],
					     array_of_dargs[i],
					     order,
					     orig_extent,
					     type_old,
					     &type_new,
					     st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		mpi_errno = MPIR_Type_block(array_of_gsizes,
					    i,
					    ndims,
					    1,
					    0,
					    MPI_DISTRIBUTE_DFLT_DARG,
					    order,
					    orig_extent,
					    type_old,
					    &type_new,
					    st_offsets+i);
		break;
	    }
	    if (i)
	    {
		MPIR_Type_free_impl(&type_old);
	    }
	    type_old = type_new;

	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}

	/* add displacement and UB */
	disps[1] = st_offsets[0];
	tmp_size = 1;
	for (i=1; i<ndims; i++) {
	    tmp_size *= array_of_gsizes[i-1];
	    disps[1] += (MPI_Aint) tmp_size * st_offsets[i];
	}
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
        /* dimension ndims-1 changes fastest */
	for (i=ndims-1; i>=0; i--) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		mpi_errno = MPIR_Type_block(array_of_gsizes,
					    i,
					    ndims,
					    array_of_psizes[i],
					    coords[i],
					    array_of_dargs[i],
					    order,
					    orig_extent,
					    type_old,
					    &type_new,
					    st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		mpi_errno = MPIR_Type_cyclic(array_of_gsizes,
					     i,
					     ndims,
					     array_of_psizes[i],
					     coords[i],
					     array_of_dargs[i],
					     order,
					     orig_extent,
					     type_old,
					     &type_new,
					     st_offsets+i);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		mpi_errno = MPIR_Type_block(array_of_gsizes,
					    i,
					    ndims,
					    array_of_psizes[i],
					    coords[i],
					    MPI_DISTRIBUTE_DFLT_DARG,
					    order,
					    orig_extent,
					    type_old,
					    &type_new,
					    st_offsets+i);
		break;
	    }
	    if (i != ndims-1)
	    {
		MPIR_Type_free_impl(&type_old);
	    }
	    type_old = type_new;

	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}

	/* add displacement and UB */
	disps[1] = st_offsets[ndims-1];
	tmp_size = 1;
	for (i=ndims-2; i>=0; i--) {
	    tmp_size *= array_of_gsizes[i+1];
	    disps[1] += (MPI_Aint) tmp_size * st_offsets[i];
	}
    }

    disps[1] *= orig_extent;

    disps[2] = orig_extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)(array_of_gsizes[i]);
	
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = type_new;
    types[2] = MPI_UB;

    mpi_errno = MPID_Type_struct(3,
				 blklens,
				 disps,
				 types,
				 &new_handle);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIR_Type_free_impl(&type_new);

    /* at this point we have the new type, and we've cleaned up any
     * intermediate types created in the process.  we just need to save
     * all our contents/envelope information.
     */

    /* Save contents */
    MPIU_CHKLMEM_MALLOC_ORJUMP(ints, int *, (4 * ndims + 4) * sizeof(int), mpi_errno, "content description");

    ints[0] = size;
    ints[1] = rank;
    ints[2] = ndims;

    for (i=0; i < ndims; i++) {
	ints[i + 3] = array_of_gsizes[i];
    }
    for (i=0; i < ndims; i++) {
	ints[i + ndims + 3] = array_of_distribs[i];
    }
    for (i=0; i < ndims; i++) {
	ints[i + 2*ndims + 3] = array_of_dargs[i];
    }
    for (i=0; i < ndims; i++) {
	ints[i + 3*ndims + 3] = array_of_psizes[i];
    }
    ints[4*ndims + 3] = order;
    MPID_Datatype_get_ptr(new_handle, datatype_ptr);
    mpi_errno = MPID_Datatype_set_contents(datatype_ptr,
					   MPI_COMBINER_DARRAY,
					   4*ndims + 4,
					   0,
					   1,
					   ints,
					   NULL,
					   &oldtype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIU_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_DARRAY);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_create_darray",
	    "**mpi_type_create_darray %d %d %d %p %p %p %p %d %D %p", size, rank, ndims, array_of_gsizes,
	    array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
