/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_shmem_pool.h"
#include "mpidu_genqi_shmem_types.h"
#include "mpidu_init_shm.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define RESIZE_TO_MAX_ALIGN(x) \
    ((((x) / MAX_ALIGNMENT) + !!((x) % MAX_ALIGNMENT)) * MAX_ALIGNMENT)

static int cell_block_alloc(MPIDU_genqi_shmem_pool_s * pool, int block_idx);
static int cell_block_alloc(MPIDU_genqi_shmem_pool_s * pool, int block_idx)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s **new_cell_headers = NULL;

    new_cell_headers =
        (MPIDU_genqi_shmem_cell_header_s **) MPL_malloc(pool->cells_per_proc
                                                        * sizeof(MPIDU_genqi_shmem_cell_header_s *),
                                                        MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!new_cell_headers, rc, MPI_ERR_OTHER, "**nomem");
    pool->cell_headers = new_cell_headers;

    /* init cell headers */
    int idx = block_idx * pool->cells_per_proc;
    for (int i = 0; i < pool->cells_per_proc; i++) {
        pool->cell_headers[i] =
            (MPIDU_genqi_shmem_cell_header_s *) ((char *) pool->cell_header_base
                                                 + (idx + i) * pool->cell_alloc_size);
        /* The handle value is the one being stored in the next, prev, head, tail pointers.
         * All valid handle must to be non-zero, a zero handle is equivalent to a NULL pointer. */
        pool->cell_headers[i]->handle =
            (uintptr_t) pool->cell_headers[i] - (uintptr_t) pool->cell_header_base + 1;
        pool->cell_headers[i]->block_idx = block_idx;
        rc = MPIDU_genq_shmem_queue_enqueue(pool, &pool->free_queues[block_idx],
                                            HEADER_TO_CELL(pool->cell_headers[i]));
        MPIR_ERR_CHECK(rc);
    }

  fn_exit:
    return rc;
  fn_fail:
    MPL_free(new_cell_headers);
    goto fn_exit;
}

int MPIDU_genq_shmem_pool_create_unsafe(uintptr_t cell_size, uintptr_t cells_per_proc,
                                        uintptr_t num_proc, int rank,
                                        MPIDU_genq_shmem_pool_t * pool)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj;
    uintptr_t slab_size = 0;
    uintptr_t aligned_cell_size = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CREATE);

    pool_obj = MPL_malloc(sizeof(MPIDU_genqi_shmem_pool_s), MPL_MEM_OTHER);

    pool_obj->cell_size = cell_size;
    aligned_cell_size = RESIZE_TO_MAX_ALIGN(cell_size);
    pool_obj->cell_alloc_size = sizeof(MPIDU_genqi_shmem_cell_header_s) + aligned_cell_size;
    pool_obj->cells_per_proc = cells_per_proc;
    pool_obj->num_proc = num_proc;
    pool_obj->rank = rank;

    /* the global_block_index is at the end of the slab to avoid extra need of alignment */
    int total_cells_size = num_proc * cells_per_proc * pool_obj->cell_alloc_size;
    int free_queue_size = num_proc * sizeof(MPIDU_genq_shmem_queue_u);
    slab_size = total_cells_size + free_queue_size;

    rc = MPIDU_Init_shm_alloc(slab_size, &pool_obj->slab);
    MPIR_ERR_CHECK(rc);

    rc = MPL_gpu_register_host(pool_obj->slab, slab_size);
    MPIR_ERR_CHECK(rc);

    pool_obj->cell_header_base = (MPIDU_genqi_shmem_cell_header_s *) pool_obj->slab;
    pool_obj->free_queues =
        (MPIDU_genq_shmem_queue_u *) ((char *) pool_obj->slab + total_cells_size);

    rc = MPIDU_genq_shmem_queue_init(&pool_obj->free_queues[rank],
                                     MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC);
    MPIR_ERR_CHECK(rc);

    rc = cell_block_alloc(pool_obj, rank);
    MPIR_ERR_CHECK(rc);

    rc = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(rc);

    *pool = (MPIDU_genq_shmem_pool_t) pool_obj;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CREATE);
    return rc;
  fn_fail:
    MPIDU_Init_shm_free(pool_obj->slab);
    MPL_free(pool_obj);
    goto fn_exit;
}

int MPIDU_genq_shmem_pool_destroy_unsafe(MPIDU_genq_shmem_pool_t pool)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_DESTROY);

    MPL_free(pool_obj->cell_headers);

    MPIDU_Init_shm_free(pool_obj->slab);

    /* free self */
    MPL_free(pool_obj);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_DESTROY);
    return rc;
}
