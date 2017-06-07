/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDU_DATATYPE_H
#define MPIDU_DATATYPE_H

#include "mpidu_dataloop.h"
#include "mpir_objects.h"

/* NOTE: 
 * - struct MPIDU_Dataloop and MPIDU_Segment are defined in 
 *   src/mpid/common/datatype/mpidu_dataloop.h (and gen_dataloop.h).
 * - MPIR_Object_alloc_t is defined in src/include/mpihandle.h
 */

#define MPIDU_Datatype_get_ptr(a,ptr)   MPIR_Getb_ptr(Datatype,a,0x000000ff,ptr)
/* MPIDU_Datatype_get_basic_id() is useful for creating and indexing into arrays
   that store data on a per-basic type basis */
#define MPIDU_Datatype_get_basic_id(a) ((a)&0x000000ff)
#define MPIDU_Datatype_get_basic_size(a) (((a)&0x0000ff00)>>8)

#define MPIDU_Datatype_add_ref(datatype_ptr) MPIR_Object_add_ref((datatype_ptr))

#define MPIDU_Datatype_get_basic_type(a,basic_type_) do {                    \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            basic_type_ = ((MPIDU_Datatype *) ptr)->basic_type;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            basic_type_ = ((MPIDU_Datatype *) ptr)->basic_type;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            basic_type_ = a;						\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    basic_type_ = 0;						\
	    break;							\
 									\
    }									\
    /* This macro returns the builtin type, if 'basic_type' is not      \
     * a builtin type, it must be a pair type composed of different     \
     * builtin types, so we return MPI_DATATYPE_NULL here.              \
     */                                                                 \
    if (HANDLE_GET_KIND(basic_type_) != HANDLE_KIND_BUILTIN)                \
        basic_type_ = MPI_DATATYPE_NULL;                                    \
 } while(0)

/* MPIDU_Datatype_release decrements the reference count on the MPIR_Datatype
 * and, if the refct is then zero, frees the MPIDU_Datatype and associated
 * structures.
 */
#define MPIDU_Datatype_release(datatype_ptr) do {                            \
    int inuse_;								    \
									    \
    MPIR_Object_release_ref((datatype_ptr),&inuse_);			    \
    if (!inuse_) {							    \
        int lmpi_errno = MPI_SUCCESS;					    \
	if (MPIR_Process.attr_free && datatype_ptr->attributes) {	    \
	    lmpi_errno = MPIR_Process.attr_free( datatype_ptr->handle,	    \
						 &datatype_ptr->attributes ); \
	}								    \
 	/* LEAVE THIS COMMENTED OUT UNTIL WE HAVE SOME USE FOR THE FREE_FN  \
	if (datatype_ptr->free_fn) {					    \
	    mpi_errno = (datatype_ptr->free_fn)( datatype_ptr );	    \
	     if (mpi_errno) {						    \
		 MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_FREE);		    \
		 return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );	    \
	     }								    \
	} */								    \
        if (lmpi_errno == MPI_SUCCESS) {				    \
	    MPIDU_Datatype_free(datatype_ptr);				    \
        }								    \
    }                                                                       \
} while(0)

/* Note: Probably there is some clever way to build all of these from a macro.
 */
#define MPIDU_Datatype_get_size_macro(a,size_) do {                      \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            size_ = ((MPIDU_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            size_ = ((MPIDU_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            size_ = MPIDU_Datatype_get_basic_size(a);			\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    size_ = 0;							\
	    break;							\
 									\
    }									\
} while(0)

/*
 * The following macro allows us to reference either the regular or 
 * hetero value for the 3 fields (NULL,_size,_depth) in the
 * MPIDU_Datatype structure.  This is used in the many
 * macros that access fields of the datatype.  We need this macro
 * to simplify the definition of the other macros in the case where
 * MPID_HAS_HETERO is *not* defined.
 */
#if defined(MPID_HAS_HETERO) || 1
#define MPIDU_GET_FIELD(hetero_,value_,fieldname_) do {                          \
        if (hetero_ != MPIDU_DATALOOP_HETEROGENEOUS)                             \
            value_ = ((MPIDU_Datatype *)ptr)->dataloop##fieldname_;              \
        else value_ = ((MPIDU_Datatype *) ptr)->hetero_dloop##fieldname_;        \
    } while(0)
