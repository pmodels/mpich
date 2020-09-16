/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED
#define MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED

#include "mpl.h"
#include "mpidu_init_shm.h"

typedef void *MPIDU_genq_shmem_pool_t;
typedef void *MPIDU_genq_shmem_queue_t;

typedef struct MPIDU_genqi_shmem_cell_header {
    uintptr_t handle;
    int block_idx;
    union {
        struct {
            uintptr_t next;
        } serial_queue;
        struct {
            uintptr_t prev;     /* used in the tail queue */
            uintptr_t next;     /* used in the detached header queue */
        } inverse_queue;
        struct {
            MPL_atomic_ptr_t next_m;
        } nem_queue;
    } u;
} MPIDU_genqi_shmem_cell_header_s;

typedef union MPIDU_genq_shmem_queue {
    struct {
        union {
            uintptr_t s;
            MPL_atomic_ptr_t m;
            uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];
        } head;
        union {
            uintptr_t s;
            MPL_atomic_ptr_t m;
            uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];
        } tail;
        unsigned flags;
    } q;
    uint8_t pad[3 * MPIDU_SHM_CACHE_LINE_LEN];
} MPIDU_genq_shmem_queue_u;

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

/* declare static inline prototypes to avoid circular inclusion */

int MPIDU_genq_shmem_queue_init(MPIDU_genq_shmem_queue_t queue, int flags);
static inline int MPIDU_genq_shmem_queue_dequeue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void **cell);
static inline int MPIDU_genq_shmem_queue_enqueue(MPIDU_genq_shmem_pool_t pool,
                                                 MPIDU_genq_shmem_queue_t queue, void *cell);

#endif /* ifndef MPIDU_GENQI_SHMEM_TYPES_H_INCLUDED */
