/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_DATATYPE_H_INCLUDED
#define MPIR_DATATYPE_H_INCLUDED

/* This value should be set to greatest value used as the type index suffix in
 * the predefined handles.  That is, look at the last two hex digits of all
 * predefined datatype handles, take the greatest one, and convert it to decimal
 * here. */
/* FIXME: I will fix this by refactor the current datatype code out-of configure.ac */
#define MPIR_DATATYPE_PAIRTYPE 5
#define MPIR_DATATYPE_N_PREDEFINED (MPIR_DATATYPE_N_BUILTIN + MPIR_DATATYPE_PAIRTYPE)

/* For internal communication and reduction, many of the builtin-datatypes are
 * equivalent. Thus, we can make equivalent swap so internally we only need
 * support a finite essential types */

/* Internal datatypes */
/* The handle bit pattern: 0xAABBCCDD, where -
 *   AA - 0x4c, works with HANDLE_IS_BUILTIN()
 *   BB - type, one of MPIR_TYPE_{FIXED,SIGNED,UNSIGNED,FLOAT,COMPLEX,FLOAT_ALT} | [PAIRTYPE MASK]
 *   CC - type size, works with MPIR_Datatype_get_basic_size()
 *   DD - the index of the builtin type that the internal type is swapped from; 0x00 otherwise.
 */

#define MPIR_TYPE_INTERNAL_MASK  0x800000       /* all internal types have this bit on */
#define MPIR_TYPE_PAIR_MASK      0x400000       /* builtin pairtypes such as MPIR_2INT32 */
#define MPIR_TYPE_TYPE_MASK      0x0f0000       /* e.g. FIXED, SIGNED, UNSIGNED, FLOAT, COMPLEX, etc. */
#define MPIR_TYPE_INDEX_MASK     0x0000ff

/* Internally we support FIXED, SIGNED, UNSIGNED, FLOAT, and COMPLEX.
 * Other types can be added when supported, e.g. bfloat16 or logicals that
 * doesn't follow the C convention of zero/nonzero.
 */
#define MPIR_TYPE_FIXED           0x000000      /* communication-only, op not supported */
#define MPIR_TYPE_SIGNED          0x010000      /* including C boolean */
#define MPIR_TYPE_UNSIGNED        0x020000
#define MPIR_TYPE_FLOAT           0x030000      /* IEEE754 floating point */
#define MPIR_TYPE_COMPLEX         0x040000      /* 2 C native floating point */
#define MPIR_TYPE_ALT_FLOAT       0x050000      /* e.g. 2-byte Bfloat, 16-byte C long double */
#define MPIR_TYPE_ALT_COMPLEX     0x060000      /* e.g. 2-byte Bfloat, 16-byte C long double */
#define MPIR_TYPE_FORTRAN_LOGICAL 0x070000      /* Fortran logical may need conversions at op */

