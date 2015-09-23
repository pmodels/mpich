/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : MEMORY
      description : affects memory allocation and usage, including MPI object handles

cvars:
    - name        : MPIR_CVAR_ABORT_ON_LEAKED_HANDLES
      category    : MEMORY
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, MPI will call MPI_Abort at MPI_Finalize if any MPI object
        handles have been leaked.  For example, if MPI_Comm_dup is called
        without calling a corresponding MPI_Comm_free.  For uninteresting
        reasons, enabling this option may prevent all known object leaks from
        being reported.  MPICH must have been configure with
        "--enable-g=handlealloc" or better in order for this functionality to
        work.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include <stdio.h>

#ifdef NEEDS_PRINT_HANDLE
static void MPIU_Print_handle( int handle );
#endif

#ifdef MPICH_DEBUG_HANDLEALLOC
static int MPIU_CheckHandlesOnFinalize( void * );
static const char *MPIR_ObjectName( MPIU_Object_alloc_t * );
#endif

/* This is the utility file for info that contains routines used to 
   manage the arrays used to store handle objects.  

   To use these routines, allocate the following in a utility file 
   used by each object (e.g., info, datatype, comm, group, ... ).  
   (The comment format // is used in this example but the usual 
   C comment convention should be used of course.)  The usage is described
   below.

   // Declarations begin here
   // Static declaration of the information about the block
   // Define the number of preallocated entries # omitted)
   define MPID_<OBJ>_PREALLOC 256
   MPIU_Object_alloc_t MPID_<obj>_mem = { 0, 0, 0, 0, MPID_<obj>, 
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
   
   The following routines are provided:
   void *MPIU_Handle_direct_init( void *direct, int direct_size, int obj_size,
                                  int handle_type )
        Initialize the preallocated array (MPID_<obj>_direct) with
        direct_size elements each of obj_size.  Returns the first available
        element (which should usually be assigned to "avail").
	handle_type is the kind of object (e.g., MPID_INFO)

   void *MPIU_Handle_indirect_init( void (**indirect)[], int *indirect_size, 
                                    int indirect_num_blocks,
                                    int indirect_num_indices, 
                                    int obj_size,
                                    int handle_type )
	Initialize the indirect array (MPID_<obj>_indirect) of size
        indirect_size, each block of which contains indirect_num_indices
	members of size obj_size.  Returns the first available element, or
	NULL if no memory is available.  
        Also incrementes indirect_size and assigns to indirect if it is null.

	The Handle_indirect routine and the data structures that it manages
	require a little more discussion.
	This routine allocates an array of pointers to a block of storage.
	The block of storage contains space for indirect_num_indices 
	instances of an object of obj_size.  These blocks are allocated
	as needed; the pointers to these blocks are stored in the 
	indirect array.  The value of indirect_size is the number of 
	valid pointers in indirect.  In other words, indirect[0] through
        indirect[*indirect_size-1] contain pointers to blocks of 
	storage of size indirect_num_indices * obj_size.  The array 
	indirect has indirect_num_blocks entries, each holding a pointer.

	The rationale for this approach is that this approach can 
	handle large amounts of memory; however, relatively little
	memory is used unless needed.  The definitions in 
        mpich/src/include/mpihandlemem.h define defaults for the
        indirect_num_blocks (HANDLE_NUM_BLOCKS = 512) and
        indirect_num_indices (HANDLE_NUM_INDICES = 16384) that permits
        the allocation of 8 Mi objects.

   int MPIU_Handle_free( void *(*indirect)[], int indirect_size )
        Frees any memory allocated for the indirect handles.  Returns 0 on
	success and nonzero on failure

   None of these routines is thread-safe.  Any routine that uses them 
   must ensure that only one thread at a time may call them.  
   
*/

/*
 * You can use this to allocated that necessary local structures
 */
