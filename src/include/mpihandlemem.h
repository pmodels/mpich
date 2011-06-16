/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIHANDLE_H_INCLUDED
#define MPIHANDLE_H_INCLUDED

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects such as 'MPI_Comm' or 'MPI_Datatype' are specified by 
  integers (in the MPICH2 implementation); the MPI standard calls these
  handles.  
  Out of range values are invalid; the value 0 is reserved.
  For most (with the possible exception of 
  'MPI_Request' for performance reasons) MPI Opaque objects, the integer
  encodes both the kind of object (allowing runtime tests to detect a datatype
  passed where a communicator is expected) and important properties of the 
  object.  Even the 'MPI_xxx_NULL' values should be encoded so that 
  different null handles can be distinguished.  The details of the encoding
  of the handles is covered in more detail in the MPICH2 Design Document.
  For the most part, the ADI uses pointers to the underlying structures
  rather than the handles themselves.  However, each structure contains an 
  'handle' field that is the corresponding integer handle for the MPI object.

  MPID objects (objects used within the implementation of MPI) are not opaque.

  T*/

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
/*E
  MPID_Object_kind - Object kind (communicator, window, or file)

  Notes:
  This enum is used by keyvals and errhandlers to indicate the type of
  object for which MPI opaque types the data is valid.  These are defined
  as bits to allow future expansion to the case where an object is value for
  multiple types (for example, we may want a universal error handler for 
  errors return).  This is also used to indicate the type of MPI object a 
  MPI handle represents.  It is an enum because only this applies only the
  the MPI and internal MPICH2 objects.

  The 'MPID_PROCGROUP' kind is used to manage process groups (different
  from MPI Groups) that are used to keep track of collections of
  processes (each 'MPID_PROCGROUP' corresponds to a group of processes
  that define an 'MPI_COMM_WORLD'.  This becomes important only 
  when MPI-2 dynamic process features are supported.  'MPID_VCONN' is
  a virtual connection; while this is not part of the overall ADI3
  design, an object that manages connections to other processes is
  a common need, and 'MPID_VCONN' may be used for that.

  Module:
  Attribute-DS
  E*/
typedef enum MPID_Object_kind { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,               /* This is not used */
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9,
  MPID_ATTR       = 0xa,
  MPID_REQUEST    = 0xb,
  MPID_PROCGROUP  = 0xc,               /* These are internal device objects */
  MPID_VCONN      = 0xd,
  MPID_GREQ_CLASS = 0xf
  } MPID_Object_kind;

#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) ( ((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT )
#define HANDLE_SET_MPI_KIND(a,kind) ((a) | ((kind) << HANDLE_MPI_KIND_SHIFT))

/* returns the name of the handle kind for debugging/logging purposes */
const char *MPIU_Handle_get_kind_str(int kind);

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

/* For indirect, the remainder of the handle has a block and index */
#define HANDLE_INDIRECT_SHIFT 16
#define HANDLE_BLOCK(a) (((a)& 0x03FF0000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x0000FFFF)

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* ------------------------------------------------------------------------- */
/* reference counting macros */

/* If we're debugging the handles (including reference counts),
   add an additional test.  The check on a max refcount helps to
   detect objects whose refcounts are not decremented as many times
   as they are incremented */
#ifdef MPICH_DEBUG_HANDLES
#define MPICH_DEBUG_MAX_REFCOUNT 64
#define MPIU_HANDLE_CHECK_REFCOUNT(objptr_,op_)                                                     \
    do {                                                                                            \
        int local_ref_count_ = MPIU_Object_get_ref(objptr_);                                        \
        if (local_ref_count_ > MPICH_DEBUG_MAX_REFCOUNT || local_ref_count_ < 0)                    \
        {                                                                                           \
            MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,                                        \
                                             "Invalid refcount (%d) in %p (0x%08x) %s",             \
                                             local_ref_count_, (objptr_), (objptr_)->handle, op_)); \
        }                                                                                           \
        MPIU_Assert(local_ref_count_ < 0);                                                          \
    } while (0)
