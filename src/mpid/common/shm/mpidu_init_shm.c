/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "mpl_shm.h"
#include "mpidimpl.h"
#include "mpir_pmi.h"
#include "mpidu_shm_seg.h"

typedef struct Init_shm_barrier {
    OPA_int_t val;
    OPA_int_t wait;
} Init_shm_barrier_t;

static int local_size;
static int my_local_rank;
static MPIDU_shm_seg_t memory;
static Init_shm_barrier_t *barrier;
static void *baseaddr;

static int sense;
static int barrier_init = 0;

static int Init_shm_barrier_init(int init_values)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_SHM_BARRIER_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_SHM_BARRIER_INIT);

    barrier = (Init_shm_barrier_t *) memory.base_addr;
    if (init_values) {
        OPA_store_int(&barrier->val, 0);
        OPA_store_int(&barrier->wait, 0);
        OPA_write_barrier();
    }
    sense = 0;
    barrier_init = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_SHM_BARRIER_INIT);

    return MPI_SUCCESS;
}

/* FIXME: this is not a scalable algorithm because everyone is polling on the same cacheline */
static int Init_shm_barrier()
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_SHM_BARRIER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_SHM_BARRIER);

    if (local_size == 1)
        goto fn_exit;

    MPIR_ERR_CHKINTERNAL(!barrier_init, mpi_errno, "barrier not initialized");

    if (OPA_fetch_and_incr_int(&barrier->val) == local_size - 1) {
        OPA_store_int(&barrier->val, 0);
        OPA_store_int(&barrier->wait, 1 - sense);
        OPA_write_barrier();
    } else {
        /* wait */
        while (OPA_load_int(&barrier->wait) == sense)
            MPL_sched_yield();  /* skip */
    }
    sense = 1 - sense;

  fn_fail:
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_SHM_BARRIER);
    return mpi_errno;
}

int MPIDU_Init_shm_init(void)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    int local_leader;
    int rank;
#ifdef OPA_USE_LOCK_BASE_PRIMITIVES
    int ret;
    int ipc_lock_offset;
    OPA_emulation_ipl_t *ipc_lock;
#endif
    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_INIT);

    rank = MPIR_Process.rank;
    local_size = MPIR_Process.local_size;
    my_local_rank = MPIR_Process.local_rank;
    local_leader = MPIR_Process.node_local_map[0];

    size_t segment_len = MPIDU_SHM_CACHE_LINE_LEN + sizeof(MPIDU_Init_shm_block_t) * local_size;

    char *serialized_hnd = NULL;
    int serialized_hnd_size = 0;

    mpl_err = MPL_shm_hnd_init(&(memory.hnd));
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

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

    memory.segment_len = segment_len;

    if (local_size == 1) {
        char *addr;

        MPIR_CHKPMEM_MALLOC(addr, char *, segment_len + MPIDU_SHM_CACHE_LINE_LEN, mpi_errno,
                            "segment", MPL_MEM_SHM);

        memory.base_addr = addr;
        baseaddr =
            (char *) (((uintptr_t) addr + (uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1) &
                      (~((uintptr_t) MPIDU_SHM_CACHE_LINE_LEN - 1)));
        memory.symmetrical = 0;

        /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASE_PRIMITIVES
        ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory.base_addr + ipc_lock_offset);
        ret = OPA_Interprocess_lock_init(ipc_lock, TRUE /* isLeader */);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif
        mpi_errno = Init_shm_barrier_init(TRUE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        if (my_local_rank == 0) {
            /* root prepare shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(memory.hnd, memory.segment_len,
                                                    (void **) &(memory.base_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            MPIR_Assert(local_leader == rank);

            mpl_err = MPL_shm_hnd_get_serialized_by_ref(memory.hnd, &serialized_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
            serialized_hnd_size = strlen(serialized_hnd);
            MPIR_Assert(serialized_hnd_size < MPIR_pmi_max_val_size());

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASE_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory.base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, TRUE /* isLeader */);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);
#endif
            mpi_errno = Init_shm_barrier_init(TRUE);
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
    if (local_size != 1) {
        MPIR_Assert(local_size > 1);
        if (my_local_rank > 0) {
            /* non-root attach shm segment */
            mpl_err = MPL_shm_hnd_deserialize(memory.hnd, serialized_hnd, strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_seg_attach(memory.hnd, memory.segment_len,
                                         (void **) &memory.base_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

            /* must come before barrier_init since we use OPA in that function */
#ifdef OPA_USE_LOCK_BASED_PRIMITIVES
            ipc_lock = (OPA_emulation_ipl_t *) ((char *) memory.base_addr + ipc_lock_offset);
            ret = OPA_Interprocess_lock_init(ipc_lock, my_local_rank == 0);
            MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", ret);

            /* Right now we rely on the assumption that OPA_Interprocess_lock_init only
             * needs to be called by the leader and the current process before use by the
             * current process.  That is, we don't assume that this collective call is
             * synchronizing and we don't assume that it requires total external
             * synchronization.  In PMIv2 we don't have a PMI_Barrier operation so we need
             * this behavior. */
#endif

            mpi_errno = Init_shm_barrier_init(FALSE);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = Init_shm_barrier();
        MPIR_ERR_CHECK(mpi_errno);

        if (my_local_rank == 0) {
            /* memory->hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(memory.hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }

        baseaddr = memory.base_addr + MPIDU_SHM_CACHE_LINE_LEN;
        memory.symmetrical = 0;
    }

    mpi_errno = Init_shm_barrier();
    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_INIT);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDU_Init_shm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS, mpl_err;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);

    mpi_errno = Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    if (local_size == 1)
        MPL_free(memory.base_addr);
    else {
        mpl_err = MPL_shm_seg_detach(memory.hnd, (void **) &(memory.base_addr), memory.segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
    }

  fn_exit:
    MPL_shm_hnd_finalize(&(memory.hnd));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_BARRIER);

    mpi_errno = Init_shm_barrier();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_BARRIER);

    return mpi_errno;
}

int MPIDU_Init_shm_put(void *orig, size_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_PUT);

    MPIR_Assert(len <= sizeof(MPIDU_Init_shm_block_t));
    MPIR_Memcpy((char *) baseaddr + my_local_rank * sizeof(MPIDU_Init_shm_block_t), orig, len);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_PUT);

    return mpi_errno;
}

int MPIDU_Init_shm_get(int local_rank, size_t len, void *target)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_GET);

    MPIR_Assert(local_rank < local_size && len <= sizeof(MPIDU_Init_shm_block_t));
    MPIR_Memcpy(target, (char *) baseaddr + local_rank * sizeof(MPIDU_Init_shm_block_t), len);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_GET);

    return mpi_errno;
}

int MPIDU_Init_shm_query(int local_rank, void **target_addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_QUERY);

    MPIR_Assert(local_rank < local_size);
    *target_addr = (char *) baseaddr + local_rank * sizeof(MPIDU_Init_shm_block_t);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_QUERY);

    return mpi_errno;
}
