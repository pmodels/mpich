/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static int MPIR_Type_block(const MPI_Aint * array_of_gsizes,
                           int dim,
                           int ndims,
                           int nprocs,
                           int rank,
                           int darg,
                           int order,
                           MPI_Aint orig_extent,
                           MPI_Datatype type_old, MPI_Datatype * type_new, MPI_Aint * st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int mpi_errno;
    MPI_Aint global_size, blksize, mysize;
    MPI_Aint stride;

    global_size = array_of_gsizes[dim];

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
        blksize = (global_size + nprocs - 1) / nprocs;
    else {
        blksize = darg;

#ifdef HAVE_ERROR_CHECKING
        if (blksize <= 0) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                             MPIR_ERR_RECOVERABLE,
                                             __func__,
                                             __LINE__,
                                             MPI_ERR_ARG,
                                             "**darrayblock", "**darrayblock %d", blksize);
            return mpi_errno;
        }
        if (blksize * nprocs < global_size) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                             MPIR_ERR_RECOVERABLE,
                                             __func__,
                                             __LINE__,
                                             MPI_ERR_ARG,
                                             "**darrayblock2",
                                             "**darrayblock2 %d %d", blksize * nprocs, global_size);
            return mpi_errno;
        }
#endif
    }

    mysize = MPL_MIN(blksize, global_size - blksize * rank);
    if (mysize < 0)
        mysize = 0;

    stride = orig_extent;
    if (order == MPI_ORDER_FORTRAN) {
        if (dim == 0) {
            mpi_errno = MPIR_Type_contiguous(mysize, type_old, type_new);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                mpi_errno =
                    MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_OTHER, "**fail", 0);
                return mpi_errno;
            }
            /* --END ERROR HANDLING-- */
        } else {
            for (int i = 0; i < dim; i++)
                stride *= array_of_gsizes[i];
            mpi_errno = MPIR_Type_vector(mysize, 1, stride, 1,  /* stride in bytes */
                                         type_old, type_new);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                mpi_errno =
                    MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_OTHER, "**fail", 0);
                return mpi_errno;
            }
            /* --END ERROR HANDLING-- */
        }
    } else {
        if (dim == ndims - 1) {
            mpi_errno = MPIR_Type_contiguous(mysize, type_old, type_new);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                mpi_errno =
                    MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_OTHER, "**fail", 0);
                return mpi_errno;
            }
            /* --END ERROR HANDLING-- */
        } else {
            for (int i = ndims - 1; i > dim; i--)
                stride *= array_of_gsizes[i];
            mpi_errno = MPIR_Type_vector(mysize, 1, stride, 1,  /* stride in bytes */
                                         type_old, type_new);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                mpi_errno =
                    MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_OTHER, "**fail", 0);
                return mpi_errno;
            }
            /* --END ERROR HANDLING-- */
        }
    }

    *st_offset = blksize * rank;
    /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0)
        *st_offset = 0;

    MPI_Datatype type_tmp;
    MPI_Aint ex;
    MPIR_Datatype_get_extent_macro(type_old, ex);
    mpi_errno = MPIR_Type_create_resized(*type_new, 0, array_of_gsizes[dim] * ex, &type_tmp);
    MPIR_Type_free_impl(type_new);
    *type_new = type_tmp;

    return MPI_SUCCESS;
}

static int MPIR_Type_cyclic(const MPI_Aint * array_of_gsizes,
                            int dim,
                            int ndims,
                            int nprocs,
                            int rank,
                            int darg,
                            int order,
                            MPI_Aint orig_extent,
                            MPI_Datatype type_old, MPI_Datatype * type_new, MPI_Aint * st_offset)
{
/* nprocs = no. of processes in dimension dim of grid
   rank = coordinate of this process in dimension dim */
    int mpi_errno;
    MPI_Aint blksize, st_index, end_index, local_size, rem, count;
    MPI_Aint stride;
    MPI_Datatype type_tmp, type_indexed;

    if (darg == MPI_DISTRIBUTE_DFLT_DARG)
        blksize = 1;
    else
        blksize = darg;

#ifdef HAVE_ERROR_CHECKING
    if (blksize <= 0) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         __func__,
                                         __LINE__,
                                         MPI_ERR_ARG,
                                         "**darraycyclic", "**darraycyclic %d", blksize);
        return mpi_errno;
    }
