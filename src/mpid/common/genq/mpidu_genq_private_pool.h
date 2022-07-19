/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQ_PRIVATE_POOL_H_INCLUDED
#define MPIDU_GENQ_PRIVATE_POOL_H_INCLUDED

#include "mpidu_genq_common.h"
#include <stdint.h>

typedef void *MPIDU_genq_private_pool_t;

typedef enum {
    MPIDU_GENQ_PRIVATE_POOL_WARN_POOL_SIZE = 1
} MPIDU_genq_private_pool_status_e;

int MPIDU_genq_private_pool_create(intptr_t cell_size, intptr_t num_cells_in_block,
                                   intptr_t max_num_cells, MPIDU_genq_malloc_fn malloc_fn,
                                   MPIDU_genq_free_fn free_fn, MPIDU_genq_private_pool_t * pool);
int MPIDU_genq_private_pool_destroy(MPIDU_genq_private_pool_t pool);
int MPIDU_genq_private_pool_alloc_cell(MPIDU_genq_private_pool_t pool, void **cell);
int MPIDU_genq_private_pool_free_cell(MPIDU_genq_private_pool_t pool, void *cell);

#endif /* ifndef MPIDU_GENQ_PRIVATE_POOL_H_INCLUDED */