#else
#define MPIU_HANDLE_CHECK_REFCOUNT(objptr_,op_) \
    MPIU_Assert(MPIU_Object_get_ref(objptr_) >= 0)
#endif

#define MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, action_str_)                                          \
    MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,                                                   \
                                     "%s %p (0x%08x kind=%s) refcount to %d",                          \
                                     (action_str_),                                                    \
                                     (objptr_),                                                        \
                                     (objptr_)->handle,                                                \
                                     MPIU_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                     MPIU_Object_get_ref(objptr_)))


/*M
   MPIU_Object_add_ref - Increment the reference count for an MPI object

   Synopsis:
.vb
    MPIU_Object_add_ref( MPIU_Object *ptr )
.ve

   Input Parameter:
.  ptr - Pointer to the object.

   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic increment operation, using, for
   example, a lock on datatypes or special assembly code.
M*/
/*M
   MPIU_Object_release_ref - Decrement the reference count for an MPI object

   Synopsis:
.vb
   MPIU_Object_release_ref( MPIU_Object *ptr, int *inuse_ptr )
.ve

   Input Parameter:
.  objptr - Pointer to the object.

   Output Parameter:
.  inuse_ptr - Pointer to the value of the reference count after decrementing.
   This value is either zero or non-zero. See below for details.

   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic decrement operation, using, for
   example, a lock on datatypes or special assembly code.

   Once the reference count is decremented to zero, it is an error to
   change it.  A correct MPI program will never do that, but an incorrect one
   (particularly a multithreaded program with a race condition) might.

   The following code is `invalid`\:
.vb
   MPIU_Object_release_ref( datatype_ptr );
   if (datatype_ptr->ref_count == 0) MPID_Datatype_free( datatype_ptr );
.ve
   In a multi-threaded implementation, the value of 'datatype_ptr->ref_count'
   may have been changed by another thread, resulting in both threads calling
   'MPID_Datatype_free'.  Instead, use
.vb
   MPIU_Object_release_ref( datatype_ptr, &inUse );
   if (!inuse)
       MPID_Datatype_free( datatype_ptr );
.ve
  M*/

/* The "_always" versions of these macros unconditionally manipulate the
 * reference count of the given object.  They exist to permit an optimization
 * of not reference counting predefined objects. */

/* The MPIU_DBG... statements are macros that vanish unless
   --enable-g=log is selected.  MPIU_HANDLE_CHECK_REFCOUNT is
   defined above, and adds an additional sanity check for the refcounts
*/
#if MPIU_THREAD_REFCOUNT == MPIU_REFCOUNT_NONE

typedef int MPIU_Handle_ref_count;
#define MPIU_HANDLE_REF_COUNT_INITIALIZER(val_) (val_)

#define MPIU_Object_set_ref(objptr_,val)                 \
    do {                                                 \
        (objptr_)->ref_count = val;                      \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "set"); \
    } while (0)

/* must be used with care, since there is no synchronization for this read */
#define MPIU_Object_get_ref(objptr_) \
    ((objptr_)->ref_count)

#define MPIU_Object_add_ref_always(objptr_)               \
    do {                                                  \
        (objptr_)->ref_count++;                           \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "incr"); \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"incr");       \
    } while (0)
#define MPIU_Object_release_ref_always(objptr_,inuse_ptr) \
    do {                                                  \
        *(inuse_ptr) = --((objptr_)->ref_count);          \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "decr"); \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"decr");       \
    } while (0)

#elif MPIU_THREAD_REFCOUNT == MPIU_REFCOUNT_LOCK

typedef volatile int MPIU_Handle_ref_count;
#define MPIU_HANDLE_REF_COUNT_INITIALIZER(val_) (val_)

