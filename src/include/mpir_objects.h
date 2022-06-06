/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_OBJECTS_H_INCLUDED
#define MPIR_OBJECTS_H_INCLUDED

#include "mpichconf.h"

/*TDSOverview.tex

  MPI has a number of data structures, most of which are represented by
  an opaque handle in an MPI program.  In the MPICH implementation of MPI,
  these handles are represented
  as integers; this makes implementation of the C/Fortran handle transfer
  calls (part of MPI-2) easy.

  MPIR objects
  are allocated by a common set of object allocation functions.

  where 'objmem' is a pointer to a memory allocation object that knows
  enough to allocate objects, including the
  size of the object and the location of preallocated memory, as well
  as the type of memory allocator.  By providing the routines to allocate and
  free the memory, we make it easy to use the same interface to allocate both
  local and shared memory for objects (always using the same kind for each
  type of object).

  The names create/destroy were chosen because they are different from
  new/delete (C++ operations) and malloc/free.
  Any name choice will have some conflicts with other uses, of course.

  Reference Counts:
  Many MPI objects have reference count semantics.
  The semantics of MPI require that many objects that have been freed by the
  user
  (e.g., with 'MPI_Type_free' or 'MPI_Comm_free') remain valid until all
  pending
  references to that object (e.g., by an 'MPI_Irecv') are complete.  There
  are several ways to implement this; MPICH uses `reference counts` in the
  objects.  To support the 'MPI_THREAD_MULTIPLE' level of thread-safety, these
  reference counts must be accessed and updated atomically.
  A reference count for
  `any` object can be incremented (atomically)
  with 'MPIR_Object_add_ref(objptr)'
  and decremented with 'MPIR_Object_release_ref(objptr,newval_ptr)'.
  These have been designed so that then can be implemented as inlined
  macros rather than function calls, even in the multithreaded case, and
  can use special processor instructions that guarantee atomicity to
  avoid thread locks.
  The decrement routine sets the value pointed at by 'inuse_ptr' to 0 if
  the postdecrement value of the reference counter is zero, and to a non-zero
  value otherwise.  If this value is zero, then the routine that decremented
  the
  reference count should free the object.  This may be as simple as
  calling 'destroy' (for simple objects with no other allocated
  storage) or may require calling a separate routine to destroy the object.
  Because MPI uses 'MPI_xxx_free' to both decrement the reference count and
  free the object if the reference count is zero, we avoid the use of 'free'
  in the MPIR destruction routines.

  The 'inuse_ptr' approach is used rather than requiring the post-decrement
  value because, for reference-count semantics, all that is necessary is
  to know when the reference count reaches zero, and this can sometimes
  be implemented more cheaply that requiring the post-decrement value (e.g.,
  on IA32, there is an instruction for this operation).

  Question:
  Should we state that this is a macro so that we can use a register for
  the output value?  That avoids a store.  Alternately, have the macro
  return the value as if it was a function?

  Structure Definitions:
  The structure definitions in this document define `only` that part of
  a structure that may be used by code that is making use of the ADI.
  Thus, some structures, such as 'MPIR_Comm', have many defined fields;
  these are used to support MPI routines such as 'MPI_Comm_size' and
  'MPI_Comm_remote_group'.  Other structures may have few or no defined
  members; these structures have no fields used outside of the ADI.
  In C++ terms,  all members of these structures are 'private'.

  For the initial implementation, we expect that the structure definitions
  will be designed for the multimethod device.  However, all items that are
  specific to a particular device (including the multi-method device)
  will be placed at the end of the structure;
  the document will clearly identify the members that all implementations
  will provide.  This simplifies much of the code in both the ADI and the
  implementation of the MPI routines because structure member can be directly
  accessed rather than using some macro or C++ style method interface.

 T*/

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects such as 'MPI_Comm' or 'MPI_Datatype' are specified by
  integers (in the MPICH implementation); the MPI standard calls these
  handles.
  Out of range values are invalid; the value 0 is reserved.
  For most (with the possible exception of
  'MPI_Request' for performance reasons) MPI Opaque objects, the integer
  encodes both the kind of object (allowing runtime tests to detect a datatype
  passed where a communicator is expected) and important properties of the
  object.  Even the 'MPI_xxx_NULL' values should be encoded so that
  different null handles can be distinguished.  The details of the encoding
  of the handles is covered in more detail in the MPICH Design Document.
  For the most part, the ADI uses pointers to the underlying structures
  rather than the handles themselves.  However, each structure contains an
  'handle' field that is the corresponding integer handle for the MPI object.

  MPIR objects are not opaque.

  T*/

