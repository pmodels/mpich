/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#include "mpl_shm.h"
#include "mpidu_init_shm.h"

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

static int check_alloc(MPIDU_shm_seg_t * memory, int num_local, int local_rank);

#define ALLOCQ_HEAD() GENERIC_Q_HEAD(allocq)
#define ALLOCQ_EMPTY() GENERIC_Q_EMPTY(allocq)
#define ALLOCQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&allocq, ep, next)
#define ALLOCQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&allocq, epp, next)

static size_t segment_len = 0;

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
int MPIDU_shm_seg_commit(MPIDU_shm_seg_t * memory, int num_local, int local_rank,
                         int local_procs_0, int rank, MPL_memory_class class)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    void *current_addr;
    void *start_addr ATTRIBUTE((unused));
    size_t size_left;
    MPIR_CHKPMEM_DECL(1);
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
            MPIR_Assert(serialized_hnd_size < MPIDU_SHM_MAX_FNAME_LEN);

            MPIDU_Init_shm_put(serialized_hnd, MPIDU_SHM_MAX_FNAME_LEN);
            MPIDU_Init_shm_barrier();
        } else {
            MPIDU_Init_shm_barrier();
            MPIDU_Init_shm_query(0, (void **) &serialized_hnd);

            mpl_err = MPL_shm_hnd_deserialize(memory->hnd, serialized_hnd, strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_seg_attach(memory->hnd, memory->segment_len,
                                         (void **) &memory->base_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");
        }

        MPIDU_Init_shm_barrier();

        if (local_rank == 0) {
            /* memory->hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(memory->hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }
        current_addr = memory->base_addr;
        memory->symmetrical = 0;
    }

    /* assign sections of the shared memory segment to their pointers */

    start_addr = current_addr;
    size_left = segment_len;

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

    mpi_errno = check_alloc(memory, num_local, local_rank);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    /* reset segment_len to zero */
    segment_len = 0;

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
static int check_alloc(MPIDU_shm_seg_t * memory, int num_local, int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_CHECK_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_CHECK_ALLOC);

    if (local_rank == 0) {
        asym_check_region_p->base_ptr = memory->base_addr;
        OPA_store_int(&asym_check_region_p->is_asym, 0);
    }

    MPIDU_Init_shm_barrier();

    if (asym_check_region_p->base_ptr != memory->base_addr)
        OPA_store_int(&asym_check_region_p->is_asym, 1);

    OPA_read_write_barrier();

    MPIDU_Init_shm_barrier();

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