#define MPIR_FIXED0             ((MPI_Datatype)0x4c800000)      /* 0-size, internally equivalent to MPI_DATATYPE_NULL */
#define MPIR_FIXED8             ((MPI_Datatype)0x4c800100)
#define MPIR_FIXED16            ((MPI_Datatype)0x4c800200)
#define MPIR_FIXED32            ((MPI_Datatype)0x4c800400)
#define MPIR_FIXED64            ((MPI_Datatype)0x4c800800)
#define MPIR_FIXED128           ((MPI_Datatype)0x4c801000)
#define MPIR_INT8               ((MPI_Datatype)0x4c810100)
#define MPIR_INT16              ((MPI_Datatype)0x4c810200)
#define MPIR_INT32              ((MPI_Datatype)0x4c810400)
#define MPIR_INT64              ((MPI_Datatype)0x4c810800)
#define MPIR_INT128             ((MPI_Datatype)0x4c811000)
#define MPIR_UINT8              ((MPI_Datatype)0x4c820100)
#define MPIR_UINT16             ((MPI_Datatype)0x4c820200)
#define MPIR_UINT32             ((MPI_Datatype)0x4c820400)
#define MPIR_UINT64             ((MPI_Datatype)0x4c820800)
#define MPIR_UINT128            ((MPI_Datatype)0x4c821000)
#define MPIR_FLOAT8             ((MPI_Datatype)0x4c830100)
#define MPIR_FLOAT16            ((MPI_Datatype)0x4c830200)
#define MPIR_FLOAT32            ((MPI_Datatype)0x4c830400)
#define MPIR_FLOAT64            ((MPI_Datatype)0x4c830800)
#define MPIR_FLOAT128           ((MPI_Datatype)0x4c831000)
#define MPIR_COMPLEX8           ((MPI_Datatype)0x4c840200)
#define MPIR_COMPLEX16          ((MPI_Datatype)0x4c840400)
#define MPIR_COMPLEX32          ((MPI_Datatype)0x4c840800)
#define MPIR_COMPLEX64          ((MPI_Datatype)0x4c841000)
#define MPIR_COMPLEX128         ((MPI_Datatype)0x4c842000)
#define MPIR_ALT_FLOAT96        ((MPI_Datatype)0x4c850c00)      /* long double (80-bit extended precision) on i386 */
#define MPIR_ALT_FLOAT128       ((MPI_Datatype)0x4c851000)      /* long double (80-bit extended precision) on x86-64 */
#define MPIR_ALT_COMPLEX96      ((MPI_Datatype)0x4c861800)      /* long double complex on i386 */
#define MPIR_ALT_COMPLEX128     ((MPI_Datatype)0x4c862000)      /* long double complex on x86-64 */
#define MPIR_FORTRAN_LOGICAL8   ((MPI_Datatype)0x4c870100)
#define MPIR_FORTRAN_LOGICAL16  ((MPI_Datatype)0x4c870200)
#define MPIR_FORTRAN_LOGICAL32  ((MPI_Datatype)0x4c870400)
#define MPIR_FORTRAN_LOGICAL64  ((MPI_Datatype)0x4c870800)
#define MPIR_FORTRAN_LOGICAL128 ((MPI_Datatype)0x4c871000)

/* Pair types support communication and MPI_MINLOC and MPI_MAXLOC ops. The value type
 * is SIGNED, UNSIGNED, FLOAT. The index type may be a SIGNED integer that is different
 * from the value type (internally a struct type) or * the same as the value type (builtin
 * pairtype).
 *
 * Struct pairtypes are created at init with handles like 0x800000[idx].
 * They are listed in global mpi_pairtypes[idx].
 *
 * Builtin pairtypes internally use the same handle pattern a mask bit MPIR_TYPE_PAIR_MASK
 *
 * Examples:
 *     0x8c000000 - MPI_FLOAT_INT, and mpi_pairtypes[0].value_type = 0x4c000483
 *     0x8c000003 - MPI_SHORT_INT, and mpi_pairtypes[3].value_type = 0x4c000281
 *
 *     0x4cc10800 - MPI_2INT, MPI_2INTEGER
 *     0x4cc30800 - MPI_2REAL
 */

/* builtin pair types - MPI_2INT, MPI_2INTEGER, etc. They have the MPIR_TYPE_PAIR_MASK on.
 * NOTE: value index types such as MPI_FLOAT_INT internally are not builtin */
#define MPIR_2INT8              ((MPI_Datatype)0x4cc10200)
#define MPIR_2INT16             ((MPI_Datatype)0x4cc10400)
#define MPIR_2INT32             ((MPI_Datatype)0x4cc10800)
#define MPIR_2INT64             ((MPI_Datatype)0x4cc11000)
#define MPIR_2INT128            ((MPI_Datatype)0x4cc12000)
#define MPIR_2UINT8             ((MPI_Datatype)0x4cc20200)
#define MPIR_2UINT16            ((MPI_Datatype)0x4cc20400)
#define MPIR_2UINT32            ((MPI_Datatype)0x4cc20800)
#define MPIR_2UINT64            ((MPI_Datatype)0x4cc21000)
#define MPIR_2UINT128           ((MPI_Datatype)0x4cc22000)
#define MPIR_2FLOAT8            ((MPI_Datatype)0x4cc30200)
#define MPIR_2FLOAT16           ((MPI_Datatype)0x4cc30400)
#define MPIR_2FLOAT32           ((MPI_Datatype)0x4cc30800)
#define MPIR_2FLOAT64           ((MPI_Datatype)0x4cc31000)
#define MPIR_2FLOAT128          ((MPI_Datatype)0x4cc32000)

