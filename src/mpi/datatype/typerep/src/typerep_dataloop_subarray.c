/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Note: This code originally appeared in ROMIO. */

#include "mpiimpl.h"
#include "typerep_internal.h"

int MPII_Typerep_convert_subarray(int ndims, MPI_Aint * array_of_sizes,
                                  MPI_Aint * array_of_subsizes, MPI_Aint * array_of_starts,
                                  int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent, blklens[3], disps[3], size;
    int i;
    MPI_Datatype tmp1, tmp2, types[3];

    MPIR_Datatype_get_extent_macro(oldtype, extent);

    if (order == MPI_ORDER_FORTRAN) {
        /* dimension 0 changes fastest */
        if (ndims == 1) {
            mpi_errno = MPIR_Type_contiguous_large_impl(array_of_subsizes[0], oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = MPIR_Type_vector_large_impl(array_of_subsizes[1], array_of_subsizes[0],
                                                    array_of_sizes[0], oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);

            size = (MPI_Aint) (array_of_sizes[0]) * extent;
            for (i = 2; i < ndims; i++) {
                size *= (MPI_Aint) (array_of_sizes[i - 1]);
                mpi_errno =
                    MPIR_Type_create_hvector_large_impl(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Type_free_impl(&tmp1);
                tmp1 = tmp2;
            }
        }

        /* add displacement and UB */
        disps[1] = (MPI_Aint) (array_of_starts[0]);
        size = 1;
        for (i = 1; i < ndims; i++) {
            size *= (MPI_Aint) (array_of_sizes[i - 1]);
            disps[1] += size * (MPI_Aint) (array_of_starts[i]);
        }
        /* rest done below for both Fortran and C order */
    } else {    /* order == MPI_ORDER_C */
        /* dimension ndims-1 changes fastest */
        if (ndims == 1) {
            mpi_errno = MPIR_Type_contiguous_large_impl(array_of_subsizes[0], oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = MPIR_Type_vector_large_impl(array_of_subsizes[ndims - 2],
                                                    array_of_subsizes[ndims - 1],
                                                    array_of_sizes[ndims - 1], oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);

            size = (MPI_Aint) (array_of_sizes[ndims - 1]) * extent;
            for (i = ndims - 3; i >= 0; i--) {
                size *= (MPI_Aint) (array_of_sizes[i + 1]);
                mpi_errno =
                    MPIR_Type_create_hvector_large_impl(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Type_free_impl(&tmp1);
                tmp1 = tmp2;
            }
        }

        /* add displacement and UB */
        disps[1] = (MPI_Aint) (array_of_starts[ndims - 1]);
        size = 1;
        for (i = ndims - 2; i >= 0; i--) {
            size *= (MPI_Aint) (array_of_sizes[i + 1]);
            disps[1] += size * (MPI_Aint) (array_of_starts[i]);
        }
    }

    disps[1] *= extent;

    disps[2] = extent;
    for (i = 0; i < ndims; i++)
        disps[2] *= (MPI_Aint) (array_of_sizes[i]);

    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = tmp1;
    types[2] = MPI_UB;

    mpi_errno = MPIR_Type_create_struct_large_impl(3, blklens, disps, types, newtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_free_impl(&tmp1);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
