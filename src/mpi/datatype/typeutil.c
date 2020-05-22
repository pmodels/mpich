/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

/* This is the utility file for datatypes that contains the basic datatype
   items and storage management.  It also contains a temporary routine
   that is used by ROMIO to test to see if datatypes are contiguous */

/* Preallocated datatype objects */
MPIR_Datatype MPIR_Datatype_builtin[MPIR_DATATYPE_N_BUILTIN];
MPIR_Datatype MPIR_Datatype_direct[MPIR_DATATYPE_PREALLOC];

MPIR_Object_alloc_t MPIR_Datatype_mem = { 0, 0, 0, 0, MPIR_DATATYPE,
    sizeof(MPIR_Datatype), MPIR_Datatype_direct,
    MPIR_DATATYPE_PREALLOC
};

static int pairtypes_finalize_cb(void *dummy);
static int datatype_attr_finalize_cb(void *dummy);

typedef struct mpi_names_t {
    MPI_Datatype dtype;
    const char *name;
} mpi_names_t;
#define type_name_entry(x_) { x_, #x_ }

static mpi_names_t mpi_dtypes[] = {
    type_name_entry(MPI_CHAR),
    type_name_entry(MPI_UNSIGNED_CHAR),
    type_name_entry(MPI_SIGNED_CHAR),
    type_name_entry(MPI_BYTE),
    type_name_entry(MPI_WCHAR),
    type_name_entry(MPI_SHORT),
    type_name_entry(MPI_UNSIGNED_SHORT),
    type_name_entry(MPI_INT),
    type_name_entry(MPI_UNSIGNED),
    type_name_entry(MPI_LONG),
    type_name_entry(MPI_UNSIGNED_LONG),
    type_name_entry(MPI_FLOAT),
    type_name_entry(MPI_DOUBLE),
    type_name_entry(MPI_LONG_DOUBLE),
    type_name_entry(MPI_LONG_LONG_INT),
    type_name_entry(MPI_UNSIGNED_LONG_LONG),
    type_name_entry(MPI_PACKED),
    type_name_entry(MPI_LB),
    type_name_entry(MPI_UB),
    type_name_entry(MPI_2INT),

    /* C99 types */
    type_name_entry(MPI_INT8_T),
    type_name_entry(MPI_INT16_T),
    type_name_entry(MPI_INT32_T),
    type_name_entry(MPI_INT64_T),
    type_name_entry(MPI_UINT8_T),
    type_name_entry(MPI_UINT16_T),
    type_name_entry(MPI_UINT32_T),
    type_name_entry(MPI_UINT64_T),
    type_name_entry(MPI_C_BOOL),
    type_name_entry(MPI_C_COMPLEX),
    type_name_entry(MPI_C_DOUBLE_COMPLEX),
    type_name_entry(MPI_C_LONG_DOUBLE_COMPLEX),

    /* address/offset/count types */
    type_name_entry(MPI_AINT),
    type_name_entry(MPI_OFFSET),
    type_name_entry(MPI_COUNT),

    /* Fortran types */
    type_name_entry(MPI_COMPLEX),
    type_name_entry(MPI_DOUBLE_COMPLEX),
    type_name_entry(MPI_LOGICAL),
    type_name_entry(MPI_REAL),
    type_name_entry(MPI_DOUBLE_PRECISION),
    type_name_entry(MPI_INTEGER),
    type_name_entry(MPI_2INTEGER),
    type_name_entry(MPI_2REAL),
    type_name_entry(MPI_2DOUBLE_PRECISION),
    type_name_entry(MPI_CHARACTER),
    /* Size-specific types; these are in section 10.2.4 (Extended Fortran
     * Support) as well as optional in MPI-1
     */
    type_name_entry(MPI_REAL4),
    type_name_entry(MPI_REAL8),
    type_name_entry(MPI_REAL16),
    type_name_entry(MPI_COMPLEX8),
    type_name_entry(MPI_COMPLEX16),
    type_name_entry(MPI_COMPLEX32),
    type_name_entry(MPI_INTEGER1),
    type_name_entry(MPI_INTEGER2),
    type_name_entry(MPI_INTEGER4),
    type_name_entry(MPI_INTEGER8),
    type_name_entry(MPI_INTEGER16),

    /* C++ types */
    type_name_entry(MPI_CXX_BOOL),
    type_name_entry(MPI_CXX_FLOAT_COMPLEX),
    type_name_entry(MPI_CXX_DOUBLE_COMPLEX),
    type_name_entry(MPI_CXX_LONG_DOUBLE_COMPLEX),
};