#else
#define MPIDU_GET_FIELD(hetero_,value_,fieldname_) \
      value_ = ((MPIDU_Datatype *)ptr)->dataloop##fieldname_
#endif

#if defined(MPID_HAS_HETERO) || 1
#define MPIDU_SET_FIELD(hetero_,value_,fieldname_) do {                          \
        if (hetero_ != MPIDU_DATALOOP_HETEROGENEOUS)                             \
            ((MPIDU_Datatype *)ptr)->dataloop##fieldname_ = value_;              \
        else ((MPIDU_Datatype *) ptr)->hetero_dloop##fieldname_ = value_;        \
    } while(0)
#else
#define MPIDU_SET_FIELD(hetero_,value_,fieldname_) \
    ((MPIDU_Datatype *)ptr)->dataloop##fieldname_ = value_
#endif
 
#define MPIDU_Datatype_get_loopdepth_macro(a,depth_,hetero_) do {        \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_GET_FIELD(hetero_,depth_,_depth);                      \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_GET_FIELD(hetero_,depth_,_depth);                      \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;                                                 \
            break;							\
    }                                                                   \
} while(0)

#define MPIDU_Datatype_get_loopsize_macro(a,depth_,hetero_) do {         \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_GET_FIELD(hetero_,depth_,_size);                       \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_GET_FIELD(hetero_,depth_,_size);                       \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;                                                 \
            break;							\
    }                                                                   \
} while(0)

#define MPIDU_Datatype_get_loopptr_macro(a,lptr_,hetero_) do {  		\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_GET_FIELD(hetero_,lptr_,);                             \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_GET_FIELD(hetero_,lptr_,);                             \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            lptr_ = 0;							\
            break;							\
    }									\
} while(0)
#define MPIDU_Datatype_set_loopdepth_macro(a,depth_,hetero_) do {        \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_SET_FIELD(hetero_,depth_,_depth);                      \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_SET_FIELD(hetero_,depth_,_depth);                      \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;                                                 \
            break;							\
    }                                                                   \
} while(0)

#define MPIDU_Datatype_set_loopsize_macro(a,depth_,hetero_) do {         \
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_SET_FIELD(hetero_,depth_,_size);                       \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_SET_FIELD(hetero_,depth_,_size);                       \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            depth_ = 0;                                                 \
            break;							\
    }                                                                   \
} while(0)

#define MPIDU_Datatype_set_loopptr_macro(a,lptr_,hetero_) do {  		\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			\
            MPIDU_SET_FIELD(hetero_,lptr_,);                             \
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPIDU_Datatype *)					\
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	\
            MPIDU_SET_FIELD(hetero_,lptr_,);                             \
            break;							\
        case HANDLE_KIND_INVALID:					\
        case HANDLE_KIND_BUILTIN:					\
        default:							\
            lptr_ = 0;							\
            break;							\
    }									\
} while(0)
        
#define MPIDU_Datatype_get_extent_macro(a,extent_) do {                      \
    void *ptr;								    \
    switch (HANDLE_GET_KIND(a)) {					    \
        case HANDLE_KIND_DIRECT:					    \
            ptr = MPIDU_Datatype_direct+HANDLE_INDEX(a);			    \
            extent_ = ((MPIDU_Datatype *) ptr)->extent;			    \
            break;							    \
        case HANDLE_KIND_INDIRECT:					    \
            ptr = ((MPIDU_Datatype *)					    \
		   MPIR_Handle_get_ptr_indirect(a,&MPIDU_Datatype_mem));	    \
            extent_ = ((MPIDU_Datatype *) ptr)->extent;			    \
            break;							    \
        case HANDLE_KIND_INVALID:					    \
        case HANDLE_KIND_BUILTIN:					    \
        default:							    \
            extent_ = MPIDU_Datatype_get_basic_size(a);  /* same as size */  \
            break;							    \
    }									    \
} while(0)

#define MPIDU_Datatype_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Datatype,ptr,MPI_ERR_TYPE,err)

