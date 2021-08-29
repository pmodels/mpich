/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_private_pool.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* We reserve the front of each cell as cell header. It must be size of maximum alignment */
#define CELL_HEADER_SIZE 16

#define CELL_HEADER_TO_CELL(p) (void *) ((char *) (p) + CELL_HEADER_SIZE)
#define CELL_TO_CELL_HEADER(p) (cell_header_s *) ((char *) (p) - CELL_HEADER_SIZE)

typedef struct cell_header cell_header_s;
typedef struct cell_block cell_block_s;

struct cell_header {
    cell_block_s *block;
    cell_header_s *next;
};

struct cell_block {
    void *slab;
    cell_block_s *next;
};

typedef struct MPIDU_genq_private_pool {
    intptr_t cell_size;
    intptr_t num_cells_in_block;
    intptr_t max_num_cells;

    MPIDU_genq_malloc_fn malloc_fn;
    MPIDU_genq_free_fn free_fn;

    intptr_t num_blocks;
    intptr_t max_num_blocks;
    cell_block_s *cell_blocks_head;
    cell_block_s *cell_blocks_tail;
    cell_header_s *free_list_head;
} private_pool_s;

static int cell_block_alloc(private_pool_s * pool, cell_block_s ** block);

int MPIDU_genq_private_pool_create_unsafe(intptr_t cell_size, intptr_t num_cells_in_block,
                                          intptr_t max_num_cells, MPIDU_genq_malloc_fn malloc_fn,
                                          MPIDU_genq_free_fn free_fn,
                                          MPIDU_genq_private_pool_t * pool)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj;

    MPIR_FUNC_ENTER;

    pool_obj = MPL_malloc(sizeof(private_pool_s), MPL_MEM_OTHER);

    pool_obj->cell_size = cell_size;
    pool_obj->num_cells_in_block = num_cells_in_block;
    pool_obj->max_num_cells = max_num_cells;

    pool_obj->malloc_fn = malloc_fn;
    pool_obj->free_fn = free_fn;

    pool_obj->num_blocks = 0;
    pool_obj->max_num_blocks = max_num_cells / num_cells_in_block;

    pool_obj->cell_blocks_head = NULL;
    pool_obj->cell_blocks_tail = NULL;

    pool_obj->free_list_head = NULL;

    *pool = (MPIDU_genq_private_pool_t) pool_obj;

    MPIR_FUNC_EXIT;
    return rc;
}

int MPIDU_genq_private_pool_destroy_unsafe(MPIDU_genq_private_pool_t pool)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj = (private_pool_s *) pool;

    MPIR_FUNC_ENTER;

    for (cell_block_s * block = pool_obj->cell_blocks_head; block;) {
        cell_block_s *next = block->next;

        pool_obj->free_fn(block->slab);
        MPL_free(block);
        block = next;
    }

    /* free self */
    MPL_free(pool_obj);

    MPIR_FUNC_EXIT;
    return rc;
}

static int cell_block_alloc(private_pool_s * pool, cell_block_s ** block)
{
    int rc = MPI_SUCCESS;
    cell_block_s *new_block = NULL;

    new_block = (cell_block_s *) MPL_malloc(sizeof(cell_block_s), MPL_MEM_OTHER);

    MPIR_ERR_CHKANDJUMP(!new_block, rc, MPI_ERR_OTHER, "**nomem");

    new_block->slab = pool->malloc_fn(pool->num_cells_in_block * pool->cell_size);
    MPIR_ERR_CHKANDJUMP(!new_block->slab, rc, MPI_ERR_OTHER, "**nomem");

    /* init cell headers */
    for (int i = 0; i < pool->num_cells_in_block; i++) {
        cell_header_s *p = (void *) ((char *) new_block->slab + i * pool->cell_size);
        p->block = new_block;
        /* push to free list */
        p->next = pool->free_list_head;
        pool->free_list_head = p;
    }

    new_block->next = NULL;

    *block = new_block;

  fn_exit:
    return rc;
  fn_fail:
    if (new_block) {
        pool->free_fn(new_block->slab);
    }
    MPL_free(new_block);
    *block = NULL;
    goto fn_exit;
}

int MPIDU_genq_private_pool_alloc_cell(MPIDU_genq_private_pool_t pool, void **cell)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj = (private_pool_s *) pool;
    cell_header_s *cell_h = NULL;

    MPIR_FUNC_ENTER;

    if (!pool_obj->free_list_head) {
        /* try allocate more blocks if no free cell found */
        MPIR_Assert(pool_obj->num_blocks <= pool_obj->max_num_blocks);
        if (pool_obj->num_blocks == pool_obj->max_num_blocks) {
            *cell = NULL;
            MPIR_ERR_SETANDJUMP(rc, MPI_ERR_OTHER, "**nomem");
        }

        cell_block_s *new_block;
        rc = cell_block_alloc(pool_obj, &new_block);
        MPIR_ERR_CHECK(rc);

        pool_obj->num_blocks++;

        if (pool_obj->cell_blocks_head == NULL) {
            pool_obj->cell_blocks_head = new_block;
            pool_obj->cell_blocks_tail = new_block;
        } else {
            pool_obj->cell_blocks_tail->next = new_block;
            pool_obj->cell_blocks_tail = new_block;
        }
    }

    MPIR_Assert(pool_obj->free_list_head != NULL);
    cell_h = pool_obj->free_list_head;
    pool_obj->free_list_head = cell_h->next;
    *cell = CELL_HEADER_TO_CELL(cell_h);

  fn_exit:
    MPIR_FUNC_EXIT;
    return rc;
  fn_fail:
    *cell = NULL;
    goto fn_exit;
}

int MPIDU_genq_private_pool_free_cell(MPIDU_genq_private_pool_t pool, void *cell)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj = (private_pool_s *) pool;
    cell_header_s *cell_h = NULL;

    MPIR_FUNC_ENTER;

    if (cell == NULL) {
        goto fn_exit;
    }

    cell_h = CELL_TO_CELL_HEADER(cell);

    cell_h->next = pool_obj->free_list_head;
    pool_obj->free_list_head = cell_h;

  fn_exit:
    MPIR_FUNC_EXIT;
    return rc;
}