/* Define following to simplify configure logic */
#define MPIR_INT0               MPI_DATATYPE_NULL
#define MPIR_UINT0              MPI_DATATYPE_NULL
#define MPIR_FLOAT0             MPI_DATATYPE_NULL
#define MPIR_COMPLEX0           MPI_DATATYPE_NULL
#define MPIR_ALT_FLOAT0         MPI_DATATYPE_NULL
#define MPIR_FORTRAN_LOGICAL0   MPI_DATATYPE_NULL
#define MPIR_2INT0              MPI_DATATYPE_NULL
#define MPIR_2UINT0             MPI_DATATYPE_NULL
#define MPIR_2FLOAT0            MPI_DATATYPE_NULL

struct MPIR_Datatype_builtin_entry {
    MPI_Datatype dtype;
    MPI_Datatype internal_type;
    const char *name;
};
extern struct MPIR_Datatype_builtin_entry MPIR_Internal_types[];

/* Swap input builtin datatype with an equivalent internal type */
#define MPIR_DATATYPE_REPLACE_BUILTIN(type) \
    do { \
        if (HANDLE_IS_BUILTIN(type) && ((type) & 0xff)) { \
            (type) = MPIR_Internal_types[(type) & 0xff].internal_type; \
        } \
    } while (0)

/* The original (builtin) type is needed for calling user op or reporting errors.
 * Note this works even if type has not been swapped since we just access from its index */
#define MPIR_DATATYPE_GET_ORIG_BUILTIN(type)  MPIR_Internal_types[(type) & 0xff].dtype

/* The "raw" internal type have index bits set to 0 */
#define MPIR_DATATYPE_GET_RAW_INTERNAL(type)  ((type) & 0xffffff00)

/* Assertion that internally we should not see external builtin types */
#define MPIR_DATATYPE_ASSERT_BUILTIN(type)  MPIR_Assert(((type) & 0xffff0000) != 0x4c000000)

struct MPIR_pairtype {
    MPI_Datatype value_type;
    /* index_type is always MPI_INT */
};
extern struct MPIR_pairtype MPIR_pairtypes[];
extern int MPIR_num_pairtypes;

MPL_STATIC_INLINE_PREFIX bool MPIR_Datatype_is_pairtype(MPI_Datatype datatype)
{
    if (HANDLE_IS_BUILTIN(datatype) && (datatype & MPIR_TYPE_PAIR_MASK)) {
        return true;
    } else if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_DIRECT) {
        if ((datatype & 0xff) < MPIR_num_pairtypes) {
            return true;
        }
    }
    return false;
}

MPL_STATIC_INLINE_PREFIX MPI_Datatype MPIR_Pairtype_get_value_type(MPI_Datatype datatype)
{
    MPIR_Assert(HANDLE_GET_KIND(datatype) == HANDLE_KIND_DIRECT);
    int idx = (datatype & 0xff);
    MPIR_Assert(idx < MPIR_num_pairtypes);
    return MPIR_pairtypes[idx].value_type;
}

/*S
  MPIR_Datatype_contents - Holds envelope and contents data for a given
                           datatype

  Notes:
  Space is allocated beyond the structure itself in order to hold the
  arrays of types, ints, and aints, in that order.

  S*/
typedef struct MPIR_Datatype_contents {
    int combiner;
    MPI_Aint nr_ints;
    MPI_Aint nr_aints;
    MPI_Aint nr_counts;
    MPI_Aint nr_types;
    /* space allocated beyond structure used to store the types[],
     * ints[], and aints[], in that order.
     */
} MPIR_Datatype_contents;