#define MPID_HANDLE_MEM_ALLOC(Name,NAME) \
MPID_##Name MPID_##Name_direct[MPID_##NAME##_PREALLOC]; \
static int initialize = 0;\
static int MPID_##Name *avail=0;\
static MPID_##Name *(*MPID_##Name##_indirect)[] = 0;\
static int MPID_##Name##_indirect_size = 0;

/* This routine is called by finalize when MPI exits */
static int MPIU_Handle_free( void *((*indirect)[]), int indirect_size )
{
    int i;
    
    /* Remove any allocated storage */
    for (i=0; i<indirect_size; i++) {
	MPIU_Free( (*indirect)[i] );
    }
    if (indirect) {
	MPIU_Free( indirect );
    }
    /* This does *not* remove any objects that the user created 
       and then did not destroy */
    return 0;
}

#if defined(MPL_VG_AVAILABLE)
#define MPIU_HANDLE_VG_LABEL(objptr_, objsize_, handle_type_, is_direct_)                        \
    do {                                                                                         \
        if (MPL_VG_RUNNING_ON_VALGRIND()) {                                                     \
            char desc_str[256];                                                                  \
            MPL_snprintf(desc_str, sizeof(desc_str)-1,                                          \
                          "[MPICH handle: objptr=%p handle=0x%x %s/%s]",                        \
                          (objptr_), (objptr_)->handle,                                          \
                          ((is_direct_) ? "DIRECT" : "INDIRECT"),                                \
                          MPIU_Handle_get_kind_str(handle_type_));                               \
            /* we don't keep track of the block descriptor because the handle */                 \
            /* values never change once allocated */                                             \
            MPL_VG_CREATE_BLOCK((objptr_), (objsize_), desc_str);                               \
        }                                                                                        \
    } while (0)
#else
#define MPIU_HANDLE_VG_LABEL(objptr_, objsize_, handle_type_, is_direct_) do{}while(0)
#endif

void *MPIU_Handle_direct_init(void *direct,
			      int direct_size, 
			      int obj_size, 
			      int handle_type)
{
    int                i;
    MPIU_Handle_common *hptr=0;
    char               *ptr = (char *)direct;

    for (i=0; i<direct_size; i++) {
	/* printf( "Adding %p in %d\n", ptr, handle_type ); */
        /* First cast to (void*) to avoid false warnings about alignment
           (consider that a requirement of the input parameters) */
	hptr = (MPIU_Handle_common *)(void *)ptr;
	ptr  = ptr + obj_size;
	hptr->next = ptr;
	hptr->handle = ((unsigned)HANDLE_KIND_DIRECT << HANDLE_KIND_SHIFT) | 
	    (handle_type << HANDLE_MPI_KIND_SHIFT) | i;

        MPIU_HANDLE_VG_LABEL(hptr, obj_size, handle_type, 1);
    }

    if (hptr)
        hptr->next = 0;
    return direct;
}

