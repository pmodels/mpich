/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static int MPIR_Type_create_subarray(int ndims, const MPI_Aint array_of_sizes[],
                                     const MPI_Aint array_of_subsizes[],
                                     const MPI_Aint array_of_starts[], int order,
                                     MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPI_Datatype tmp1, tmp2;

    /* these variables are from the original version in ROMIO */
    MPI_Aint size, extent, disps[3];

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
            for (int i = 2; i < ndims; i++) {
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
        for (int i = 1; i < ndims; i++) {
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
            for (int i = ndims - 3; i >= 0; i--) {
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
        for (int i = ndims - 2; i >= 0; i--) {
            size *= (MPI_Aint) (array_of_sizes[i + 1]);
            disps[1] += size * (MPI_Aint) (array_of_starts[i]);
        }
    }

    disps[1] *= extent;

    disps[2] = extent;
    for (int i = 0; i < ndims; i++)
        disps[2] *= (MPI_Aint) (array_of_sizes[i]);

    disps[0] = 0;

    mpi_errno = MPIR_Type_blockindexed(1, 1, &disps[1], 1,      /* 1 means disp is in bytes */
                                       tmp1, &tmp2);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Type_create_resized(tmp2, 0, disps[2], &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_free_impl(&tmp1);
    MPIR_Type_free_impl(&tmp2);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_subarray_impl(int ndims, const int array_of_sizes[],
                                   const int array_of_subsizes[], const int array_of_starts[],
                                   int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint *p_sizes, *p_subsizes, *p_starts;
    int *ints;
    MPIR_CHKLMEM_DECL(4);

    MPIR_CHKLMEM_MALLOC_ORJUMP(p_sizes, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "real array_of_sizes", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC_ORJUMP(p_subsizes, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "real array_of_subsizes", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC_ORJUMP(p_starts, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "real array_of_starts", MPL_MEM_BUFFER);
    for (int i = 0; i < ndims; i++) {
        p_sizes[i] = array_of_sizes[i];
        p_subsizes[i] = array_of_subsizes[i];
        p_starts[i] = array_of_starts[i];
    }

    mpi_errno = MPIR_Type_create_subarray(ndims, p_sizes, p_subsizes, p_starts,
                                          order, oldtype, newtype);
    MPIR_ERR_CHECK(mpi_errno);

    /* Save contents */
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (3 * ndims + 2) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = ndims;
    for (int i = 0; i < ndims; i++) {
        ints[i + 1] = array_of_sizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + ndims + 1] = array_of_subsizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + 2 * ndims + 1] = array_of_starts[i];
    }
    ints[3 * ndims + 1] = order;

    MPIR_Datatype *new_dtp;
    MPIR_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_SUBARRAY,
                                           3 * ndims + 2, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_subarray_large_impl(int ndims, const MPI_Aint array_of_sizes[],
                                         const MPI_Aint array_of_subsizes[],
                                         const MPI_Aint array_of_starts[], int order,
                                         MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts,
                                          order, oldtype, newtype);
    MPIR_ERR_CHECK(mpi_errno);

    /* Save contents */
    MPIR_CHKLMEM_MALLOC_ORJUMP(counts, MPI_Aint *, (3 * ndims) * sizeof(MPI_Aint), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    int ints[2];
    ints[0] = ndims;
    ints[1] = order;
    for (int i = 0; i < ndims; i++) {
        counts[i] = array_of_sizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        counts[i + ndims] = array_of_subsizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        counts[i + 2 * ndims] = array_of_starts[i];
    }

    MPIR_Datatype *new_dtp;
    MPIR_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_SUBARRAY,
                                           2, 0, 3 * ndims, 1, ints, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
