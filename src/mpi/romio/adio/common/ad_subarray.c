/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "adio.h"
#include "adio_extern.h"

int ADIO_Type_create_subarray(int ndims,
                              const int *array_of_sizes,
                              const int *array_of_subsizes,
                              const int *array_of_starts,
                              int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    MPI_Aint extent, disp, size, lb;
    int i, blklen;
    MPI_Datatype tmp1, tmp2, tmp_type;

    MPI_Type_get_extent(oldtype, &lb, &extent);

    if (order == MPI_ORDER_FORTRAN) {
        /* dimension 0 changes fastest */
        if (ndims == 1) {
            MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
        } else {
            MPI_Type_vector(array_of_subsizes[1],
                            array_of_subsizes[0], array_of_sizes[0], oldtype, &tmp1);

            size = (MPI_Aint) array_of_sizes[0] * extent;
            for (i = 2; i < ndims; i++) {
                size *= (MPI_Aint) array_of_sizes[i - 1];
                MPI_Type_create_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                MPI_Type_free(&tmp1);
                tmp1 = tmp2;
            }
        }

        /* add displacement and UB */
        disp = array_of_starts[0];
        size = 1;
        for (i = 1; i < ndims; i++) {
            size *= (MPI_Aint) array_of_sizes[i - 1];
            disp += size * (MPI_Aint) array_of_starts[i];
        }
        /* rest done below for both Fortran and C order */
    }

    else {      /* order == MPI_ORDER_C */

        /* dimension ndims-1 changes fastest */
        if (ndims == 1) {
            MPI_Type_contiguous(array_of_subsizes[0], oldtype, &tmp1);
        } else {
            MPI_Type_vector(array_of_subsizes[ndims - 2],
                            array_of_subsizes[ndims - 1],
                            array_of_sizes[ndims - 1], oldtype, &tmp1);

            size = (MPI_Aint) array_of_sizes[ndims - 1] * extent;
            for (i = ndims - 3; i >= 0; i--) {
                size *= (MPI_Aint) array_of_sizes[i + 1];
                MPI_Type_create_hvector(array_of_subsizes[i], 1, size, tmp1, &tmp2);
                MPI_Type_free(&tmp1);
                tmp1 = tmp2;
            }
        }

        /* add displacement and UB */
        disp = array_of_starts[ndims - 1];
        size = 1;
        for (i = ndims - 2; i >= 0; i--) {
            size *= (MPI_Aint) array_of_sizes[i + 1];
            disp += size * (MPI_Aint) array_of_starts[i];
        }
    }

    disp *= extent;

    for (i = 0; i < ndims; i++)
        extent *= (MPI_Aint) array_of_sizes[i];

    blklen = 1;

    MPI_Type_create_struct(1, &blklen, &disp, &tmp1, &tmp_type);

    MPI_Type_create_resized(tmp_type, 0, extent, newtype);

    MPI_Type_free(&tmp_type);
    MPI_Type_free(&tmp1);

    return MPI_SUCCESS;
}
