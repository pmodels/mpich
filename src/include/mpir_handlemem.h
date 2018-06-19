/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_HANDLEMEM_H_INCLUDED
#define MPIR_HANDLEMEM_H_INCLUDED

#include "mpichconfconst.h"
#include "mpichconf.h"
#include "mpir_objects.h"
#include "mpir_thread.h"

#ifdef MPICH_DEBUG_HANDLEALLOC
int MPIR_check_handles_on_finalize(void *objmem_ptr);
#endif

/* This is the utility file for info that contains routines used to
   manage the arrays used to store handle objects.

   To use these routines, allocate the following in a utility file
   used by each object (e.g., info, datatype, comm, group, ...).
   (The comment format // is used in this example but the usual
   C comment convention should be used of course.)  The usage is described
   below.

   // Declarations begin here
   // Static declaration of the information about the block
   // Define the number of preallocated entries # omitted)
   define MPID_<OBJ>_PREALLOC 256
   MPIR_Object_alloc_t MPID_<obj>_mem = { 0, 0, 0, 0, MPID_<obj>,
                                          sizeof(MPID_<obj>), MPID_<obj>_direct,
                                          MPID_<OBJ>_PREALLOC, };

   // Preallocated objects
   MPID_<obj> MPID_<obj>_direct[MPID_<OBJ>_PREALLOC];
   static int initialized = 0;

   // Next available object
   static int MPID_<obj> *avail = 0;

   // Extension (indirect) objects
   static MPID_<obj> *(*MPID_<obj>_indirect)[] = 0;
   static int MPID_<obj>_indirect_size = 0;
    // Declarations end here

   These provide for lazy initialization; applications that do not need a
   particular object will not include any of the code or even reference
   the space.

   Note that these routines are designed for the MPI objects, and include the
   creation of a "handle" that is appropriate for the MPI object value.

   None of these routines is thread-safe.  Any routine that uses them
   must ensure that only one thread at a time may call them.
*/

/* This routine is called by finalize when MPI exits */
static inline int MPIR_Handle_free(void *((*indirect)[]), int indirect_size)
{
    int i;

    /* Remove any allocated storage */
    for (i = 0; i < indirect_size; i++) {
        MPL_free((*indirect)[i]);
    }
    if (indirect) {
        MPL_free(indirect);
    }
    /* This does *not* remove any objects that the user created
     * and then did not destroy */
    return 0;
}

#if defined(MPL_VG_AVAILABLE)
#define HANDLE_VG_LABEL(objptr_, objsize_, handle_type_, is_direct_)                             \
    do {                                                                                         \
        if (MPL_VG_RUNNING_ON_VALGRIND()) {                                                      \
            char desc_str[256];                                                                  \
            MPL_snprintf(desc_str, sizeof(desc_str)-1,                                           \
                          "[MPICH handle: objptr=%p handle=0x%x %s/%s]",                         \
                          (objptr_), (objptr_)->handle,                                          \
                          ((is_direct_) ? "DIRECT" : "INDIRECT"),                                \
                          MPIR_Handle_get_kind_str(handle_type_));                               \
            /* we don't keep track of the block descriptor because the handle */                 \
            /* values never change once allocated */                                             \
            MPL_VG_CREATE_BLOCK((objptr_), (objsize_), desc_str);                                \
        }                                                                                        \
    } while (0)
#else
#define HANDLE_VG_LABEL(objptr_, objsize_, handle_type_, is_direct_) do {} while (0)
#endif

static inline void *MPIR_Handle_direct_init(void *direct,
                                            int direct_size, int obj_size, int handle_type)
{
    int i;
    MPIR_Handle_common *hptr = 0;
    char *ptr = (char *) direct;

    for (i = 0; i < direct_size; i++) {
        /* printf("Adding %p in %d\n", ptr, handle_type); */
        /* First cast to (void*) to avoid false warnings about alignment
         * (consider that a requirement of the input parameters) */
        hptr = (MPIR_Handle_common *) (void *) ptr;
        ptr = ptr + obj_size;
        hptr->next = ptr;
        hptr->handle = ((unsigned) HANDLE_KIND_DIRECT << HANDLE_KIND_SHIFT) |
            (handle_type << HANDLE_MPI_KIND_SHIFT) | i;

        HANDLE_VG_LABEL(hptr, obj_size, handle_type, 1);
    }

    if (hptr)
        hptr->next = 0;
    return direct;
}

