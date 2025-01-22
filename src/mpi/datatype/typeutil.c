/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* This is the utility file for datatypes that contains the basic datatype
   items and storage management.  It also contains a temporary routine
   that is used by ROMIO to test to see if datatypes are contiguous */

/* Preallocated datatype objects */
MPIR_Datatype MPIR_Datatype_builtin[MPIR_DATATYPE_N_BUILTIN];
MPIR_Datatype MPIR_Datatype_direct[MPIR_DATATYPE_PREALLOC];

MPIR_Object_alloc_t MPIR_Datatype_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_DATATYPE,
    sizeof(MPIR_Datatype), MPIR_Datatype_direct,
    MPIR_DATATYPE_PREALLOC,
    NULL, {0}
};

MPI_Datatype MPIR_Datatype_index_to_predefined[MPIR_DATATYPE_N_PREDEFINED];

static int pairtypes_finalize_cb(void *dummy);
static int datatype_attr_finalize_cb(void *dummy);

#define type_name_entry(x_) { MPI_##x_, MPIR_##x_##_INTERNAL, "MPI_" #x_ }
#define type_name_x(x_) { MPIX_##x_, MPIR_##x_##_INTERNAL, "MPIX_" #x_ }
#define type_name_null {MPI_DATATYPE_NULL, MPI_DATATYPE_NULL, NULL}

struct MPIR_Datatype_builtin_entry MPIR_Internal_types[] = {
    /* *INDENT-OFF* */
    type_name_null,                           /* 0x00 */
    type_name_entry(CHAR),                    /* 0x01 */
    type_name_entry(UNSIGNED_CHAR),           /* 0x02 */
    type_name_entry(SHORT),                   /* 0x03 */
    type_name_entry(UNSIGNED_SHORT),          /* 0x04 */
    type_name_entry(INT),                     /* 0x05 */
    type_name_entry(UNSIGNED),                /* 0x06 */
    type_name_entry(LONG),                    /* 0x07 */
    type_name_entry(UNSIGNED_LONG),           /* 0x08 */
    type_name_entry(LONG_LONG_INT),           /* 0x09 */
    type_name_entry(FLOAT),                   /* 0x0a */
    type_name_entry(DOUBLE),                  /* 0x0b */
    type_name_entry(LONG_DOUBLE),             /* 0x0c */
    type_name_entry(BYTE),                    /* 0x0d */
    type_name_entry(WCHAR),                   /* 0x0e */
    type_name_entry(PACKED),                  /* 0x0f */
    type_name_entry(LB),                      /* 0x10 */
    type_name_entry(UB),                      /* 0x11 */
    type_name_null,                           /* 0x12 */
    type_name_null,                           /* 0x13 */
    type_name_null,                           /* 0x14 */
    type_name_null,                           /* 0x15 */
    type_name_entry(2INT),                    /* 0x16 */
    type_name_null,                           /* 0x17 */
    type_name_entry(SIGNED_CHAR),             /* 0x18 */
    type_name_entry(UNSIGNED_LONG_LONG),      /* 0x19 */
    type_name_entry(CHARACTER),               /* 0x1a */
    type_name_entry(INTEGER),                 /* 0x1b */
    type_name_entry(REAL),                    /* 0x1c */
    type_name_entry(LOGICAL),                 /* 0x1d */
    type_name_entry(COMPLEX),                 /* 0x1e */
    type_name_entry(DOUBLE_PRECISION),        /* 0x1f */
    type_name_entry(2INTEGER),                /* 0x20 */
    type_name_entry(2REAL),                   /* 0x21 */
    type_name_entry(DOUBLE_COMPLEX),          /* 0x22 */
    type_name_entry(2DOUBLE_PRECISION),       /* 0x23 */
    type_name_null,                           /* 0x24 */
    type_name_null,                           /* 0x25 */
    type_name_entry(REAL2),                   /* 0x26 */
    type_name_entry(REAL4),                   /* 0x27 */
    type_name_entry(COMPLEX8),                /* 0x28 */
    type_name_entry(REAL8),                   /* 0x29 */
    type_name_entry(COMPLEX16),               /* 0x2a */
    type_name_entry(REAL16),                  /* 0x2b */
    type_name_entry(COMPLEX32),               /* 0x2c */
    type_name_entry(INTEGER1),                /* 0x2d */
    type_name_entry(COMPLEX4),                /* 0x2e */
    type_name_entry(INTEGER2),                /* 0x2f */
    type_name_entry(INTEGER4),                /* 0x30 */
    type_name_entry(INTEGER8),                /* 0x31 */
    type_name_entry(INTEGER16),               /* 0x32 */
    type_name_entry(CXX_BOOL),                /* 0x33 */
    type_name_entry(CXX_FLOAT_COMPLEX),       /* 0x34 */
    type_name_entry(CXX_DOUBLE_COMPLEX),      /* 0x35 */
    type_name_entry(CXX_LONG_DOUBLE_COMPLEX), /* 0x36 */
    type_name_entry(INT8_T),                  /* 0x37 */
    type_name_entry(INT16_T),                 /* 0x38 */
    type_name_entry(INT32_T),                 /* 0x39 */
    type_name_entry(INT64_T),                 /* 0x3a */
    type_name_entry(UINT8_T),                 /* 0x3b */
    type_name_entry(UINT16_T),                /* 0x3c */
    type_name_entry(UINT32_T),                /* 0x3d */
    type_name_entry(UINT64_T),                /* 0x3e */
    type_name_entry(C_BOOL),                  /* 0x3f */
    type_name_entry(C_COMPLEX),               /* 0x40 */
    type_name_entry(C_DOUBLE_COMPLEX),        /* 0x41 */
    type_name_entry(C_LONG_DOUBLE_COMPLEX),   /* 0x42 */
    type_name_entry(AINT),                    /* 0x43 */
    type_name_entry(OFFSET),                  /* 0x44 */
    type_name_entry(COUNT),                   /* 0x45 */
    type_name_x(C_FLOAT16),                   /* 0x46 */
    /* *INDENT-ON* */
};