/* indirect is really a pointer to a pointer to an array of pointers */
static void *MPIU_Handle_indirect_init( void *(**indirect)[], 
					int *indirect_size, 
					int indirect_num_blocks,
					int indirect_num_indices,
                                        int obj_size,
					int handle_type )
{
    void               *block_ptr;
    MPIU_Handle_common *hptr=0;
    char               *ptr;
    int                i;

    /* Must create new storage for dynamically allocated objects */
    /* Create the table */
    if (!*indirect) {
	/* printf( "Creating indirect table with %d pointers to blocks in it\n", indirect_num_blocks ); */
	*indirect = (void *)MPIU_Calloc(indirect_num_blocks, sizeof(void *));
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
    block_ptr = (void *)MPIU_Calloc( indirect_num_indices, obj_size );
    if (!block_ptr) { 
	return 0;
    }
    ptr = (char *)block_ptr;
    for (i=0; i<indirect_num_indices; i++) {
        /* Cast to (void*) to avoid false warning about alignment */
	hptr       = (MPIU_Handle_common *)(void*)ptr;
	ptr        = ptr + obj_size;
	hptr->next = ptr;
	hptr->handle   = ((unsigned)HANDLE_KIND_INDIRECT << HANDLE_KIND_SHIFT) | 
	    (handle_type << HANDLE_MPI_KIND_SHIFT) | 
	    (*indirect_size << HANDLE_INDIRECT_SHIFT) | i;
        /* printf("handle=%#x handle_type=%x *indirect_size=%d i=%d\n", hptr->handle, handle_type, *indirect_size, i); */

        MPIU_HANDLE_VG_LABEL(hptr, obj_size, handle_type, 0);
    }
    hptr->next = 0;
    /* We're here because avail is null, so there is no need to set 
       the last block ptr to avail */
    /* printf( "loc of update is %x\n", &(**indirect)[*indirect_size] );  */
    (**indirect)[*indirect_size] = block_ptr;
    *indirect_size = *indirect_size + 1;
    return block_ptr;
}

/*
  Create and return a pointer to an info object.  Returns null if there is 
  an error such as out-of-memory.  Does not allocate space for the
  key or value.

 */

static int MPIU_Handle_finalize( void *objmem_ptr )
{
    MPIU_Object_alloc_t *objmem = (MPIU_Object_alloc_t *)objmem_ptr;

    (void)MPIU_Handle_free( objmem->indirect, objmem->indirect_size );
    /* This does *not* remove any Info objects that the user created 
       and then did not destroy */

    /* at this point we are done with the memory pool, inform valgrind */
    MPL_VG_DESTROY_MEMPOOL(objmem_ptr);

    return 0;
}

/* FIXME: The alloc_complete routine should be removed.
   It is used only in typeutil.c (in MPIR_Datatype_init, which is only 
   executed from within the MPI_Init/MPI_Init_thread startup and hence is
   guaranteed to be single threaded).  When used by the obj_alloc, it
   adds unnecessary overhead, particularly when MPI is single threaded */

void MPIU_Handle_obj_alloc_complete(MPIU_Object_alloc_t *objmem,
				    int initialized)
{
    if (initialized) {
	/* obj_alloc initialized region during this allocation;
	 * perform any ancillary operations associated with
	 * initialization prior to releasing control over region.
	 */

	/* Tell finalize to free up any memory that we allocate.
	 * The 0 makes this the lowest priority callback, so 
	 * that other callbacks will finish before this one is invoked.
	 */
	MPIR_Add_finalize(MPIU_Handle_finalize, objmem, 0);
    }
}

/*+
  MPIU_Handle_obj_alloc - Create an object using the handle allocator

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

  Threading: The 'MPID_THREAD_CS_ENTER/EXIT(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX)' enables both 
  finer-grain
  locking with a single global mutex and with a mutex specific for handles.

  +*/
#undef FUNCNAME
#define FUNCNAME MPIU_Handle_obj_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void *MPIU_Handle_obj_alloc(MPIU_Object_alloc_t *objmem)
{
    void *ret;
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    ret = MPIU_Handle_obj_alloc_unsafe(objmem);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIU_Handle_obj_alloc_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void *MPIU_Handle_obj_alloc_unsafe(MPIU_Object_alloc_t *objmem)
{
    MPIU_Handle_common *ptr;

    if (objmem->avail) {
	ptr	      = objmem->avail;
	objmem->avail = objmem->avail->next;
	/* We do not clear ptr->next as we set it to an invalid pattern
	   when doing memory debugging and we don't need to set it 
	   for the production/default case */
	/* ptr points to object to allocate */
    }
    else {
	int objsize, objkind;
	int performed_initialize = 0;

	objsize = objmem->size;
	objkind = objmem->kind;

	if (!objmem->initialized) {
	    performed_initialize = 1;

            MPL_VG_CREATE_MEMPOOL(objmem, 0/*rzB*/, 0/*is_zeroed*/);

	    /* Setup the first block.  This is done here so that short MPI
	       jobs do not need to include any of the Info code if no
	       Info-using routines are used */
	    objmem->initialized = 1;
	    ptr = MPIU_Handle_direct_init(objmem->direct,
					  objmem->direct_size,
					  objsize,
					  objkind);
	    if (ptr) {
		objmem->avail = ptr->next;
	    }

#ifdef MPICH_DEBUG_HANDLEALLOC
	    /* The priority of these callbacks must be greater than
	       the priority of the callback that frees the objmem direct and 
	       indirect storage. */
	    MPIR_Add_finalize(MPIU_CheckHandlesOnFinalize, objmem, MPIR_FINALIZE_CALLBACK_HANDLE_CHECK_PRIO);
#endif
	    /* ptr points to object to allocate */
	}
	else {
	    /* no space left in direct block; setup the indirect block. */

	    ptr = MPIU_Handle_indirect_init(&objmem->indirect, 
					    &objmem->indirect_size, 
					    HANDLE_NUM_BLOCKS,
					    HANDLE_NUM_INDICES,
					    objsize,
					    objkind);
	    if (ptr) {
		objmem->avail = ptr->next;
	    }

	    /* ptr points to object to allocate */
	}
	MPIU_Handle_obj_alloc_complete(objmem, performed_initialize);
    }

    if (ptr) {
#ifdef USE_MEMORY_TRACING
    /* We set the object to an invalid pattern.  This is similar to 
       what is done by MPIU_trmalloc by default (except that trmalloc uses
       0xda as the byte in the memset)
    */
        /* if the object was previously freed then MEMPOOL_FREE marked it as
         * NOACCESS, so we need to make it addressable again before memsetting
         * it */
        /* save and restore the handle -- it's a more robust method than
         * encoding the layout of the structure */
        int tmp_handle;
        MPL_VG_MAKE_MEM_DEFINED(ptr, objmem->size);
        tmp_handle = ptr->handle ;
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

        MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,
                                         "Allocating object ptr %p (handle val 0x%08x)",
                                         ptr, ptr->handle));
    }

    return ptr;
}