#define MPIU_Object_set_ref(objptr_,val)                 \
    do {                                                 \
        MPIU_THREAD_CS_ENTER(HANDLE,objptr_);            \
        (objptr_)->ref_count = val;                      \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "set"); \
        MPIU_THREAD_CS_EXIT(HANDLE,objptr_);             \
    } while (0)

/* must be used with care, since there is no synchronization for this read */
#define MPIU_Object_get_ref(objptr_) \
    ((objptr_)->ref_count)

#define MPIU_Object_add_ref_always(objptr_)               \
    do {                                                  \
        MPIU_THREAD_CS_ENTER(HANDLE,objptr_);             \
        (objptr_)->ref_count++;                           \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "incr"); \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"incr");       \
        MPIU_THREAD_CS_EXIT(HANDLE,objptr_);              \
    } while (0)
#define MPIU_Object_release_ref_always(objptr_,inuse_ptr) \
    do {                                                  \
        MPIU_THREAD_CS_ENTER(HANDLE,objptr_);             \
        *(inuse_ptr) = --((objptr_)->ref_count);          \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "decr"); \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"decr");       \
        MPIU_THREAD_CS_EXIT(HANDLE,objptr_);              \
    } while (0)

#elif MPIU_THREAD_REFCOUNT == MPIU_REFCOUNT_LOCKFREE

#include "opa_primitives.h"
typedef OPA_int_t MPIU_Handle_ref_count;
#define MPIU_HANDLE_REF_COUNT_INITIALIZER(val_) OPA_INT_T_INITIALIZER(val_)

#define MPIU_Object_set_ref(objptr_,val)                 \
    do {                                                 \
        OPA_store_int(&(objptr_)->ref_count, val);       \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "set"); \
    } while (0)

/* must be used with care, since there is no synchronization for this read */
#define MPIU_Object_get_ref(objptr_) \
    (OPA_load_int(&(objptr_)->ref_count))

#define MPIU_Object_add_ref_always(objptr_)               \
    do {                                                  \
        OPA_incr_int(&((objptr_)->ref_count));            \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "incr"); \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"incr");       \
    } while (0)
#define MPIU_Object_release_ref_always(objptr_,inuse_ptr)               \
    do {                                                                \
        int got_zero_ = OPA_decr_and_test_int(&((objptr_)->ref_count)); \
        *(inuse_ptr) = got_zero_ ? 0 : 1;                               \
        MPIU_HANDLE_LOG_REFCOUNT_CHANGE(objptr_, "decr");               \
        MPIU_HANDLE_CHECK_REFCOUNT(objptr_,"decr");                     \
    } while (0)
#else
#error invalid value for MPIU_THREAD_REFCOUNT
#endif

/* TODO someday we should probably always suppress predefined object refcounting,
 * but we don't have total confidence in it yet.  So until we gain sufficient
 * confidence, this is a configurable option. */
#if defined(MPIU_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS)

/* The assumption here is that objects with handles of type HANDLE_KIND_BUILTIN
 * will be created/destroyed only at MPI_Init/MPI_Finalize time and don't need
 * to be reference counted.  This can be a big performance win on some
 * platforms, such as BG/P.
 *
 * It is also assumed that any object being reference counted via these macros
 * will have a valid value in the handle field, even if it is
 * HANDLE_SET_KIND(0, HANDLE_KIND_INVALID) */
/* TODO profile and examine the assembly that is generated for this if() on Blue
 * Gene (and elsewhere).  We may need to mark it unlikely(). */
#define MPIU_Object_add_ref(objptr_)                           \
    do {                                                       \
        int handle_kind_ = HANDLE_GET_KIND((objptr_)->handle); \
        if (handle_kind_ != HANDLE_KIND_BUILTIN) {             \
            MPIU_Object_add_ref_always((objptr_));             \
        }                                                      \
        else {                                                                                                 \
            MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,                                                   \
                                             "skipping add_ref on %p (0x%08x kind=%s) refcount=%d",            \
                                             (objptr_),                                                        \
                                             (objptr_)->handle,                                                \
                                             MPIU_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                             MPIU_Object_get_ref(objptr_)))                                    \
        }                                                                                                      \
    } while (0)
