/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#include "mpl_shm.h"

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

#include "mpidu_shm_impl.h"
#include "mpidu_generic_queue.h"

typedef struct alloc_elem {
    struct alloc_elem *next;
    void **ptr_p;
    size_t len;
} alloc_elem_t;

static struct { alloc_elem_t *head, *tail; } allocq = {0};

static int check_alloc(MPIDU_shm_seg_t *memory, MPIDU_shm_barrier_t *barrier,
                       int num_local, int local_rank);

#define ALLOCQ_HEAD() GENERIC_Q_HEAD(allocq)
#define ALLOCQ_EMPTY() GENERIC_Q_EMPTY(allocq)
#define ALLOCQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&allocq, ep, next)
#define ALLOCQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&allocq, epp, next)

#define ROUND_UP_8(x) (((x) + (size_t)7) & ~(size_t)7) /* rounds up to multiple of 8 */

static size_t segment_len = 0;

static int num_segments = 0;

typedef struct asym_check_region 
{
    void *base_ptr;
    OPA_int_t is_asym;
} asym_check_region;

static asym_check_region* asym_check_region_p = NULL;

/* MPIDU_shm_seg_alloc(len, ptr_p)

   This function is used to allow the caller to reserve a len sized
   region in the shared memory segment.  Once the shared memory
   segment is actually allocated, when MPIDU_SHM_Seg_commit() is
   called, the pointer *ptr_p will be set to point to the reserved
   region in the shared memory segment.

   Note that no shared memory is actually allocated by this function,
   and the *ptr_p pointer will be valid only after
   MPIDU_SHM_Seg_commit() is called.
*/
#undef FUNCNAME
#define FUNCNAME MPIDU_shm_seg_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_seg_alloc(size_t len, void **ptr_p)
{
    int mpi_errno = MPI_SUCCESS;
    alloc_elem_t *ep;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_ALLOC);

    /* round up to multiple of 8 to ensure the start of the next
       region is 64-bit aligned. */
    len = ROUND_UP_8(len);

    MPIR_Assert(len);
    MPIR_Assert(ptr_p);

    MPIR_CHKPMEM_MALLOC(ep, alloc_elem_t *, sizeof(alloc_elem_t), mpi_errno, "el");
    
    ep->ptr_p = ptr_p;
    ep->len = len;

    ALLOCQ_ENQUEUE(ep);

    segment_len += len;
    
 fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_SEG_ALLOC);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* MPIDU_shm_seg_commit(memory, num_local, local_rank)

   This function allocates a shared memory segment large enough to
   hold all of the regions previously requested by calls to
   MPIDU_shm_seg_alloc().  For each request, this function sets the
   associated pointer to point to the reserved region in the allocated
   shared memory segment.

   If there is only one process local to this node, then a shared
   memory region is not allocated.  Instead, memory is allocated from
   the heap.

   At least one call to MPIDU_SHM_Seg_alloc() must be made before
   calling MPIDU_SHM_Seg_commit().
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_shm_seg_commit
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_seg_commit(MPIDU_shm_seg_t *memory, MPIDU_shm_barrier_t **barrier,
                     int num_local, int local_rank, int local_procs_0, int rank)
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
    char kvs_name[256];
    char *serialized_hnd = NULL;
    void *current_addr;
    void *start_addr ATTRIBUTE((unused));
    size_t size_left;
    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(2);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_COMMIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_COMMIT);

    /* MPIDU_shm_seg_alloc() needs to have been called before this function */
    MPIR_Assert(!ALLOCQ_EMPTY());
    MPIR_Assert(segment_len > 0);

    /* allocate an area to check if the segment was allocated symmetrically */
    mpi_errno = MPIDU_shm_seg_alloc(sizeof(asym_check_region), (void **) &asym_check_region_p);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPL_shm_hnd_init(&(memory->hnd));
    if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);

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
    MPIR_Assert(MPIDU_SHM_CACHE_LINE_LEN >= sizeof(MPIDU_shm_barrier_t));
    segment_len += MPIDU_SHM_CACHE_LINE_LEN;

