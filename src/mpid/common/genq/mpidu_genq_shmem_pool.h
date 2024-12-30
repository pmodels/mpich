/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_SHMEM_POOL_H_INCLUDED
#define MPIDU_GENQ_SHMEM_POOL_H_INCLUDED

#include "mpidimpl.h"
#include "mpidu_genqi_shmem_types.h"
#include "mpidu_genq_shmem_queue.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int MPIDU_genq_shmem_pool_size(int cell_size, int cells_per_free_queue,
                               int num_proc, int num_free_queue);
int MPIDU_genq_shmem_pool_create(void *slab, int slab_size,
                                 int cell_size, int cells_per_free_queue,
                                 int num_proc, int rank, int num_free_queue,
                                 int *queue_types, MPIDU_genq_shmem_pool_t * pool);
int MPIDU_genq_shmem_pool_destroy(MPIDU_genq_shmem_pool_t pool);
int MPIDU_genqi_shmem_pool_register(MPIDU_genqi_shmem_pool_s * pool_obj);

static inline int MPIDU_genq_shmem_pool_cell_alloc(MPIDU_genq_shmem_pool_t pool, void **cell,
                                                   int block_idx, int free_queue_idx,
                                                   const void *src_buf)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    *cell = NULL;

    MPIR_FUNC_ENTER;

    /* lazy registration of the shmem pool with the gpu */
    if (!pool_obj->gpu_registered && MPIR_GPU_query_pointer_is_dev(src_buf)) {
        rc = MPIDU_genqi_shmem_pool_register(pool_obj);
        MPIR_ERR_CHECK(rc);
    }

    int idx = block_idx * pool_obj->num_free_queue + free_queue_idx;

    rc = MPIDU_genq_shmem_queue_dequeue(pool, &pool_obj->free_queues[idx], cell);
    MPIR_ERR_CHECK(rc);

  fn_exit:
    MPIR_FUNC_EXIT;
    return rc;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDU_genq_shmem_pool_cell_free(MPIDU_genq_shmem_pool_t pool, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    MPIR_FUNC_ENTER;

    rc = MPIDU_genq_shmem_queue_enqueue(pool, &pool_obj->free_queues[cell_h->block_idx], cell);
    MPIR_FUNC_EXIT;
    return rc;
}

#endif /* ifndef MPIDU_GENQ_SHMEM_POOL_H_INCLUDED */