/* to be used only after MPIDU_Datatype_valid_ptr(); the check on
 * err == MPI_SUCCESS ensures that we won't try to dereference the
 * pointer if something has already been detected as wrong.
 */
#define MPIDU_Datatype_committed_ptr(ptr,err) do {               \
    if ((err == MPI_SUCCESS) && !((ptr)->is_committed))		\
        err = MPIR_Err_create_code(MPI_SUCCESS,			\
				   MPIR_ERR_RECOVERABLE,	\
				   FCNAME,			\
				   __LINE__,			\
				   MPI_ERR_TYPE,		\
				   "**dtypecommit",		\
				   0);				\
} while(0)

/*S
  MPIDU_Datatype_contents - Holds envelope and contents data for a given
                           datatype

  Notes:
  Space is allocated beyond the structure itself in order to hold the
  arrays of types, ints, and aints, in that order.

  S*/
typedef struct MPIDU_Datatype_contents {
    int combiner;
    int nr_ints;
    int nr_aints;
    int nr_types;
    /* space allocated beyond structure used to store the types[],
     * ints[], and aints[], in that order.
     */
} MPIDU_Datatype_contents;

/* Datatype Structure */
/*S
  MPIDU_Datatype - Description of the MPID Datatype structure

  Notes:
  The 'ref_count' is needed for nonblocking operations such as
.vb
   MPI_Type_struct( ... , &newtype );
   MPI_Irecv( buf, 1000, newtype, ..., &request );
   MPI_Type_free( &newtype );
   ...
   MPI_Wait( &request, &status );
.ve

  Module:
  Datatype-DS

  Notes:

  Alternatives:
  The following alternatives for the layout of this structure were considered.
  Most were not chosen because any benefit in performance or memory 
  efficiency was outweighed by the added complexity of the implementation.

  A number of fields contain only boolean inforation ('is_contig', 
  'has_sticky_ub', 'has_sticky_lb', 'is_permanent', 'is_committed').  These 
  could be combined and stored in a single bit vector.  

  'MPI_Type_dup' could be implemented with a shallow copy, where most of the
  data fields, would not be copied into the new object created by
  'MPI_Type_dup'; instead, the new object could point to the data fields in
  the old object.  However, this requires more code to make sure that fields
  are found in the correct objects and that deleting the old object doesn't
  invalidate the dup'ed datatype.

  Originally we attempted to keep contents/envelope data in a non-optimized
  dataloop.  The subarray and darray types were particularly problematic,
  and eventually we decided it would be simpler to just keep contents/
  envelope data in arrays separately.

  Earlier versions of the ADI used a single API to change the 'ref_count', 
  with each MPI object type having a separate routine.  Since reference
  count changes are always up or down one, and since all MPI objects 
  are defined to have the 'ref_count' field in the same place, the current
  ADI3 API uses two routines, 'MPIR_Object_add_ref' and
  'MPIR_Object_release_ref', to increment and decrement the reference count.

  S*/