/* Known MPI object types.  These are used for both the error handlers
   and for the handles.  This is a 4 bit value.  0 is reserved for so
   that all-zero handles can be flagged as an error. */
/*E
  MPII_Object_kind - Object kind (communicator, window, or file)

  Notes:
  This enum is used by keyvals and errhandlers to indicate the type of
  object for which MPI opaque types the data is valid.  These are defined
  as bits to allow future expansion to the case where an object is value for
  multiple types (for example, we may want a universal error handler for
  errors return).  This is also used to indicate the type of MPI object a
  MPI handle represents.  It is an enum because only this applies only the
  the MPI and internal MPICH objects.

  'MPIR_VCONN' is a virtual connection; while this is not part of the
  overall ADI3 design, an object that manages connections to other processes
  is a common need, and 'MPIR_VCONN' may be used for that.

  Module:
  Attribute-DS
  E*/
typedef enum MPII_Object_kind {
    /* NOTE: if we modify these, make sure update mpi.h for all the builtin constants */
    MPIR_INTERNAL = 0x0,        /* used for various MPICH internal objects that
                                 * do not require a handle */
    MPIR_COMM = 0x1,
    MPIR_GROUP = 0x2,
    MPIR_DATATYPE = 0x3,
    MPIR_FILE = 0x4,    /* only used obliquely inside MPIR_Errhandler objs */
    MPIR_ERRHANDLER = 0x5,
    MPIR_OP = 0x6,
    MPIR_INFO = 0x7,
    MPIR_WIN = 0x8,
    MPIR_KEYVAL = 0x9,
    MPIR_ATTR = 0xa,
    MPIR_REQUEST = 0xb,
    MPIR_VCONN = 0xc,
    MPIR_GREQ_CLASS = 0xd,
    MPIR_SESSION = 0xe,
    MPIR_STREAM = 0xf,
} MPII_Object_kind;


#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) (((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT)
#define HANDLE_SET_MPI_KIND(a,kind) ((a) | ((kind) << HANDLE_MPI_KIND_SHIFT))

/* returns the name of the handle kind for debugging/logging purposes */
const char *MPIR_Handle_get_kind_str(int kind);

/* Handle types.  These are really 2 bits */
#define HANDLE_KIND_INVALID  0x0
#define HANDLE_KIND_BUILTIN  0x1
#define HANDLE_KIND_DIRECT   0x2
#define HANDLE_KIND_INDIRECT 0x3
/* Mask assumes that ints are at least 4 bytes */
#define HANDLE_KIND_MASK 0xc0000000
#define HANDLE_KIND_SHIFT 30
#define HANDLE_GET_KIND(a) (((unsigned)(a)&HANDLE_KIND_MASK)>>HANDLE_KIND_SHIFT)
#define HANDLE_SET_KIND(a,kind) ((a)|((kind)<<HANDLE_KIND_SHIFT))
#define HANDLE_IS_BUILTIN(a) (HANDLE_GET_KIND((a)) == HANDLE_KIND_BUILTIN)

/* For indirect, the remainder of the handle has a block and index within that
 * block */
#define HANDLE_INDIRECT_SHIFT 12
#define HANDLE_BLOCK(a) (((a)& 0x03FFF000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x00000FFF)

/* Number of blocks is between 1 and 16384 */
#if defined MPID_HANDLE_NUM_BLOCKS
#define HANDLE_NUM_BLOCKS MPID_HANDLE_NUM_BLOCKS
#else
#define HANDLE_NUM_BLOCKS 8192
#endif /* MPID_HANDLE_NUM_BLOCKS */

