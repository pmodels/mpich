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

static int datatype_attr_finalize_cb(void *dummy);

#define type_name_entry(x_, g_) { MPI_##x_, MPIR_##x_##_INTERNAL, MPIR_DT_GROUP_##g_, "MPI_" #x_ }
#define type_name_x(x_, g_) { MPIX_##x_, MPIR_##x_##_INTERNAL, MPIR_DT_GROUP_##g_, "MPIX_" #x_ }
#define type_name_null {MPI_DATATYPE_NULL, MPI_DATATYPE_NULL, MPIR_DT_GROUP_NULL, NULL}

struct MPIR_Datatype_builtin_entry MPIR_Internal_types[] = {
    /* *INDENT-OFF* */
    type_name_null,                                       /* 0x00 */
    type_name_entry(CHAR,               NULL),            /* 0x01 */
    type_name_entry(UNSIGNED_CHAR,      C_INTEGER),       /* 0x02 */
    type_name_entry(SHORT,              C_INTEGER),       /* 0x03 */
    type_name_entry(UNSIGNED_SHORT,     C_INTEGER),       /* 0x04 */
    type_name_entry(INT,                C_INTEGER),       /* 0x05 */
    type_name_entry(UNSIGNED,           C_INTEGER),       /* 0x06 */
    type_name_entry(LONG,               C_INTEGER),       /* 0x07 */
    type_name_entry(UNSIGNED_LONG,      C_INTEGER),       /* 0x08 */
    type_name_entry(LONG_LONG_INT,      C_INTEGER),       /* 0x09 */
    type_name_entry(FLOAT,              FLOATING_POINT),  /* 0x0a */
    type_name_entry(DOUBLE,             FLOATING_POINT),  /* 0x0b */
    type_name_entry(LONG_DOUBLE,        FLOATING_POINT),  /* 0x0c */
    type_name_entry(BYTE,               BYTE),            /* 0x0d */
    type_name_entry(WCHAR,              NULL),            /* 0x0e */
    type_name_entry(PACKED,             NULL),            /* 0x0f */
    type_name_entry(LB,                 NULL),            /* 0x10 */
    type_name_entry(UB,                 NULL),            /* 0x11 */
    type_name_null,                                       /* 0x12 */
    type_name_null,                                       /* 0x13 */
    type_name_null,                                       /* 0x14 */
    type_name_null,                                       /* 0x15 */
    type_name_entry(2INT,               NULL),            /* 0x16 */
    type_name_null,                                       /* 0x17 */
    type_name_entry(SIGNED_CHAR,        C_INTEGER),       /* 0x18 */
    type_name_entry(UNSIGNED_LONG_LONG, C_INTEGER),       /* 0x19 */
    type_name_entry(CHARACTER,          NULL),            /* 0x1a */
    type_name_entry(INTEGER,            FORTRAN_INTEGER), /* 0x1b */
    type_name_entry(REAL,               FLOATING_POINT),  /* 0x1c */
    type_name_entry(LOGICAL,            LOGICAL),         /* 0x1d */
    type_name_entry(COMPLEX,            COMPLEX),         /* 0x1e */
    type_name_entry(DOUBLE_PRECISION,   FLOATING_POINT),  /* 0x1f */
    type_name_entry(2INTEGER,           NULL),            /* 0x20 */
    type_name_entry(2REAL,              NULL),            /* 0x21 */
    type_name_entry(DOUBLE_COMPLEX,     COMPLEX),         /* 0x22 */
    type_name_entry(2DOUBLE_PRECISION,  NULL),            /* 0x23 */
    type_name_null,                                       /* 0x24 */
    type_name_null,                                       /* 0x25 */
    type_name_entry(REAL2,              FLOATING_POINT),  /* 0x26 */
    type_name_entry(REAL4,              FLOATING_POINT),  /* 0x27 */
    type_name_entry(COMPLEX8,           COMPLEX),         /* 0x28 */
    type_name_entry(REAL8,              FLOATING_POINT),  /* 0x29 */
    type_name_entry(COMPLEX16,          COMPLEX),         /* 0x2a */
    type_name_entry(REAL16,             FLOATING_POINT),  /* 0x2b */
    type_name_entry(COMPLEX32,          COMPLEX),         /* 0x2c */
    type_name_entry(INTEGER1,           FORTRAN_INTEGER), /* 0x2d */
    type_name_entry(COMPLEX4,           COMPLEX),         /* 0x2e */
    type_name_entry(INTEGER2,           FORTRAN_INTEGER), /* 0x2f */
    type_name_entry(INTEGER4,           FORTRAN_INTEGER), /* 0x30 */
    type_name_entry(INTEGER8,           FORTRAN_INTEGER), /* 0x31 */
    type_name_entry(INTEGER16,          FORTRAN_INTEGER), /* 0x32 */
    type_name_entry(CXX_BOOL,           LOGICAL),         /* 0x33 */
    type_name_entry(CXX_FLOAT_COMPLEX,  COMPLEX),         /* 0x34 */
    type_name_entry(CXX_DOUBLE_COMPLEX, COMPLEX),         /* 0x35 */
    type_name_entry(CXX_LONG_DOUBLE_COMPLEX, COMPLEX),    /* 0x36 */
    type_name_entry(INT8_T,             C_INTEGER),       /* 0x37 */
    type_name_entry(INT16_T,            C_INTEGER),       /* 0x38 */
    type_name_entry(INT32_T,            C_INTEGER),       /* 0x39 */
    type_name_entry(INT64_T,            C_INTEGER),       /* 0x3a */
    type_name_entry(UINT8_T,            C_INTEGER),       /* 0x3b */
    type_name_entry(UINT16_T,           C_INTEGER),       /* 0x3c */
    type_name_entry(UINT32_T,           C_INTEGER),       /* 0x3d */
    type_name_entry(UINT64_T,           C_INTEGER),       /* 0x3e */
    type_name_entry(C_BOOL,             LOGICAL),         /* 0x3f */
    type_name_entry(C_COMPLEX,          COMPLEX),         /* 0x40 */
    type_name_entry(C_DOUBLE_COMPLEX,   COMPLEX),         /* 0x41 */
    type_name_entry(C_LONG_DOUBLE_COMPLEX, COMPLEX),      /* 0x42 */
    type_name_entry(AINT,               MULTI),           /* 0x43 */
    type_name_entry(OFFSET,             MULTI),           /* 0x44 */
    type_name_entry(COUNT,              MULTI),           /* 0x45 */
    type_name_x(C_FLOAT16,              FLOATING_POINT),  /* 0x46 */
    type_name_entry(LOGICAL1,           LOGICAL),         /* 0x47 */
    type_name_entry(LOGICAL2,           LOGICAL),         /* 0x48 */
    type_name_entry(LOGICAL4,           LOGICAL),         /* 0x49 */
    type_name_entry(LOGICAL8,           LOGICAL),         /* 0x4a */
    type_name_entry(LOGICAL16,          LOGICAL),         /* 0x4b */
    type_name_x(BFLOAT16,               FLOATING_POINT),  /* 0x4c */
    /* *INDENT-ON* */
};

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
            goto fn_fail;
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

  fn_fail:
    return mpi_errno;
}