/* Datatype Structure */
/*S
  MPIR_Datatype - Description of the MPID Datatype structure

  Notes:
  The 'ref_count' is needed for nonblocking operations such as
.vb
   MPI_Type_struct(... , &newtype);
   MPI_Irecv(buf, 1000, newtype, ..., &request);
   MPI_Type_free(&newtype);
   ...
   MPI_Wait(&request, &status);
.ve

  Module:
  Datatype-DS

  Notes:

  Alternatives:
  The following alternatives for the layout of this structure were considered.
  Most were not chosen because any benefit in performance or memory
  efficiency was outweighed by the added complexity of the implementation.

  A number of fields contain only boolean information ('is_contig',
  'is_committed').  These
  could be combined and stored in a single bit vector.

  'MPI_Type_dup' could be implemented with a shallow copy, where most of the
  data fields, would not be copied into the new object created by
  'MPI_Type_dup'; instead, the new object could point to the data fields in
  the old object.  However, this requires more code to make sure that fields
  are found in the correct objects and that deleting the old object doesn't
  invalidate the dup'ed datatype.

  Originally we attempted to keep contents/envelope data in a non-optimized
  typerep.  The subarray and darray types were particularly problematic,
  and eventually we decided it would be simpler to just keep contents/
  envelope data in arrays separately.

  Earlier versions of the ADI used a single API to change the 'ref_count',
  with each MPI object type having a separate routine.  Since reference
  count changes are always up or down one, and since all MPI objects
  are defined to have the 'ref_count' field in the same place, the current
  ADI3 API uses two routines, 'MPIR_Object_add_ref' and
  'MPIR_Object_release_ref', to increment and decrement the reference count.

  S*/
struct MPIR_Datatype {
    /* handle and ref_count are filled in by MPIR_Handle_obj_alloc() */
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */

    /* user-visible parameters */
    MPI_Aint size;              /* MPI_Count could be 128 bits, so use MPI_Aint */
    MPI_Aint extent, ub, lb, true_ub, true_lb;
    struct MPIR_Attribute *attributes;
    char name[MPI_MAX_OBJECT_NAME];


    /* private fields */
    /* chars affecting subsequent datatype processing and creation */
    MPI_Aint alignsize;
    int is_committed;

    /* element information; used for accumulate and get elements
     * basic_type: describes basic type (predefined type). If the
     *             type is composed of the same basic type, it is
     *             set to that type, otherwise it is set to MPI_DATATYPE_NULL.
     * n_builtin_elements: refers to the number of builtin type elements.
     * builtin_element_size: refers to the size of builtin type. If the
     *                       type is composed of the same builtin type,
     *                       it is set to size of that type, otherwise it
     *                       is set to -1.
     */
    int basic_type;
    MPI_Aint n_builtin_elements;
    MPI_Aint builtin_element_size;

    /* information on contiguity of type, for processing shortcuts.
     *
     * is_contig is non-zero only if N instances of the type would be
     * contiguous.
     */
    int is_contig;

    /* pointer to contents and envelope data for the datatype */
    MPIR_Datatype_contents *contents;

    /* flattened representation */
    void *flattened;
    int flattened_sz;

    /* handle to the backend datatype engine + some content that we
     * query from it and cache over here for performance reasons */
    struct {
        MPIR_TYPEREP_HANDLE_TYPE handle;
        MPI_Aint num_contig_blocks;     /* contig blocks in one datatype element */
    } typerep;

    /* Other, device-specific information */
#ifdef MPID_DEV_DATATYPE_DECL
     MPID_DEV_DATATYPE_DECL
#endif
};

extern MPIR_Datatype MPIR_Datatype_builtin[MPIR_DATATYPE_N_BUILTIN];
extern MPIR_Datatype MPIR_Datatype_direct[];
extern MPIR_Object_alloc_t MPIR_Datatype_mem;
extern MPI_Datatype MPIR_Datatype_index_to_predefined[MPIR_DATATYPE_N_PREDEFINED];

void MPIR_Datatype_free(MPIR_Datatype * ptr);
void MPIR_Datatype_get_flattened(MPI_Datatype type, void **flattened, int *flattened_sz);
int MPIR_Datatype_init_predefined(void);
int MPIR_Datatype_init_pairtypes(void);
int MPIR_Datatype_finalize_pairtypes(void);
int MPIR_Datatype_builtintype_alignment(MPI_Datatype type);

/* internal debugging functions */
void MPII_Datatype_printf(MPI_Datatype type, int depth, MPI_Aint displacement, int blocklength,
                          int header);

#define MPIR_Datatype_ptr_add_ref(datatype_ptr) MPIR_Object_add_ref((datatype_ptr))