typedef struct MPIDU_Datatype { 
    /* handle and ref_count are filled in by MPIR_Handle_obj_alloc() */
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */

    /* basic parameters for datatype, accessible via MPI calls */
    MPI_Aint size;   /* MPI_Count could be 128 bits, so use MPI_Aint */
    MPI_Aint extent, ub, lb, true_ub, true_lb;

    /* chars affecting subsequent datatype processing and creation */
    MPI_Aint alignsize;
    int has_sticky_ub, has_sticky_lb;
    int is_permanent; /* non-zero if datatype is a predefined type */
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
    int      basic_type;
    MPI_Aint n_builtin_elements;
    MPI_Aint builtin_element_size;

    /* information on contiguity of type, for processing shortcuts.
     *
     * is_contig is non-zero only if N instances of the type would be
     * contiguous.
     */
    int is_contig;
    /* Upper bound on the number of contig blocks for one instance.
     * It is not trivial to calculate the *real* number of contig 
     * blocks in the case where old datatype is non-contiguous
     */
    MPI_Aint max_contig_blocks;

    /* pointer to contents and envelope data for the datatype */
    MPIDU_Datatype_contents *contents;

    /* dataloop members, including a pointer to the loop, the size in bytes,
     * and a depth used to verify that we can process it (limited stack depth
     */
    struct MPIDU_Dataloop *dataloop; /* might be optimized for homogenous */
    MPI_Aint              dataloop_size;
    int                   dataloop_depth;
#if defined(MPID_HAS_HETERO) || 1
    struct MPIDU_Dataloop *hetero_dloop; /* heterogeneous dataloop */
    MPI_Aint              hetero_dloop_size;
    int                   hetero_dloop_depth;
#endif /* MPID_HAS_HETERO */
    /* MPI-2 attributes and name */
    struct MPIR_Attribute *attributes;
    char                  name[MPI_MAX_OBJECT_NAME];

    /* not yet used; will be used to track what processes have cached
     * copies of this type.
     */
    int32_t cache_id;
    /* MPID_Lpidmask mask; */

    /* int (*free_fn)( struct MPIDU_Datatype * ); */ /* Function to free this datatype */

    /* Other, device-specific information */
#ifdef MPID_DEV_DATATYPE_DECL
    MPID_DEV_DATATYPE_DECL
#endif
#ifdef HAVE_EXT_COLL
    MPIC_DT_DECL
#endif
} MPIDU_Datatype;

extern MPIR_Object_alloc_t MPIDU_Datatype_mem;

/* Preallocated datatype objects */
/* This value should be set to greatest value used as the type index suffix in
 * the predefined handles.  That is, look at the last two hex digits of all
 * predefined datatype handles, take the greatest one, and convert it to decimal
 * here. */
/* FIXME calculating this value this way is foolish, we should make this more
 * automatic and less error prone */
/* FIXME: Given that this is relatively static, an adequate alternative is
   to provide a check that this value is valid. */
#define MPIDU_DATATYPE_N_BUILTIN 69
extern MPIDU_Datatype MPIDU_Datatype_builtin[MPIDU_DATATYPE_N_BUILTIN + 1];
extern MPIDU_Datatype MPIDU_Datatype_direct[];

#define MPIDU_DTYPE_BEGINNING  0
#define MPIDU_DTYPE_END       -1

/* LB/UB calculation helper macros */

/* MPIDU_DATATYPE_CONTIG_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the
 * old type's LB, UB, and extent, and a count of these types in the
 * block.
 *
 * Note: if the displacement is non-zero, the MPIDU_DATATYPE_BLOCK_LB_UB()
 * should be used instead (see below).
 */
#define MPIDU_DATATYPE_CONTIG_LB_UB(cnt_,		\
				   old_lb_,		\
				   old_ub_,		\
				   old_extent_,	\
				   lb_,		\
				   ub_)		\
do {							\
    if (cnt_ == 0) {					\
	lb_ = old_lb_;				\
	ub_ = old_ub_;				\
    }							\
    else if (old_ub_ >= old_lb_) {			\
        lb_ = old_lb_;				\
        ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);	\
    }							\
    else /* negative extent */ {			\
	lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);	\
	ub_ = old_ub_;				\
    }                                                   \
} while(0)

/* MPIDU_DATATYPE_VECTOR_LB_UB()
 *
 * Determines the new LB and UB for a vector of blocks of old types
 * given the old type's LB, UB, and extent, and a count, stride, and
 * blocklen describing the vectorization.
 */
#define MPIDU_DATATYPE_VECTOR_LB_UB(cnt_,			\
				   stride_,			\
				   blklen_,			\
				   old_lb_,			\
				   old_ub_,			\
				   old_extent_,		\
				   lb_,			\
				   ub_)			\
do {								\
    if (cnt_ == 0 || blklen_ == 0) {				\
	lb_ = old_lb_;					\
	ub_ = old_ub_;					\
    }								\
    else if (stride_ >= 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_;					\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
    }								\
    else if (stride_ < 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_ + (stride_) * ((cnt_) - 1);		\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);	\
    }								\
    else if (stride_ >= 0 && (old_extent_) < 0) {		\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);	\
	ub_ = old_ub_ + (stride_) * ((cnt_) - 1);		\
    }								\
    else {							\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
	ub_ = old_ub_;					\
    }								\
} while(0)