static mpi_names_t mpi_pairtypes[] = {
    type_name_entry(MPI_FLOAT_INT),
    type_name_entry(MPI_DOUBLE_INT),
    type_name_entry(MPI_LONG_INT),
    type_name_entry(MPI_SHORT_INT),
    type_name_entry(MPI_LONG_DOUBLE_INT),
};

static int pairtypes_finalize_cb(void *dummy ATTRIBUTE((unused)))
{
    int i;
    MPIR_Datatype *dptr;

    for (i = 0; i < sizeof(mpi_pairtypes) / sizeof(mpi_pairtypes[0]); i++) {
        if (mpi_pairtypes[i].dtype != MPI_DATATYPE_NULL) {
            MPIR_Datatype_get_ptr(mpi_pairtypes[i].dtype, dptr);
            MPIR_Datatype_ptr_release(dptr);
            mpi_pairtypes[i].dtype = MPI_DATATYPE_NULL;
        }
    }
    return 0;
}

/* Call this routine to associate a MPIR_Datatype with each predefined
   datatype. */
int MPIR_Datatype_init_predefined(void)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned int i;
    MPIR_Datatype *dptr;
    MPI_Datatype d = MPI_DATATYPE_NULL;

    for (i = 0; i < sizeof(mpi_dtypes) / sizeof(mpi_dtypes[0]); i++) {
        /* Compute the index from the value of the handle */
        d = mpi_dtypes[i].dtype;

        /* Some of the size-specific types may be null, as might be types
         * based on 'long long' and 'long double' if those types were
         * disabled at configure time.  skip those cases. */
        if (d == MPI_DATATYPE_NULL)
            continue;

        MPIR_Datatype_get_ptr(d, dptr);
        /* --BEGIN ERROR HANDLING-- */
        if (dptr < MPIR_Datatype_builtin || dptr > MPIR_Datatype_builtin + MPIR_DATATYPE_N_BUILTIN) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                             MPIR_ERR_FATAL, __func__,
                                             __LINE__, MPI_ERR_INTERN,
                                             "**typeinitbadmem", "**typeinitbadmem %d", i);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */

        /* dptr will point into MPIR_Datatype_builtin */
        dptr->handle = d;
        dptr->is_contig = 1;
        MPIR_Object_set_ref(dptr, 1);
        dptr->size = MPIR_Datatype_get_basic_size(d);
        dptr->extent = dptr->size;
        dptr->ub = dptr->size;
        dptr->true_ub = dptr->size;
        dptr->contents = NULL;  /* should never get referenced? */
        MPL_strncpy(dptr->name, mpi_dtypes[i].name, MPI_MAX_OBJECT_NAME);
    }

    /* Setup pairtypes. The following assertions ensure that:
     * - this function is called before other types are allocated
     * - there are enough spaces in the direct block to hold our types
     * - we actually get the values we expect (otherwise errors regarding
     *   these types could be terribly difficult to track down!)
     */
    MPIR_Assert(MPIR_Datatype_mem.initialized == 0);
    MPIR_Assert(MPIR_DATATYPE_PREALLOC >= 5);

    for (i = 0; i < sizeof(mpi_pairtypes) / sizeof(mpi_pairtypes[0]); ++i) {
        /* types based on 'long long' and 'long double', may be disabled at
         * configure time, and their values set to MPI_DATATYPE_NULL.  skip any
         * such types. */
        if (mpi_pairtypes[i].dtype == MPI_DATATYPE_NULL)
            continue;
        /* XXX: this allocation strategy isn't right if one or more of the
         * pairtypes is MPI_DATATYPE_NULL.  in fact, the assert below will
         * fail if any type other than the las in the list is equal to
         * MPI_DATATYPE_NULL.  obviously, this should be fixed, but I need
         * to talk to Rob R. first. -- BRT */
        /* XXX DJG it does work, but only because MPI_LONG_DOUBLE_INT is the
         * only one that is ever optional and it comes last */

        /* we use the _unsafe version because we are still in MPI_Init, before
         * multiple threads are permitted and possibly before support for
         * critical sections is entirely setup */
        dptr = (MPIR_Datatype *) MPIR_Handle_obj_alloc_unsafe(&MPIR_Datatype_mem);

        MPIR_Assert(dptr);
        MPIR_Assert(dptr->handle == mpi_pairtypes[i].dtype);
        /* this is a redundant alternative to the previous statement */
        MPIR_Assert(HANDLE_INDEX(mpi_pairtypes[i].dtype) == i);

        mpi_errno = MPIR_Type_create_pairtype(mpi_pairtypes[i].dtype, (MPIR_Datatype *) dptr);
        MPIR_ERR_CHECK(mpi_errno);
        MPL_strncpy(dptr->name, mpi_pairtypes[i].name, MPI_MAX_OBJECT_NAME);
    }

    MPIR_Add_finalize(pairtypes_finalize_cb, 0, MPIR_FINALIZE_CALLBACK_PRIO - 1);

  fn_fail:
    return mpi_errno;
}