#define MPIU_Object_release_ref(objptr_,inuse_ptr_)                  \
    do {                                                             \
        int handle_kind_ = HANDLE_GET_KIND((objptr_)->handle);       \
        if (handle_kind_ != HANDLE_KIND_BUILTIN) {                   \
            MPIU_Object_release_ref_always((objptr_), (inuse_ptr_)); \
        }                                                            \
        else {                                                       \
            *(inuse_ptr_) = 1;                                       \
            MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,                                                   \
                                             "skipping release_ref on %p (0x%08x kind=%s) refcount=%d",        \
                                             (objptr_),                                                        \
                                             (objptr_)->handle,                                                \
                                             MPIU_Handle_get_kind_str(HANDLE_GET_MPI_KIND((objptr_)->handle)), \
                                             MPIU_Object_get_ref(objptr_)))                                    \
        }                                                            \
    } while (0)

#else /* !defined(MPIU_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS) */

/* the base case, where we just always manipulate the reference counts */
#define MPIU_Object_add_ref(objptr_) \
    MPIU_Object_add_ref_always((objptr_))
#define MPIU_Object_release_ref(objptr_,inuse_ptr_) \
    MPIU_Object_release_ref_always((objptr_),(inuse_ptr_))

#endif


/* end reference counting macros */
/* ------------------------------------------------------------------------- */

/* This macro defines structure fields that are needed in order to use the
 * reference counting and object allocation macros/functions in MPICH2.  This
 * allows us to avoid casting and violating C's strict aliasing rules in most
 * cases.
 *
 * All *active* (in use) objects have the handle as the first value; objects
 * with referene counts have the reference count as the second value.  See
 * MPIU_Object_add_ref and MPIU_Object_release_ref.
 *
 * NOTE: This macro *must* be invoked as the very first element of the structure! */
#define MPIU_OBJECT_HEADER             \
    int handle;                        \
    MPIU_THREAD_OBJECT_HOOK/*no-semi*/ \
    MPIU_Handle_ref_count ref_count/*semicolon intentionally omitted*/

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct MPIU_Handle_common {
    MPIU_OBJECT_HEADER;
    void *next;   /* Free handles use this field to point to the next
                     free object */
} MPIU_Handle_common;

/* Provides a type to which a specific object structure can be casted.  In
 * general this should not be used, since most uses are violations of C's strict
 * aliasing rules. */
typedef struct MPIU_Handle_head {
    MPIU_OBJECT_HEADER;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIU_Object_alloc_t {
    MPIU_Handle_common *avail;          /* Next available object */
    int                initialized;     /* */
    void              *(*indirect)[];   /* Pointer to indirect object blocks */
    int                indirect_size;   /* Number of allocated indirect blocks */
    MPID_Object_kind   kind;            /* Kind of object this is for */
    int                size;            /* Size of an individual object */
    void               *direct;         /* Pointer to direct block, used 
                                           for allocation */
    int                direct_size;     /* Size of direct block */
} MPIU_Object_alloc_t;
extern void *MPIU_Handle_obj_alloc(MPIU_Object_alloc_t *);
extern void *MPIU_Handle_obj_alloc_unsafe(MPIU_Object_alloc_t *);
extern void MPIU_Handle_obj_alloc_complete(MPIU_Object_alloc_t *, int);
extern void MPIU_Handle_obj_free( MPIU_Object_alloc_t *, void * );
void *MPIU_Handle_get_ptr_indirect( int, MPIU_Object_alloc_t * );
extern void *MPIU_Handle_direct_init(void *direct, int direct_size, 
				     int obj_size, int handle_type);
int MPIU_Handle_obj_outstanding(const MPIU_Object_alloc_t *objmem);
#endif
