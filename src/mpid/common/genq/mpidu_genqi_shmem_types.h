/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED
#define MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED

#include "mpl.h"
#include "mpidu_init_shm.h"

union MPIDU_genq_shmem_queue;

typedef struct MPIDU_genqi_shmem_cell_header {
    uintptr_t handle;
    int block_idx;
    MPL_atomic_int_t in_use;
    uintptr_t next;
    uintptr_t prev;
} MPIDU_genqi_shmem_cell_header_s;

typedef struct MPIDU_genqi_shmem_pool {
    uintptr_t cell_size;
    uintptr_t cell_alloc_size;
    uintptr_t cells_per_proc;
    uintptr_t num_proc;
    int rank;

    void *slab;
    MPIDU_genqi_shmem_cell_header_s *cell_header_base;
    MPIDU_genqi_shmem_cell_header_s **cell_headers;
    union MPIDU_genq_shmem_queue *free_queues;
} MPIDU_genqi_shmem_pool_s;

#define HEADER_TO_CELL(header) \
    ((char *) (header) + sizeof(MPIDU_genqi_shmem_cell_header_s))

#define CELL_TO_HEADER(cell) \
    ((MPIDU_genqi_shmem_cell_header_s *) ((char *) (cell) \
                                          - sizeof(MPIDU_genqi_shmem_cell_header_s)))

/* The handle value is calculated using (address_offset + 1), see MPIDU_genq_shmem_pool.h */
#define HANDLE_TO_HEADER(pool, handle) \
    ((MPIDU_genqi_shmem_cell_header_s *) ((char *) (pool)->cell_header_base + (uintptr_t) (handle) \
                                          - 1))

#endif /* ifndef MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED */
