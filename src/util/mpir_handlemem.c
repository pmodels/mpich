/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
      class       : none
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

/* style: allow:printf:5 sig:0 */
#ifdef MPICH_DEBUG_HANDLEALLOC
/* The following is a handler that may be added to finalize to test whether
   handles remain allocated, including those from the direct blocks.

   When adding memory checking, this routine should be invoked as

   MPIR_Add_finalize(MPIR_check_handles_on_finalize, objmem, 1);

   as part of the object initialization.

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
int MPIR_check_handles_on_finalize(void *objmem_ptr)
{
    MPIR_Object_alloc_t *objmem = (MPIR_Object_alloc_t *) objmem_ptr;
    int i;
    MPIR_Handle_common *ptr;
    int leaked_handles = FALSE;
    int directSize = objmem->direct_size;
    /* NOTE: direct could be NULL (and directSize == 0) */
    char *direct = (char *) objmem->direct;
    char *directEnd = (char *) direct + directSize * objmem->size - 1;
    int nDirect = 0;
    int *nIndirect = 0;

    /* Return immediately if this object has not allocated any space */
    if (!objmem->initialized) {
        return 0;
    }

    if (objmem->indirect_size > 0) {
        nIndirect = (int *) MPL_calloc(objmem->indirect_size, sizeof(int), MPL_MEM_OBJECT);
    }
    /* Count the number of items in the avail list.  These include
     * all objects, whether direct or indirect allocation */
    ptr = objmem->avail;
    while (ptr) {
        /* printf("Looking at %p\n", ptr); */
        /* Find where this object belongs */
        if (direct && (char *) ptr >= direct && (char *) ptr < directEnd) {
            nDirect++;
        } else {
            void **indirect = (void **) objmem->indirect;
            for (i = 0; i < objmem->indirect_size; i++) {
                char *start = indirect[i];
                char *end = start + HANDLE_NUM_INDICES * objmem->size;
                if ((char *) ptr >= start && (char *) ptr < end) {
                    nIndirect[i]++;
                    break;
                }
            }
            if (i == objmem->indirect_size) {
                /* Error - could not find the owning memory */
                /* Temp */
                printf("Could not place object at %p in handle memory for type %s\n", ptr,
                       MPIR_Handle_get_kind_str(objmem->kind));
                printf("direct block is [%p,%p]\n", direct, directEnd);
                if (objmem->indirect_size) {
                    printf("indirect block is [%p,%p]\n", indirect[0],
                           (char *) indirect[0] + HANDLE_NUM_INDICES * objmem->size);
                }
            }
        }
        ptr = ptr->next;
    }

    if (0) {
        /* Produce a report */
        printf("Object handles:\n\ttype  \t%s\n\tsize  \t%d\n\tdirect size\t%d\n\
\tindirect size\t%d\n", MPIR_Handle_get_kind_str(objmem->kind), objmem->size, objmem->direct_size, objmem->indirect_size);
    }
    if (nDirect != directSize) {
        leaked_handles = TRUE;
        printf("In direct memory block for handle type %s, %d handles are still allocated\n",
               MPIR_Handle_get_kind_str(objmem->kind), directSize - nDirect);
    }
    for (i = 0; i < objmem->indirect_size; i++) {
        if (nIndirect[i] != HANDLE_NUM_INDICES) {
            leaked_handles = TRUE;
            printf
                ("In indirect memory block %d for handle type %s, %d handles are still allocated\n",
                 i, MPIR_Handle_get_kind_str(objmem->kind), HANDLE_NUM_INDICES - nIndirect[i]);
        }
    }

    MPL_free(nIndirect);

    if (leaked_handles && MPIR_CVAR_ABORT_ON_LEAKED_HANDLES) {
        /* comm_world has been (or should have been) destroyed by this point,
         * pass comm=NULL */
        MPID_Abort(NULL, MPI_ERR_OTHER, 1, "ERROR: leaked handles detected, aborting");
        MPIR_Assert(0);
    }

    return 0;
}
#endif

/* returns the name of the handle kind for debugging/logging purposes */
const char *MPIR_Handle_get_kind_str(int kind)
{
#define mpiu_name_case_(name_) case MPIR_##name_: return (#name_)
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
            mpiu_name_case_(VCONN);
            mpiu_name_case_(GREQ_CLASS);
            mpiu_name_case_(SESSION);
            mpiu_name_case_(STREAM);
        default:
            return "unknown";
    }
#undef mpiu_name_case_
}

static MPL_initlock_t info_handle_obj_lock = MPL_INITLOCK_INITIALIZER;
/*+
  MPIR_Info_handle_obj_alloc - Create an INFO object using the handle allocator
                               even before MPI_Init() or after MPI_Finalize().

  This routine has an independent "static" lock that is different from
  MPIR_THREAD_POBJ_HANDLE_MUTEX and MPIR_THREAD_VCI_HANDLE_MUTEX.  Since this
  routine does not share the lock with MPIR_Handle_obj_alloc() and MPIR_Handle_obj_free(),
  this routine may not update any global data that can be updated by MPIR_Handle_obj_alloc()
  and MPIR_Handle_obj_free().
  +*/
void *MPIR_Info_handle_obj_alloc(MPIR_Object_alloc_t * objmem)
{
    /* Non-MPI_Info objects must call MPIR_Handle_obj_alloc(). */
    MPIR_Assert(objmem->kind == MPIR_INFO);

    void *ret;
    MPL_initlock_lock(&info_handle_obj_lock);
    ret = MPIR_Handle_obj_alloc_unsafe(objmem, HANDLE_NUM_BLOCKS, HANDLE_NUM_INDICES);
    MPL_initlock_unlock(&info_handle_obj_lock);
    return ret;
}

/*+
  MPIR_Info_handle_obj_free - Free an INFO object allocated by MPIR_Info_handle_obj_alloc()
                              even before MPI_Init() or after MPI_Finalize().

  This routine has an independent "static" lock that is different from
  MPIR_THREAD_POBJ_HANDLE_MUTEX and MPIR_THREAD_VCI_HANDLE_MUTEX.  Since this
  routine does not share the lock with MPIR_Handle_obj_alloc() and MPIR_Handle_obj_free(),
  this routine may not update any global data that can be updated by MPIR_Handle_obj_alloc()
  and MPIR_Handle_obj_free().
  +*/
void MPIR_Info_handle_obj_free(MPIR_Object_alloc_t * objmem, void *object)
{
    MPIR_Assert(objmem->kind == MPIR_INFO);

    MPL_initlock_lock(&info_handle_obj_lock);
    MPIR_Handle_obj_free_unsafe(objmem, object, /* info object */ TRUE);
    MPL_initlock_unlock(&info_handle_obj_lock);
}
