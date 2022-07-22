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

int MPIDU_genq_shmem_pool_create(uintptr_t cell_size, uintptr_t cells_per_proc,
                                 uintptr_t num_proc, int rank, MPIDU_genq_shmem_pool_t * pool);
int MPIDU_genq_shmem_pool_destroy(MPIDU_genq_shmem_pool_t pool);

static inline int MPIDU_genq_shmem_pool_cell_alloc(MPIDU_genq_shmem_pool_t pool, void **cell,
                                                   int block_idx)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    *cell = NULL;

    MPIR_FUNC_ENTER;

    rc = MPIDU_genq_shmem_queue_dequeue(pool, &pool_obj->free_queues[block_idx], cell);
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