/* Number of objects in a block is bewtween 1 and 4096 (each obj has an index
 * within its block) */
#if defined MPID_HANDLE_NUM_INDICES
#define HANDLE_NUM_INDICES MPID_HANDLE_NUM_INDICES
#else
#define HANDLE_NUM_INDICES 1024
#endif /* MPID_HANDLE_NUM_INDICES */

/* For direct, the remainder of the handle is the index into a predefined
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* Define N_BUILTIN and PREALLOC for all kinds of objects */
#define MPIR_COMM_N_BUILTIN 3
#ifdef MPID_COMM_PREALLOC
#define MPIR_COMM_PREALLOC MPID_COMM_PREALLOC
#else
#define MPIR_COMM_PREALLOC 8
#endif

#define MPIR_GROUP_N_BUILTIN 1
#ifdef MPID_GROUP_PREALLOC
#define MPIR_GROUP_PREALLOC MPID_GROUP_PREALLOC
#else
#define MPIR_GROUP_PREALLOC 8
#endif

#define MPIR_DATATYPE_N_BUILTIN 71
#ifdef MPID_DATATYPE_PREALLOC
#define MPIR_DATATYPE_PREALLOC MPID_DATATYPE_PREALLOC
#else
#define MPIR_DATATYPE_PREALLOC 8
#endif

#define MPIR_ERRHANDLER_N_BUILTIN 4
#ifdef MPID_ERRHANDLER_PREALLOC
#define MPIR_ERRHANDLER_PREALLOC MPID_ERRHANDLER_PREALLOC
#else
#define MPIR_ERRHANDLER_PREALLOC 8
#endif

#define MPIR_INFO_N_BUILTIN 2
#ifdef MPID_INFO_PREALLOC
#define MPIR_INFO_PREALLOC MPID_INFO_PREALLOC
#else
#define MPIR_INFO_PREALLOC 8
#endif

#define MPIR_OP_N_BUILTIN 15
#ifdef MPID_OP_PREALLOC
#define MPIR_OP_PREALLOC MPID_OP_PREALLOC
#else
#define MPIR_OP_PREALLOC 16
#endif

#define MPIR_REQUEST_N_BUILTIN 0x11
#define MPIR_REQUEST_PREALLOC 8

#ifdef MPID_ATTR_PREALLOC
#define MPIR_ATTR_PREALLOC MPID_ATTR_PREALLOC
#else
#define MPIR_ATTR_PREALLOC 32
#endif

#ifdef MPID_SESSION_PREALLOC
#define MPIR_SESSION_PREALLOC MPID_SESSION_PREALLOC
#else
#define MPIR_SESSION_PREALLOC 2
#endif

#ifdef MPID_GREQ_CLASS_PREALLOC
#define MPIR_GREQ_CLASS_PREALLOC MPID_GREQ_CLASS_PREALLOC
#else
#define MPIR_GREQ_CLASS_PREALLOC 2
#endif

#ifdef MPID_WIN_PREALLOC
#define MPIR_WIN_PREALLOC MPID_WIN_PREALLOC
#else
#define MPIR_WIN_PREALLOC 8
#endif

#ifdef MPID_STREAM_PREALLOC
#define MPIR_STREAM_PREALLOC MPID_STREAM_PREALLOC
#else
#define MPIR_STREAM_PREALLOC 8
#endif

#if defined (MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIR_DBG_HANDLE;
#endif /* MPL_USE_DBG_LOGGING */

/* ------------------------------------------------------------------------- */
/* reference counting macros */

/* If we're debugging the handles (including reference counts),
   add an additional test.  The check on a max refcount helps to
   detect objects whose refcounts are not decremented as many times
   as they are incremented */
