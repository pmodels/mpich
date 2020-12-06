/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

void MPIR_Get_count_impl(const MPI_Status * status, MPI_Datatype datatype, MPI_Aint * count)
{
    MPI_Aint size;

    MPIR_Datatype_get_size_macro(datatype, size);
    MPIR_Assert(size >= 0 && MPIR_STATUS_GET_COUNT(*status) >= 0);
    if (size != 0) {
        /* MPI-3 says return MPI_UNDEFINED if too large for an int */
        if ((MPIR_STATUS_GET_COUNT(*status) % size) != 0)
            (*count) = MPI_UNDEFINED;
        else
            (*count) = (MPI_Aint) (MPIR_STATUS_GET_COUNT(*status) / size);
    } else {
        if (MPIR_STATUS_GET_COUNT(*status) > 0) {
            /* --BEGIN ERROR HANDLING-- */

            /* case where datatype size is 0 and count is > 0 should
             * never occur.
             */

            (*count) = MPI_UNDEFINED;
            /* --END ERROR HANDLING-- */
        } else {
            /* This is ambiguous.  However, discussions on MPI Forum
             * reached a consensus that this is the correct return
             * value
             */
            (*count) = 0;
        }
    }
}

void MPIR_Pack_size_impl(int incount, MPI_Datatype datatype, MPI_Aint * size)
{
    MPI_Aint typesize;
    MPIR_Datatype_get_size_macro(datatype, typesize);
    *size = incount * typesize;
}

int MPIR_Status_set_elements_x_impl(MPI_Status * status, MPI_Datatype datatype, MPI_Count count)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Count size_x;

    MPIR_Datatype_get_size_macro(datatype, size_x);

    /* overflow check, should probably be a real error check? */
    if (count != 0) {
        MPIR_Assert(size_x >= 0 && count > 0);
        MPIR_Assert(count * size_x < MPIR_COUNT_MAX);
    }

    MPIR_STATUS_SET_COUNT(*status, size_x * count);

    return mpi_errno;
}

int MPIR_Type_commit_impl(MPI_Datatype * datatype_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr;

    MPIR_Assert(!HANDLE_IS_BUILTIN(*datatype_p));

    MPIR_Datatype_get_ptr(*datatype_p, datatype_ptr);

    if (datatype_ptr->is_committed == 0) {
        datatype_ptr->is_committed = 1;

        MPIR_Typerep_commit(*datatype_p);

        MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, TERSE, "# contig blocks = %d\n",
                      (int) datatype_ptr->typerep.num_contig_blocks);

        MPID_Type_commit_hook(datatype_ptr);

    }
    return mpi_errno;
}