/*+
  MPIU_Handle_obj_free - Free an object allocated with MPID_Handle_obj_new

Input Parameters:
+ objmem - Pointer to object block
- object - Object to delete

  Notes: 
  This routine assumes that only a single thread calls it at a time; this
  is true for the SINGLE_CS approach to thread safety
  +*/
void MPIU_Handle_obj_free( MPIU_Object_alloc_t *objmem, void *object )
{
    MPIU_Handle_common *obj = (MPIU_Handle_common *)object;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);

    MPIU_DBG_MSG_FMT(HANDLE,TYPICAL,(MPIU_DBG_FDEST,
                                     "Freeing object ptr %p (0x%08x kind=%s) refcount=%d",
                                     (obj),
                                     (obj)->handle,
                                     MPIU_Handle_get_kind_str(HANDLE_GET_MPI_KIND((obj)->handle)),
                                     MPIU_Object_get_ref(obj)));

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
         * MPIU_Handle_common area that is used for internal book keeping
         * addressable again. */
        MPL_VG_MAKE_MEM_DEFINED(&obj->handle, sizeof(obj->handle));
        MPL_VG_MAKE_MEM_UNDEFINED(&obj->next, sizeof(obj->next));

        /* tt#1626: MEMPOOL_FREE also uses the value of `--free-fill=0x..` to
         * memset the object, so we need to restore the handle */
        obj->handle = tmp_handle;

        /* Necessary to prevent annotations from being misinterpreted.  HB/HA arcs
         * will be drawn between a req object in across a free/alloc boundary
         * otherwise.  Specifically, stores to obj->next when obj is actually an
         * MPID_Request falsely look like a race to DRD and Helgrind because of the
         * other lockfree synchronization used with requests. */
        MPL_VG_ANNOTATE_NEW_MEMORY(obj, objmem->size);
    }

    obj->next	        = objmem->avail;
    objmem->avail	= obj;
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_HANDLE_MUTEX);
}

