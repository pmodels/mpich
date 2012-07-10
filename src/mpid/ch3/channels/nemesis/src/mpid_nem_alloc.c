/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include <errno.h>

#if defined (HAVE_SYSV_SHARED_MEM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if defined( HAVE_MKSTEMP ) && defined( NEEDS_MKSTEMP_DECL )
extern int mkstemp(char *t);
#endif

static int check_alloc(int num_local, int local_rank);

typedef struct alloc_elem
{
    struct alloc_elem *next;
    void **ptr_p;
    size_t len;
} alloc_elem_t;

static struct { alloc_elem_t *head, *tail; } allocq = {0};

#define ALLOCQ_HEAD() GENERIC_Q_HEAD(allocq)
#define ALLOCQ_EMPTY() GENERIC_Q_EMPTY(allocq)
#define ALLOCQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&allocq, ep, next)
#define ALLOCQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&allocq, epp, next)

#define ROUND_UP_8(x) (((x) + (size_t)7) & ~(size_t)7) /* rounds up to multiple of 8 */

static size_t segment_len = 0;

typedef struct asym_check_region 
{
    void *base_ptr;
    OPA_int_t is_asym;
} asym_check_region;

static asym_check_region* asym_check_region_p = NULL;

/* MPIDI_CH3I_Seg_alloc(len, ptr_p)

   This function is used to allow the caller to reserve a len sized
   region in the shared memory segment.  Once the shared memory
   segment is actually allocated, when MPIDI_CH3I_Seg_commit() is
   called, the pointer *ptr_p will be set to point to the reserved
   region in the shared memory segment.

   Note that no shared memory is actually allocated by this function,
   and the *ptr_p pointer will be valid only after
   MPIDI_CH3I_Seg_commit() is called.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_alloc(size_t len, void **ptr_p)
{
    int mpi_errno = MPI_SUCCESS;
    alloc_elem_t *ep;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);

    /* round up to multiple of 8 to ensure the start of the next
       region is 64-bit aligned. */
    len = ROUND_UP_8(len);

    MPIU_Assert(len);
    MPIU_Assert(ptr_p);

    MPIU_CHKPMEM_MALLOC(ep, alloc_elem_t *, sizeof(alloc_elem_t), mpi_errno, "el");
    
    ep->ptr_p = ptr_p;
    ep->len = len;

    ALLOCQ_ENQUEUE(ep);

    segment_len += len;
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

/* MPIDI_CH3I_Seg_commit(memory, num_local, local_rank)

   This function allocates a shared memory segment large enough to
   hold all of the regions previously requested by calls to
   MPIDI_CH3I_Seg_alloc().  For each request, this function sets the
   associated pointer to point to the reserved region in the allocated
   shared memory segment.

   If there is only one process local to this node, then a shared
   memory region is not allocated.  Instead, memory is allocated from
   the heap.

   At least one call to MPIDI_CH3I_Seg_alloc() must be made before
   calling MPIDI_CH3I_Seg_commit().
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_commit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_commit(MPID_nem_seg_ptr_t memory, int num_local, int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    int ret;
    int ipc_lock_offset;
    OPA_emulation_ipl_t *ipc_lock;
#endif
    int key_max_sz;
    int val_max_sz;
    char *key;
    char *val;
    char *kvs_name;
    char *serialized_hnd = NULL;
    void *current_addr;
    void *start_addr;
    size_t size_left;
    MPIU_CHKPMEM_DECL (1);
    MPIU_CHKLMEM_DECL (2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);

    /* MPIDI_CH3I_Seg_alloc() needs to have been called before this function */
    MPIU_Assert(!ALLOCQ_EMPTY());
    MPIU_Assert(segment_len > 0);

    /* allocate an area to check if the segment was allocated symmetrically */
    mpi_errno = MPIDI_CH3I_Seg_alloc(sizeof(asym_check_region), (void **)&asym_check_region_p);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIU_SHMW_Hnd_init(&(memory->hnd));
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

    /* Shared memory barrier variables are in the front of the shared
       memory region.  We do this here explicitly, rather than use the
       Seg_alloc() function because we need to use the barrier inside
       this function, before we've assigned the variables to their
       regions.  To do this, we add extra space to the segment_len,
       initialize the variables as soon as the shared memory region is
       allocated/attached, then before we do the assignments of the
       pointers provided in Seg_alloc(), we make sure to skip the
       region containing the barrier vars. */
    
    /* add space for local barrier region.  Use a whole cacheline. */
    MPIU_Assert(MPID_NEM_CACHE_LINE_LEN >= sizeof(MPID_nem_barrier_t));
    segment_len += MPID_NEM_CACHE_LINE_LEN;