int MPIR_Datatype_commit_pairtypes(void)
{
    /* commit pairtypes */
    for (int i = 0; i < sizeof(mpi_pairtypes) / sizeof(mpi_pairtypes[0]); i++) {
        if (mpi_pairtypes[i].dtype != MPI_DATATYPE_NULL) {
            int err;

            err = MPIR_Type_commit(&mpi_pairtypes[i].dtype);

            /* --BEGIN ERROR HANDLING-- */
            if (err) {
                return MPIR_Err_create_code(MPI_SUCCESS,
                                            MPIR_ERR_RECOVERABLE,
                                            "MPIR_Type_commit",
                                            __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            }
            /* --END ERROR HANDLING-- */
        }
    }

    return MPI_SUCCESS;
}

/* This will eventually be removed once ROMIO knows more about MPICH */
void MPIR_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    if (HANDLE_IS_BUILTIN(datatype))
        *flag = 1;
    else {
        MPIR_Datatype_is_contig(datatype, flag);
    }
}

/* If an attribute is added to a predefined type, we free the attributes
   in Finalize */
static int datatype_attr_finalize_cb(void *dummy ATTRIBUTE((unused)))
{
    MPIR_Datatype *dtype;
    int i, mpi_errno = MPI_SUCCESS;

    for (i = 0; i < MPIR_DATATYPE_N_BUILTIN; i++) {
        dtype = &MPIR_Datatype_builtin[i];
        if (dtype && MPIR_Process.attr_free && dtype->attributes) {
            mpi_errno = MPIR_Process.attr_free(dtype->handle, &dtype->attributes);
            /* During finalize, we ignore error returns from the free */
        }
    }
    return mpi_errno;
}

void MPII_Datatype_attr_finalize(void)
{
    static int called = 0;

    /* FIXME: This needs to be make thread safe */
    if (!called) {
        called = 1;
        MPIR_Add_finalize(datatype_attr_finalize_cb, 0, MPIR_FINALIZE_CALLBACK_PRIO - 1);
    }
}