/* indirect is really a pointer to a pointer to an array of pointers */
static inline void *MPIR_Handle_indirect_init(void *(**indirect)[],
                                              int *indirect_size,
                                              int indirect_num_blocks,
                                              int indirect_num_indices,
                                              int obj_size, int handle_type)
{
    void *block_ptr;
    MPIR_Handle_common *hptr = 0;
    char *ptr;
    int i;

    /* Must create new storage for dynamically allocated objects */
    /* Create the table */
    if (!*indirect) {
        /* printf("Creating indirect table with %d pointers to blocks in it\n", indirect_num_blocks); */
        *indirect = (void *) MPL_calloc(indirect_num_blocks, sizeof(void *), MPL_MEM_OBJECT);
        if (!*indirect) {
            return 0;
        }
        *indirect_size = 0;
    }

    /* See if we can allocate another block */
    if (*indirect_size >= indirect_num_blocks) {
        /* printf("Out of space in indirect table\n"); */
        return 0;
    }

    /* Create the next block */
    /* printf("Creating indirect block number %d with %d objects in it\n", *indirect_size, indirect_num_indices); */
    block_ptr = (void *) MPL_calloc(indirect_num_indices, obj_size, MPL_MEM_OBJECT);
    if (!block_ptr) {
        return 0;
    }
    ptr = (char *) block_ptr;
    for (i = 0; i < indirect_num_indices; i++) {
        /* Cast to (void*) to avoid false warning about alignment */
        hptr = (MPIR_Handle_common *) (void *) ptr;
        ptr = ptr + obj_size;
        hptr->next = ptr;
        hptr->handle = ((unsigned) HANDLE_KIND_INDIRECT << HANDLE_KIND_SHIFT) |
            (handle_type << HANDLE_MPI_KIND_SHIFT) | (*indirect_size << HANDLE_INDIRECT_SHIFT) | i;
        /* printf("handle=%#x handle_type=%x *indirect_size=%d i=%d\n", hptr->handle, handle_type, *indirect_size, i); */

        HANDLE_VG_LABEL(hptr, obj_size, handle_type, 0);
    }
    hptr->next = 0;
    /* We're here because avail is null, so there is no need to set
     * the last block ptr to avail */
    /* printf("loc of update is %x\n", &(**indirect)[*indirect_size]);  */
    (**indirect)[*indirect_size] = block_ptr;
    *indirect_size = *indirect_size + 1;
    return block_ptr;
}

static inline int MPIR_Handle_finalize(void *objmem_ptr)
{
    MPIR_Object_alloc_t *objmem = (MPIR_Object_alloc_t *) objmem_ptr;

    (void) MPIR_Handle_free(objmem->indirect, objmem->indirect_size);
    /* This does *not* remove any Info objects that the user created
     * and then did not destroy */

    /* at this point we are done with the memory pool, inform valgrind */
    MPL_VG_DESTROY_MEMPOOL(objmem_ptr);

    return 0;
}

/*+
  MPIR_Handle_obj_alloc - Create an object using the handle allocator

Input Parameters:
. objmem - Pointer to object memory block.

  Return Value:
  Pointer to new object.  Null if no more objects are available or can
  be allocated.

  Notes:
  In addition to returning a pointer to a new object, this routine may
  allocate additional space for more objects.

  This routine is thread-safe.

  This routine is performance-critical (it may be used to allocate
  MPI_Requests) and should not call any other routines in the common
  case.

  Threading: The 'MPID_THREAD_CS_ENTER/EXIT(POBJ/VNI, MPIR_THREAD_POBJ_HANDLE_MUTEX)' enables both
  finer-grain
  locking with a single global mutex and with a mutex specific for handles.

  +*/
