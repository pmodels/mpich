/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 * Note: This code originally appeared in ROMIO.
 */

#include "dataloop.h"

static int MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims,
			    int nprocs, int rank, int darg, int order, MPI_Aint orig_extent,
			    MPI_Datatype type_old, MPI_Datatype *type_new,
			    MPI_Aint *st_offset);
static int MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset);


#undef FUNCNAME
#define FUNCNAME PREPEND_PREFIX(Type_convert_darray)
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int PREPEND_PREFIX(Type_convert_darray)(int size,
					int rank,
					int ndims,
					int *array_of_gsizes,
					int *array_of_distribs,
					int *array_of_dargs,
					int *array_of_psizes,
					int order,
					MPI_Datatype oldtype,
					MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype type_old, type_new=MPI_DATATYPE_NULL, types[3];
    int procs, tmp_rank, i, tmp_size, blklens[3], *coords;
    MPI_Aint *st_offsets, orig_extent, disps[3];

    MPIR_Type_extent_impl(oldtype, &orig_extent);

/* calculate position in Cartesian grid as MPI would (row-major
   ordering) */
    coords = (int *) DLOOP_Malloc(ndims*sizeof(int));
    procs = size;
    tmp_rank = rank;
    for (i=0; i<ndims; i++) {
	procs = procs/array_of_psizes[i];
	coords[i] = tmp_rank/procs;
	tmp_rank = tmp_rank % procs;
    }

    st_offsets = (MPI_Aint *) DLOOP_Malloc(ndims*sizeof(MPI_Aint));
    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
      /* dimension 0 changes fastest */
	for (i=0; i<ndims; i++) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		mpi_errno = MPIOI_Type_block(array_of_gsizes, i, ndims,
                                             array_of_psizes[i],
                                             coords[i], array_of_dargs[i],
                                             order, orig_extent,
                                             type_old, &type_new,
                                             st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		mpi_errno = MPIOI_Type_cyclic(array_of_gsizes, i, ndims,
                                              array_of_psizes[i], coords[i],
                                              array_of_dargs[i], order,
                                              orig_extent, type_old,
                                              &type_new, st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		mpi_errno = MPIOI_Type_block(array_of_gsizes, i, ndims, 1, 0,
                                             MPI_DISTRIBUTE_DFLT_DARG, order,
                                             orig_extent,
                                             type_old, &type_new,
                                             st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    }
	    if (i) MPIR_Type_free_impl(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[0];
	tmp_size = 1;
	for (i=1; i<ndims; i++) {
	    tmp_size *= array_of_gsizes[i-1];
	    disps[1] += ((MPI_Aint) tmp_size) * st_offsets[i];
	}
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
        /* dimension ndims-1 changes fastest */
	for (i=ndims-1; i>=0; i--) {
	    switch(array_of_distribs[i]) {
	    case MPI_DISTRIBUTE_BLOCK:
		mpi_errno = MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
                                             coords[i], array_of_dargs[i], order,
                                             orig_extent, type_old, &type_new,
                                             st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    case MPI_DISTRIBUTE_CYCLIC:
		mpi_errno = MPIOI_Type_cyclic(array_of_gsizes, i, ndims,
                                              array_of_psizes[i], coords[i],
                                              array_of_dargs[i], order,
                                              orig_extent, type_old, &type_new,
                                              st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    case MPI_DISTRIBUTE_NONE:
		/* treat it as a block distribution on 1 process */
		mpi_errno = MPIOI_Type_block(array_of_gsizes, i, ndims, array_of_psizes[i],
                                             coords[i], MPI_DISTRIBUTE_DFLT_DARG, order, orig_extent,
                                             type_old, &type_new, st_offsets+i);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		break;
	    }
	    if (i != ndims-1) MPIR_Type_free_impl(&type_old);
	    type_old = type_new;
	}

	/* add displacement and UB */
	disps[1] = st_offsets[ndims-1];
	tmp_size = 1;
	for (i=ndims-2; i>=0; i--) {
	    tmp_size *= array_of_gsizes[i+1];
	    disps[1] += ((MPI_Aint) tmp_size) * st_offsets[i];
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
    
    mpi_errno = MPIR_Type_struct_impl(3, blklens, disps, types, newtype);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIR_Type_free_impl(&type_new);

    DLOOP_Free(st_offsets);
    DLOOP_Free(coords);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
#undef FUNCNAME
#define FUNCNAME MPIOI_Type_block
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIOI_Type_block(int *array_of_gsizes, int dim, int ndims, int nprocs,
                            int rank, int darg, int order, MPI_Aint orig_extent,
                            MPI_Datatype type_old, MPI_Datatype *type_new,
                            MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int mpi_errno = MPI_SUCCESS;
    int blksize, global_size, mysize, i, j;
    MPI_Aint stride;

    global_size = array_of_gsizes[dim];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
	blksize = (global_size + nprocs - 1)/nprocs;
    else {
	blksize = darg;

        MPIU_ERR_CHKINTERNAL(blksize <= 0, mpi_errno, "blksize must be > 0");
        MPIU_ERR_CHKINTERNAL(blksize * nprocs < global_size, mpi_errno, "blksize * nprocs must be >= global size");
    }

    j = global_size - blksize*rank;
    mysize = (blksize < j) ? blksize : j;
    if (mysize < 0) mysize = 0;

    stride = orig_extent;
    if (order == MPI_ORDER_FORTRAN) {
	if (dim == 0) {
	    mpi_errno = MPIR_Type_contiguous_impl(mysize, type_old, type_new);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        } else {
	    for (i=0; i<dim; i++) stride *= (MPI_Aint)(array_of_gsizes[i]);
	    mpi_errno = MPIR_Type_hvector_impl(mysize, 1, stride, type_old, type_new);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	if (dim == ndims-1) {
	    mpi_errno = MPIR_Type_contiguous_impl(mysize, type_old, type_new);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        } else {
	    for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)(array_of_gsizes[i]);
	    mpi_errno = MPIR_Type_hvector_impl(mysize, 1, stride, type_old, type_new);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	}
    }

    *st_offset = blksize * rank;
     /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0) *st_offset = 0;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* Returns MPI_SUCCESS on success, an MPI error code on failure.  Code above
 * needs to call MPIO_Err_return_xxx.
 */
static int MPIOI_Type_cyclic(int *array_of_gsizes, int dim, int ndims, int nprocs,
		      int rank, int darg, int order, MPI_Aint orig_extent,
		      MPI_Datatype type_old, MPI_Datatype *type_new,
		      MPI_Aint *st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int mpi_errno = MPI_SUCCESS;
    int blksize, i, blklens[3], st_index, end_index, local_size, rem, count;
    MPI_Aint stride, disps[3];
    MPI_Datatype type_tmp, types[3];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG) blksize = 1;
    else blksize = darg;

    MPIU_ERR_CHKINTERNAL(blksize <= 0, mpi_errno, "blksize must be > 0");

    st_index = rank*blksize;
    end_index = array_of_gsizes[dim] - 1;

    if (end_index < st_index) local_size = 0;
    else {
	local_size = ((end_index - st_index + 1)/(nprocs*blksize))*blksize;
	rem = (end_index - st_index + 1) % (nprocs*blksize);
	local_size += (rem < blksize) ? rem : blksize;
    }

    count = local_size/blksize;
    rem = local_size % blksize;

    stride = ((MPI_Aint) nprocs) * ((MPI_Aint) blksize) * orig_extent;
    if (order == MPI_ORDER_FORTRAN)
	for (i=0; i<dim; i++) stride *= (MPI_Aint)(array_of_gsizes[i]);
    else for (i=ndims-1; i>dim; i--) stride *= (MPI_Aint)(array_of_gsizes[i]);

    mpi_errno = MPIR_Type_hvector_impl(count, blksize, stride, type_old, type_new);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (rem) {
	/* if the last block is of size less than blksize, include
	   it separately using MPI_Type_struct */

	types[0] = *type_new;
	types[1] = type_old;
	disps[0] = 0;
	disps[1] = ((MPI_Aint) count) * stride;
	blklens[0] = 1;
	blklens[1] = rem;

	mpi_errno = MPIR_Type_struct_impl(2, blklens, disps, types, &type_tmp);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	MPIR_Type_free_impl(type_new);
	*type_new = type_tmp;
    }

    /* In the first iteration, we need to set the displacement in that
       dimension correctly. */
    if (((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
	((order == MPI_ORDER_C) && (dim == ndims-1))) {
        types[0] = MPI_LB;
        disps[0] = 0;
        types[1] = *type_new;
        disps[1] = ((MPI_Aint) rank) * ((MPI_Aint) blksize) * orig_extent;
        types[2] = MPI_UB;
        disps[2] = orig_extent * ((MPI_Aint)(array_of_gsizes[dim]));
        blklens[0] = blklens[1] = blklens[2] = 1;

        mpi_errno = MPIR_Type_struct_impl(3, blklens, disps, types, &type_tmp);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
        MPIR_Type_free_impl(type_new);
        *type_new = type_tmp;

        *st_offset = 0;  /* set it to 0 because it is taken care of in
                            the struct above */
    }
    else {
        *st_offset = ((MPI_Aint) rank) * ((MPI_Aint) blksize);
        /* st_offset is in terms of no. of elements of type oldtype in
         * this dimension */
    }

    if (local_size == 0) *st_offset = 0;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