/*@
  MPII_Type_zerolen - create an empty datatype

Input Parameters:
. none

Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/

int MPII_Type_zerolen(MPI_Datatype * newtype)
{
    int mpi_errno;
    MPIR_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPII_Type_zerolen",
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

    new_dtp->typerep = NULL;

    new_dtp->size = 0;
    new_dtp->has_sticky_ub = 0;
    new_dtp->has_sticky_lb = 0;
    new_dtp->lb = 0;
    new_dtp->ub = 0;
    new_dtp->true_lb = 0;
    new_dtp->true_ub = 0;
    new_dtp->extent = 0;

    new_dtp->alignsize = 0;
    new_dtp->builtin_element_size = 0;
    new_dtp->basic_type = 0;
    new_dtp->n_builtin_elements = 0;
    new_dtp->is_contig = 1;

    *newtype = new_dtp->handle;
    return MPI_SUCCESS;
}

void MPII_Datatype_get_contents_ints(MPIR_Datatype_contents * cp, int *user_ints)
{
    char *ptr;
    int align_sz, epsilon;
    int struct_sz, types_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
#else
    align_sz = 8;
#endif

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz = cp->nr_types * sizeof(MPI_Datatype);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
        struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
        types_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz + types_sz;
    MPIR_Memcpy(user_ints, ptr, cp->nr_ints * sizeof(int));

    return;
}

void MPII_Datatype_get_contents_aints(MPIR_Datatype_contents * cp, MPI_Aint * user_aints)
{
    char *ptr;
    int align_sz = 8, epsilon;
    int struct_sz, ints_sz, types_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
#else
    align_sz = 8;
#endif

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz = cp->nr_types * sizeof(MPI_Datatype);
    ints_sz = cp->nr_ints * sizeof(int);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
        struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
        types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
        ints_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz + types_sz + ints_sz;
    MPIR_Memcpy(user_aints, ptr, cp->nr_aints * sizeof(MPI_Aint));

    return;
}

void MPII_Datatype_get_contents_types(MPIR_Datatype_contents * cp, MPI_Datatype * user_types)
{
    char *ptr;
    int align_sz = 8, epsilon;
    int struct_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
#else
    align_sz = 8;
#endif

    struct_sz = sizeof(MPIR_Datatype_contents);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
        struct_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz;
    MPIR_Memcpy(user_types, ptr, cp->nr_types * sizeof(MPI_Datatype));

    return;
}

/* MPII_Datatype_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * Extent passed in is for the original type.
 */
MPI_Aint MPII_Datatype_indexed_count_contig(MPI_Aint count,
                                            const MPI_Aint * blocklength_array,
                                            const void *displacement_array,
                                            int dispinbytes, MPI_Aint old_extent)
{
    MPI_Aint i, contig_count = 1;
    MPI_Aint cur_blklen, first;

    if (count) {
        /* Skip any initial zero-length blocks */
        for (first = 0; first < count; ++first)
            if ((MPI_Aint) blocklength_array[first])
                break;

        if (first == count) {   /* avoid invalid reads later on */
            contig_count = 0;
            return contig_count;
        }

        cur_blklen = (MPI_Aint) blocklength_array[first];
        if (!dispinbytes) {
            MPI_Aint cur_tdisp = (MPI_Aint) ((int *) displacement_array)[first];

            for (i = first + 1; i < count; ++i) {
                if (blocklength_array[i] == 0) {
                    continue;
                } else if (cur_tdisp + (MPI_Aint) cur_blklen ==
                           (MPI_Aint) ((int *) displacement_array)[i]) {
                    /* adjacent to current block; add to block */
                    cur_blklen += (MPI_Aint) blocklength_array[i];
                } else {
                    cur_tdisp = (MPI_Aint) ((int *) displacement_array)[i];
                    cur_blklen = (MPI_Aint) blocklength_array[i];
                    contig_count++;
                }
            }
        } else {
            MPI_Aint cur_bdisp = (MPI_Aint) ((MPI_Aint *) displacement_array)[first];

            for (i = first + 1; i < count; ++i) {
                if (blocklength_array[i] == 0) {
                    continue;
                } else if (cur_bdisp + (MPI_Aint) cur_blklen * old_extent ==
                           (MPI_Aint) ((MPI_Aint *) displacement_array)[i]) {
                    /* adjacent to current block; add to block */
                    cur_blklen += (MPI_Aint) blocklength_array[i];
                } else {
                    cur_bdisp = (MPI_Aint) ((MPI_Aint *) displacement_array)[i];
                    cur_blklen = (MPI_Aint) blocklength_array[i];
                    contig_count++;
                }
            }
        }
    }
    return contig_count;
}

