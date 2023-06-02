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
    if (basic_type == MPI_FLOAT || basic_type == MPI_DOUBLE) { \
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
    MPIR_Datatype *old_dtp;
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
        if (!HANDLE_IS_BUILTIN(array_of_types[i])) {
            MPIR_Datatype_get_ptr(array_of_types[i], old_dtp);
            MPIR_Datatype_valid_ptr(old_dtp, mpi_errno);
            MPIR_Datatype_ptr_add_ref(old_dtp);
        }
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
        if (!HANDLE_IS_BUILTIN(types[i])) {
            MPIR_Datatype *old_dtp;
            MPIR_Datatype_get_ptr(types[i], old_dtp);
            MPIR_Datatype_ptr_release(old_dtp);
        }
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

MPL_STATIC_INLINE_PREFIX MPI_Datatype MPIR_Datatype_predefined_get_type(uint32_t index)
{
    MPIR_Assert(index < MPIR_DATATYPE_N_PREDEFINED);
    return MPIR_Datatype_index_to_predefined[index];
}

MPL_STATIC_INLINE_PREFIX int MPIR_Datatype_predefined_get_index(MPI_Datatype datatype)
{
    int dtype_index = 0;
    switch (HANDLE_GET_KIND(datatype)) {
        case HANDLE_KIND_BUILTIN:
            /* Predefined builtin index mask for dtype. See MPIR_Datatype_get_ptr. */
            dtype_index = datatype & 0x000000ff;
            MPIR_Assert(dtype_index < MPIR_DATATYPE_N_BUILTIN);
            break;
        case HANDLE_KIND_DIRECT:
            /* pairtype */
            dtype_index = HANDLE_INDEX(datatype) + MPIR_DATATYPE_N_BUILTIN;
            MPIR_Assert(dtype_index < MPIR_DATATYPE_N_BUILTIN + MPIR_DATATYPE_N_BUILTIN);
            break;
        default:
            /* should be called only by builtin or pairtype */
            MPIR_Assert(HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
                        HANDLE_GET_KIND(datatype) == HANDLE_KIND_DIRECT);
            break;
    }
    return dtype_index;
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
int MPIR_Type_create_pairtype(MPI_Datatype datatype, MPIR_Datatype * new_dtp);

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