/* 
 * Get an pointer to dynamically allocated storage for objects.
 */
void *MPIU_Handle_get_ptr_indirect( int handle, MPIU_Object_alloc_t *objmem )
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
     could do something like 
       return &( (MPID_Info**)*MPIU_Info_mem.indirect)[block_num][index_num];
     since we cannot, we do the calculation by hand.
    */
    /* Get the pointer to the block of addresses.  This is an array of 
       void * */
    {
	char *block_ptr;
	/* Get the pointer to the block */
	block_ptr = (char *)(*(objmem->indirect))[block_num];
	/* Get the item */
	block_ptr += index_num * objmem->size;
	return block_ptr;
    }
}

/* returns the name of the handle kind for debugging/logging purposes */
const char *MPIU_Handle_get_kind_str(int kind)
{
#define mpiu_name_case_(name_) case MPID_##name_: return (#name_)
    switch (kind) {
        mpiu_name_case_(COMM);
        mpiu_name_case_(GROUP);
        mpiu_name_case_(DATATYPE);
        mpiu_name_case_(FILE);
        mpiu_name_case_(ERRHANDLER);
        mpiu_name_case_(OP);
        mpiu_name_case_(INFO);
        mpiu_name_case_(WIN);
        mpiu_name_case_(KEYVAL);
        mpiu_name_case_(ATTR);
        mpiu_name_case_(REQUEST);
        mpiu_name_case_(PROCGROUP);
        mpiu_name_case_(VCONN);
        mpiu_name_case_(GREQ_CLASS);
        default:
            return "unknown";
    }
#undef mpiu_name_case_
}

