/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 * Note: This code originally appeared in ROMIO.
 */

#include "dataloop.h"
#undef FUNCNAME
#define FUNCNAME PREPEND_PREFIX(Type_convert_subarray)
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int PREPEND_PREFIX(Type_convert_subarray)(int ndims,
					  int *array_of_sizes,
					  int *array_of_subsizes,
					  int *array_of_starts,
					  int order,
					  MPI_Datatype oldtype,
					  MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent, disps[3], size;
    int i, blklens[3];
    MPI_Datatype tmp1, tmp2, types[3];

    MPIR_Type_extent_impl(oldtype, &extent);

    if (order == MPI_ORDER_FORTRAN) {
	/* dimension 0 changes fastest */
	if (ndims == 1) {
	    mpi_errno = MPIR_Type_contiguous_impl(array_of_subsizes[0], oldtype, &tmp1);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	}
	else {
	    mpi_errno = MPIR_Type_vector_impl(array_of_subsizes[1],
                                              array_of_subsizes[0],
                                              array_of_sizes[0], oldtype, &tmp1);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

	    size = (MPI_Aint)(array_of_sizes[0]) * extent;
	    for (i=2; i<ndims; i++) {
		size *= (MPI_Aint)(array_of_sizes[i-1]);
		mpi_errno = MPIR_Type_hvector_impl(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		MPIR_Type_free_impl(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = (MPI_Aint)(array_of_starts[0]);
	size = 1;
	for (i=1; i<ndims; i++) {
	    size *= (MPI_Aint)(array_of_sizes[i-1]);
	    disps[1] += size * (MPI_Aint)(array_of_starts[i]);
	}
        /* rest done below for both Fortran and C order */
    }

    else /* order == MPI_ORDER_C */ {
	/* dimension ndims-1 changes fastest */
	if (ndims == 1) {
	    mpi_errno = MPIR_Type_contiguous_impl(array_of_subsizes[0], oldtype, &tmp1);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	}
	else {
	    mpi_errno = MPIR_Type_vector_impl(array_of_subsizes[ndims-2],
                                              array_of_subsizes[ndims-1],
                                              array_of_sizes[ndims-1], oldtype, &tmp1);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

	    size = (MPI_Aint)(array_of_sizes[ndims-1]) * extent;
	    for (i=ndims-3; i>=0; i--) {
		size *= (MPI_Aint)(array_of_sizes[i+1]);
		mpi_errno = MPIR_Type_hvector_impl(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		MPIR_Type_free_impl(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	disps[1] = (MPI_Aint)(array_of_starts[ndims-1]);
	size = 1;
	for (i=ndims-2; i>=0; i--) {
	    size *= (MPI_Aint)(array_of_sizes[i+1]);
	    disps[1] += size * (MPI_Aint)(array_of_starts[i]);
	}
    }

    disps[1] *= extent;

    disps[2] = extent;
    for (i=0; i<ndims; i++) disps[2] *= (MPI_Aint)(array_of_sizes[i]);

    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = tmp1;
    types[2] = MPI_UB;
    
    mpi_errno = MPIR_Type_struct_impl(3, blklens, disps, types, newtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Type_free_impl(&tmp1);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