int MPIR_Datatype_builtintype_alignment(MPI_Datatype type)
{
    if (type == MPI_DATATYPE_NULL)
        return 1;

    /* mask off the index bits and MPIR_TYPE_PAIR_MASK */
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type) & ~MPIR_TYPE_PAIR_MASK) {
        case MPIR_FIXED8:
        case MPIR_INT8:
        case MPIR_UINT8:
        case MPIR_FLOAT8:
            return ALIGNOF_INT8_T;
        case MPIR_FIXED16:
        case MPIR_INT16:
        case MPIR_UINT16:
        case MPIR_FLOAT16:
        case MPIR_BFLOAT16:
            return ALIGNOF_INT16_T;
        case MPIR_FIXED32:
        case MPIR_INT32:
        case MPIR_UINT32:
            return ALIGNOF_INT32_T;
        case MPIR_FIXED64:
        case MPIR_INT64:
        case MPIR_UINT64:
            return ALIGNOF_INT64_T;
        case MPIR_FLOAT32:
        case MPIR_COMPLEX32:
            return ALIGNOF_FLOAT;
        case MPIR_FLOAT64:
        case MPIR_COMPLEX64:
            return ALIGNOF_DOUBLE;
        case MPIR_ALT_FLOAT96:
        case MPIR_ALT_FLOAT128:
        case MPIR_ALT_COMPLEX96:
        case MPIR_ALT_COMPLEX128:
            return ALIGNOF_LONG_DOUBLE;
        default:
            MPIR_Assert(0);
            return 1;
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