#if 0
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
            MPIR_Datatype_free(dptr);
            mpi_pairtypes[i].dtype = MPI_DATATYPE_NULL;
        }
    }
    return 0;
}

static void init_pairtypes(void)
{
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

        dptr = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);

        MPIR_Assert(dptr);
        MPIR_Assert(dptr->handle == mpi_pairtypes[i].dtype);
        /* this is a redundant alternative to the previous statement */
        MPIR_Assert(HANDLE_INDEX(mpi_pairtypes[i].dtype) == i);

        mpi_errno = MPIR_Type_create_pairtype(mpi_pairtypes[i].dtype, (MPIR_Datatype *) dptr);
        MPIR_ERR_CHECK(mpi_errno);
        MPL_strncpy(dptr->name, mpi_pairtypes[i].name, MPI_MAX_OBJECT_NAME);
    }
}
#endif

/* Call this routine to associate a MPIR_Datatype with each predefined
   datatype. */
int MPIR_Datatype_init_predefined(void)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned int i;
    MPIR_Datatype *dptr;
    MPI_Datatype d = MPI_DATATYPE_NULL;

    MPIR_Assert(sizeof(MPIR_Internal_types) / sizeof(MPIR_Internal_types[0]) ==
                MPIR_DATATYPE_N_BUILTIN);
    for (i = 0; i < MPIR_DATATYPE_N_BUILTIN; i++) {
        d = MPIR_Internal_types[i].dtype;
        if (d == MPI_DATATYPE_NULL)
            continue;

        MPIR_Internal_types[i].internal_type |= i;

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
        MPL_strncpy(dptr->name, MPIR_Internal_types[i].name, MPI_MAX_OBJECT_NAME);
    }

    /* Setup pairtypes. The following assertions ensure that:
     * - this function is called before other types are allocated
     * - there are enough spaces in the direct block to hold our types
     * - we actually get the values we expect (otherwise errors regarding
     *   these types could be terribly difficult to track down!)
     */
    MPIR_Assert(MPIR_Datatype_mem.initialized == 0);
    MPIR_Assert(MPIR_DATATYPE_PREALLOC >= 5);

    /* FIXME: setup pairtypes in later commits */

  fn_fail:
    return mpi_errno;
}