#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    /* We have a similar bootstrapping problem when using OpenPA in
     * lock-based emulation mode.  We use OPA_* functions during the
     * check_alloc function but we were previously initializing OpenPA
     * after return from this function.  So we put the emulation lock
     * right after the barrier var space. */

    /* offset from memory->base_addr to the start of ipc_lock */
    ipc_lock_offset = MPID_NEM_CACHE_LINE_LEN;

    MPIU_Assert(ipc_lock_offset >= sizeof(OPA_emulation_ipl_t));
    segment_len += MPID_NEM_CACHE_LINE_LEN;
#endif

    memory->segment_len = segment_len;

#ifdef USE_PMI2_API
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1)
    {
        char *addr;

        MPIU_CHKPMEM_MALLOC (addr, char *, segment_len + MPID_NEM_CACHE_LINE_LEN, mpi_errno, "segment");

        memory->base_addr = addr;
        current_addr = (char *)(((MPIR_Upint)addr + (MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1) & (~((MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1)));
        memory->symmetrical = 0;

        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE/*isLeader*/);
        MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

        mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, TRUE);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {

        if (local_rank == 0) {
            mpi_errno = MPIU_SHMW_Seg_create_and_attach(memory->hnd, memory->segment_len, &(memory->base_addr), 0);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* post name of shared file */
            MPIU_Assert (MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);

            mpi_errno = MPIU_SHMW_Hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, TRUE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* The opa and nem barrier initializations must come before we (the
             * leader) put the sharedFilename attribute.  Since this is a
             * serializing operation with our peers on the local node this
             * ensures that these initializations have occurred before any peer
             * attempts to use the resources. */
            mpi_errno = PMI2_Info_PutNodeAttr("sharedFilename", serialized_hnd);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        } else {
            int found = FALSE;

            /* Allocate space for pmi key and val */
            MPIU_CHKLMEM_MALLOC(val, char *, PMI2_MAX_VALLEN, mpi_errno, "val");

            /* get name of shared file */
            mpi_errno = PMI2_Info_GetNodeAttr("sharedFilename", val, PMI2_MAX_VALLEN, &found, TRUE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIU_ERR_CHKINTERNAL(!found, mpi_errno, "nodeattr not found");

            mpi_errno = MPIU_SHMW_Hnd_deserialize(memory->hnd, val, strlen(val));
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPIU_SHMW_Seg_attach(memory->hnd, memory->segment_len, (char **)&memory->base_addr, 0);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);

            /* Right now we rely on the assumption that OPA_Interprocess_lock_init only
             * needs to be called by the leader and the current process before use by the
             * current process.  That is, we don't assume that this collective call is
             * synchronizing and we don't assume that it requires total external
             * synchronization.  In PMIv2 we don't have a PMI_Barrier operation so we need
             * this behavior. */
#endif

            mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, FALSE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_barrier();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (local_rank == 0) {
            mpi_errno = MPIU_SHMW_Seg_remove(memory->hnd);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0 ;
    }
#else /* we are using PMIv1 */
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1)
    {
        char *addr;

        MPIU_CHKPMEM_MALLOC (addr, char *, segment_len + MPID_NEM_CACHE_LINE_LEN, mpi_errno, "segment");

        memory->base_addr = addr;
        current_addr = (char *)(((MPIR_Upint)addr + (MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1) & (~((MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1)));
        memory->symmetrical = 0 ;

        /* we still need to call barrier */
	pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE/*isLeader*/);
        MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif
        mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, TRUE);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* Allocate space for pmi key and val */
        pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

        pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIU_CHKLMEM_MALLOC(val, char *, val_max_sz, mpi_errno, "val");

        mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        if (local_rank == 0){
            mpi_errno = MPIU_SHMW_Seg_create_and_attach(memory->hnd, memory->segment_len, &(memory->base_addr), 0);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

            /* post name of shared file */
            MPIU_Assert (MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);
            MPIU_Snprintf (key, key_max_sz, "sharedFilename[%i]", MPID_nem_mem_region.rank);

            mpi_errno = MPIU_SHMW_Hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

            pmi_errno = PMI_KVS_Put (kvs_name, key, serialized_hnd);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

            pmi_errno = PMI_KVS_Commit (kvs_name);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, TRUE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            pmi_errno = PMI_Barrier();
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
        }
        else
        {
            pmi_errno = PMI_Barrier();
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

            /* get name of shared file */
            MPIU_Snprintf (key, key_max_sz, "sharedFilename[%i]", MPID_nem_mem_region.local_procs[0]);
            pmi_errno = PMI_KVS_Get (kvs_name, key, val, val_max_sz);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

            mpi_errno = MPIU_SHMW_Hnd_deserialize(memory->hnd, val, strlen(val));
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPIU_SHMW_Seg_attach(memory->hnd, memory->segment_len, (char **)&memory->base_addr, 0);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPID_nem_barrier_init((MPID_nem_barrier_t *)memory->base_addr, FALSE);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_barrier();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
        if (local_rank == 0)
        {
            mpi_errno = MPIU_SHMW_Seg_remove(memory->hnd);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0 ;
    }
#endif
    /* assign sections of the shared memory segment to their pointers */

    start_addr = current_addr;
    size_left = segment_len;

    /* reserve room for shared mem barrier (We used a whole cacheline) */
    current_addr = (char *)current_addr + MPID_NEM_CACHE_LINE_LEN;
    MPIU_Assert(size_left >= MPID_NEM_CACHE_LINE_LEN);
    size_left -= MPID_NEM_CACHE_LINE_LEN;

#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    /* reserve room for the opa emulation lock */
    current_addr = (char *)current_addr + MPID_NEM_CACHE_LINE_LEN;
    MPIU_Assert(size_left >= MPID_NEM_CACHE_LINE_LEN);
    size_left -= MPID_NEM_CACHE_LINE_LEN;
#endif

    do
    {
        alloc_elem_t *ep;

        ALLOCQ_DEQUEUE(&ep);

        *(ep->ptr_p) = current_addr;
        MPIU_Assert(size_left >= ep->len);
        size_left -= ep->len;
        current_addr = (char *)current_addr + ep->len;

        MPIU_Free(ep);

        MPIU_Assert((char *)current_addr <= (char *)start_addr + segment_len);
    }
    while (!ALLOCQ_EMPTY());

    mpi_errno = check_alloc(num_local, local_rank);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_SHMW_Seg_remove(memory->hnd);
    MPIU_SHMW_Hnd_finalize(&(memory->hnd));
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPIDI_CH3I_Seg_destroy() free the shared memory segment */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_destroy(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);

    if (MPID_nem_mem_region.num_local == 1)
        MPIU_Free(MPID_nem_mem_region.memory.base_addr);
    else
    {
        mpi_errno = MPIU_SHMW_Seg_detach(MPID_nem_mem_region.memory.hnd, 
                        &(MPID_nem_mem_region.memory.base_addr), MPID_nem_mem_region.memory.segment_len);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    MPIU_SHMW_Hnd_finalize(&(MPID_nem_mem_region.memory.hnd));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* check_alloc() checks to see whether the shared memory segment is
   allocated at the same virtual memory address at each process.
*/
#undef FUNCNAME
#define FUNCNAME check_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int check_alloc(int num_local, int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CHECK_ALLOC);

    if (local_rank == 0) {
        asym_check_region_p->base_ptr = MPID_nem_mem_region.memory.base_addr;
        OPA_store_int(&asym_check_region_p->is_asym, 0);
    }

    mpi_errno = MPID_nem_barrier();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (asym_check_region_p->base_ptr != MPID_nem_mem_region.memory.base_addr)
        OPA_store_int(&asym_check_region_p->is_asym, 1);

    OPA_read_write_barrier();

    mpi_errno = MPID_nem_barrier();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (OPA_load_int(&asym_check_region_p->is_asym))
    {
	MPID_nem_mem_region.memory.symmetrical = 0;
	MPID_nem_asymm_base_addr = MPID_nem_mem_region.memory.base_addr;
#ifdef MPID_NEM_SYMMETRIC_QUEUES
        MPIU_ERR_INTERNALANDJUMP(mpi_errno, "queues are not symmetrically allocated as expected");
#endif
    }
    else
    {
	MPID_nem_mem_region.memory.symmetrical = 1;
	MPID_nem_asymm_base_addr = NULL;
    }
      
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CHECK_ALLOC);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

