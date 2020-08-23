/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_SHMEM_POOL_H_INCLUDED
#define MPIDU_GENQ_SHMEM_POOL_H_INCLUDED

#include "mpidimpl.h"
#include "mpidu_genqi_shmem_types.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int MPIDU_genq_shmem_pool_create_unsafe(uintptr_t cell_size, uintptr_t cells_per_proc,
                                        uintptr_t num_proc, int rank,
                                        MPIDU_genq_shmem_pool_t * pool);
int MPIDU_genq_shmem_pool_destroy_unsafe(MPIDU_genq_shmem_pool_t pool);

static inline int MPIDU_genq_shmem_pool_cell_alloc(MPIDU_genq_shmem_pool_t pool, void **cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_pool_s *pool_obj = (MPIDU_genqi_shmem_pool_s *) pool;

    *cell = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_ALLOC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_ALLOC);

    /* iteratively find a unused cell */
    for (int i = 0; i < pool_obj->cells_per_proc; i++) {
        if (!MPL_atomic_load_int(&pool_obj->cell_headers[i]->in_use)) {
            *cell = HEADER_TO_CELL(pool_obj->cell_headers[i]);
            MPL_atomic_store_int(&pool_obj->cell_headers[i]->in_use, 1);
            goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_ALLOC);
    return rc;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDU_genq_shmem_pool_cell_free(MPIDU_genq_shmem_pool_t pool, void *cell)
{
    int rc = MPI_SUCCESS;
    MPIDU_genqi_shmem_cell_header_s *cell_h = CELL_TO_HEADER(cell);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_FREE);

    MPL_atomic_store_int(&cell_h->in_use, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_GENQ_SHMEM_POOL_CELL_FREE);
    return rc;
}

#endif /* ifndef MPIDU_GENQ_SHMEM_POOL_H_INCLUDED */