#endif

    st_index = rank * blksize;
    end_index = array_of_gsizes[dim] - 1;

    if (end_index < st_index)
        local_size = 0;
    else {
        local_size = ((end_index - st_index + 1) / (nprocs * blksize)) * blksize;
        rem = (end_index - st_index + 1) % (nprocs * blksize);
        local_size += MPL_MIN(rem, blksize);
    }

    count = local_size / blksize;
    rem = local_size % blksize;

    stride = (MPI_Aint) nprocs *(MPI_Aint) blksize *orig_extent;
    if (order == MPI_ORDER_FORTRAN)
        for (int i = 0; i < dim; i++)
            stride *= array_of_gsizes[i];
    else
        for (int i = ndims - 1; i > dim; i--)
            stride *= array_of_gsizes[i];

    mpi_errno = MPIR_Type_vector(count, blksize, stride, 1,     /* stride in bytes */
                                 type_old, type_new);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**fail", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (rem) {
        /* if the last block is of size less than blksize, include
         * it separately using MPI_Type_struct */
        MPI_Aint blklens[3];
        MPI_Aint disps[3];
        MPI_Datatype types[3];

        types[0] = *type_new;
        types[1] = type_old;
        disps[0] = 0;
        disps[1] = (MPI_Aint) count *stride;
        blklens[0] = 1;
        blklens[1] = rem;

        mpi_errno = MPIR_Type_struct(2, blklens, disps, types, &type_tmp);
        MPIR_Type_free_impl(type_new);
        *type_new = type_tmp;

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            mpi_errno =
                MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**fail", 0);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */
    }

    /* In the first iteration, we need to set the displacement in that
     * dimension correctly. */
    if (((order == MPI_ORDER_FORTRAN) && (dim == 0)) ||
        ((order == MPI_ORDER_C) && (dim == ndims - 1))) {
        MPI_Aint disps[3];
        disps[0] = 0;
        disps[1] = (MPI_Aint) rank *(MPI_Aint) blksize *orig_extent;
        disps[2] = orig_extent * array_of_gsizes[dim];

        mpi_errno = MPIR_Type_blockindexed(1, 1, &disps[1], 1,  /* 1 means disp is in bytes */
                                           *type_new, &type_indexed);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            mpi_errno =
                MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**fail", 0);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */

        mpi_errno = MPIR_Type_create_resized(type_indexed, 0, disps[2], &type_tmp);

        MPIR_Type_free_impl(&type_indexed);
        MPIR_Type_free_impl(type_new);
        *type_new = type_tmp;

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            mpi_errno =
                MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**fail", 0);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */

        *st_offset = 0; /* set it to 0 because it is taken care of in
                         * the struct above */
    } else {
        *st_offset = (MPI_Aint) rank *(MPI_Aint) blksize;
        /* st_offset is in terms of no. of elements of type oldtype in
         * this dimension */
    }

    if (local_size == 0)
        *st_offset = 0;

    MPI_Aint ex;
    MPIR_Datatype_get_extent_macro(type_old, ex);
    mpi_errno = MPIR_Type_create_resized(*type_new, 0, array_of_gsizes[dim] * ex, &type_tmp);
    MPIR_Type_free_impl(type_new);
    *type_new = type_tmp;

    return MPI_SUCCESS;
}