int MPIR_Datatype_builtintype_alignment(MPI_Datatype type)
{
    if (type == MPI_DATATYPE_NULL)
        return 1;

    int size = MPIR_Datatype_get_basic_size(type);

    if (type == MPI_CHAR || type == MPI_UNSIGNED_CHAR || type == MPI_SIGNED_CHAR) {
        return ALIGNOF_CHAR;
    } else if (type == MPI_BYTE || type == MPI_UINT8_T || type == MPI_INT8_T ||
               type == MPI_PACKED || type == MPI_LB || type == MPI_UB) {
        return ALIGNOF_INT8_T;
    } else if (type == MPI_WCHAR) {
        return ALIGNOF_WCHAR_T;
    } else if (type == MPI_SHORT || type == MPI_UNSIGNED_SHORT) {
        return ALIGNOF_SHORT;
    } else if (type == MPI_INT || type == MPI_UNSIGNED || type == MPI_2INT) {
        return ALIGNOF_INT;
    } else if (type == MPI_LONG || type == MPI_UNSIGNED_LONG) {
        return ALIGNOF_LONG;
    } else if (type == MPI_FLOAT || type == MPI_C_COMPLEX) {
        return ALIGNOF_FLOAT;
    } else if (type == MPI_DOUBLE || type == MPI_C_DOUBLE_COMPLEX) {
        return ALIGNOF_DOUBLE;
    } else if (type == MPI_LONG_DOUBLE || type == MPI_C_LONG_DOUBLE_COMPLEX) {
        return ALIGNOF_LONG_DOUBLE;
    } else if (type == MPI_LONG_LONG_INT || type == MPI_UNSIGNED_LONG_LONG) {
        return ALIGNOF_LONG_LONG;
    } else if (type == MPI_INT16_T || type == MPI_UINT16_T) {
        return ALIGNOF_INT16_T;
    } else if (type == MPI_INT32_T || type == MPI_UINT32_T) {
        return ALIGNOF_INT32_T;
    } else if (type == MPI_INT64_T || type == MPI_UINT64_T) {
        return ALIGNOF_INT64_T;
    } else if (type == MPI_C_BOOL) {
        return ALIGNOF_BOOL;
    } else if (type == MPI_AINT || type == MPI_OFFSET || type == MPI_COUNT) {
        if (size == sizeof(int8_t))
            return ALIGNOF_INT8_T;
        else if (size == sizeof(int16_t))
            return ALIGNOF_INT16_T;
        else if (size == sizeof(int32_t))
            return ALIGNOF_INT32_T;
        else if (size == sizeof(int64_t))
            return ALIGNOF_INT64_T;
#ifdef HAVE_FORTRAN_BINDING
    } else if (type == MPI_CHARACTER) {
        return ALIGNOF_CHAR;
    } else if (type == MPI_LOGICAL || type == MPI_INTEGER || type == MPI_2INTEGER ||
               type == MPI_INTEGER1 || type == MPI_INTEGER2 || type == MPI_INTEGER4 ||
               type == MPI_INTEGER8 || type == MPI_INTEGER16) {
        if (size == sizeof(int8_t))
            return ALIGNOF_INT8_T;
        else if (size == sizeof(int16_t))
            return ALIGNOF_INT16_T;
        else if (size == sizeof(int32_t))
            return ALIGNOF_INT32_T;
        else if (size == sizeof(int64_t))
            return ALIGNOF_INT64_T;
    } else if (type == MPI_COMPLEX || type == MPI_DOUBLE_COMPLEX || type == MPI_REAL ||
               type == MPI_DOUBLE_PRECISION || type == MPI_2REAL || type == MPI_2DOUBLE_PRECISION ||
               type == MPI_REAL4 || type == MPI_REAL8 || type == MPI_REAL16) {
        if (size == sizeof(float))
            return ALIGNOF_FLOAT;
        else if (size == sizeof(double))
            return ALIGNOF_DOUBLE;
        else if (size == sizeof(long double))
            return ALIGNOF_LONG_DOUBLE;
    } else if (type == MPI_COMPLEX8 || type == MPI_COMPLEX16 || type == MPI_COMPLEX32) {
        if (size / 2 == sizeof(float))
            return ALIGNOF_FLOAT;
        else if (size / 2 == sizeof(double))
            return ALIGNOF_DOUBLE;
        else if (size / 2 == sizeof(long double))
            return ALIGNOF_LONG_DOUBLE;
#endif /* HAVE_FORTRAN_BINDING */

    } else if (type == MPI_CXX_BOOL) {
        return ALIGNOF_BOOL;
    } else if (type == MPI_CXX_FLOAT_COMPLEX) {
        return ALIGNOF_FLOAT;
    } else if (type == MPI_CXX_DOUBLE_COMPLEX) {
        return ALIGNOF_DOUBLE;
    } else if (type == MPI_CXX_LONG_DOUBLE_COMPLEX) {
        return ALIGNOF_LONG_DOUBLE;
    }

    return 1;
}