/* MPIDU_DATATYPE_BLOCK_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the LB,
 * UB, and extent of the old type as well as a new displacement and count
 * of types.
 *
 * Note: we need the extent here in addition to the lb and ub because the
 * extent might have some padding in it that we need to take into account.
 */
#define MPIDU_DATATYPE_BLOCK_LB_UB(cnt_,				\
				  disp_,				\
				  old_lb_,				\
				  old_ub_,				\
				  old_extent_,				\
				  lb_,					\
				  ub_)					\
do {									\
    if (cnt_ == 0) {							\
	lb_ = old_lb_ + (disp_);					\
	ub_ = old_ub_ + (disp_);					\
    }									\
    else if (old_ub_ >= old_lb_) {					\
        lb_ = old_lb_ + (disp_);					\
        ub_ = old_ub_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
    }									\
    else /* negative extent */ {					\
	lb_ = old_lb_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
	ub_ = old_ub_ + (disp_);					\
    }									\
} while(0)

/* helper macro: takes an MPI_Datatype handle value and returns TRUE in
 * (*is_config_) if the type is contiguous */
#define MPIDU_Datatype_is_contig(dtype_, is_contig_)                            \
    do {                                                                       \
        if (HANDLE_GET_KIND(dtype_) == HANDLE_KIND_BUILTIN) {                  \
            *(is_contig_) = TRUE;                                              \
        }                                                                      \
        else {                                                                 \
            MPIDU_Datatype *dtp_ = NULL;                                        \
            MPIDU_Datatype_get_ptr((dtype_), dtp_);                             \
            *(is_contig_) = dtp_->is_contig;                                   \
        }                                                                      \
    } while (0)

/* helper macro: takes an MPI_Datatype handle value and returns true_lb in
 * (*true_lb_) */
#define MPIDU_Datatype_get_true_lb(dtype_, true_lb_)                            \
    do {                                                                       \
        if (HANDLE_GET_KIND(dtype_) == HANDLE_KIND_BUILTIN) {                  \
            *(true_lb_) = 0;                                                   \
        }                                                                      \
        else {                                                                 \
            MPIDU_Datatype *dtp_ = NULL;                                        \
            MPIDU_Datatype_get_ptr((dtype_), dtp_);                             \
            *(true_lb_) = dtp_->true_lb;                                       \
        }                                                                      \
    } while (0)

/* Datatype functions */
int MPIDU_Type_commit(MPI_Datatype *type);

int MPIDU_Type_dup(MPI_Datatype oldtype,
		  MPI_Datatype *newtype);

int MPIDU_Type_struct(int count,
		     const int *blocklength_array,
		     const MPI_Aint *displacement_array,
		     const MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype);

int MPIDU_Type_indexed(int count,
		      const int *blocklength_array,
		      const void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype);

int MPIDU_Type_blockindexed(int count,
			   int blocklength,
			   const void *displacement_array,
			   int dispinbytes,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype);

int MPIDU_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype);

int MPIDU_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype);

int MPIDU_Type_zerolen(MPI_Datatype *newtype);

int MPIDU_Type_create_resized(MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
			     MPI_Datatype *newtype);

int MPIDU_Type_get_envelope(MPI_Datatype datatype,
			   int *num_integers,
			   int *num_addresses,
			   int *num_datatypes,
			   int *combiner);

int MPIDU_Type_get_contents(MPI_Datatype datatype, 
			   int max_integers, 
			   int max_addresses, 
			   int max_datatypes, 
			   int array_of_integers[], 
			   MPI_Aint array_of_addresses[], 
			   MPI_Datatype array_of_datatypes[]);

int MPIDU_Type_create_pairtype(MPI_Datatype datatype,
                              MPIDU_Datatype *new_dtp);

/* internal debugging functions */
void MPIDI_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header);

/* Dataloop functions */
void MPIDU_Dataloop_copy(void *dest,
			void *src,
			MPI_Aint size);

void MPIDU_Dataloop_print(struct MPIDU_Dataloop *dataloop,
			 int depth);