static int MPIR_Type_create_darray(int size, int rank, int ndims,
                                   const MPI_Aint array_of_gsizes[], const int array_of_distribs[],
                                   const int array_of_dargs[], const int array_of_psizes[],
                                   int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;

    int procs, tmp_rank, *coords;
    MPI_Aint tmp_size;
    MPI_Aint *st_offsets, orig_extent, disps[3];
    MPI_Datatype type_old, type_new = MPI_DATATYPE_NULL, tmp_type;

    MPIR_CHKLMEM_DECL(2);

    /* calculate position in Cartesian grid as MPI would (row-major ordering) */
    MPIR_CHKLMEM_MALLOC_ORJUMP(coords, int *, ndims * sizeof(int), mpi_errno,
                               "position is Cartesian grid", MPL_MEM_COMM);

    MPIR_Datatype_get_extent_macro(oldtype, orig_extent);
    procs = size;
    tmp_rank = rank;
    for (int i = 0; i < ndims; i++) {
        procs = procs / array_of_psizes[i];
        coords[i] = tmp_rank / procs;
        tmp_rank = tmp_rank % procs;
    }

    MPIR_CHKLMEM_MALLOC_ORJUMP(st_offsets, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "st_offsets", MPL_MEM_COMM);

    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
        /* dimension 0 changes fastest */
        for (int i = 0; i < ndims; i++) {
            switch (array_of_distribs[i]) {
                case MPI_DISTRIBUTE_BLOCK:
                    mpi_errno = MPIR_Type_block(array_of_gsizes,
                                                i,
                                                ndims,
                                                array_of_psizes[i],
                                                coords[i],
                                                array_of_dargs[i],
                                                order,
                                                orig_extent, type_old, &type_new, st_offsets + i);
                    break;
                case MPI_DISTRIBUTE_CYCLIC:
                    mpi_errno = MPIR_Type_cyclic(array_of_gsizes,
                                                 i,
                                                 ndims,
                                                 array_of_psizes[i],
                                                 coords[i],
                                                 array_of_dargs[i],
                                                 order,
                                                 orig_extent, type_old, &type_new, st_offsets + i);
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
                                                orig_extent, type_old, &type_new, st_offsets + i);
                    break;
            }
            if (i) {
                MPIR_Type_free_impl(&type_old);
            }
            type_old = type_new;

            MPIR_ERR_CHECK(mpi_errno);
        }

        /* add displacement and UB */
        disps[1] = st_offsets[0];
        tmp_size = 1;
        for (int i = 1; i < ndims; i++) {
            tmp_size *= array_of_gsizes[i - 1];
            disps[1] += (MPI_Aint) tmp_size *st_offsets[i];
        }
        /* rest done below for both Fortran and C order */
    }

    else {      /* order == MPI_ORDER_C */

        /* dimension ndims-1 changes fastest */
        for (int i = ndims - 1; i >= 0; i--) {
            switch (array_of_distribs[i]) {
                case MPI_DISTRIBUTE_BLOCK:
                    mpi_errno = MPIR_Type_block(array_of_gsizes,
                                                i,
                                                ndims,
                                                array_of_psizes[i],
                                                coords[i],
                                                array_of_dargs[i],
                                                order,
                                                orig_extent, type_old, &type_new, st_offsets + i);
                    break;
                case MPI_DISTRIBUTE_CYCLIC:
                    mpi_errno = MPIR_Type_cyclic(array_of_gsizes,
                                                 i,
                                                 ndims,
                                                 array_of_psizes[i],
                                                 coords[i],
                                                 array_of_dargs[i],
                                                 order,
                                                 orig_extent, type_old, &type_new, st_offsets + i);
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
                                                orig_extent, type_old, &type_new, st_offsets + i);
                    break;
            }
            if (i != ndims - 1) {
                MPIR_Type_free_impl(&type_old);
            }
            type_old = type_new;

            MPIR_ERR_CHECK(mpi_errno);
        }

        /* add displacement and UB */
        disps[1] = st_offsets[ndims - 1];
        tmp_size = 1;
        for (int i = ndims - 2; i >= 0; i--) {
            tmp_size *= array_of_gsizes[i + 1];
            disps[1] += (MPI_Aint) tmp_size *st_offsets[i];
        }
    }

    disps[1] *= orig_extent;

    disps[2] = orig_extent;
    for (int i = 0; i < ndims; i++)
        disps[2] *= (MPI_Aint) (array_of_gsizes[i]);

    disps[0] = 0;

    mpi_errno = MPIR_Type_blockindexed(1, 1, &disps[1], 1,      /* 1 means disp is in bytes */
                                       type_new, &tmp_type);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Type_create_resized(tmp_type, 0, disps[2], &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Type_free_impl(&tmp_type);
    MPIR_Type_free_impl(&type_new);

    *newtype = new_handle;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_darray_impl(int size, int rank, int ndims,
                                 const int array_of_gsizes[], const int array_of_distribs[],
                                 const int array_of_dargs[], const int array_of_psizes[],
                                 int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint *real_array_of_gsizes;
    int *ints;
    MPIR_CHKLMEM_DECL(2);

    MPIR_CHKLMEM_MALLOC_ORJUMP(real_array_of_gsizes, MPI_Aint *, ndims * sizeof(MPI_Aint),
                               mpi_errno, "real_array_of_gsizes", MPL_MEM_COMM);
    for (int i = 0; i < ndims; i++) {
        real_array_of_gsizes[i] = array_of_gsizes[i];
    }

    mpi_errno = MPIR_Type_create_darray(size, rank, ndims, real_array_of_gsizes, array_of_distribs,
                                        array_of_dargs, array_of_psizes, order, oldtype, newtype);
    MPIR_ERR_CHECK(mpi_errno);

    /* at this point we have the new type, and we've cleaned up any
     * intermediate types created in the process.  we just need to save
     * all our contents/envelope information.
     */

    /* Save contents */
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (4 * ndims + 4) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = size;
    ints[1] = rank;
    ints[2] = ndims;

    for (int i = 0; i < ndims; i++) {
        ints[i + 3] = array_of_gsizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + ndims + 3] = array_of_distribs[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + 2 * ndims + 3] = array_of_dargs[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + 3 * ndims + 3] = array_of_psizes[i];
    }
    ints[4 * ndims + 3] = order;

    MPIR_Datatype *datatype_ptr;
    MPIR_Datatype_get_ptr(*newtype, datatype_ptr);
    mpi_errno = MPIR_Datatype_set_contents(datatype_ptr,
                                           MPI_COMBINER_DARRAY,
                                           4 * ndims + 4, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_darray_large_impl(int size, int rank, int ndims,
                                       const MPI_Aint array_of_gsizes[],
                                       const int array_of_distribs[], const int array_of_dargs[],
                                       const int array_of_psizes[], int order, MPI_Datatype oldtype,
                                       MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    int *ints;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(2);

    mpi_errno = MPIR_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
                                        array_of_dargs, array_of_psizes, order, oldtype, newtype);
    MPIR_ERR_CHECK(mpi_errno);

    /* Save contents */
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (3 * ndims + 4) * sizeof(int), mpi_errno,
                               "content ints array", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC_ORJUMP(counts, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "content counts array", MPL_MEM_BUFFER);

    ints[0] = size;
    ints[1] = rank;
    ints[2] = ndims;

    for (int i = 0; i < ndims; i++) {
        counts[i] = array_of_gsizes[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + 3] = array_of_distribs[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + ndims + 3] = array_of_dargs[i];
    }
    for (int i = 0; i < ndims; i++) {
        ints[i + 2 * ndims + 3] = array_of_psizes[i];
    }
    ints[3 * ndims + 3] = order;

    MPIR_Datatype *datatype_ptr;
    MPIR_Datatype_get_ptr(*newtype, datatype_ptr);
    mpi_errno = MPIR_Datatype_set_contents(datatype_ptr,
                                           MPI_COMBINER_DARRAY,
                                           3 * ndims + 4, 0, ndims, 1, ints, NULL, counts,
                                           &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
