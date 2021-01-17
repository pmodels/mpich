/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Type_create_subarray_impl(int ndims, const int array_of_sizes[],
                                   const int array_of_subsizes[], const int array_of_starts[],
                                   int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Datatype new_handle;
    MPI_Datatype tmp1, tmp2;

    /* these variables are from the original version in ROMIO */
    MPI_Aint size, extent, disps[3];

    /* for saving contents */
    int *ints;
    MPIR_Datatype *new_dtp;

    MPIR_CHKLMEM_DECL(1);
    /* TODO: CHECK THE ERROR RETURNS FROM ALL THESE!!! */

    /* TODO: GRAB EXTENT WITH A MACRO OR SOMETHING FASTER */
    MPIR_Datatype_get_extent_macro(oldtype, extent);

    if (order == MPI_ORDER_FORTRAN) {
        if (ndims == 1)
            mpi_errno = MPIR_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
        else {
            mpi_errno = MPIR_Type_vector(array_of_subsizes[1], array_of_subsizes[0], (MPI_Aint) (array_of_sizes[0]), 0, /* stride in types */
                                         oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);

            size = ((MPI_Aint) (array_of_sizes[0])) * extent;
            for (i = 2; i < ndims; i++) {
                size *= (MPI_Aint) (array_of_sizes[i - 1]);
                mpi_errno = MPIR_Type_vector(array_of_subsizes[i], 1, size, 1,  /* stride in bytes */
                                             tmp1, &tmp2);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Type_free_impl(&tmp1);
                tmp1 = tmp2;
            }
        }
        MPIR_ERR_CHECK(mpi_errno);

        /* add displacement and UB */

        disps[1] = (MPI_Aint) (array_of_starts[0]);
        size = 1;
        for (i = 1; i < ndims; i++) {
            size *= (MPI_Aint) (array_of_sizes[i - 1]);
            disps[1] += size * (MPI_Aint) (array_of_starts[i]);
        }
        /* rest done below for both Fortran and C order */
    } else {    /* MPI_ORDER_C */

        /* dimension ndims-1 changes fastest */
        if (ndims == 1) {
            mpi_errno = MPIR_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);

        } else {
            mpi_errno = MPIR_Type_vector(array_of_subsizes[ndims - 2], array_of_subsizes[ndims - 1], (MPI_Aint) (array_of_sizes[ndims - 1]), 0, /* stride in types */
                                         oldtype, &tmp1);
            MPIR_ERR_CHECK(mpi_errno);

            size = (MPI_Aint) (array_of_sizes[ndims - 1]) * extent;
            for (i = ndims - 3; i >= 0; i--) {
                size *= (MPI_Aint) (array_of_sizes[i + 1]);
                mpi_errno = MPIR_Type_vector(array_of_subsizes[i], 1,   /* blocklen */
                                             size,      /* stride */
                                             1, /* stride in bytes */
                                             tmp1,      /* old type */
                                             &tmp2);
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

    mpi_errno = MPIR_Type_blockindexed(1, 1, &disps[1], 1,      /* 1 means disp is in bytes */
                                       tmp1, &tmp2);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Type_create_resized(tmp2, 0, disps[2], &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_free_impl(&tmp1);
    MPIR_Type_free_impl(&tmp2);

    /* at this point we have the new type, and we've cleaned up any
     * intermediate types created in the process.  we just need to save
     * all our contents/envelope information.
     */

    /* Save contents */
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (3 * ndims + 2) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = ndims;
    for (i = 0; i < ndims; i++) {
        ints[i + 1] = array_of_sizes[i];
    }
    for (i = 0; i < ndims; i++) {
        ints[i + ndims + 1] = array_of_subsizes[i];
    }
    for (i = 0; i < ndims; i++) {
        ints[i + 2 * ndims + 1] = array_of_starts[i];
    }
    ints[3 * ndims + 1] = order;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_SUBARRAY,
                                           3 * ndims + 2, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Typerep_create_subarray(ndims, array_of_sizes, array_of_subsizes,
                                             array_of_starts, order, oldtype, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