/* to be used only after MPIR_Datatype_valid_ptr(); the check on
 * err == MPI_SUCCESS ensures that we won't try to dereference the
 * pointer if something has already been detected as wrong.
 */
#define MPIR_Datatype_committed_ptr(ptr,err) do {               \
    if ((err == MPI_SUCCESS) && !((ptr)->is_committed))         \
        err = MPIR_Err_create_code(MPI_SUCCESS,                 \
                                   MPIR_ERR_RECOVERABLE,        \
                                   __func__,                      \
                                   __LINE__,                    \
                                   MPI_ERR_TYPE,                \
                                   "**dtypecommit",             \
                                   0);                          \
} while (0)

#define MPIR_Datatype_get_basic_size(a) (((a)&0x0000ff00)>>8)

#define MPIR_Datatype_get_basic_type(a,basic_type_) do {            \
    void *ptr;                                                      \
    switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                    \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);  \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);             \
            basic_type_ = ((MPIR_Datatype *) ptr)->basic_type;      \
            break;                                                  \
        case HANDLE_KIND_INDIRECT:                                  \
            ptr = ((MPIR_Datatype *)                                \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));   \
            basic_type_ = ((MPIR_Datatype *) ptr)->basic_type;      \
            break;                                                  \
        case HANDLE_KIND_BUILTIN:                                   \
            basic_type_ = a;                                        \
            break;                                                  \
        case HANDLE_KIND_INVALID:                                   \
        default:                                                    \
         basic_type_ = 0;                                           \
         break;                                                     \
                                                                    \
    }                                                               \
    /* This macro returns the builtin type, if 'basic_type' is not  \
     * a builtin type, it must be a pair type composed of different \
     * builtin types, so we return MPI_DATATYPE_NULL here.          \
     */                                                             \
    if (!HANDLE_IS_BUILTIN((basic_type_)))                          \
        basic_type_ = MPI_DATATYPE_NULL;                            \
 } while (0)

#define MPIR_Datatype_is_float(a, is_float) do { \
    MPI_Datatype basic_type; \
    MPIR_Datatype_get_basic_type(a, basic_type); \
    int type = basic_type & MPIR_TYPE_TYPE_MASK; \
    if (type == MPIR_TYPE_FLOAT || type == MPIR_TYPE_ALT_FLOAT) { \
        is_float = true; \
    } else { \
        is_float = false; \
    } \
} while (0)

#define MPIR_Datatype_get_ptr(a,ptr)   MPIR_Getb_ptr(Datatype,DATATYPE,a,0x000000ff,ptr)

/* Note: Probably there is some clever way to build all of these from a macro.
 */
#define MPIR_Datatype_get_size_macro(a,size_) do {                     \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            size_ = ((MPIR_Datatype *) ptr)->size;                      \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));       \
            MPIR_Assert(ptr != NULL);                                   \
            size_ = ((MPIR_Datatype *) ptr)->size;                      \
            break;                                                      \
        case HANDLE_KIND_BUILTIN:                                       \
            size_ = MPIR_Datatype_get_basic_size(a);                    \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        default:                                                        \
         size_ = 0;                                                     \
         break;                                                         \
                                                                        \
    }                                                                   \
} while (0)

#define MPIR_Datatype_get_extent_macro(a,extent_) do {                  \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            extent_ = ((MPIR_Datatype *) ptr)->extent;                  \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));       \
            MPIR_Assert(ptr != NULL);                                   \
            extent_ = ((MPIR_Datatype *) ptr)->extent;                  \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            extent_ = MPIR_Datatype_get_basic_size(a);  /* same as size */  \
            break;                                                      \
    }                                                                   \
} while (0)

/* helper macro: takes an MPI_Datatype handle value and returns TRUE in
 * (*is_config_) if the type is contiguous */
#define MPIR_Datatype_is_contig(dtype_, is_contig_)                            \
    do {                                                                       \
        if (HANDLE_IS_BUILTIN((dtype_))) {                                     \
            *(is_contig_) = TRUE;                                              \
        }                                                                      \
        else {                                                                 \
            MPIR_Datatype *dtp_ = NULL;                                        \
            MPIR_Datatype_get_ptr((dtype_), dtp_);                             \
            MPIR_Assert(dtp_ != NULL);                                         \
            *(is_contig_) = dtp_->is_contig;                                   \
        }                                                                      \
    } while (0)

