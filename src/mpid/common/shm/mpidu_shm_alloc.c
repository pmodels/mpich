/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#include "mpl_shm.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#if defined (HAVE_SYSV_SHARED_MEM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if defined(HAVE_MKSTEMP) && defined(NEEDS_MKSTEMP_DECL)
extern int mkstemp(char *t);
#endif

#include "mpidu_shm_impl.h"
#include "mpidu_generic_queue.h"

typedef struct alloc_elem {
    struct alloc_elem *next;
    void **ptr_p;
    size_t len;
} alloc_elem_t;

static struct {
    alloc_elem_t *head, *tail;
} allocq = {
0};

static int check_alloc(MPIDU_shm_seg_t * memory, MPIDU_shm_barrier_t * barrier,
                       int num_local, int local_rank);

#define ALLOCQ_HEAD() GENERIC_Q_HEAD(allocq)
#define ALLOCQ_EMPTY() GENERIC_Q_EMPTY(allocq)
#define ALLOCQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&allocq, ep, next)
#define ALLOCQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&allocq, epp, next)

static size_t segment_len = 0;

static int num_segments = 0;

typedef struct asym_check_region {
    void *base_ptr;
    OPA_int_t is_asym;
} asym_check_region;

static asym_check_region *asym_check_region_p = NULL;

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
int MPIDU_shm_seg_alloc(size_t len, void **ptr_p, MPL_memory_class class)
{
    int mpi_errno = MPI_SUCCESS;
    alloc_elem_t *ep;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_ALLOC);

    /* round up to multiple of 8 to ensure the start of the next
     * region is 64-bit aligned. */
    len = MPL_ROUND_UP_ALIGN(len, (size_t) 8);
    MPIR_Assert(len);
    MPIR_Assert(ptr_p);

    MPIR_CHKPMEM_MALLOC(ep, alloc_elem_t *, sizeof(alloc_elem_t), mpi_errno, "el", class);

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
int MPIDU_shm_seg_commit(MPIDU_shm_seg_t * memory, MPIDU_shm_barrier_t ** barrier,
                         int num_local, int local_rank, int local_procs_0, int rank,
                         MPL_memory_class class)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
    int ret;
    int ipc_lock_offset;
    OPA_emulation_ipl_t *ipc_lock;
#endif
    void *current_addr;
    void *start_addr ATTRIBUTE((unused));
    size_t size_left;
    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_COMMIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_COMMIT);

    /* MPIDU_shm_seg_alloc() needs to have been called before this function */
    MPIR_Assert(!ALLOCQ_EMPTY());
    MPIR_Assert(segment_len > 0);

    /* allocate an area to check if the segment was allocated symmetrically */
    mpl_err = MPIDU_shm_seg_alloc(sizeof(asym_check_region), (void **) &asym_check_region_p, class);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    mpl_err = MPL_shm_hnd_init(&(memory->hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    /* Shared memory barrier variables are in the front of the shared
     * memory region.  We do this here explicitly, rather than use the
     * Seg_alloc() function because we need to use the barrier inside
     * this function, before we've assigned the variables to their
     * regions.  To do this, we add extra space to the segment_len,
     * initialize the variables as soon as the shared memory region is
     * allocated/attached, then before we do the assignments of the
     * pointers provided in Seg_alloc(), we make sure to skip the
     * region containing the barrier vars. */

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

    char *serialized_hnd = NULL;
    int serialized_hnd_size = 0;
    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1) {
        char *addr;

        MPIR_CHKPMEM_MALLOC(addr, char *, segment_len + MPIDU_SHM_CACHE_LINE_LEN, mpi_errno,
                            "segment", class);

        memory->base_addr = addr;
        current_addr =
            (char *) (((uintptr_t) addr + (uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1) &
                      (~((uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1)));
        memory->symmetrical = 0;


        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory->base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE /*isLeader */);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif
        mpi_errno =
            MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        if (local_rank == 0) {
            /* root prepare shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(memory->hnd, memory->segment_len,
                                                    (void **) &(memory->base_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            MPIR_Assert(local_procs_0 == rank);

            mpl_err = MPL_shm_hnd_get_serialized_by_ref(memory->hnd, &serialized_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
            serialized_hnd_size = strlen(serialized_hnd);
            MPIR_Assert(serialized_hnd_size < MPIR_pmi_max_val_size());

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif

            mpi_errno =
                MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, TRUE);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* non-root prepare to recv */
            serialized_hnd_size = MPIR_pmi_max_val_size();
            MPIR_CHKLMEM_MALLOC(serialized_hnd, char *, serialized_hnd_size, mpi_errno, "val",
                                MPL_MEM_OTHER);
        }
    }
    /* All processes need call MPIR_pmi_bcast. This is because we may need call MPIR_pmi_barrier
     * inside depend on PMI versions, and all processes need participate.
     */
    MPIR_pmi_bcast(serialized_hnd, serialized_hnd_size, MPIR_PMI_DOMAIN_LOCAL);
    if (num_local != 1) {
        MPIR_Assert(num_local > 1);
        if (local_rank > 0) {
            /* non-root attach shm segment */
            mpl_err = MPL_shm_hnd_deserialize(memory->hnd, serialized_hnd, strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_seg_attach(memory->hnd, memory->segment_len,
                                         (void **) &memory->base_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory->base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);

            /* Right now we rely on the assumption that OPA_Interprocess_lock_init only
             * needs to be called by the leader and the current process before use by the
             * current process.  That is, we don't assume that this collective call is
             * synchronizing and we don't assume that it requires total external
             * synchronization.  In PMIv2 we don't have a PMI_Barrier operation so we need
             * this behavior. */
#endif

            mpi_errno =
                MPIDU_shm_barrier_init((MPIDU_shm_barrier_t *) memory->base_addr, barrier, FALSE);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = MPIDU_shm_barrier(*barrier, num_local);
        MPIR_ERR_CHECK(mpi_errno);

        if (local_rank == 0) {
            /* memory->hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(memory->hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0;
    }

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

    do {
        alloc_elem_t *ep;

        ALLOCQ_DEQUEUE(&ep);

        *(ep->ptr_p) = current_addr;
        MPIR_Assert(size_left >= ep->len);
        size_left -= ep->len;
        current_addr = (char *) current_addr + ep->len;

        MPL_free(ep);

        MPIR_Assert((char *) current_addr <= (char *) start_addr + segment_len);
    }
    while (!ALLOCQ_EMPTY());

    mpi_errno = check_alloc(memory, *barrier, num_local, local_rank);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    /* reset segment_len to zero */
    segment_len = 0;

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
int MPIDU_shm_seg_destroy(MPIDU_shm_seg_t * memory, int num_local)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_SEG_DESTROY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_SEG_DESTROY);

    if (num_local == 1)
        MPL_free(memory->base_addr);
    else {
        mpl_err = MPL_shm_seg_detach(memory->hnd, (void **) &(memory->base_addr),
                                     memory->segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
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
static int check_alloc(MPIDU_shm_seg_t * memory, MPIDU_shm_barrier_t * barrier,
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
    MPIR_ERR_CHECK(mpi_errno);

    if (asym_check_region_p->base_ptr != memory->base_addr)
        OPA_store_int(&asym_check_region_p->is_asym, 1);

    OPA_read_write_barrier();

    mpi_errno = MPIDU_shm_barrier(barrier, num_local);
    MPIR_ERR_CHECK(mpi_errno);

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