#ifdef MPICH_DEBUG_HANDLES
#define MPICH_DEBUG_MAX_REFCOUNT 64
#define HANDLE_CHECK_REFCOUNT(objptr_,local_ref_count_,op_)             \
    do {                                                                \
        if (local_ref_count_ > MPICH_DEBUG_MAX_REFCOUNT || local_ref_count_ < 0) \
        {                                                               \
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE,TYPICAL,(MPL_DBG_FDEST,     \
                                                     "Invalid refcount (%d) in %p (0x%08x) %s", \
                                                     local_ref_count_, (objptr_), (objptr_)->handle, op_)); \
        }                                                               \
        MPIR_Assert(local_ref_count_ >= 0);                             \
    } while (0)
#else
#define HANDLE_CHECK_REFCOUNT(objptr_,local_ref_count_,op_)     \
    MPIR_Assert(local_ref_count_ >= 0)
#endif

#define HANDLE_LOG_REFCOUNT_CHANGE(objptr_, new_refcount_, action_str_) \
    MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE,TYPICAL,(MPL_DBG_FDEST,             \
                                             "%s %p (0x%08x kind=%s) refcount to %d", \
                                             (action_str_),             \
                                             (objptr_),                 \
                                             (objptr_)->handle,         \
                                             MPIR_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                             new_refcount_))

/* The "_always" versions of these macros unconditionally manipulate the
 * reference count of the given object.  They exist to permit an optimization
 * of not reference counting predefined objects. */

/* The MPL_DBG... statements are macros that vanish unless
   --enable-g=log is selected.  HANDLE_CHECK_REFCOUNT is
   defined above, and adds an additional sanity check for the refcounts
*/
#if MPICH_THREAD_REFCOUNT == MPICH_REFCOUNT__NONE

typedef int Handle_ref_count;

#define MPIR_Object_set_ref(objptr_,val)                        \
    do {                                                        \
        (objptr_)->ref_count = val;                             \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, val, "set");        \
    } while (0)

/* must be used with care, since there is no synchronization for this read */
#define MPIR_Object_get_ref(objptr_)            \
    ((objptr_)->ref_count)

#define MPIR_Object_add_ref_always(objptr_)                             \
    do {                                                                \
        (objptr_)->ref_count++;                                         \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, (objptr_)->ref_count, "incr"); \
        HANDLE_CHECK_REFCOUNT(objptr_,(objptr_)->ref_count,"incr");     \
    } while (0)
#define MPIR_Object_release_ref_always(objptr_,inuse_ptr)               \
    do {                                                                \
        *(inuse_ptr) = --((objptr_)->ref_count);                        \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, (objptr_)->ref_count, "decr"); \
        HANDLE_CHECK_REFCOUNT(objptr_,(objptr_)->ref_count,"decr");     \
    } while (0)

#elif MPICH_THREAD_REFCOUNT == MPICH_REFCOUNT__LOCKFREE

typedef MPL_atomic_int_t Handle_ref_count;

#define MPIR_Object_set_ref(objptr_,val)                        \
    do {                                                        \
        MPL_atomic_store_int(&(objptr_)->ref_count, val); \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, val, "set");        \
    } while (0)

/* must be used with care, since there is no synchronization for this read */
#define MPIR_Object_get_ref(objptr_) \
    (MPL_atomic_load_int(&(objptr_)->ref_count))

#ifdef MPICH_DEBUG_HANDLES
/*
  For non-debug builds, we use non-fetch atomics here, because they may be
  slightly faster than fetch versions, and we don't care about exact value
  of the refcount (other than whether it hit zero.)
  For debug builds (when MPICH_DEBUG_HANDLES is set), we need fetch atomics
  in order to know the correct refcount value when multiple threads present.
*/

/* MPICH_THREAD_REFCOUNT == MPICH_REFCOUNT__LOCKFREE && MPICH_DEBUG_HANDLES */
#define MPIR_Object_add_ref_always(objptr_)                             \
    do {                                                                \
        int new_ref_;                                                   \
        new_ref_ = MPL_atomic_fetch_add_int(&((objptr_)->ref_count), 1) + 1; \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, new_ref_, "incr");          \
        HANDLE_CHECK_REFCOUNT(objptr_,new_ref_,"incr");                 \
    } while (0)