int MPIR_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_contiguous",
                                         __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = MPIR_Datatype_get_basic_size(oldtype);

        new_dtp->size = count * el_sz;
        new_dtp->true_lb = 0;
        new_dtp->lb = 0;
        new_dtp->true_ub = count * el_sz;
        new_dtp->ub = new_dtp->true_ub;
        new_dtp->extent = new_dtp->ub - new_dtp->lb;

        new_dtp->alignsize = el_sz;
        new_dtp->n_builtin_elements = count;
        new_dtp->builtin_element_size = el_sz;
        new_dtp->basic_type = oldtype;
        new_dtp->is_contig = 1;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        new_dtp->size = count * old_dtp->size;

        MPII_DATATYPE_CONTIG_LB_UB((MPI_Aint) count,
                                   old_dtp->lb,
                                   old_dtp->ub, old_dtp->extent, new_dtp->lb, new_dtp->ub);

        /* easiest to calc true lb/ub relative to lb/ub; doesn't matter
         * if there are sticky lb/ubs or not when doing this.
         */
        new_dtp->true_lb = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
        new_dtp->true_ub = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
        new_dtp->extent = new_dtp->ub - new_dtp->lb;

        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->n_builtin_elements = count * old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;

        MPIR_Datatype_is_contig(oldtype, &new_dtp->is_contig);
    }

    mpi_errno = MPIR_Typerep_create_contig(count, oldtype, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "contig type %x created.", new_dtp->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_contiguous_impl(int count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;
    MPI_Datatype new_handle;

    mpi_errno = MPIR_Type_contiguous(count, oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_CONTIGUOUS, 1, /* ints (count) */
                                           0, 1, &count, NULL, &oldtype);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Type_contiguous_x_impl(MPI_Count count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    /* to make 'count' fit MPI-3 type processing routines (which take integer
     * counts), we construct a type consisting of N INT_MAX chunks followed by
     * a remainder.  e.g for a count of 4000000000 bytes you would end up with
     * one 2147483647-byte chunk followed immediately by a 1852516353-byte
     * chunk */
    MPI_Datatype chunks, remainder;
    MPI_Aint lb, extent, disps[2];
    int blocklens[2];
    MPI_Datatype types[2];
    int mpi_errno;

    /* truly stupendously large counts will overflow an integer with this math,
     * but that is a problem for a few decades from now.  Sorry, few decades
     * from now! */
    MPIR_Assert(count / INT_MAX == (int) (count / INT_MAX));
    int c = (int) (count / INT_MAX);    /* OK to cast until 'count' is 256 bits */
    int r = count % INT_MAX;

    mpi_errno = MPIR_Type_vector_impl(c, INT_MAX, INT_MAX, oldtype, &chunks);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    mpi_errno = MPIR_Type_contiguous_impl(r, oldtype, &remainder);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_Type_get_extent_impl(oldtype, &lb, &extent);

    blocklens[0] = 1;
    blocklens[1] = 1;
    disps[0] = 0;
    disps[1] = c * extent * INT_MAX;
    types[0] = chunks;
    types[1] = remainder;

    mpi_errno = MPIR_Type_create_struct_impl(2, blocklens, disps, types, newtype);

    MPIR_Type_free_impl(&chunks);
    MPIR_Type_free_impl(&remainder);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIR_Type_block(const int *array_of_gsizes,
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
    int mpi_errno, blksize, global_size, mysize, i, j;
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

    j = global_size - blksize * rank;
    mysize = MPL_MIN(blksize, j);
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
            for (i = 0; i < dim; i++)
                stride *= (MPI_Aint) (array_of_gsizes[i]);
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
            for (i = ndims - 1; i > dim; i--)
                stride *= (MPI_Aint) (array_of_gsizes[i]);
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

    *st_offset = (MPI_Aint) blksize *(MPI_Aint) rank;
    /* in terms of no. of elements of type oldtype in this dimension */
    if (mysize == 0)
        *st_offset = 0;

    return MPI_SUCCESS;
}

static int MPIR_Type_cyclic(const int *array_of_gsizes,
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
    int mpi_errno, blksize, i, blklens[3], st_index, end_index, local_size, rem, count;
    MPI_Aint stride, disps[3];
    MPI_Datatype type_tmp, type_indexed, types[3];

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
        for (i = 0; i < dim; i++)
            stride *= (MPI_Aint) (array_of_gsizes[i]);
    else
        for (i = ndims - 1; i > dim; i--)
            stride *= (MPI_Aint) (array_of_gsizes[i]);

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
        disps[0] = 0;
        disps[1] = (MPI_Aint) rank *(MPI_Aint) blksize *orig_extent;
        disps[2] = orig_extent * (MPI_Aint) (array_of_gsizes[dim]);

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

    return MPI_SUCCESS;
}

int MPIR_Type_create_hvector_impl(int count, int blocklength, MPI_Aint stride,
                                  MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int ints[2];

    mpi_errno = MPIR_Type_vector(count, blocklength, stride, 1, /* stride in bytes */
                                 oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    ints[0] = count;
    ints[1] = blocklength;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HVECTOR, 2,    /* ints (count, blocklength) */
                                           1,   /* aints */
                                           1,   /* types */
                                           ints, &stride, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hindexed_impl(int count, const int array_of_blocklengths[],
                                   const MPI_Aint array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int *ints;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_indexed(count, array_of_blocklengths, array_of_displacements, 1,      /* displacements in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 1) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);
    ints[0] = count;

    for (int i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED, count + 1,   /* ints (count, blocklengths) */
                                           count,       /* aints (displacements) */
                                           1,   /* types */
                                           ints, array_of_displacements, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hindexed_block_impl(int count, int blocklength,
                                         const MPI_Aint array_of_displacements[],
                                         MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int ints[2];

    mpi_errno = MPIR_Type_blockindexed(count, blocklength, array_of_displacements, 1,
                                       oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    ints[0] = count;
    ints[1] = blocklength;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED_BLOCK, 2,     /* ints */
                                           count,       /* aints */
                                           1,   /* types */
                                           ints, array_of_displacements, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_indexed_block_impl(int count,
                                        int blocklength,
                                        const int array_of_displacements[],
                                        MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(1);
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int i, *ints;

    mpi_errno = MPIR_Type_blockindexed(count, blocklength, array_of_displacements, 0,   /* dispinbytes */
                                       oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 2) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = count;
    ints[1] = blocklength;

    for (i = 0; i < count; i++)
        ints[i + 2] = array_of_displacements[i];

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED_BLOCK, count + 2,      /* ints */
                                           0,   /* aints */
                                           1,   /* types */
                                           ints, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_resized(MPI_Datatype oldtype,
                             MPI_Aint lb, MPI_Aint extent, MPI_Datatype * newtype_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;

    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_create_resized",
                                         __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = 0;
    new_dtp->name[0] = 0;
    new_dtp->contents = 0;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    /* if oldtype is a basic, we build a contiguous typerep of count = 1 */
    if (HANDLE_IS_BUILTIN(oldtype)) {
        int oldsize = MPIR_Datatype_get_basic_size(oldtype);

        new_dtp->size = oldsize;
        new_dtp->true_lb = 0;
        new_dtp->lb = lb;
        new_dtp->true_ub = oldsize;
        new_dtp->ub = lb + extent;
        new_dtp->extent = extent;
        new_dtp->alignsize = oldsize;   /* FIXME ??? */
        new_dtp->n_builtin_elements = 1;
        new_dtp->builtin_element_size = oldsize;
        new_dtp->is_contig = (extent == oldsize) ? 1 : 0;
        new_dtp->basic_type = oldtype;
    } else {
        /* user-defined base type */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        new_dtp->size = old_dtp->size;
        new_dtp->true_lb = old_dtp->true_lb;
        new_dtp->lb = lb;
        new_dtp->true_ub = old_dtp->true_ub;
        new_dtp->ub = lb + extent;
        new_dtp->extent = extent;
        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->n_builtin_elements = old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;

        if (extent == old_dtp->size)
            MPIR_Datatype_is_contig(oldtype, &new_dtp->is_contig);
        else
            new_dtp->is_contig = 0;
    }

    mpi_errno = MPIR_Typerep_create_resized(oldtype, lb, extent, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype_p = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "resized type %x created.", new_dtp->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_struct_impl(int count,
                                 const int array_of_blocklengths[],
                                 const MPI_Aint array_of_displacements[],
                                 const MPI_Datatype array_of_types[], MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ints;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_struct(count,
                                 array_of_blocklengths,
                                 array_of_displacements, array_of_types, &new_handle);

    MPIR_ERR_CHECK(mpi_errno);


    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 1) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = count;
    for (i = 0; i < count; i++)
        ints[i + 1] = array_of_blocklengths[i];

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_STRUCT, count + 1,     /* ints (cnt,blklen) */
                                           count,       /* aints (disps) */
                                           count,       /* types */
                                           ints, array_of_displacements, array_of_types);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

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
    int i;
    MPI_Datatype new_handle;

    int procs, tmp_rank, tmp_size, *coords;
    MPI_Aint *st_offsets, orig_extent, disps[3];
    MPI_Datatype type_old, type_new = MPI_DATATYPE_NULL, tmp_type;

    int *ints;
    MPIR_Datatype *datatype_ptr = NULL;
    MPIR_CHKLMEM_DECL(3);

    /* calculate position in Cartesian grid as MPI would (row-major ordering) */
    MPIR_CHKLMEM_MALLOC_ORJUMP(coords, int *, ndims * sizeof(int), mpi_errno,
                               "position is Cartesian grid", MPL_MEM_COMM);

    MPIR_Datatype_get_ptr(oldtype, datatype_ptr);
    MPIR_Datatype_get_extent_macro(oldtype, orig_extent);
    procs = size;
    tmp_rank = rank;
    for (i = 0; i < ndims; i++) {
        procs = procs / array_of_psizes[i];
        coords[i] = tmp_rank / procs;
        tmp_rank = tmp_rank % procs;
    }

    MPIR_CHKLMEM_MALLOC_ORJUMP(st_offsets, MPI_Aint *, ndims * sizeof(MPI_Aint), mpi_errno,
                               "st_offsets", MPL_MEM_COMM);

    type_old = oldtype;

    if (order == MPI_ORDER_FORTRAN) {
        /* dimension 0 changes fastest */
        for (i = 0; i < ndims; i++) {
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

            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* --END ERROR HANDLING-- */
        }

        /* add displacement and UB */
        disps[1] = st_offsets[0];
        tmp_size = 1;
        for (i = 1; i < ndims; i++) {
            tmp_size *= array_of_gsizes[i - 1];
            disps[1] += (MPI_Aint) tmp_size *st_offsets[i];
        }
        /* rest done below for both Fortran and C order */
    }

    else {      /* order == MPI_ORDER_C */

        /* dimension ndims-1 changes fastest */
        for (i = ndims - 1; i >= 0; i--) {
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

            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* --END ERROR HANDLING-- */
        }

        /* add displacement and UB */
        disps[1] = st_offsets[ndims - 1];
        tmp_size = 1;
        for (i = ndims - 2; i >= 0; i--) {
            tmp_size *= array_of_gsizes[i + 1];
            disps[1] += (MPI_Aint) tmp_size *st_offsets[i];
        }
    }

    disps[1] *= orig_extent;

    disps[2] = orig_extent;
    for (i = 0; i < ndims; i++)
        disps[2] *= (MPI_Aint) (array_of_gsizes[i]);

    disps[0] = 0;

    mpi_errno = MPIR_Type_blockindexed(1, 1, &disps[1], 1,      /* 1 means disp is in bytes */
                                       type_new, &tmp_type);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIR_Type_create_resized(tmp_type, 0, disps[2], &new_handle);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    MPIR_Type_free_impl(&tmp_type);
    MPIR_Type_free_impl(&type_new);

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

    for (i = 0; i < ndims; i++) {
        ints[i + 3] = array_of_gsizes[i];
    }
    for (i = 0; i < ndims; i++) {
        ints[i + ndims + 3] = array_of_distribs[i];
    }
    for (i = 0; i < ndims; i++) {
        ints[i + 2 * ndims + 3] = array_of_dargs[i];
    }
    for (i = 0; i < ndims; i++) {
        ints[i + 3 * ndims + 3] = array_of_psizes[i];
    }
    ints[4 * ndims + 3] = order;
    MPIR_Datatype_get_ptr(new_handle, datatype_ptr);
    mpi_errno = MPIR_Datatype_set_contents(datatype_ptr,
                                           MPI_COMBINER_DARRAY,
                                           4 * ndims + 4, 0, 1, ints, NULL, &oldtype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIR_Typerep_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
                                           array_of_dargs, array_of_psizes, order, oldtype,
                                           datatype_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_SUBARRAY, 3 * ndims + 2,       /* ints */
                                           0,   /* aints */
                                           1,   /* types */
                                           ints, NULL, &oldtype);
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

void MPIR_Type_free_impl(MPI_Datatype * datatype)
{
    MPIR_Datatype *datatype_ptr = NULL;

    MPIR_Datatype_get_ptr(*datatype, datatype_ptr);
    MPIR_Assert(datatype_ptr);
    MPIR_Datatype_ptr_release(datatype_ptr);
    *datatype = MPI_DATATYPE_NULL;
}

int MPIR_Type_get_contents_impl(MPI_Datatype datatype, int max_integers, int max_addresses,
                                int max_datatypes, int array_of_integers[],
                                MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[])
{
    int i, mpi_errno;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    /* --BEGIN ERROR HANDLING-- */
    /* these are checked at the MPI layer, so I feel that asserts
     * are appropriate.
     */
    MPIR_Assert(!HANDLE_IS_BUILTIN(datatype));
    MPIR_Assert(datatype != MPI_FLOAT_INT &&
                datatype != MPI_DOUBLE_INT &&
                datatype != MPI_LONG_INT &&
                datatype != MPI_SHORT_INT && datatype != MPI_LONG_DOUBLE_INT);
    /* --END ERROR HANDLING-- */

    MPIR_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    /* --BEGIN ERROR HANDLING-- */
    if (max_integers < cp->nr_ints || max_addresses < cp->nr_aints || max_datatypes < cp->nr_types) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_get_contents_impl", __LINE__,
                                         MPI_ERR_OTHER, "**dtype", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (cp->nr_ints > 0) {
        MPII_Datatype_get_contents_ints(cp, array_of_integers);
    }

    if (cp->nr_aints > 0) {
        MPII_Datatype_get_contents_aints(cp, array_of_addresses);
    }

    if (cp->nr_types > 0) {
        MPII_Datatype_get_contents_types(cp, array_of_datatypes);
    }

    for (i = 0; i < cp->nr_types; i++) {
        if (!HANDLE_IS_BUILTIN(array_of_datatypes[i])) {
            MPIR_Datatype_get_ptr(array_of_datatypes[i], dtp);
            MPIR_Datatype_ptr_add_ref(dtp);
        }
    }

    return MPI_SUCCESS;
}

void MPIR_Type_get_envelope(MPI_Datatype datatype, int *num_integers,
                            int *num_addresses, int *num_datatypes, int *combiner)
{
    if (HANDLE_IS_BUILTIN(datatype) ||
        datatype == MPI_FLOAT_INT ||
        datatype == MPI_DOUBLE_INT ||
        datatype == MPI_LONG_INT || datatype == MPI_SHORT_INT || datatype == MPI_LONG_DOUBLE_INT) {
        *combiner = MPI_COMBINER_NAMED;
        *num_integers = 0;
        *num_addresses = 0;
        *num_datatypes = 0;
    } else {
        MPIR_Datatype *dtp;

        MPIR_Datatype_get_ptr(datatype, dtp);

        *combiner = dtp->contents->combiner;
        *num_integers = dtp->contents->nr_ints;
        *num_addresses = dtp->contents->nr_aints;
        *num_datatypes = dtp->contents->nr_types;
    }

}

void MPIR_Type_get_extent_impl(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
    MPI_Count lb_x, extent_x;

    MPIR_Type_get_extent_x_impl(datatype, &lb_x, &extent_x);
    *lb = (lb_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) lb_x;
    *extent = (extent_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) extent_x;
}

/* any non-MPI functions go here, especially non-static ones */

void MPIR_Type_get_extent_x_impl(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent)
{
    MPIR_Datatype *datatype_ptr = NULL;

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);

    if (HANDLE_IS_BUILTIN(datatype)) {
        *lb = 0;
        *extent = MPIR_Datatype_get_basic_size(datatype);
    } else {
        *lb = datatype_ptr->lb;
        *extent = datatype_ptr->extent; /* derived, should be same as ub - lb */
    }
}

void MPIR_Type_get_true_extent_impl(MPI_Datatype datatype, MPI_Aint * true_lb,
                                    MPI_Aint * true_extent)
{
    MPI_Count true_lb_x, true_extent_x;

    MPIR_Type_get_true_extent_x_impl(datatype, &true_lb_x, &true_extent_x);
    *true_lb = (true_lb_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) true_lb_x;
    *true_extent = (true_extent_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) true_extent_x;
}

/* any non-MPI functions go here, especially non-static ones */

void MPIR_Type_get_true_extent_x_impl(MPI_Datatype datatype, MPI_Count * true_lb,
                                      MPI_Count * true_extent)
{
    MPIR_Datatype *datatype_ptr = NULL;

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);

    if (HANDLE_IS_BUILTIN(datatype)) {
        *true_lb = 0;
        *true_extent = MPIR_Datatype_get_basic_size(datatype);
    } else {
        *true_lb = datatype_ptr->true_lb;
        *true_extent = datatype_ptr->true_ub - datatype_ptr->true_lb;
    }
}

int MPIR_Type_indexed(int count,
                      const int *blocklength_array,
                      const void *displacement_array,
                      int dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int old_is_contig;
    int i;
    MPI_Aint contig_count;
    MPI_Aint el_ct, old_ct, old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

    /* sanity check that blocklens are all non-negative */
    for (i = 0; i < count; ++i) {
        MPIR_Assert(blocklength_array[i] >= 0);
    }

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_indexed",
                                         __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        /* builtins are handled differently than user-defined types because
         * they have no associated typerep or datatype structure.
         */
        MPI_Aint el_sz = MPIR_Datatype_get_basic_size(oldtype);
        old_sz = el_sz;
        el_ct = 1;

        old_lb = 0;
        old_true_lb = 0;
        old_ub = (MPI_Aint) el_sz;
        old_true_ub = (MPI_Aint) el_sz;
        old_extent = (MPI_Aint) el_sz;
        old_is_contig = 1;

        MPIR_Assign_trunc(new_dtp->alignsize, el_sz, MPI_Aint);
        new_dtp->builtin_element_size = el_sz;
        new_dtp->basic_type = oldtype;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        /* Ensure that "builtin_element_size" fits into an int datatype. */
        MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

        old_sz = old_dtp->size;
        el_ct = old_dtp->n_builtin_elements;

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_extent = old_dtp->extent;
        MPIR_Datatype_is_contig(oldtype, &old_is_contig);

        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;
    }

    /* find the first nonzero blocklength element */
    i = 0;
    while (i < count && blocklength_array[i] == 0)
        i++;

    if (i == count) {
        MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
        return MPII_Type_zerolen(newtype);
    }

    /* priming for loop */
    old_ct = blocklength_array[i];
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
        (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);

    MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
                              eff_disp, old_lb, old_ub, old_extent, min_lb, max_ub);

    /* determine min lb, max ub, and count of old types in remaining
     * nonzero size blocks
     */
    for (i++; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub;

        if (blocklength_array[i] > 0) {
            old_ct += blocklength_array[i];     /* add more oldtypes */

            eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
                (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);

            /* calculate ub and lb for this block */
            MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) (blocklength_array[i]),
                                      eff_disp, old_lb, old_ub, old_extent, tmp_lb, tmp_ub);

            if (tmp_lb < min_lb)
                min_lb = tmp_lb;
            if (tmp_ub > max_ub)
                max_ub = tmp_ub;
        }
    }

    new_dtp->size = old_ct * old_sz;

    new_dtp->lb = min_lb;
    new_dtp->ub = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent = max_ub - min_lb;

    new_dtp->n_builtin_elements = old_ct * el_ct;

    /* new type is only contig for N types if it's all one big
     * block, its size and extent are the same, and the old type
     * was also contiguous.
     */
    new_dtp->is_contig = 0;
    if (old_is_contig) {
        MPI_Aint *blklens = MPL_malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
        MPIR_Assert(blklens != NULL);
        for (i = 0; i < count; i++)
            blklens[i] = blocklength_array[i];
        contig_count = MPII_Datatype_indexed_count_contig(count,
                                                          blklens,
                                                          displacement_array, dispinbytes,
                                                          old_extent);
        if ((contig_count == 1) && ((MPI_Aint) new_dtp->size == new_dtp->extent)) {
            new_dtp->is_contig = 1;
        }
        MPL_free(blklens);
    }

    if (dispinbytes) {
        mpi_errno =
            MPIR_Typerep_create_hindexed(count, blocklength_array, displacement_array, oldtype,
                                         new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno =
            MPIR_Typerep_create_indexed(count, blocklength_array, displacement_array, oldtype,
                                        new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *newtype = new_dtp->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_indexed_impl(int count, const int *array_of_blocklengths,
                           const int *array_of_displacements,
                           MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int i, *ints;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_indexed(count, array_of_blocklengths, array_of_displacements, 0,      /* displacements not in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy all integer values into a temporary buffer; this
     * includes the count, the blocklengths, and the displacements.
     */
    MPIR_CHKLMEM_MALLOC(ints, int *, (2 * count + 1) * sizeof(int), mpi_errno,
                        "contents integer array", MPL_MEM_BUFFER);

    ints[0] = count;

    for (i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }
    for (i = 0; i < count; i++) {
        ints[i + count + 1] = array_of_displacements[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED, 2 * count + 1,        /* ints */
                                           0,   /* aints  */
                                           1,   /* types */
                                           ints, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIR_Type_lb_impl(MPI_Datatype datatype, MPI_Aint * displacement)
{
    if (HANDLE_IS_BUILTIN(datatype)) {
        *displacement = 0;
    } else {
        MPIR_Datatype *datatype_ptr = NULL;
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);
        *displacement = datatype_ptr->lb;
    }
}

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Type_size_x_impl(MPI_Datatype datatype, MPI_Count * size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype_get_size_macro(datatype, *size);

    return mpi_errno;
}

static MPI_Aint struct_alignsize(int count, const MPI_Datatype * oldtype_array)
{
    MPI_Aint max_alignsize = 0, tmp_alignsize;

    for (int i = 0; i < count; i++) {
        if (HANDLE_IS_BUILTIN(oldtype_array[i])) {
            tmp_alignsize = MPIR_Datatype_builtintype_alignment(oldtype_array[i]);
        } else {
            MPIR_Datatype *dtp;
            MPIR_Datatype_get_ptr(oldtype_array[i], dtp);
            tmp_alignsize = dtp->alignsize;
        }
        if (max_alignsize < tmp_alignsize)
            max_alignsize = tmp_alignsize;
    }

    return max_alignsize;
}

static int type_struct(int count,
                       const int *blocklength_array,
                       const MPI_Aint * displacement_array,
                       const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, old_are_contig = 1, definitely_not_contig = 0;
    int found_true_lb = 0, found_true_ub = 0, found_el_type = 0, found_lb = 0, found_ub = 0;
    MPI_Aint el_sz = 0;
    MPI_Aint size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, lb_disp = 0, ub_disp = 0;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

#ifdef MPID_STRUCT_DEBUG
    MPII_Datatype_printf(oldtype_array[0], 1, displacement_array[0], blocklength_array[0], 1);
    for (i = 1; i < count; i++) {
        MPII_Datatype_printf(oldtype_array[i], 1, displacement_array[i], blocklength_array[i], 0);
    }
#endif

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_struct", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    /* check for junk struct with all zero blocks */
    for (i = 0; i < count; i++)
        if (blocklength_array[i] != 0)
            break;

    if (i == count) {
        MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
        return MPII_Type_zerolen(newtype);
    }

    for (i = 0; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
        MPI_Aint tmp_el_sz;
        MPI_Datatype tmp_el_type;
        MPIR_Datatype *old_dtp = NULL;
        int old_is_contig;

        /* Interpreting typemap to not include 0 blklen things. -- Rob
         * Ross, 10/31/2005
         */
        if (blocklength_array[i] == 0)
            continue;

        if (HANDLE_IS_BUILTIN(oldtype_array[i])) {
            tmp_el_sz = MPIR_Datatype_get_basic_size(oldtype_array[i]);
            tmp_el_type = oldtype_array[i];

            MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) (blocklength_array[i]),
                                      displacement_array[i],
                                      0, tmp_el_sz, tmp_el_sz, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb;
            tmp_true_ub = tmp_ub;

            size += tmp_el_sz * blocklength_array[i];
        } else {
            MPIR_Datatype_get_ptr(oldtype_array[i], old_dtp);

            /* Ensure that "builtin_element_size" fits into an int datatype. */
            MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

            tmp_el_sz = old_dtp->builtin_element_size;
            tmp_el_type = old_dtp->basic_type;

            MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
                                      displacement_array[i],
                                      old_dtp->lb, old_dtp->ub, old_dtp->extent, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb + (old_dtp->true_lb - old_dtp->lb);
            tmp_true_ub = tmp_ub + (old_dtp->true_ub - old_dtp->ub);

            size += old_dtp->size * blocklength_array[i];
        }

        /* element size and type */
        if (found_el_type == 0) {
            el_sz = tmp_el_sz;
            el_type = tmp_el_type;
            found_el_type = 1;
        } else if (el_sz != tmp_el_sz) {
            el_sz = -1;
            el_type = MPI_DATATYPE_NULL;
        } else if (el_type != tmp_el_type) {
            /* Q: should we set el_sz = -1 even though the same? */
            el_type = MPI_DATATYPE_NULL;
        }

        /* keep lowest lb/true_lb and highest ub/true_ub
         *
         * note: checking for contiguity at the same time, to avoid
         *       yet another pass over the arrays
         */
        if (!found_true_lb) {
            found_true_lb = 1;
            true_lb_disp = tmp_true_lb;
        } else if (true_lb_disp > tmp_true_lb) {
            /* element starts before previous */
            true_lb_disp = tmp_true_lb;
            definitely_not_contig = 1;
        }

        if (!found_lb) {
            found_lb = 1;
            lb_disp = tmp_lb;
        } else if (lb_disp > tmp_lb) {
            /* lb before previous */
            lb_disp = tmp_lb;
            definitely_not_contig = 1;
        }

        if (!found_true_ub) {
            found_true_ub = 1;
            true_ub_disp = tmp_true_ub;
        } else if (true_ub_disp < tmp_true_ub) {
            true_ub_disp = tmp_true_ub;
        } else {
            /* element ends before previous ended */
            definitely_not_contig = 1;
        }

        if (!found_ub) {
            found_ub = 1;
            ub_disp = tmp_ub;
        } else if (ub_disp < tmp_ub) {
            ub_disp = tmp_ub;
        } else {
            /* ub before previous */
            definitely_not_contig = 1;
        }

        MPIR_Datatype_is_contig(oldtype_array[i], &old_is_contig);
        if (!old_is_contig)
            old_are_contig = 0;
    }

    new_dtp->n_builtin_elements = -1;   /* TODO */
    new_dtp->builtin_element_size = el_sz;
    new_dtp->basic_type = el_type;

    new_dtp->true_lb = true_lb_disp;
    new_dtp->lb = lb_disp;

    new_dtp->true_ub = true_ub_disp;
    new_dtp->ub = ub_disp;

    new_dtp->alignsize = struct_alignsize(count, oldtype_array);

    new_dtp->extent = new_dtp->ub - new_dtp->lb;
    /* account for padding */
    MPI_Aint epsilon = (new_dtp->alignsize > 0) ?
        new_dtp->extent % ((MPI_Aint) (new_dtp->alignsize)) : 0;

    if (epsilon) {
        new_dtp->ub += ((MPI_Aint) (new_dtp->alignsize) - epsilon);
        new_dtp->extent = new_dtp->ub - new_dtp->lb;
    }

    new_dtp->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous, and we didn't see
     * something noncontiguous based on true ub/ub.
     */
    if (((MPI_Aint) (new_dtp->size) == new_dtp->extent) &&
        old_are_contig && (!definitely_not_contig)) {
        new_dtp->is_contig = 1;
    } else {
        new_dtp->is_contig = 0;
    }

    mpi_errno = MPIR_Typerep_create_struct(count, blocklength_array, displacement_array,
                                           oldtype_array, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype = new_dtp->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_struct(int count,
                     const int *blocklength_array,
                     const MPI_Aint * displacement_array,
                     const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    /* detect if the old MPI_LB/MPI_UB API is used */
    bool using_old_api = false;
    for (int i = 0; i < count; i++) {
        if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB) {
            using_old_api = true;
            break;
        }
    }

    if (!using_old_api) {
        mpi_errno =
            type_struct(count, blocklength_array, displacement_array, oldtype_array, newtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int *real_blocklength_array = (int *) MPL_malloc(count * sizeof(int), MPL_MEM_DATATYPE);
        MPI_Aint *real_displacement_array = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint),
                                                                    MPL_MEM_DATATYPE);
        MPI_Datatype *real_oldtype_array = (MPI_Datatype *) MPL_malloc(count * sizeof(MPI_Datatype),
                                                                       MPL_MEM_DATATYPE);

        int real_count = 0;
        for (int i = 0; i < count; i++) {
            if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB) {
                real_blocklength_array[real_count] = blocklength_array[i];
                real_displacement_array[real_count] = displacement_array[i];
                real_oldtype_array[real_count] = oldtype_array[i];
                real_count++;
            }
        }

        MPI_Datatype tmptype;
        mpi_errno = type_struct(real_count, real_blocklength_array, real_displacement_array,
                                real_oldtype_array, &tmptype);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(real_oldtype_array);
        MPL_free(real_displacement_array);
        MPL_free(real_blocklength_array);

        MPIR_Datatype *tmptype_ptr;
        MPIR_Datatype_get_ptr(tmptype, tmptype_ptr);

        MPI_Aint lb = tmptype_ptr->lb, ub = tmptype_ptr->ub;
        for (int i = 0; i < count; i++) {
            if (oldtype_array[i] == MPI_LB)
                lb = displacement_array[i];
            else if (oldtype_array[i] == MPI_UB)
                ub = displacement_array[i];
        }

        mpi_errno = MPIR_Type_create_resized(tmptype, lb, ub - lb, newtype);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Type_free_impl(&tmptype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_vector(int count,
                     int blocklength,
                     MPI_Aint stride,
                     int strideinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int old_is_contig;
    MPI_Aint old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub, eff_stride;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    if (!new_dtp) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_vector", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = (MPI_Aint) MPIR_Datatype_get_basic_size(oldtype);

        old_lb = 0;
        old_true_lb = 0;
        old_ub = el_sz;
        old_true_ub = el_sz;
        old_sz = el_sz;
        old_extent = el_sz;
        old_is_contig = 1;

        new_dtp->size = (MPI_Aint) count *(MPI_Aint) blocklength *el_sz;

        new_dtp->alignsize = el_sz;     /* ??? */
        new_dtp->n_builtin_elements = count * blocklength;
        new_dtp->builtin_element_size = el_sz;
        new_dtp->basic_type = oldtype;

        eff_stride = (strideinbytes) ? stride : (stride * el_sz);
    } else {    /* user-defined base type (oldtype) */

        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_sz = old_dtp->size;
        old_extent = old_dtp->extent;
        MPIR_Datatype_is_contig(oldtype, &old_is_contig);

        new_dtp->size = count * blocklength * old_dtp->size;

        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->n_builtin_elements = count * blocklength * old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;

        eff_stride = (strideinbytes) ? stride : (stride * old_dtp->extent);
    }

    MPII_DATATYPE_VECTOR_LB_UB((MPI_Aint) count,
                               eff_stride,
                               (MPI_Aint) blocklength,
                               old_lb, old_ub, old_extent, new_dtp->lb, new_dtp->ub);
    new_dtp->true_lb = new_dtp->lb + (old_true_lb - old_lb);
    new_dtp->true_ub = new_dtp->ub + (old_true_ub - old_ub);
    new_dtp->extent = new_dtp->ub - new_dtp->lb;

    /* new type is only contig for N types if old one was, and
     * size and extent of new type are equivalent, and stride is
     * equal to blocklength * size of old type.
     */
    if ((MPI_Aint) (new_dtp->size) == new_dtp->extent &&
        eff_stride == (MPI_Aint) blocklength * old_sz && old_is_contig) {
        new_dtp->is_contig = 1;
    } else {
        new_dtp->is_contig = 0;
    }

    if (strideinbytes) {
        mpi_errno = MPIR_Typerep_create_hvector(count, blocklength, stride, oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Typerep_create_vector(count, blocklength, stride, oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *newtype = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "vector type %x created.", new_dtp->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_vector_impl(int count, int blocklength, int stride, MPI_Datatype oldtype,
                          MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int ints[3];

    mpi_errno = MPIR_Type_vector(count, blocklength, (MPI_Aint) stride, 0,      /* stride not in bytes, in extents */
                                 oldtype, &new_handle);

    MPIR_ERR_CHECK(mpi_errno);

    ints[0] = count;
    ints[1] = blocklength;
    ints[2] = stride;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_VECTOR, 3,     /* ints (cnt, blklen, str) */
                                           0,   /* aints */
                                           1,   /* types */
                                           ints, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_resized_impl(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                  MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint aints[2];

    mpi_errno = MPIR_Type_create_resized(oldtype, lb, extent, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    aints[0] = lb;
    aints[1] = extent;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_RESIZED, 0, 2, /* Aints */
                                           1, NULL, aints, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp = 0, *old_dtp;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        /* create a new type and commit it. */
        mpi_errno = MPIR_Type_contiguous(1, oldtype, newtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* allocate new datatype object and handle */
        new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
        if (!new_dtp) {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             "MPIR_Type_dup", __LINE__, MPI_ERR_OTHER,
                                             "**nomem", 0);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        /* fill in datatype */
        MPIR_Object_set_ref(new_dtp, 1);
        /* new_dtp->handle is filled in by MPIR_Handle_obj_alloc() */
        new_dtp->is_contig = old_dtp->is_contig;
        new_dtp->size = old_dtp->size;
        new_dtp->extent = old_dtp->extent;
        new_dtp->ub = old_dtp->ub;
        new_dtp->lb = old_dtp->lb;
        new_dtp->true_ub = old_dtp->true_ub;
        new_dtp->true_lb = old_dtp->true_lb;
        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->is_committed = old_dtp->is_committed;

        new_dtp->attributes = NULL;     /* Attributes are copied in the
                                         * top-level MPI_Type_dup routine */
        new_dtp->name[0] = 0;   /* The Object name is not copied on
                                 * a dup */
        new_dtp->n_builtin_elements = old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;

        new_dtp->typerep.handle = NULL;
        *newtype = new_dtp->handle;

        if (old_dtp->is_committed) {
            MPID_Type_commit_hook(new_dtp);
        }

        mpi_errno = MPIR_Typerep_create_dup(oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, VERBOSE, "dup type %x created.", *newtype);

  fn_fail:
    return mpi_errno;
}

int MPIR_Type_dup_impl(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;

    mpi_errno = MPIR_Type_dup(oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_DUP, 0,        /* ints */
                                           0,   /* aints */
                                           1,   /* types */
                                           NULL, NULL, &oldtype);

    mpi_errno = MPIR_Type_commit_impl(&new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
     * structure to prevent type_dup from forcing the linking of the
     * attribute functions.  The actual function is (by default)
     * MPIR_Attr_dup_list
     */
    MPIR_Datatype *old_dtp;
    MPIR_Datatype_get_ptr(oldtype, old_dtp);
    if (MPIR_Process.attr_dup) {
        new_dtp->attributes = 0;
        mpi_errno = MPIR_Process.attr_dup(oldtype, old_dtp->attributes, &new_dtp->attributes);
        if (mpi_errno) {
            MPIR_Datatype_ptr_release(new_dtp);
            goto fn_fail;
        }
    }

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_match_size_impl(int typeclass, int size, MPI_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    const char *tname;
#endif
    /* Note that all of the datatype have values, even if the type is
     * not available. We test for that case separately.  We also
     * prefer the Fortran types to the C type, if they are available */
    static MPI_Datatype real_types[] = {
        MPI_REAL4, MPI_REAL8, MPI_REAL16,
        MPI_REAL, MPI_DOUBLE_PRECISION,
        MPI_FLOAT, MPI_DOUBLE, MPI_LONG_DOUBLE
    };
    static MPI_Datatype int_types[] = {
        MPI_INTEGER1, MPI_INTEGER2, MPI_INTEGER4, MPI_INTEGER8, MPI_INTEGER16,
        MPI_INTEGER,
        MPI_CHAR, MPI_SHORT, MPI_INT,
        MPI_LONG, MPI_LONG_LONG
    };
    static MPI_Datatype complex_types[] = {
        MPI_COMPLEX8, MPI_COMPLEX16, MPI_COMPLEX32,
        MPI_COMPLEX, MPI_DOUBLE_COMPLEX,
        MPI_C_COMPLEX, MPI_C_DOUBLE_COMPLEX, MPI_C_LONG_DOUBLE_COMPLEX,
    };
    MPI_Datatype matched_datatype = MPI_DATATYPE_NULL;
    int i;
    MPI_Aint tsize;

    /* The following implementation follows the suggestion in the
     * MPI-2 standard.
     * The version in the MPI-2 spec makes use of the Fortran optional types;
     * currently, we don't support these from C (see mpi.h.in).
     * Thus, we look at the candidate types and make use of the first fit.
     * Note that the standard doesn't require that this routine return
     * any particular choice of MPI datatype; e.g., it is not required
     * to return MPI_INTEGER4 if a 4-byte integer is requested.
     */
    switch (typeclass) {
        case MPI_TYPECLASS_REAL:
            {
                int nRealTypes = sizeof(real_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
                tname = "MPI_TYPECLASS_REAL";
#endif
                for (i = 0; i < nRealTypes; i++) {
                    if (real_types[i] == MPI_DATATYPE_NULL) {
                        continue;
                    }
                    MPIR_Datatype_get_size_macro(real_types[i], tsize);
                    if (tsize == size) {
                        matched_datatype = real_types[i];
                        break;
                    }
                }
            }
            break;
        case MPI_TYPECLASS_INTEGER:
            {
                int nIntTypes = sizeof(int_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
                tname = "MPI_TYPECLASS_INTEGER";
#endif
                for (i = 0; i < nIntTypes; i++) {
                    if (int_types[i] == MPI_DATATYPE_NULL) {
                        continue;
                    }
                    MPIR_Datatype_get_size_macro(int_types[i], tsize);
                    if (tsize == size) {
                        matched_datatype = int_types[i];
                        break;
                    }
                }
            }
            break;
        case MPI_TYPECLASS_COMPLEX:
            {
                int nComplexTypes = sizeof(complex_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
                tname = "MPI_TYPECLASS_COMPLEX";
#endif
                for (i = 0; i < nComplexTypes; i++) {
                    if (complex_types[i] == MPI_DATATYPE_NULL) {
                        continue;
                    }
                    MPIR_Datatype_get_size_macro(complex_types[i], tsize);
                    if (tsize == size) {
                        matched_datatype = complex_types[i];
                        break;
                    }
                }
            }
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, break, "**typematchnoclass");
            /* --END ERROR HANDLING-- */
    }

    if (mpi_errno == MPI_SUCCESS) {
        if (matched_datatype == MPI_DATATYPE_NULL) {
            /* --BEGIN ERROR HANDLING-- */
            MPIR_ERR_SETANDSTMT2(mpi_errno, MPI_ERR_ARG,;, "**typematchsize",
                                 "**typematchsize %s %d", tname, size);
            /* --END ERROR HANDLING-- */
        } else {
            *datatype = matched_datatype;
        }
    }
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
