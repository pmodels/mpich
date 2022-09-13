/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

/* There are three sets of datatype creation routines.
 *
 * The first set, named as MPIR_Xxx (e.g. MPIR_Type_dup, MPIR_Type_indexed, etc),
 * creates the "core" datatype, and is complete for internal use.
 *
 * The second set, named as MPIR_Xxx_impl, will first create the datatype by
 * calling MPIR_Xxx, then furnish the datatype with COMBINER contents, and copies
 * attributes in the Type_dup case.
 *
 * The last set, are wrapper routines to support types created on top of other types,
 * e.g. large count datatypes
 *
 * The functions within each set follows the same ordering, i.e. contiguous,
 * vector, indexed_block, indexed, struct, dup, resized.
 */

/* ---- MPIR_Xxx type creation routines ---- */

#define CREATE_NEW_DTP(new_dtp) \
    do { \
        new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem); \
        MPIR_ERR_CHKHANDLEMEM(new_dtp); \
        MPIR_Object_set_ref(new_dtp, 1); \
        new_dtp->is_committed = 0; \
        new_dtp->attributes = NULL; \
        new_dtp->name[0] = 0; \
        new_dtp->contents = NULL; \
        new_dtp->flattened = NULL; \
        new_dtp->typerep.handle = MPIR_TYPEREP_HANDLE_NULL; \
    } while (0)

static bool type_size_is_zero(MPI_Datatype dt)
{
    MPI_Aint dt_size;
    MPIR_Datatype_get_size_macro(dt, dt_size);
    return dt_size == 0;
}