#define MPIR_Object_release_ref_always(objptr_,inuse_ptr)                      \
    do {                                                                       \
        int new_ref_;                                                          \
        /* If it is 1, we will just free it without a heavy atomic operation.  \
         * Note that any concurrent add_ref() to a handle whose count is 1 is  \
         * illegal and we do not consider.                                     \
         *                                                                     \
         * The similar optimization caused an error and was reverted.          \
         * (See a5686ec3c42f0357119cab7f21df46389c7acec8)                      \
         * The following uses acquire_load() instead of OPA_load_int(). */     \
        if (MPL_atomic_acquire_load_int(&((objptr_)->ref_count)) == 1) {       \
            MPL_atomic_relaxed_store_int(&((objptr_)->ref_count), 0);          \
            new_ref_ = 0;                                                      \
            *(inuse_ptr) = 0;                                                  \
        } else {                                                               \
            new_ref_ =                                                         \
                MPL_atomic_fetch_sub_int(&((objptr_)->ref_count), 1) - 1;      \
            *(inuse_ptr) = new_ref_;                                           \
        }                                                                      \
        HANDLE_LOG_REFCOUNT_CHANGE(objptr_, new_ref_, "decr");                 \
        HANDLE_CHECK_REFCOUNT(objptr_,new_ref_,"decr");                        \
    } while (0)
#else /* MPICH_DEBUG_HANDLES */
/* MPICH_THREAD_REFCOUNT == MPICH_REFCOUNT__LOCKFREE && !MPICH_DEBUG_HANDLES */
#define MPIR_Object_add_ref_always(objptr_)     \
    do {                                        \
        MPL_atomic_fetch_add_int(&((objptr_)->ref_count), 1);  \
    } while (0)
#define MPIR_Object_release_ref_always(objptr_,inuse_ptr)                   \
    do {                                                                    \
        /* If it is 1, we will free it without a heavy atomic operation. */ \
        if (MPL_atomic_acquire_load_int(&((objptr_)->ref_count)) == 1) {    \
            MPL_atomic_relaxed_store_int(&((objptr_)->ref_count), 0);       \
            *(inuse_ptr) = 0;                                               \
        } else {                                                            \
            int new_ref_ =                                                  \
                MPL_atomic_fetch_sub_int(&((objptr_)->ref_count), 1) - 1;   \
            *(inuse_ptr) = new_ref_;                                        \
        }                                                                   \
    } while (0)
#endif /* MPICH_DEBUG_HANDLES */
#else
#error invalid value for MPICH_THREAD_REFCOUNT
#endif

/* TODO someday we should probably always suppress predefined object refcounting,
 * but we don't have total confidence in it yet.  So until we gain sufficient
 * confidence, this is a configurable option. */
#if defined(MPICH_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS)

/* The assumption here is that objects with handles of type HANDLE_KIND_BUILTIN
 * will be created/destroyed only at MPI_Init/MPI_Finalize time and don't need
 * to be reference counted.  This can be a big performance win on some
 * platforms, such as BG/P.
 *
 * It is also assumed that any object being reference counted via these macros
 * will have a valid value in the handle field, even if it is
 * HANDLE_SET_KIND(0, HANDLE_KIND_INVALID) */
/* TODO profile and examine the assembly that is generated for this if () on Blue
 * Gene (and elsewhere).  We may need to mark it unlikely(). */
#define MPIR_Object_add_ref(objptr_)                                    \
    do {                                                                \
        int handle_kind_ = HANDLE_GET_KIND((objptr_)->handle);          \
        if (unlikely(handle_kind_ != HANDLE_KIND_BUILTIN)) {            \
            MPIR_Object_add_ref_always((objptr_));                      \
        }                                                               \
        else {                                                          \
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE,TYPICAL,(MPL_DBG_FDEST,     \
                                                     "skipping add_ref on %p (0x%08x kind=%s) refcount=%d", \
                                                     (objptr_),         \
                                                     (objptr_)->handle, \
                                                     MPIR_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                                     MPIR_Object_get_ref(objptr_))) \
                }                                                       \
    } while (0)