#define MPIR_Datatype_get_density(datatype_, density_)          \
    do {                                                        \
        int is_contig_;                                         \
        MPIR_Datatype_is_contig((datatype_), &is_contig_);      \
        if (is_contig_) {                                       \
            (density_) = INT_MAX;                               \
        } else {                                                \
            MPIR_Datatype *dt_ptr_;                             \
            MPIR_Datatype_get_ptr((datatype_), dt_ptr_);        \
            MPI_Aint size_;                                     \
            MPIR_Datatype_get_size_macro((datatype_), size_);   \
            (density_) = size_ / dt_ptr_->typerep.num_contig_blocks;    \
        }                                                       \
    } while (0)

/* MPIR_Datatype_ptr_release decrements the reference count on the MPIR_Datatype
 * and, if the refct is then zero, frees the MPIR_Datatype and associated
 * structures.
 */
#define MPIR_Datatype_ptr_release(datatype_ptr) do {                            \
    int inuse_;                                                             \
                                                                            \
    MPIR_Object_release_ref((datatype_ptr),&inuse_);                        \
    if (!inuse_) {                                                          \
        int lmpi_errno = MPI_SUCCESS;                                       \
     if (MPIR_Process.attr_free && datatype_ptr->attributes) {              \
         lmpi_errno = MPIR_Process.attr_free(datatype_ptr->handle,         \
                               &datatype_ptr->attributes);                 \
     }                                                                      \
     /* LEAVE THIS COMMENTED OUT UNTIL WE HAVE SOME USE FOR THE FREE_FN     \
     if (datatype_ptr->free_fn) {                                           \
         mpi_errno = (datatype_ptr->free_fn)(datatype_ptr);               \
          if (mpi_errno) {                                                  \
           MPIR_FUNC_EXIT;                  \
           return MPIR_Err_return_comm(0, __func__, mpi_errno);             \
          }                                                                 \
     } */                                                                   \
        if (lmpi_errno == MPI_SUCCESS) {                                    \
         MPIR_Datatype_free(datatype_ptr);                                 \
        }                                                                   \
    }                                                                       \
} while (0)

/* helper macro: takes an MPI_Datatype handle value and returns true_lb in
 * (*true_lb_) */
#define MPIR_Datatype_get_true_lb(dtype_, true_lb_)                            \
    do {                                                                       \
        if (HANDLE_IS_BUILTIN((dtype_))) {                                     \
            *(true_lb_) = 0;                                                   \
        }                                                                      \
        else {                                                                 \
            MPIR_Datatype *dtp_ = NULL;                                        \
            MPIR_Datatype_get_ptr((dtype_), dtp_);                             \
            *(true_lb_) = dtp_->true_lb;                                       \
        }                                                                      \
    } while (0)

#define MPIR_Datatype_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Datatype,ptr,MPI_ERR_TYPE,err)

/* we pessimistically assume that MPI_DATATYPE_NULL may be passed as a "valid" type
 * for send/recv when MPI_PROC_NULL is the destination/src */
#define MPIR_Datatype_add_ref_if_not_builtin(datatype_)             \
    do {                                                            \
    if ((datatype_) != MPI_DATATYPE_NULL &&                         \
        !MPIR_DATATYPE_IS_PREDEFINED((datatype_)))                  \
    {                                                               \
        MPIR_Datatype *dtp_ = NULL;                                 \
        MPIR_Datatype_get_ptr((datatype_), dtp_);                   \
        MPIR_Assert(dtp_ != NULL);                                  \
        MPIR_Datatype_ptr_add_ref(dtp_);                            \
    }                                                               \
    } while (0)

#define MPIR_Datatype_release_if_not_builtin(datatype_)             \
    do {                                                            \
    if ((datatype_) != MPI_DATATYPE_NULL &&                         \
        !MPIR_DATATYPE_IS_PREDEFINED((datatype_)))                  \
    {                                                               \
        MPIR_Datatype *dtp_ = NULL;                                 \
        MPIR_Datatype_get_ptr((datatype_), dtp_);                   \
        MPIR_Assert(dtp_ != NULL);                                  \
        MPIR_Datatype_ptr_release(dtp_);                            \
    }                                                               \
    } while (0)

