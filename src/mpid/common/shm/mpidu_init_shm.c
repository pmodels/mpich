/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "mpl_shm.h"
#include "mpidimpl.h"
#include "mpir_pmi.h"

static int init_shm_initialized;

int MPIDU_Init_shm_local_size;
int MPIDU_Init_shm_local_rank;

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
    /* fields that support async shm alloc */
    MPL_atomic_int_t lock;
    MPL_atomic_int_t alloc_count;
    char serialized_hnd[MPIDU_INIT_SHM_BLOCK_SIZE];
} Init_shm_barrier_t;

static size_t init_shm_len;
static MPL_shm_hnd_t init_shm_hnd;
static char *init_shm_addr;

static Init_shm_barrier_t *barrier;
static void *baseaddr;

static int sense;
static int barrier_init = 0;

static int Init_shm_barrier_init(int is_root)
{

    MPIR_FUNC_ENTER;

    barrier = (Init_shm_barrier_t *) init_shm_addr;
    if (is_root) {
        MPL_atomic_store_int(&barrier->val, 0);
        MPL_atomic_store_int(&barrier->wait, 0);
    }
    sense = 0;
    barrier_init = 1;

    MPIR_FUNC_EXIT;

    return MPI_SUCCESS;
}

static int Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_size == 1)
        goto fn_exit;

    MPIR_ERR_CHKINTERNAL(!barrier_init, mpi_errno, "barrier not initialized");

    if (MPL_atomic_fetch_add_int(&barrier->val, 1) == MPIDU_Init_shm_local_size - 1) {
        MPL_atomic_store_int(&barrier->val, 0);
        MPL_atomic_store_int(&barrier->wait, 1 - sense);
    } else {
        /* wait */
        while (MPL_atomic_load_int(&barrier->wait) == sense)
            MPID_Thread_yield();        /* skip */
    }
    sense = 1 - sense;

  fn_fail:
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDU_Init_shm_init(void)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    MPIR_CHKLMEM_DECL();

    MPIR_FUNC_ENTER;

    MPIDU_Init_shm_local_size = MPIR_Process.local_size;
    MPIDU_Init_shm_local_rank = MPIR_Process.local_rank;

    if (MPIDU_Init_shm_local_size == 1) {
        /* All processes need call MPIR_pmi_bcast. This is because we may need call MPIR_pmi_barrier
         * inside depend on PMI versions, and all processes need participate.
         */
        int dummy;
        mpi_errno = MPIR_pmi_bcast(&dummy, sizeof(int), MPIR_PMI_DOMAIN_LOCAL);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        size_t segment_len = sizeof(Init_shm_barrier_t) +
            MPIDU_INIT_SHM_BLOCK_SIZE * MPIDU_Init_shm_local_size;

        char *serialized_hnd = NULL;
        int serialized_hnd_size = 0;

        mpl_err = MPL_shm_hnd_init(&init_shm_hnd);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

        init_shm_len = segment_len;

        if (MPIDU_Init_shm_local_rank == 0) {
            /* root prepare shm segment */
            mpl_err = MPL_shm_seg_create_and_attach(init_shm_hnd, init_shm_len,
                                                    (void **) &(init_shm_addr), 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            MPIR_Assert(MPIR_Process.node_local_map[0] == MPIR_Process.rank);

            mpl_err = MPL_shm_hnd_get_serialized_by_ref(init_shm_hnd, &serialized_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");
            serialized_hnd_size = strlen(serialized_hnd) + 1;
            MPIR_Assert(serialized_hnd_size < MPIR_pmi_max_val_size());

            mpi_errno = Init_shm_barrier_init(TRUE);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* non-root prepare to recv */
            serialized_hnd_size = MPIR_pmi_max_val_size();
            MPIR_CHKLMEM_MALLOC(serialized_hnd, serialized_hnd_size);
        }
        /* All processes need call MPIR_pmi_bcast. This is because we may need call MPIR_pmi_barrier
         * inside depend on PMI versions, and all processes need participate.
         */
        mpi_errno = MPIR_pmi_bcast(serialized_hnd, serialized_hnd_size, MPIR_PMI_DOMAIN_LOCAL);
        MPIR_ERR_CHECK(mpi_errno);

        if (MPIDU_Init_shm_local_rank > 0) {
            /* non-root attach shm segment */
            mpl_err = MPL_shm_hnd_deserialize(init_shm_hnd, serialized_hnd, strlen(serialized_hnd));
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

            mpl_err = MPL_shm_seg_attach(init_shm_hnd, init_shm_len, (void **) &init_shm_addr, 0);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem");

            mpi_errno = Init_shm_barrier_init(FALSE);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = Init_shm_barrier();
        MPIR_ERR_CHECK(mpi_errno);

        if (MPIDU_Init_shm_local_rank == 0) {
            /* init_shm_hnd no longer needed */
            mpl_err = MPL_shm_seg_remove(init_shm_hnd);
            MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
        }

        baseaddr = init_shm_addr + sizeof(Init_shm_barrier_t);

        mpi_errno = Init_shm_barrier();
    }

    init_shm_initialized = 1;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS, mpl_err;

    MPIR_FUNC_ENTER;

    if (!init_shm_initialized || MPIDU_Init_shm_local_size == 1) {
        goto fn_exit;
    }

    mpl_err = MPL_shm_seg_detach(init_shm_hnd, (void **) &(init_shm_addr), init_shm_len);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

    MPL_shm_hnd_finalize(&(init_shm_hnd));

    init_shm_initialized = 0;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_size > 1) {
        mpi_errno = Init_shm_barrier();
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDU_Init_shm_put(void *orig, size_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDU_Init_shm_local_size > 1) {
        MPIR_Assert(len <= MPIDU_INIT_SHM_BLOCK_SIZE);
        MPIR_Memcpy((char *) baseaddr + MPIDU_Init_shm_local_rank * MPIDU_INIT_SHM_BLOCK_SIZE,
                    orig, len);
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDU_Init_shm_get(int local_rank, size_t len, void *target)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* a single process should not get its own put */
    MPIR_Assert(MPIDU_Init_shm_local_size > 1);

    MPIR_Assert(local_rank < MPIDU_Init_shm_local_size && len <= MPIDU_INIT_SHM_BLOCK_SIZE);
    MPIR_Memcpy(target, (char *) baseaddr + local_rank * MPIDU_INIT_SHM_BLOCK_SIZE, len);

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDU_Init_shm_query(int local_rank, void **target_addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* a single process should not get its own put */
    MPIR_Assert(MPIDU_Init_shm_local_size > 1);

    MPIR_Assert(local_rank < MPIDU_Init_shm_local_size);
    *target_addr = (char *) baseaddr + local_rank * MPIDU_INIT_SHM_BLOCK_SIZE;

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

int MPIDU_Init_shm_atomic_count(void)
{
    return MPL_atomic_load_int(&barrier->alloc_count);
}

int MPIDU_Init_shm_atomic_put(void *orig, size_t len)
{
    /* get spin lock */
    while (MPL_atomic_cas_int(&barrier->lock, 0, 1)) {
    }
    /* set the data */
    MPIR_Assert(len <= MPIDU_INIT_SHM_BLOCK_SIZE);
    MPIR_Memcpy(barrier->serialized_hnd, orig, len);
    MPL_atomic_store_int(&barrier->alloc_count, 1);
    /* unlock */
    MPL_atomic_store_int(&barrier->lock, 0);
    return MPI_SUCCESS;
}

int MPIDU_Init_shm_atomic_get(void *target, size_t len)
{
    /* get spin lock */
    while (MPL_atomic_cas_int(&barrier->lock, 0, 1)) {
    }
    /* copy the data */
    MPIR_Memcpy(target, barrier->serialized_hnd, len);
    /* unlock */
    MPL_atomic_store_int(&barrier->lock, 0);
    return MPI_SUCCESS;
}

#endif /* ENABLE_NO_LOCAL */