#define MPIR_Object_release_ref(objptr_,inuse_ptr_)                     \
    do {                                                                \
        int handle_kind_ = HANDLE_GET_KIND((objptr_)->handle);          \
        if (unlikely(handle_kind_ != HANDLE_KIND_BUILTIN)) {            \
            MPIR_Object_release_ref_always((objptr_), (inuse_ptr_));    \
        }                                                               \
        else {                                                          \
            *(inuse_ptr_) = 1;                                          \
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE,TYPICAL,(MPL_DBG_FDEST,     \
                                                     "skipping release_ref on %p (0x%08x kind=%s) refcount=%d", \
                                                     (objptr_),         \
                                                     (objptr_)->handle, \
                                                     MPIR_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                                     MPIR_Object_get_ref(objptr_))) \
                }                                                       \
    } while (0)

#else /* !defined(MPICH_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS) */

/* the base case, where we just always manipulate the reference counts */
#define MPIR_Object_add_ref(objptr_)            \
    MPIR_Object_add_ref_always((objptr_))
#define MPIR_Object_release_ref(objptr_,inuse_ptr_)             \
    MPIR_Object_release_ref_always((objptr_),(inuse_ptr_))

#endif


/* end reference counting macros */
/* ------------------------------------------------------------------------- */

/* This macro defines structure fields that are needed in order to use the
 * reference counting and object allocation macros/functions in MPICH.  This
 * allows us to avoid casting and violating C's strict aliasing rules in most
 * cases.
 *
 * All *active* (in use) objects have the handle as the first value; objects
 * with reference counts have the reference count as the second value.  See
 * MPIR_Object_add_ref and MPIR_Object_release_ref.
 *
 * NOTE: This macro *must* be invoked as the very first element of the structure! */
#define MPIR_OBJECT_HEADER                                              \
    int handle;                                                         \
    Handle_ref_count ref_count  /*semicolon intentionally omitted */

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects
   have MPIR_Handle_common as the head */
typedef struct MPIR_Handle_common {
    MPIR_OBJECT_HEADER;
    void *next;                 /* Free handles use this field to point to the next
                                 * free object */
} MPIR_Handle_common;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIR_Object_alloc_t {
    MPIR_Handle_common *avail;  /* Next available object */
    int initialized;            /* */
    void **indirect;            /* Pointer to indirect object blocks */
    int indirect_size;          /* Number of allocated indirect blocks */
    int num_allocated;          /* Total capacity of this allocator including both
                                 * direct and indirect */
    int num_avail;              /* Number of available objects including both direct and indirect */
    MPII_Object_kind kind;      /* Kind of object this is for */
    int size;                   /* Size of an individual object */
    void *direct;               /* Pointer to direct block, used
                                 * for allocation */
    int direct_size;            /* Size of direct block */
    void *lock;                 /* lower-layer may register a lock to use. This is
                                 * mostly for multipool requests. For other objects
                                 * or not per-vci thread granularity, this lock
                                 * pointer is ignored. Ref. mpir_request.h.
                                 * NOTE: it is `void *` because mutex type not defined yet.
                                 */
    /* The following padding is to avoid cache line sharing with other MPIR_Object_alloc_t.  This
     * padding is particularly important for an array of per-vci MPI_Request pools. */
    char pad[MPL_CACHELINE_SIZE];
} MPIR_Object_alloc_t;
static inline void *MPIR_Handle_obj_alloc(MPIR_Object_alloc_t *);
void *MPIR_Info_handle_obj_alloc(MPIR_Object_alloc_t *);
static inline void *MPIR_Handle_obj_alloc_unsafe(MPIR_Object_alloc_t *,
                                                 int max_blocks, int max_indices);
static inline void MPIR_Handle_obj_free(MPIR_Object_alloc_t *, void *);
void MPIR_Info_handle_obj_free(MPIR_Object_alloc_t *, void *);
static inline void MPIR_Handle_obj_free_unsafe(MPIR_Object_alloc_t *, void *, bool is_info);
static inline void *MPIR_Handle_get_ptr_indirect(int, MPIR_Object_alloc_t *);