int MPIR_Datatype_commit_pairtypes(void)
{
    /* commit pairtypes */
    for (int i = 0; i < sizeof(mpi_pairtypes) / sizeof(mpi_pairtypes[0]); i++) {
        if (mpi_pairtypes[i].dtype != MPI_DATATYPE_NULL) {
            int err;

            err = MPIR_Type_commit_impl(&mpi_pairtypes[i].dtype);

            /* --BEGIN ERROR HANDLING-- */
            if (err) {
                return MPIR_Err_create_code(MPI_SUCCESS,
                                            MPIR_ERR_RECOVERABLE,
                                            __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            }
            /* --END ERROR HANDLING-- */
        }
    }

    return MPI_SUCCESS;
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

    new_dtp->typerep.handle = MPIR_TYPEREP_HANDLE_NULL;

    new_dtp->size = 0;
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
                                            const MPI_Aint * displacement_array,
                                            int dispinbytes, MPI_Aint old_extent)
{
    MPI_Aint i, contig_count = 1;
    MPI_Aint cur_blklen, first;

    if (count) {
        /* Skip any initial zero-length blocks */
        for (first = 0; first < count; ++first)
            if (blocklength_array[first])
                break;

        if (first == count) {   /* avoid invalid reads later on */
            contig_count = 0;
            return contig_count;
        }

        cur_blklen = blocklength_array[first];
        if (!dispinbytes) {
            MPI_Aint cur_tdisp = displacement_array[first];

            for (i = first + 1; i < count; ++i) {
                if (blocklength_array[i] == 0) {
                    continue;
                } else if (cur_tdisp + cur_blklen == displacement_array[i]) {
                    /* adjacent to current block; add to block */
                    cur_blklen += blocklength_array[i];
                } else {
                    cur_tdisp = displacement_array[i];
                    cur_blklen = blocklength_array[i];
                    contig_count++;
                }
            }
        } else {
            MPI_Aint cur_bdisp = displacement_array[first];

            for (i = first + 1; i < count; ++i) {
                if (blocklength_array[i] == 0) {
                    continue;
                } else if (cur_bdisp + cur_blklen * old_extent == displacement_array[i]) {
                    /* adjacent to current block; add to block */
                    cur_blklen += blocklength_array[i];
                } else {
                    cur_bdisp = displacement_array[i];
                    cur_blklen = blocklength_array[i];
                    contig_count++;
                }
            }
        }
    }
    return contig_count;
}

MPI_Aint MPII_Datatype_blockindexed_count_contig(MPI_Aint count,
                                                 MPI_Aint blklen,
                                                 const MPI_Aint disp_array[],
                                                 int dispinbytes, MPI_Aint old_extent)
{
    int i, contig_count = 1;

    if (!dispinbytes) {
        MPI_Aint cur_tdisp = disp_array[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_tdisp = disp_array[i];

            if (cur_tdisp + blklen != next_tdisp) {
                contig_count++;
            }
            cur_tdisp = next_tdisp;
        }
    } else {
        MPI_Aint cur_bdisp = disp_array[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_bdisp = disp_array[i];

            if (cur_bdisp + blklen * old_extent != next_bdisp) {
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
    if (ptr->typerep.handle) {
        MPIR_Typerep_free(ptr);
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