int MPIR_Type_contiguous(MPI_Aint count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;

    if (type_size_is_zero(oldtype) || count == 0)
        return MPII_Type_zerolen(newtype);

    CREATE_NEW_DTP(new_dtp);

    mpi_errno = MPIR_Typerep_create_contig(count, oldtype, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "contig type %x created.", new_dtp->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* common routine for vector and hvector, distinguished by dispinbytes */
int MPIR_Type_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                     bool strideinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;

    if (type_size_is_zero(oldtype) || count == 0)
        return MPII_Type_zerolen(newtype);

    CREATE_NEW_DTP(new_dtp);

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

/* common routine for indexed_block and hindexed_block, distinguished by dispinbytes */
int MPIR_Type_blockindexed(MPI_Aint count, MPI_Aint blocklength,
                           const MPI_Aint displacement_array[], bool dispinbytes,
                           MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype *new_dtp;

    if (type_size_is_zero(oldtype) || count == 0)
        return MPII_Type_zerolen(newtype);

    CREATE_NEW_DTP(new_dtp);

    if (dispinbytes) {
        mpi_errno = MPIR_Typerep_create_hindexed_block(count, blocklength, displacement_array,
                                                       oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Typerep_create_indexed_block(count, blocklength, displacement_array,
                                                      oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *newtype = new_dtp->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* common routine for indexed and hindexed, distinguished by dispinbytes */
int MPIR_Type_indexed(MPI_Aint count, const MPI_Aint * blocklength_array,
                      const MPI_Aint * displacement_array,
                      bool dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;
    MPI_Aint i;

    if (type_size_is_zero(oldtype) || count == 0)
        return MPII_Type_zerolen(newtype);

    /* sanity check that blocklens are all non-negative */
    for (i = 0; i < count; ++i) {
        MPIR_Assert(blocklength_array[i] >= 0);
    }

    CREATE_NEW_DTP(new_dtp);

    i = 0;
    while (i < count && blocklength_array[i] == 0)
        i++;

    if (i == count) {
        MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
        return MPII_Type_zerolen(newtype);
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

/* begin struct */
static int type_struct(MPI_Aint count,
                       const MPI_Aint * blocklength_array,
                       const MPI_Aint * displacement_array,
                       const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint i;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

#ifdef MPID_STRUCT_DEBUG
    MPII_Datatype_printf(oldtype_array[0], 1, displacement_array[0], blocklength_array[0], 1);
    for (i = 1; i < count; i++) {
        MPII_Datatype_printf(oldtype_array[i], 1, displacement_array[i], blocklength_array[i], 0);
    }
#endif

    CREATE_NEW_DTP(new_dtp);

    /* check for junk struct with all zero blocks */
    for (i = 0; i < count; i++)
        if (blocklength_array[i] != 0)
            break;

    if (i == count) {
        MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
        return MPII_Type_zerolen(newtype);
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

int MPIR_Type_struct(MPI_Aint count,
                     const MPI_Aint * blocklength_array,
                     const MPI_Aint * displacement_array,
                     const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    /* detect if the old MPI_LB/MPI_UB API is used */
    bool using_old_api = false;
    for (MPI_Aint i = 0; i < count; i++) {
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
        MPI_Aint *real_blocklength_array = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint),
                                                                   MPL_MEM_DATATYPE);
        MPI_Aint *real_displacement_array = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint),
                                                                    MPL_MEM_DATATYPE);
        MPI_Datatype *real_oldtype_array = (MPI_Datatype *) MPL_malloc(count * sizeof(MPI_Datatype),
                                                                       MPL_MEM_DATATYPE);

        MPI_Aint real_count = 0;
        for (MPI_Aint i = 0; i < count; i++) {
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
        for (MPI_Aint i = 0; i < count; i++) {
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

/* end struct */

int MPIR_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp = 0;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        mpi_errno = MPIR_Type_contiguous(1, oldtype, newtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        CREATE_NEW_DTP(new_dtp);

        mpi_errno = MPIR_Typerep_create_dup(oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);

        *newtype = new_dtp->handle;
    }

    MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, VERBOSE, "dup type %x created.", *newtype);

  fn_fail:
    return mpi_errno;
}

int MPIR_Type_create_resized(MPI_Datatype oldtype,
                             MPI_Aint lb, MPI_Aint extent, MPI_Datatype * newtype_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;

    CREATE_NEW_DTP(new_dtp);

    mpi_errno = MPIR_Typerep_create_resized(oldtype, lb, extent, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype_p = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "resized type %x created.", new_dtp->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- MPIR_Xxx_impl routines ---- */

int MPIR_Type_contiguous_impl(int count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;
    MPI_Datatype new_handle;

    mpi_errno = MPIR_Type_contiguous(count, oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_CONTIGUOUS,
                                           1, 0, 0, 1, &count, NULL, NULL, &oldtype);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Type_contiguous_large_impl(MPI_Aint count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *new_dtp;
    MPI_Datatype new_handle;

    mpi_errno = MPIR_Type_contiguous(count, oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPI_Aint counts[1];
    counts[0] = count;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_CONTIGUOUS,
                                           0, 0, 1, 1, NULL, NULL, counts, &oldtype);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

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

    mpi_errno = MPIR_Type_vector(count, blocklength, (MPI_Aint) stride, 0,      /* stride not in bytes, in extents */
                                 oldtype, &new_handle);

    MPIR_ERR_CHECK(mpi_errno);

    int ints[3];
    ints[0] = count;
    ints[1] = blocklength;
    ints[2] = stride;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_VECTOR,
                                           3, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_vector_large_impl(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;

    mpi_errno = MPIR_Type_vector(count, blocklength, stride, 0, /* stride not in bytes, in extents */
                                 oldtype, &new_handle);

    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint counts[3];
    counts[0] = count;
    counts[1] = blocklength;
    counts[2] = stride;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_VECTOR,
                                           0, 0, 3, 1, NULL, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hvector_impl(int count, int blocklength, MPI_Aint stride,
                                  MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;

    mpi_errno = MPIR_Type_vector(count, blocklength, stride, 1, /* stride in bytes */
                                 oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    int ints[2];
    ints[0] = count;
    ints[1] = blocklength;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HVECTOR,
                                           2, 1, 0, 1, ints, &stride, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hvector_large_impl(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                        MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;

    mpi_errno = MPIR_Type_vector(count, blocklength, stride, 1, /* stride in bytes */
                                 oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint counts[3];
    counts[0] = count;
    counts[1] = blocklength;
    counts[2] = stride;
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HVECTOR,
                                           0, 0, 3, 1, NULL, NULL, counts, &oldtype);
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
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *p_disp;
    int *ints;
    MPIR_CHKLMEM_DECL(2);

    if (sizeof(MPI_Aint) == sizeof(int)) {
        p_disp = (MPI_Aint *) array_of_displacements;
    } else {
        MPIR_CHKLMEM_MALLOC_ORJUMP(p_disp, MPI_Aint *, count * sizeof(MPI_Aint), mpi_errno,
                                   "aint displacement array", MPL_MEM_BUFFER);
        for (int i = 0; i < count; i++) {
            p_disp[i] = array_of_displacements[i];
        }
    }
    mpi_errno = MPIR_Type_blockindexed(count, blocklength, p_disp, 0,   /* dispinbytes */
                                       oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 2) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    ints[0] = count;
    ints[1] = blocklength;

    for (int i = 0; i < count; i++)
        ints[i + 2] = array_of_displacements[i];

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED_BLOCK,
                                           count + 2, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_indexed_block_large_impl(MPI_Aint count, MPI_Aint blocklength,
                                              const MPI_Aint array_of_displacements[],
                                              MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_blockindexed(count, blocklength, array_of_displacements, 0,   /* dispinbytes */
                                       oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKLMEM_MALLOC_ORJUMP(counts, MPI_Aint *, (count + 2) * sizeof(MPI_Aint), mpi_errno,
                               "content description", MPL_MEM_BUFFER);

    counts[0] = count;
    counts[1] = blocklength;

    for (MPI_Aint i = 0; i < count; i++)
        counts[i + 2] = array_of_displacements[i];

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED_BLOCK,
                                           0, 0, count + 2, 1, NULL, NULL, counts, &oldtype);
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
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED_BLOCK,
                                           2, count, 0, 1,
                                           ints, array_of_displacements, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hindexed_block_large_impl(MPI_Aint count, MPI_Aint blocklength,
                                               const MPI_Aint array_of_displacements[],
                                               MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_blockindexed(count, blocklength, array_of_displacements,
                                       1, oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKLMEM_MALLOC_ORJUMP(counts, MPI_Aint *, (count + 2) * sizeof(MPI_Aint), mpi_errno,
                               "content description", MPL_MEM_BUFFER);
    counts[0] = count;
    counts[1] = blocklength;
    for (MPI_Aint i = 0; i < count; i++)
        counts[i + 2] = array_of_displacements[i];

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED_BLOCK,
                                           0, 0, count + 2, 1, NULL, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
    MPI_Aint *p_blkl, *p_disp;
    int *ints;
    MPIR_CHKLMEM_DECL(3);

    if (sizeof(MPI_Aint) == sizeof(int)) {
        p_blkl = (MPI_Aint *) array_of_blocklengths;
        p_disp = (MPI_Aint *) array_of_displacements;
    } else {
        MPIR_CHKLMEM_MALLOC_ORJUMP(p_blkl, MPI_Aint *, count * sizeof(MPI_Aint), mpi_errno,
                                   "aint blocklengths array", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC_ORJUMP(p_disp, MPI_Aint *, count * sizeof(MPI_Aint), mpi_errno,
                                   "aint displacements array", MPL_MEM_BUFFER);
        for (int i = 0; i < count; i++) {
            p_blkl[i] = array_of_blocklengths[i];
            p_disp[i] = array_of_displacements[i];
        }
    }
    mpi_errno = MPIR_Type_indexed(count, p_blkl, p_disp, 0,     /* displacements not in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy all integer values into a temporary buffer; this
     * includes the count, the blocklengths, and the displacements.
     */
    MPIR_CHKLMEM_MALLOC(ints, int *, (2 * count + 1) * sizeof(int), mpi_errno,
                        "contents integer array", MPL_MEM_BUFFER);

    ints[0] = count;

    for (int i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }
    for (int i = 0; i < count; i++) {
        ints[i + count + 1] = array_of_displacements[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED,
                                           2 * count + 1, 0, 0, 1, ints, NULL, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_indexed_large_impl(MPI_Aint count,
                                 const MPI_Aint * array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements,
                                 MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_indexed(count, array_of_blocklengths, array_of_displacements, 0,      /* displacements not in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    /* copy all integer values into a temporary buffer; this
     * includes the count, the blocklengths, and the displacements.
     */
    MPIR_CHKLMEM_MALLOC(counts, MPI_Aint *, (2 * count + 1) * sizeof(MPI_Aint), mpi_errno,
                        "contents counts array", MPL_MEM_BUFFER);

    counts[0] = count;

    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + 1] = array_of_blocklengths[i];
    }
    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + count + 1] = array_of_displacements[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_INDEXED,
                                           0, 0, 2 * count + 1, 1, NULL, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
    MPI_Aint *p_blkl;
    int *ints;
    MPIR_CHKLMEM_DECL(2);

    if (sizeof(MPI_Aint) == sizeof(int)) {
        p_blkl = (MPI_Aint *) array_of_blocklengths;
    } else {
        MPIR_CHKLMEM_MALLOC_ORJUMP(p_blkl, MPI_Aint *, count * sizeof(MPI_Aint), mpi_errno,
                                   "aint blocklengths array", MPL_MEM_BUFFER);
        for (int i = 0; i < count; i++) {
            p_blkl[i] = array_of_blocklengths[i];
        }
    }
    mpi_errno = MPIR_Type_indexed(count, p_blkl, array_of_displacements, 1,     /* displacements in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC_ORJUMP(ints, int *, (count + 1) * sizeof(int), mpi_errno,
                               "content description", MPL_MEM_BUFFER);
    ints[0] = count;

    for (int i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED,
                                           count + 1, count, 0, 1,
                                           ints, array_of_displacements, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_hindexed_large_impl(MPI_Aint count,
                                         const MPI_Aint array_of_blocklengths[],
                                         const MPI_Aint array_of_displacements[],
                                         MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_indexed(count, array_of_blocklengths, array_of_displacements, 1,      /* displacements in bytes */
                                  oldtype, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC_ORJUMP(counts, MPI_Aint *, (count * 2 + 1) * sizeof(MPI_Aint), mpi_errno,
                               "content description", MPL_MEM_BUFFER);
    counts[0] = count;

    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + 1] = array_of_blocklengths[i];
    }
    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + count + 1] = array_of_displacements[i];
    }
    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_HINDEXED,
                                           0, 0, count * 2 + 1, 1, NULL, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_struct_large_impl(MPI_Aint count,
                                       const MPI_Aint * array_of_blocklengths,
                                       const MPI_Aint * array_of_displacements,
                                       const MPI_Datatype * array_of_types, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *counts;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_struct(count, array_of_blocklengths,
                                 array_of_displacements, array_of_types, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);


    MPIR_CHKLMEM_MALLOC(counts, MPI_Aint *, (count * 2 + 1) * sizeof(MPI_Aint), mpi_errno,
                        "contents counts array", MPL_MEM_BUFFER);

    counts[0] = count;
    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + 1] = array_of_blocklengths[i];
    }
    for (MPI_Aint i = 0; i < count; i++) {
        counts[i + count + 1] = array_of_displacements[i];
    }

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_STRUCT,
                                           0, 0, count * 2 + 1, count,
                                           NULL, NULL, counts, array_of_types);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_struct_impl(int count, const int *array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements,
                                 const MPI_Datatype * array_of_types, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint *p_blkl;
    int *ints;
    MPIR_CHKLMEM_DECL(2);

    if (sizeof(MPI_Aint) == sizeof(int)) {
        p_blkl = (MPI_Aint *) array_of_blocklengths;
    } else {
        MPIR_CHKLMEM_MALLOC_ORJUMP(p_blkl, MPI_Aint *, count * sizeof(MPI_Aint), mpi_errno,
                                   "aint blocklengths array", MPL_MEM_BUFFER);
        for (int i = 0; i < count; i++) {
            p_blkl[i] = array_of_blocklengths[i];
        }
    }
    mpi_errno = MPIR_Type_struct(count, p_blkl,
                                 array_of_displacements, array_of_types, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);


    MPIR_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array",
                        MPL_MEM_BUFFER);

    ints[0] = count;
    for (int i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_STRUCT,
                                           count + 1, count, 0, count,
                                           ints, array_of_displacements, NULL, array_of_types);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_dup_impl(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp, *old_dtp;

    mpi_errno = MPIR_Type_dup(oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_DUP,
                                           0, 0, 0, 1, NULL, NULL, NULL, &oldtype);

    MPIR_Datatype_get_ptr(oldtype, old_dtp);

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
     * structure to prevent type_dup from forcing the linking of the
     * attribute functions.  The actual function is (by default)
     * MPIR_Attr_dup_list
     */
    if (MPIR_Process.attr_dup) {
        new_dtp->attributes = 0;
        mpi_errno = MPIR_Process.attr_dup(oldtype, old_dtp->attributes, &new_dtp->attributes);
        if (mpi_errno) {
            MPIR_Datatype_ptr_release(new_dtp);
            goto fn_fail;
        }
    }

    /* The new type should have the same committed status as the old type */
    if (HANDLE_IS_BUILTIN(oldtype) || old_dtp->is_committed) {
        mpi_errno = MPIR_Type_commit_impl(&new_handle);
        MPIR_ERR_CHECK(mpi_errno);
    }

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
    MPI_Datatype new_handle = MPI_DATATYPE_NULL;
    MPIR_Datatype *new_dtp;
    MPI_Aint aints[2];

    mpi_errno = MPIR_Type_create_resized(oldtype, lb, extent, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    aints[0] = lb;
    aints[1] = extent;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_RESIZED,
                                           0, 2, 0, 1, NULL, aints, NULL, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_resized_large_impl(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                        MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle = MPI_DATATYPE_NULL;
    MPIR_Datatype *new_dtp;
    MPI_Aint counts[2];

    mpi_errno = MPIR_Type_create_resized(oldtype, lb, extent, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);

    counts[0] = lb;
    counts[1] = extent;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_RESIZED,
                                           0, 0, 2, 1, NULL, NULL, counts, &oldtype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* FIXME: replace this routine with MPIR_Type_contiguous_large_impl -- require yaksa large count support */
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