void MPIDU_Dataloop_alloc(int kind,
			 MPI_Aint count,
			 DLOOP_Dataloop **new_loop_p,
			 MPI_Aint *new_loop_sz_p);

void MPIDU_Dataloop_alloc_and_copy(int kind,
				  MPI_Aint count,
				  struct DLOOP_Dataloop *old_loop,
				  MPI_Aint old_loop_sz,
				  struct DLOOP_Dataloop **new_loop_p,
				  MPI_Aint *new_loop_sz_p);
void MPIDU_Dataloop_struct_alloc(MPI_Aint count,
				MPI_Aint old_loop_sz,
				int basic_ct,
				DLOOP_Dataloop **old_loop_p,
				DLOOP_Dataloop **new_loop_p,
				MPI_Aint *new_loop_sz_p);
void MPIDU_Dataloop_dup(DLOOP_Dataloop *old_loop,
		       MPI_Aint old_loop_sz,
		       DLOOP_Dataloop **new_loop_p);
void MPIDU_Dataloop_free(struct MPIDU_Dataloop **dataloop);

/* Segment functions specific to MPICH */
void MPIDU_Segment_pack_vector(struct DLOOP_Segment *segp,
			      DLOOP_Offset first,
			      DLOOP_Offset *lastp,
			      DLOOP_VECTOR *vector,
			      int *lengthp);

void MPIDU_Segment_unpack_vector(struct DLOOP_Segment *segp,
				DLOOP_Offset first,
				DLOOP_Offset *lastp,
				DLOOP_VECTOR *vector,
				int *lengthp);

void MPIDU_Segment_flatten(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_Offset *offp,
			  DLOOP_Size *sizep,
			  DLOOP_Offset *lengthp);

/* misc */
int MPIDU_Datatype_set_contents(struct MPIDU_Datatype *ptr,
			       int combiner,
			       int nr_ints,
			       int nr_aints,
			       int nr_types,
			       int *ints,
			       const MPI_Aint *aints,
			       const MPI_Datatype *types);

void MPIDU_Datatype_free_contents(struct MPIDU_Datatype *ptr);
void MPIDI_Datatype_get_contents_aints(MPIDU_Datatype_contents *cp,
				       MPI_Aint *user_aints);
void MPIDI_Datatype_get_contents_types(MPIDU_Datatype_contents *cp,
				       MPI_Datatype *user_types);
void MPIDI_Datatype_get_contents_ints(MPIDU_Datatype_contents *cp,
				      int *user_ints);

void MPIDU_Datatype_free(struct MPIDU_Datatype *ptr);

void MPIDU_Dataloop_update(struct DLOOP_Dataloop *dataloop,
			  MPI_Aint ptrdiff);

int MPIR_Type_flatten(MPI_Datatype type,
		      MPI_Aint *off_array,
		      DLOOP_Size *size_array,
		      MPI_Aint *array_len_p);

void MPIDU_Segment_pack_external32(struct DLOOP_Segment *segp,
				  DLOOP_Offset first,
				  DLOOP_Offset *lastp, 
				  void *pack_buffer);

void MPIDU_Segment_unpack_external32(struct DLOOP_Segment *segp,
				    DLOOP_Offset first,
				    DLOOP_Offset *lastp,
				    DLOOP_Buffer unpack_buffer);

MPI_Aint MPIDU_Datatype_size_external32(MPI_Datatype type);
MPI_Aint MPIDI_Datatype_get_basic_size_external32(MPI_Datatype el_type);

/* debugging helper functions */
char *MPIDU_Datatype_builtin_to_string(MPI_Datatype type);
char *MPIDU_Datatype_combiner_to_string(int combiner);
void MPIDU_Datatype_debug(MPI_Datatype type, int array_ct);

/* contents accessor functions */
void MPIDU_Type_access_contents(MPI_Datatype type,
			       int **ints_p,
			       MPI_Aint **aints_p,
			       MPI_Datatype **types_p);
void MPIDU_Type_release_contents(MPI_Datatype type,
				int **ints_p,
				MPI_Aint **aints_p,
				MPI_Datatype **types_p);

/* end of file */
#endif