#define MPIR_DATATYPE_IS_PREDEFINED(type) \
    ((HANDLE_IS_BUILTIN((type))) || \
     (type == MPI_FLOAT_INT) || (type == MPI_DOUBLE_INT) || \
     (type == MPI_LONG_INT) || (type == MPI_SHORT_INT) || \
     (type == MPI_LONG_DOUBLE_INT))

/*@
  MPIR_Datatype_set_contents - store contents information for use in
  MPI_Type_get_contents.

  Returns MPI_SUCCESS on success, MPI error code on error.
  @*/
static inline int MPIR_Datatype_set_contents(MPIR_Datatype * new_dtp,
                                             int combiner,
                                             MPI_Aint nr_ints,
                                             MPI_Aint nr_aints,
                                             MPI_Aint nr_counts,
                                             MPI_Aint nr_types,
                                             int array_of_ints[],
                                             const MPI_Aint array_of_aints[],
                                             const MPI_Aint array_of_counts[],
                                             const MPI_Datatype array_of_types[])
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint struct_sz, ints_sz, aints_sz, counts_sz, types_sz, contents_size;
    MPIR_Datatype_contents *cp;
    char *ptr;

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz = nr_types * sizeof(MPI_Datatype);
    ints_sz = nr_ints * sizeof(int);
    aints_sz = nr_aints * sizeof(MPI_Aint);
    counts_sz = nr_counts * sizeof(MPI_Aint);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the counts,
     *       because they are last in the region.
     *       Padding it anyway for readability.
     */
    MPI_Aint epsilon;
    if ((epsilon = struct_sz % MAX_ALIGNMENT)) {
        struct_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = types_sz % MAX_ALIGNMENT)) {
        types_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = ints_sz % MAX_ALIGNMENT)) {
        ints_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = aints_sz % MAX_ALIGNMENT)) {
        aints_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = counts_sz % MAX_ALIGNMENT)) {
        counts_sz += MAX_ALIGNMENT - epsilon;
    }

    contents_size = struct_sz + types_sz + ints_sz + aints_sz + counts_sz;

    cp = (MPIR_Datatype_contents *) MPL_malloc(contents_size, MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (cp == NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         "MPIR_Datatype_set_contents",
                                         __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    cp->combiner = combiner;
    cp->nr_ints = nr_ints;
    cp->nr_aints = nr_aints;
    cp->nr_types = nr_types;
    cp->nr_counts = nr_counts;

    /* arrays are stored in the following order: types, ints, aints,
     * following the structure itself.
     */
    ptr = ((char *) cp) + struct_sz;
    /* Fortran90 combiner types do not have a "base" type */
    if (nr_types > 0) {
        MPIR_Memcpy(ptr, array_of_types, nr_types * sizeof(MPI_Datatype));
    }

    ptr = ((char *) cp) + struct_sz + types_sz;
    if (nr_ints > 0) {
        MPIR_Memcpy(ptr, array_of_ints, nr_ints * sizeof(int));
    }

    ptr = ((char *) cp) + struct_sz + types_sz + ints_sz;
    if (nr_aints > 0) {
        MPIR_Memcpy(ptr, array_of_aints, nr_aints * sizeof(MPI_Aint));
    }

    ptr = ((char *) cp) + struct_sz + types_sz + ints_sz + aints_sz;
    if (nr_counts > 0) {
        MPIR_Memcpy(ptr, array_of_counts, nr_counts * sizeof(MPI_Aint));
    }

    new_dtp->contents = cp;
    new_dtp->flattened = NULL;

    /* increment reference counts on all the derived types used here */
    for (MPI_Aint i = 0; i < nr_types; i++) {
        MPIR_Datatype_add_ref_if_not_builtin(array_of_types[i]);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX void MPIR_Datatype_access_contents(MPIR_Datatype_contents * cp,
                                                            int **p_ints,
                                                            MPI_Aint ** p_aints,
                                                            MPI_Aint ** p_counts,
                                                            MPI_Datatype ** p_types)
{
    MPI_Aint struct_sz, ints_sz, aints_sz, counts_sz, types_sz;

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz = cp->nr_types * sizeof(MPI_Datatype);
    ints_sz = cp->nr_ints * sizeof(int);
    aints_sz = cp->nr_aints * sizeof(MPI_Aint);
    counts_sz = cp->nr_counts * sizeof(MPI_Aint);

    MPI_Aint epsilon;
    if ((epsilon = struct_sz % MAX_ALIGNMENT)) {
        struct_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = types_sz % MAX_ALIGNMENT)) {
        types_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = ints_sz % MAX_ALIGNMENT)) {
        ints_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = ints_sz % MAX_ALIGNMENT)) {
        aints_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = counts_sz % MAX_ALIGNMENT)) {
        counts_sz += MAX_ALIGNMENT - epsilon;
    }

    *p_types = (void *) ((char *) cp + struct_sz);
    *p_ints = (void *) ((char *) cp + struct_sz + types_sz);
    *p_aints = (void *) ((char *) cp + struct_sz + types_sz + ints_sz);
    *p_counts = (void *) ((char *) cp + struct_sz + types_sz + ints_sz + aints_sz);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Datatype_free_contents(MPIR_Datatype * dtp)
{
    MPIR_Datatype_contents *cp = dtp->contents;

    int *ints;
    MPI_Aint *aints, *counts;
    MPI_Datatype *types;

    MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);

    for (int i = 0; i < cp->nr_types; i++) {
        MPIR_Datatype_release_if_not_builtin(types[i]);
    }

    MPL_free(cp);
    dtp->contents = NULL;
}