/* style: allow:printf:5 sig:0 */
#ifdef MPICH_DEBUG_HANDLEALLOC
/* The following is a handler that may be added to finalize to test whether
   handles remain allocated, including those from the direct blocks.
   
   When adding memory checking, this routine should be invoked as

   MPIR_Add_finalize(MPIU_CheckHandlesOnFinalize, objmem, 1);

   as part of the object intialization.

   The algorithm follows the following approach:
   
   The memory allocation approach manages a list of available objects.
   These objects are allocated from several places:
      "direct" - this is a block of preallocated space
      "indirect" - this is a block of blocks that are allocated as necessary.
                   E.g., objmem_ptr->indirect[0..objmem_ptr->indirect_size-1]
		   are pointers (or null) to a block of memory.  This block is
		   then divided into objects that are added to the avail list.

   To provide information on the handles that are still in use, we must 
   "repatriate" all of the free objects, at least virtually.  To give
   the best information, for each free item, we determine to which block
   it belongs.  
*/
static int MPIU_CheckHandlesOnFinalize( void *objmem_ptr )
{
    MPIU_Object_alloc_t *objmem = (MPIU_Object_alloc_t *)objmem_ptr;
    int i;
    MPIU_Handle_common *ptr;
    int leaked_handles = FALSE;
    int   directSize = objmem->direct_size;
    char *direct = (char *)objmem->direct;
    char *directEnd = (char *)direct + directSize * objmem->size - 1;
    int   nDirect = 0;
    int  *nIndirect = 0;

    /* Return immediately if this object has not allocated any space */
    if (!objmem->initialized) {
	return 0;
    }

    if (objmem->indirect_size > 0) {
	nIndirect = (int *)MPIU_Calloc( objmem->indirect_size, sizeof(int) );
    }
    /* Count the number of items in the avail list.  These include
       all objects, whether direct or indirect allocation */
    ptr = objmem->avail;
    while (ptr) {
	/* printf( "Looking at %p\n", ptr ); */
	/* Find where this object belongs */
	if ((char *)ptr >= direct && (char *)ptr < directEnd) {
	    nDirect++;
	}
	else {
	    void **indirect = (void **)objmem->indirect;
	    for (i=0; i<objmem->indirect_size; i++) {
		char *start = indirect[i];
		char *end   = start + HANDLE_NUM_INDICES * objmem->size;
		if ((char *)ptr >= start && (char *)ptr < end) {
		    nIndirect[i]++;
		    break;
		}
	    }
	    if (i == objmem->indirect_size) {
		/* Error - could not find the owning memory */
		/* Temp */
		printf( "Could not place object at %p in handle memory for type %s\n", ptr, MPIR_ObjectName( objmem ) );
		printf( "direct block is [%p,%p]\n", direct, directEnd );
		if (objmem->indirect_size) {
		    printf( "indirect block is [%p,%p]\n", indirect[0], 
			    (char *)indirect[0] + HANDLE_NUM_INDICES * 
			    objmem->size );
		}
	    }
	}
	ptr = ptr->next;
    }

    if (0) {
	/* Produce a report */
	printf( "Object handles:\n\ttype  \t%s\n\tsize  \t%d\n\tdirect size\t%d\n\
\tindirect size\t%d\n",
		MPIR_ObjectName( objmem ), objmem->size, objmem->direct_size, 
		objmem->indirect_size );
    }
    if (nDirect != directSize) {
        leaked_handles = TRUE;
	printf( "In direct memory block for handle type %s, %d handles are still allocated\n", MPIR_ObjectName( objmem ), directSize - nDirect );
    }
    for (i=0; i<objmem->indirect_size; i++) {
	if (nIndirect[i] != HANDLE_NUM_INDICES) {
            leaked_handles = TRUE;
	    printf( "In indirect memory block %d for handle type %s, %d handles are still allocated\n", i, MPIR_ObjectName( objmem ), HANDLE_NUM_INDICES - nIndirect[i] );
	}
    }

    if (nIndirect) { 
	MPIU_Free( nIndirect );
    }

    if (leaked_handles && MPIR_CVAR_ABORT_ON_LEAKED_HANDLES) {
        /* comm_world has been (or should have been) destroyed by this point,
         * pass comm=NULL */
        MPID_Abort(NULL, MPI_ERR_OTHER, 1, "ERROR: leaked handles detected, aborting");
        MPIU_Assert(0);
    }

    return 0;
}

static const char *MPIR_ObjectName( MPIU_Object_alloc_t *objmem )
{
    return MPIU_Handle_get_kind_str(objmem->kind);
}
#endif    

#ifdef NEEDS_PRINT_HANDLE
/* For debugging */
static void MPIU_Print_handle( int handle )
{
    int type, kind, block, index;

    type = HANDLE_GET_MPI_KIND(handle);
    kind = HANDLE_GET_KIND(handle);
    switch (type) {
    case HANDLE_KIND_INVALID:
	printf( "invalid" );
	break;
    case HANDLE_KIND_BUILTIN:
	printf( "builtin" );
	break;
    case HANDLE_KIND_DIRECT:
	index = HANDLE_INDEX(handle);
	printf( "direct: %d", index );
	break;
    case HANDLE_KIND_INDIRECT:
	block = HANDLE_BLOCK(handle);
	index = HANDLE_BLOCK_INDEX(handle);
	printf( "indirect: block %d index %d", block, index );
	break;
    }
}
#endif

/*
 * Count the number of objects allocated and the number of objects
 * on the available free list.  Return the difference.
 */
int MPIU_Handle_obj_outstanding(const MPIU_Object_alloc_t *objmem)
{
    int allocated = 0;
    int available = 0;
    MPIU_Handle_common *ptr;

    ptr = objmem->avail;
    while(ptr)
    {
        available++;
        ptr = ptr->next;
    }

    if(objmem->initialized)
    {
        allocated = objmem->direct_size +
            objmem->indirect_size * HANDLE_NUM_INDICES;
    }

    return allocated - available;
}