MPI_Aint MPII_Datatype_blockindexed_count_contig(MPI_Aint count,
                                                 MPI_Aint blklen,
                                                 const void *disp_array,
                                                 int dispinbytes, MPI_Aint old_extent)
{
    int i, contig_count = 1;

    if (!dispinbytes) {
        /* this is from the MPI type, is of type int */
        MPI_Aint cur_tdisp = (MPI_Aint) ((int *) disp_array)[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_tdisp = (MPI_Aint) ((int *) disp_array)[i];

            if (cur_tdisp + (MPI_Aint) blklen != next_tdisp) {
                contig_count++;
            }
            cur_tdisp = next_tdisp;
        }
    } else {
        /* this is from the MPI type, is of type MPI_Aint */
        MPI_Aint cur_bdisp = (MPI_Aint) ((MPI_Aint *) disp_array)[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_bdisp = (MPI_Aint) ((MPI_Aint *) disp_array)[i];

            if (cur_bdisp + (MPI_Aint) blklen * old_extent != next_bdisp) {
                contig_count++;
            }
            cur_bdisp = next_bdisp;
        }
    }
    return contig_count;
}

/*@
  MPIR_Datatype_free

Input Parameters:
. MPIR_Datatype ptr - pointer to MPID datatype structure that is no longer
  referenced

Output Parameters:
  none

  Return Value:
  none

  This function handles freeing dynamically allocated memory associated with
  the datatype.  In the process MPIR_Datatype_free_contents() is also called,
  which handles decrementing reference counts to constituent types (in
  addition to freeing the space used for contents information).
  MPIR_Datatype_free_contents() will call MPIR_Datatype_free() on constituent
  types that are no longer referenced as well.

  @*/
void MPIR_Datatype_free(MPIR_Datatype * ptr)
{
    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE, VERBOSE, "type %x freed.", ptr->handle);

    MPID_Type_free_hook(ptr);

    /* before freeing the contents, check whether the pointer is not
     * null because it is null in the case of a datatype shipped to the target
     * for RMA ops */
    if (ptr->contents) {
        MPIR_Datatype_free_contents(ptr);
    }
    if (ptr->typerep) {
        MPIR_Typerep_free(&(ptr->typerep));
    }
    MPL_free(ptr->flattened);
    MPIR_Handle_obj_free(&MPIR_Datatype_mem, ptr);
}

void MPIR_Datatype_get_flattened(MPI_Datatype type, void **flattened, int *flattened_sz)
{
    MPIR_Datatype *dt_ptr;

    MPIR_Datatype_get_ptr(type, dt_ptr);
    if (dt_ptr->flattened == NULL) {
        MPIR_Typerep_flatten_size(dt_ptr, &dt_ptr->flattened_sz);
        dt_ptr->flattened = MPL_malloc(dt_ptr->flattened_sz, MPL_MEM_DATATYPE);
        MPIR_Assert(dt_ptr->flattened);
        MPIR_Typerep_flatten(dt_ptr, dt_ptr->flattened);
    }

    *flattened = dt_ptr->flattened;
    *flattened_sz = dt_ptr->flattened_sz;
}
