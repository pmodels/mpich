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

#ifdef ENABLE_NO_LOCAL
/* shared memory disabled, just stubs */

int MPIDU_Init_shm_init(void)
{
    return MPI_SUCCESS;
}

int MPIDU_Init_shm_finalize(void)
{
    return MPI_SUCCESS;
}

int MPIDU_Init_shm_barrier(void)
{
    return MPI_SUCCESS;
}

/* proper code should never call following under NO_LOCAL */
int MPIDU_Init_shm_put(void *orig, size_t len)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDU_Init_shm_get(int local_rank, size_t len, void *target)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDU_Init_shm_query(int local_rank, void **target_addr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#else /* ENABLE_NO_LOCAL */
typedef struct Init_shm_barrier {
    MPL_atomic_int_t val;
    MPL_atomic_int_t wait;
} Init_shm_barrier_t;

static int local_size;
static int my_local_rank;
static MPIDU_shm_seg_t memory;
static Init_shm_barrier_t *barrier;
static void *baseaddr;

static int sense;
static int barrier_init = 0;

static int Init_shm_barrier_init(int is_root)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_SHM_BARRIER_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_SHM_BARRIER_INIT);

    barrier = (Init_shm_barrier_t *) memory.base_addr;
    if (is_root) {
        MPL_atomic_store_int(&barrier->val, 0);
        MPL_atomic_store_int(&barrier->wait, 0);
    }
    sense = 0;
    barrier_init = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_SHM_BARRIER_INIT);

    return MPI_SUCCESS;
}

static int Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_SHM_BARRIER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_SHM_BARRIER);

    if (local_size == 1)
        goto fn_exit;

    MPIR_ERR_CHKINTERNAL(!barrier_init, mpi_errno, "barrier not initialized");

    if (MPL_atomic_fetch_add_int(&barrier->val, 1) == local_size - 1) {
        MPL_atomic_store_int(&barrier->val, 0);
        MPL_atomic_store_int(&barrier->wait, 1 - sense);
    } else {
        /* wait */
        while (MPL_atomic_load_int(&barrier->wait) == sense)
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

#endif /* ENABLE_NO_LOCAL */