/* Convert Handles to objects for MPI types that have predefined objects */
/* TODO examine generated assembly for this construct, it's probably suboptimal
 * on Blue Gene.  An if/else if/else might help the compiler out.  It also lets
 * us hint that one case is likely(), usually the BUILTIN case. */
#define MPIR_Getb_ptr(kind,KIND,a,bmsk,ptr)                             \
    {                                                                   \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_BUILTIN:                                       \
            MPIR_Assert(((a)&(bmsk)) < MPIR_##KIND##_N_BUILTIN);        \
            ptr=MPIR_##kind##_builtin+((a)&(bmsk));                     \
            break;                                                      \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_##KIND##_PREALLOC);      \
            ptr=MPIR_##kind##_direct+HANDLE_INDEX(a);                   \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr=((MPIR_##kind*)                                         \
                 MPIR_Handle_get_ptr_indirect(a,&MPIR_##kind##_mem));   \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        default:                                                        \
            ptr=0;                                                      \
            break;                                                      \
        }                                                               \
    }

/* Convert handles to objects for MPI types that do _not_ have any predefined
   objects */
#define MPIR_Get_ptr(kind,a,ptr)                                        \
    {                                                                   \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                        \
            ptr=MPIR_##kind##_direct+HANDLE_INDEX(a);                   \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr=((MPIR_##kind*)                                         \
                 MPIR_Handle_get_ptr_indirect(a,&MPIR_##kind##_mem));   \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            ptr=0;                                                      \
            break;                                                      \
        }                                                               \
    }

/* FIXME: the masks should be defined with the handle definitions instead
   of inserted here as literals */
#define MPIR_Comm_get_ptr(a,ptr)       MPIR_Getb_ptr(Comm,COMM,a,0x03ffffff,ptr)
#define MPIR_Group_get_ptr(a,ptr)      MPIR_Getb_ptr(Group,GROUP,a,0x03ffffff,ptr)
#define MPIR_Errhandler_get_ptr(a,ptr) MPIR_Getb_ptr(Errhandler,ERRHANDLER,a,0x3,ptr)
#define MPIR_Op_get_ptr(a,ptr)         MPIR_Getb_ptr(Op,OP,a,0x000000ff,ptr)
#define MPIR_Info_get_ptr(a,ptr)       MPIR_Getb_ptr(Info,INFO,a,0x03ffffff,ptr)
#define MPIR_Win_get_ptr(a,ptr)        MPIR_Get_ptr(Win,a,ptr)
#define MPIR_Session_get_ptr(a,ptr)    MPIR_Get_ptr(Session,a,ptr)
#define MPIR_Stream_get_ptr(a,ptr)     MPIR_Get_ptr(Stream,a,ptr)
/* Request objects are handled differently. See mpir_request.h */
#define MPIR_Grequest_class_get_ptr(a,ptr) MPIR_Get_ptr(Grequest_class,a,ptr)
/* Keyvals have a special format. This is roughly MPIR_Get_ptrb, but
   the handle index is in a smaller bit field.  In addition,
   there is no storage for the builtin keyvals.
   For the indirect case, we mask off the part of the keyval that is
   in the bits normally used for the indirect block index.
*/
#define MPII_Keyval_get_ptr(a,ptr)                                      \
    {                                                                   \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_BUILTIN:                                       \
            ptr=0;                                                      \
            break;                                                      \
        case HANDLE_KIND_DIRECT:                                        \
            ptr=MPII_Keyval_direct+((a)&0x3fffff);                      \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr=((MPII_Keyval*)                                         \
                 MPIR_Handle_get_ptr_indirect((a)&0xfc3fffff,&MPII_Keyval_mem)); \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        default:                                                        \
            ptr=0;                                                      \
            break;                                                      \
        }                                                               \
    }

#endif /* MPIR_OBJECTS_H_INCLUDED */