#undef FUNCNAME
#define FUNCNAME MPIR_Handle_obj_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void *MPIR_Handle_obj_alloc(MPIR_Object_alloc_t * objmem)
{
    void *ret;
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    MPID_THREAD_CS_ENTER(VNI, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    ret = MPIR_Handle_obj_alloc_unsafe(objmem);
    MPID_THREAD_CS_EXIT(VNI, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Handle_obj_alloc_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void *MPIR_Handle_obj_alloc_unsafe(MPIR_Object_alloc_t * objmem)
{
    MPIR_Handle_common *ptr;

    if (objmem->avail) {
        ptr = objmem->avail;
        objmem->avail = objmem->avail->next;
        /* We do not clear ptr->next as we set it to an invalid pattern
         * when doing memory debugging and we don't need to set it
         * for the production/default case */
        /* ptr points to object to allocate */
    } else {
        int objsize, objkind;

        objsize = objmem->size;
        objkind = objmem->kind;

        if (!objmem->initialized) {
            MPL_VG_CREATE_MEMPOOL(objmem, 0 /*rzB */ , 0 /*is_zeroed */);

            /* Setup the first block.  This is done here so that short MPI
             * jobs do not need to include any of the Info code if no
             * Info-using routines are used */
            objmem->initialized = 1;
            ptr = MPIR_Handle_direct_init(objmem->direct, objmem->direct_size, objsize, objkind);
            if (ptr) {
                objmem->avail = ptr->next;
            }
#ifdef MPICH_DEBUG_HANDLEALLOC
            /* The priority of these callbacks must be greater than
             * the priority of the callback that frees the objmem direct and
             * indirect storage. */
            MPIR_Add_finalize(MPIR_check_handles_on_finalize, objmem,
                              MPIR_FINALIZE_CALLBACK_HANDLE_CHECK_PRIO);
#endif
            MPIR_Add_finalize(MPIR_Handle_finalize, objmem, 0);
            /* ptr points to object to allocate */
        } else {
            /* no space left in direct block; setup the indirect block. */

            ptr = MPIR_Handle_indirect_init(&objmem->indirect,
                                            &objmem->indirect_size,
                                            HANDLE_NUM_BLOCKS,
                                            HANDLE_NUM_INDICES, objsize, objkind);
            if (ptr) {
                objmem->avail = ptr->next;
            }

            /* ptr points to object to allocate */
        }
    }

    if (ptr) {
#ifdef USE_MEMORY_TRACING
        /* We set the object to an invalid pattern.  This is similar to
         * what is done by MPL_trmalloc by default (except that trmalloc uses
         * 0xda as the byte in the memset)
         */
        /* if the object was previously freed then MEMPOOL_FREE marked it as
         * NOACCESS, so we need to make it addressable again before memsetting
         * it */
        /* save and restore the handle -- it's a more robust method than
         * encoding the layout of the structure */
        int tmp_handle;
        MPL_VG_MAKE_MEM_DEFINED(ptr, objmem->size);
        tmp_handle = ptr->handle;
        memset(ptr, 0xef, objmem->size);
        ptr->handle = tmp_handle;
#endif /* USE_MEMORY_TRACING */
        /* mark the mem as addressable yet undefined if valgrind is available */
        MPL_VG_MEMPOOL_ALLOC(objmem, ptr, objmem->size);
        /* the handle value is always valid at return from this function */
        MPL_VG_MAKE_MEM_DEFINED(&ptr->handle, sizeof(ptr->handle));

        /* necessary to prevent annotations from being misinterpreted.  HB/HA
         * arcs will be drawn between a req object in across a free/alloc
         * boundary otherwise */
        /* NOTE: basically causes DRD's --trace-addr option to be useless for
         * handlemem-allocated objects. Consider one of the trace-inducing
         * annotations instead. */
        MPL_VG_ANNOTATE_NEW_MEMORY(ptr, objmem->size);

        MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL, (MPL_DBG_FDEST,
                                                   "Allocating object ptr %p (handle val 0x%08x)",
                                                   ptr, ptr->handle));
    }

    return ptr;
}

/*+
  MPIR_Handle_obj_free - Free an object allocated with MPID_Handle_obj_new

Input Parameters:
+ objmem - Pointer to object block
- object - Object to delete

  Notes:
  This routine assumes that only a single thread calls it at a time; this
  is true for the SINGLE_CS approach to thread safety
  +*/
static inline void MPIR_Handle_obj_free(MPIR_Object_alloc_t * objmem, void *object)
{
    MPIR_Handle_common *obj = (MPIR_Handle_common *) object;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    MPID_THREAD_CS_ENTER(VNI, MPIR_THREAD_POBJ_HANDLE_MUTEX);

    MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL, (MPL_DBG_FDEST,
                                               "Freeing object ptr %p (0x%08x kind=%s) refcount=%d",
                                               (obj),
                                               (obj)->handle,
                                               MPIR_Handle_get_kind_str(HANDLE_GET_MPI_KIND
                                                                        ((obj)->handle)),
                                               MPIR_Object_get_ref(obj)));

#ifdef USE_MEMORY_TRACING
    {
        /* set the object memory to an invalid value (0xec), except for the handle field */
        int tmp_handle;
        tmp_handle = obj->handle;
        memset(obj, 0xec, objmem->size);
        obj->handle = tmp_handle;
    }
#endif

    if (MPL_VG_RUNNING_ON_VALGRIND()) {
        int tmp_handle = obj->handle;

        MPL_VG_MEMPOOL_FREE(objmem, obj);
        /* MEMPOOL_FREE marks the object NOACCESS, so we have to make the
         * MPIR_Handle_common area that is used for internal book keeping
         * addressable again. */
        MPL_VG_MAKE_MEM_DEFINED(&obj->handle, sizeof(obj->handle));
        MPL_VG_MAKE_MEM_UNDEFINED(&obj->next, sizeof(obj->next));

        /* tt#1626: MEMPOOL_FREE also uses the value of `--free-fill=0x..` to
         * memset the object, so we need to restore the handle */
        obj->handle = tmp_handle;

        /* Necessary to prevent annotations from being misinterpreted.  HB/HA arcs
         * will be drawn between a req object in across a free/alloc boundary
         * otherwise.  Specifically, stores to obj->next when obj is actually an
         * MPIR_Request falsely look like a race to DRD and Helgrind because of the
         * other lockfree synchronization used with requests. */
        MPL_VG_ANNOTATE_NEW_MEMORY(obj, objmem->size);
    }

    obj->next = objmem->avail;
    objmem->avail = obj;
    MPID_THREAD_CS_EXIT(VNI, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
}

/*
 * Get an pointer to dynamically allocated storage for objects.
 */
static inline void *MPIR_Handle_get_ptr_indirect(int handle, MPIR_Object_alloc_t * objmem)
{
    int block_num, index_num;

    /* Check for a valid handle type */
    if (HANDLE_GET_MPI_KIND(handle) != objmem->kind) {
        return 0;
    }

    /* Find the block */
    block_num = HANDLE_BLOCK(handle);
    if (block_num >= objmem->indirect_size) {
        return 0;
    }

    /* Find the entry */
    index_num = HANDLE_BLOCK_INDEX(handle);
    /* If we could declare the blocks to a known size object, we
     * could do something like
     * return &((MPIR_Info**)*MPIU_Info_mem.indirect)[block_num][index_num];
     * since we cannot, we do the calculation by hand.
     */
    /* Get the pointer to the block of addresses.  This is an array of
     * void * */
    {
        char *block_ptr;
        /* Get the pointer to the block */
        block_ptr = (char *) (*(objmem->indirect))[block_num];
        /* Get the item */
        block_ptr += index_num * objmem->size;
        return block_ptr;
    }
}


#endif /* MPIR_HANDLEMEM_H_INCLUDED */
