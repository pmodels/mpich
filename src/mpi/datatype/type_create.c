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
 * The functions within each set follows the same ordering, i.e. contigous,
 * vector, indexed_block, indexed, struct, dup, resized.
 */

/* ---- MPIR_Xxx type creation routines ---- */

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
int MPIR_Type_vector(int count, int blocklength, MPI_Aint stride,
                     int strideinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
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
int MPIR_Type_blockindexed(int count, int blocklength, const void *displacement_array,
                           int dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
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
                                         "MPIR_Type_vector", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
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
int MPIR_Type_indexed(int count, const int *blocklength_array, const void *displacement_array,
                      int dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
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
static int type_struct(int count,
                       const int *blocklength_array,
                       const MPI_Aint * displacement_array,
                       const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

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

/* end struct */

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
        new_dtp->is_committed = old_dtp->is_committed;

        new_dtp->attributes = NULL;     /* Attributes are copied in the
                                         * top-level MPI_Type_dup routine */
        new_dtp->name[0] = 0;   /* The Object name is not copied on
                                 * a dup */

        new_dtp->typerep.handle = NULL;
        *newtype = new_dtp->handle;

        mpi_errno = MPIR_Typerep_create_dup(oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);

        /* if old_dtp is commited, user will not call `MPI_Type_commit` on the new type,
         * but the device still need be notified (e.g. ucx need register the type) */
        if (old_dtp->is_committed) {
            MPID_Type_commit_hook(new_dtp);
        }
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

int MPIR_Type_create_hvector_impl(int count, int blocklength, MPI_Aint stride,
                                  MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    int ints[2];

    mpi_errno = MPIR_Type_vector(count, blocklength, (MPI_Aint) stride, 1,      /* stride in bytes */
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

int MPIR_Type_create_struct_impl(int count, const int *array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements,
                                 const MPI_Datatype * array_of_types, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    int i, *ints;
    MPIR_Datatype *new_dtp;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_struct(count,
                                 array_of_blocklengths,
                                 array_of_displacements, array_of_types, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);


    MPIR_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array",
                        MPL_MEM_BUFFER);

    ints[0] = count;
    for (i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_STRUCT, count + 1,     /* ints (count, blocklengths) */
                                           count,       /* aints (displacements) */
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

int MPIR_Type_dup_impl(MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;

    mpi_errno = MPIR_Type_dup(oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

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

/* ---- MPIR_Xxx_x_impl larget count routines ---- */

/* FIXME: make MPIR_Type_contiguous support large type natively */
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