MPL_STATIC_INLINE_PREFIX int MPIR_Type_get_combiner(MPI_Datatype datatype)
{
    if (MPIR_DATATYPE_IS_PREDEFINED(datatype)) {
        return MPI_COMBINER_NAMED;
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);
        return dtp->contents->combiner;
    }
}

/* This routine is used to install an attribute free routine for datatypes
   at finalize-time */
void MPII_Datatype_attr_finalize(void);
int MPII_Type_zerolen(MPI_Datatype * newtype);

int MPIR_Get_elements_x_impl(MPI_Count * bytes, MPI_Datatype datatype, MPI_Count * elements);
int MPIR_Type_contiguous_x_impl(MPI_Count count, MPI_Datatype old_type, MPI_Datatype * new_type_p);
void MPIR_Pack_size(MPI_Aint incount, MPI_Datatype datatype, MPI_Aint * size);

/* Datatype functions */
int MPII_Type_zerolen(MPI_Datatype * newtype);

int MPIR_Type_contiguous(MPI_Aint count, MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPIR_Type_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride, bool strideinbytes,
                     MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPIR_Type_blockindexed(MPI_Aint count, MPI_Aint blocklength,
                           const MPI_Aint displacement_array[],
                           bool dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPIR_Type_indexed(MPI_Aint count, const MPI_Aint * blocklength_array,
                      const MPI_Aint * displacement_array,
                      bool dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPIR_Type_struct(MPI_Aint count, const MPI_Aint * blocklength_array,
                     const MPI_Aint * displacement_array,
                     const MPI_Datatype * oldtype_array, MPI_Datatype * newtype);
int MPIR_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                             MPI_Datatype * new_type);
int MPIR_Type_dup(MPI_Datatype oldtype, MPI_Datatype * newtype);

/* debugging helper functions */
char *MPIR_Datatype_builtin_to_string(MPI_Datatype type);
char *MPIR_Datatype_combiner_to_string(int combiner);
void MPIR_Datatype_debug(MPI_Datatype type, int array_ct);

MPI_Aint MPII_Datatype_indexed_count_contig(MPI_Aint count,
                                            const MPI_Aint * blocklength_array,
                                            const MPI_Aint * displacement_array,
                                            int dispinbytes, MPI_Aint old_extent);

MPI_Aint MPII_Datatype_blockindexed_count_contig(MPI_Aint count,
                                                 MPI_Aint blklen,
                                                 const MPI_Aint disp_array[],
                                                 int dispinbytes, MPI_Aint old_extent);

#endif /* MPIR_DATATYPE_H_INCLUDED */