#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    /* We have a similar bootstrapping problem when using OpenPA in
     * lock-based emulation mode.  We use OPA_* functions during the
     * check_alloc function but we were previously initializing OpenPA
     * after return from this function.  So we put the emulation lock
     * right after the barrier var space. */

    /* offset from memory->base_addr to the start of ipc_lock */
    ipc_lock_offset = MPIDU_SHM_CACHE_LINE_LEN;

    MPIR_Assert(ipc_lock_offset >= sizeof(OPA_emulation_ipl_t));
    segment_len += MPIDU_SHM_CACHE_LINE_LEN;
#endif

    memory->segment_len = segment_len;

#ifdef USE_PMI2_API
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1)
    {
        char *addr;

        MPIR_CHKPMEM_MALLOC(addr, char *, segment_len + MPIDU_SHM_CACHE_LINE_LEN, mpi_errno, "segment");

        memory->base_addr = addr;
        current_addr =
            (char *) (((uintptr_t) addr + (uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1) &
                      (~((uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1)));
        memory->symmetrical = 0;

        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE/*isLeader*/);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

        mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIR_CHKLMEM_MALLOC(key, char *, PMI2_MAX_KEYLEN, mpi_errno, "key");
        MPL_snprintf(key, PMI2_MAX_KEYLEN, "sharedFilename-%d", num_segments);

        if (local_rank == 0) {
            mpi_errno = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len, &(memory->base_addr), 0);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* post name of shared file */
            MPIR_Assert(local_procs_0 == rank);

            mpi_errno = MPL_shm_hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* The opa and barrier initializations must come before we (the
             * leader) put the sharedFilename attribute.  Since this is a
             * serializing operation with our peers on the local node this
             * ensures that these initializations have occurred before any peer
             * attempts to use the resources. */
            mpi_errno = PMI2_Info_PutNodeAttr(key, serialized_hnd);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        } else {
            int found = FALSE;

            /* Allocate space for pmi key and val */
            MPIR_CHKLMEM_MALLOC(val, char *, PMI2_MAX_VALLEN, mpi_errno, "val");

            /* get name of shared file */
            mpi_errno = PMI2_Info_GetNodeAttr(key, val, PMI2_MAX_VALLEN, &found, TRUE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "nodeattr not found");

            mpi_errno = MPL_shm_hnd_deserialize(memory->hnd, val, strlen(val));
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPL_shm_seg_attach(memory->hnd, memory->segment_len, (char **)&memory->base_addr, 0);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);

            /* Right now we rely on the assumption that OPA_Interprocess_lock_init only
             * needs to be called by the leader and the current process before use by the
             * current process.  That is, we don't assume that this collective call is
             * synchronizing and we don't assume that it requires total external
             * synchronization.  In PMIv2 we don't have a PMI_Barrier operation so we need
             * this behavior. */
#endif

            mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, FALSE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIDU_shm_barrier(*barrier, num_local);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (local_rank == 0) {
            mpi_errno = MPL_shm_seg_remove(memory->hnd);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0 ;
    }
#else /* we are using PMIv1 */
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1)
    {
        char *addr;

        MPIR_CHKPMEM_MALLOC(addr, char *, segment_len + MPIDU_SHM_CACHE_LINE_LEN, mpi_errno, "segment");

        memory->base_addr = addr;
        current_addr =
            (char *) (((uintptr_t) addr + (uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1) &
                      (~((uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1)));
        memory->symmetrical = 0;

        /* we still need to call barrier */
	pmi_errno = PMI_Barrier();
        MPIR_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE/*isLeader*/);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif
        mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else{
        /* Allocate space for pmi key and val */
        pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIR_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

        pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
        MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIR_CHKLMEM_MALLOC(val, char *, val_max_sz, mpi_errno, "val");

        mpi_errno = PMI_KVS_Get_my_name(kvs_name, 256);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);

        if (local_rank == 0){
            mpi_errno = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len, &(memory->base_addr), 0);
            if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);

            /* post name of shared file */
            MPIR_Assert(local_procs_0 == rank);
            MPL_snprintf(key, key_max_sz, "sharedFilename[%i]-%i", rank, num_segments);

            mpi_errno = MPL_shm_hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);

            pmi_errno = PMI_KVS_Put (kvs_name, key, serialized_hnd);
            MPIR_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

            pmi_errno = PMI_KVS_Commit (kvs_name);
            MPIR_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            pmi_errno = PMI_Barrier();
            MPIR_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
        }
        else
        {
            pmi_errno = PMI_Barrier();
            MPIR_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

            /* get name of shared file */
            MPL_snprintf(key, key_max_sz, "sharedFilename[%i]-%i", local_procs_0, num_segments);
            pmi_errno = PMI_KVS_Get(kvs_name, key, val, val_max_sz);
            MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                 "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

            mpi_errno = MPL_shm_hnd_deserialize(memory->hnd, val, strlen(val));
            if(mpi_errno != MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPL_shm_seg_attach(memory->hnd, memory->segment_len, (char **)&memory->base_addr, 0);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *)((char *)memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno = MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, FALSE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIDU_shm_barrier(*barrier, num_local);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (local_rank == 0)
        {
            mpi_errno = MPL_shm_seg_remove(memory->hnd);
            if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0 ;
    }
#endif
    num_segments++;

    /* assign sections of the shared memory segment to their pointers */

    start_addr = current_addr;
    size_left = segment_len;

    /* reserve room for shared mem barrier (We used a whole cacheline) */
    current_addr = (char *) current_addr + MPIDU_SHM_CACHE_LINE_LEN;
    MPIR_Assert(size_left >= MPIDU_SHM_CACHE_LINE_LEN);
    size_left -= MPIDU_SHM_CACHE_LINE_LEN;

#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    /* reserve room for the opa emulation lock */
    current_addr = (char *) current_addr + MPIDU_SHM_CACHE_LINE_LEN;
    MPIR_Assert(size_left >= MPIDU_SHM_CACHE_LINE_LEN);
    size_left -= MPIDU_SHM_CACHE_LINE_LEN;
#endif

    do
    {
        alloc_elem_t *ep;

        ALLOCQ_DEQUEUE(&ep);

        *(ep->ptr_p) = current_addr;
        MPIR_Assert(size_left >= ep->len);
        size_left -= ep->len;
        current_addr = (char *)current_addr + ep->len;

        MPL_free(ep);

        MPIR_Assert((char *)current_addr <= (char *)start_addr + segment_len);
    }
    while (!ALLOCQ_EMPTY());

    mpi_errno = check_alloc(memory, *barrier, num_local, local_rank);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_SEG_COMMIT);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPL_shm_seg_remove(memory->hnd);
    MPL_shm_hnd_finalize(&(memory->hnd));
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPIDU_SHM_Seg_destroy() free the shared memory segment */
#undef FUNCNAME
#define FUNCNAME MPIDU_shm_seg_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_seg_destroy(MPIDU_shm_seg_t *memory, int num_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_DESTROY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_DESTROY);

    if (num_local == 1)
        MPL_free(memory->base_addr);
    else
    {
        mpi_errno = MPL_shm_seg_detach(memory->hnd, 
                        &(memory->base_addr), memory->segment_len);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

  fn_exit:
    MPL_shm_hnd_finalize(&(memory->hnd));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_SEG_DESTROY);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int check_alloc(MPIDU_shm_seg_t *memory, MPIDU_shm_barrier_t *barrier,
                       int num_local, int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_CHECK_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_CHECK_ALLOC);

    if (local_rank == 0) {
        asym_check_region_p->base_ptr = memory->base_addr;
        OPA_store_int(&asym_check_region_p->is_asym, 0);
    }

    mpi_errno = MPIDU_shm_barrier(barrier, num_local);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (asym_check_region_p->base_ptr != memory->base_addr)
        OPA_store_int(&asym_check_region_p->is_asym, 1);

    OPA_read_write_barrier();

    mpi_errno = MPIDU_shm_barrier(barrier, num_local);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (OPA_load_int(&asym_check_region_p->is_asym)) {
        memory->symmetrical = 0;
    } else {
        memory->symmetrical = 1;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_CHECK_ALLOC);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
