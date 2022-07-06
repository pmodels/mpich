/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_genq_private_pool.h"
#include "utlist.h"

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
    intptr_t num_used_cells;
    cell_block_s *prev;
    cell_block_s *next;
    cell_header_s *free_list_head;
    cell_block_s *next_free;
};

typedef struct MPIDU_genq_private_pool {
    intptr_t cell_size;
    intptr_t num_cells_in_block;
    intptr_t max_num_cells;

    MPIDU_genq_malloc_fn malloc_fn;
    MPIDU_genq_free_fn free_fn;

    intptr_t num_blocks;
    intptr_t max_num_blocks;
    cell_block_s *cell_blocks;
    cell_block_s *free_blocks_head;
    cell_block_s *free_blocks_tail;
} private_pool_s;

static int cell_block_alloc(private_pool_s * pool, cell_block_s ** block);

int MPIDU_genq_private_pool_create(intptr_t cell_size, intptr_t num_cells_in_block,
                                   intptr_t max_num_cells, MPIDU_genq_malloc_fn malloc_fn,
                                   MPIDU_genq_free_fn free_fn, MPIDU_genq_private_pool_t * pool)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj;

    MPIR_FUNC_ENTER;

    pool_obj = MPL_malloc(sizeof(private_pool_s), MPL_MEM_OTHER);

    /* internally enlarge the cell_size to accommodate cell header */
    pool_obj->cell_size = cell_size + CELL_HEADER_SIZE;
    pool_obj->num_cells_in_block = num_cells_in_block;
    MPIR_Assert(max_num_cells >= 0);
    if (max_num_cells == 0) {
        /* 0 means unlimited */
        pool_obj->max_num_blocks = 0;
    } else {
        pool_obj->max_num_blocks = max_num_cells / num_cells_in_block;
    }

    pool_obj->malloc_fn = malloc_fn;
    pool_obj->free_fn = free_fn;

    pool_obj->num_blocks = 0;

    pool_obj->cell_blocks = NULL;
    pool_obj->free_blocks_head = NULL;
    pool_obj->free_blocks_tail = NULL;

    *pool = (MPIDU_genq_private_pool_t) pool_obj;

    MPIR_FUNC_EXIT;
    return rc;
}

int MPIDU_genq_private_pool_destroy(MPIDU_genq_private_pool_t pool)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj = (private_pool_s *) pool;

    MPIR_FUNC_ENTER;

    cell_block_s *curr, *tmp;
    DL_FOREACH_SAFE(pool_obj->cell_blocks, curr, tmp) {
        DL_DELETE(pool_obj->cell_blocks, curr);
        pool_obj->free_fn(curr->slab);
        MPL_free(curr);
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

    new_block->free_list_head = NULL;
    /* init cell headers */
    for (int i = 0; i < pool->num_cells_in_block; i++) {
        cell_header_s *p = (void *) ((char *) new_block->slab + i * pool->cell_size);
        p->block = new_block;
        /* push to free list */
        p->next = new_block->free_list_head;
        new_block->free_list_head = p;
    }

    new_block->num_used_cells = 0;
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

static void cell_block_free(private_pool_s * pool_obj, cell_block_s * block)
{
    pool_obj->free_fn(block->slab);
    MPL_free(block);
}

static void append_free_blocks(private_pool_s * pool_obj, cell_block_s * block)
{
    block->next_free = NULL;
    if (pool_obj->free_blocks_head == NULL) {
        pool_obj->free_blocks_head = block;
        pool_obj->free_blocks_tail = block;
    } else {
        pool_obj->free_blocks_tail->next_free = block;
        pool_obj->free_blocks_tail = block;
    }
}

static void shift_free_blocks(private_pool_s * pool_obj, cell_block_s * block)
{
    pool_obj->free_blocks_head = pool_obj->free_blocks_head->next_free;
    if (pool_obj->free_blocks_head == NULL) {
        pool_obj->free_blocks_tail = NULL;
    }
}

static void remove_free_blocks(private_pool_s * pool_obj, cell_block_s * block)
{
    if (pool_obj->free_blocks_head == block) {
        shift_free_blocks(pool_obj, block);
    } else {
        cell_block_s *tmp_block = pool_obj->free_blocks_head;
        while (tmp_block->next_free != block) {
            tmp_block = tmp_block->next_free;
        }
        MPIR_Assert(tmp_block->next_free == block);
        tmp_block->next_free = tmp_block->next_free->next_free;
        if (pool_obj->free_blocks_tail == block) {
            pool_obj->free_blocks_tail = tmp_block;
        }
    }
}

int MPIDU_genq_private_pool_alloc_cell(MPIDU_genq_private_pool_t pool, void **cell)
{
    int rc = MPI_SUCCESS;
    private_pool_s *pool_obj = (private_pool_s *) pool;

    MPIR_FUNC_ENTER;

    if (!pool_obj->free_blocks_head) {
        /* try allocate more blocks if no free cell found */
        if (pool_obj->max_num_blocks > 0 && pool_obj->num_blocks >= pool_obj->max_num_blocks) {
            MPIR_ERR_SETANDJUMP(rc, MPI_ERR_OTHER, "**nomem");
        }

        cell_block_s *new_block;
        rc = cell_block_alloc(pool_obj, &new_block);
        MPIR_ERR_CHECK(rc);

        pool_obj->num_blocks++;
        DL_APPEND(pool_obj->cell_blocks, new_block);
        append_free_blocks(pool_obj, new_block);
    }

    cell_block_s *block = NULL;
    cell_header_s *cell_h = NULL;

    block = pool_obj->free_blocks_head;
    cell_h = block->free_list_head;
    block->free_list_head = cell_h->next;

    *cell = CELL_HEADER_TO_CELL(cell_h);
    MPIR_Assert(cell_h->block == block);
    block->num_used_cells++;

    /* remove from free_blocks_head if all cells are used */
    if (block->num_used_cells == pool_obj->num_cells_in_block) {
        shift_free_blocks(pool_obj, block);
    }

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
    cell_block_s *block = NULL;
    cell_header_s *cell_h = NULL;

    MPIR_FUNC_ENTER;

    if (cell == NULL) {
        goto fn_exit;
    }

    cell_h = CELL_TO_CELL_HEADER(cell);
    block = cell_h->block;

    cell_h->next = block->free_list_head;
    block->free_list_head = cell_h;

    block->num_used_cells--;

    if (block->num_used_cells == pool_obj->num_cells_in_block - 1) {
        append_free_blocks(pool_obj, block);
    } else if (block->num_used_cells == 0) {
        /* Avoid frequent re-allocation by preserving the last block.
         * All blocks will be freed when the pool is destroyed */
        if (pool_obj->num_blocks > 1 && pool_obj->max_num_blocks == 0) {
            remove_free_blocks(pool_obj, block);
            DL_DELETE(pool_obj->cell_blocks, block);
            cell_block_free(pool_obj, block);
            pool_obj->num_blocks--;
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return rc;
}
