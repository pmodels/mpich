/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED
#define POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_shm.h"

#define MPIDI_POSIX_EAGER_IQUEUE_DEFAULT_CELL_SIZE (64 * 1024)
#define MPIDI_POSIX_EAGER_IQUEUE_DEFAULT_NUMBER_OF_CELLS (32)

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL 0
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HEAD 1
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_TAIL 2

typedef struct MPIDI_POSIX_EAGER_IQUEUE_cell MPIDI_POSIX_EAGER_IQUEUE_cell_t;

struct MPIDI_POSIX_EAGER_IQUEUE_cell {
    uint16_t type;
    uint16_t from;
    uint32_t payload_size;
    uintptr_t prev;
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *next;
    MPIDI_POSIX_am_header_t am_header;
};

typedef union {
    /* head of inverted queue of cells */
    volatile uintptr_t head;
    uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];
} MPIDI_POSIX_EAGER_IQUEUE_terminal_t;

typedef struct MPIDI_POSIX_EAGER_IQUEUE_transport {
    MPIDU_shm_seg_t memory;
    MPIDU_shm_seg_info_t *seg;
    MPIDU_shm_barrier_t *barrier;
    int num_local;
    int num_cells;
    int size_of_cell;
    int local_rank;
    int *local_ranks;
    int *local_procs;
    void *pointer_to_shared_memory;
    void *cells;
    MPIDI_POSIX_EAGER_IQUEUE_terminal_t *terminals;
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *first_cell;
    MPIDI_POSIX_EAGER_IQUEUE_cell_t *last_cell;
} MPIDI_POSIX_EAGER_IQUEUE_transport_t;

extern MPIDI_POSIX_EAGER_IQUEUE_transport_t MPIDI_POSIX_EAGER_IQUEUE_transport_global;

static inline MPIDI_POSIX_EAGER_IQUEUE_transport_t *MPIDI_POSIX_EAGER_IQUEUE_get_transport()
{
    return &MPIDI_POSIX_EAGER_IQUEUE_transport_global;
}

#define MPIDI_POSIX_EAGER_IQUEUE_THIS_CELL(transport, index) \
    (MPIDI_POSIX_EAGER_IQUEUE_cell_t*)((char*)(transport)->cells + (size_t)(transport)->size_of_cell * (index))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell) \
    ((char*)(cell) + sizeof(MPIDI_POSIX_EAGER_IQUEUE_cell_t))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport) \
    ((transport)->size_of_cell - sizeof(MPIDI_POSIX_EAGER_IQUEUE_cell_t))

#define MPIDI_POSIX_EAGER_IQUEUE_GET_HANDLE(transport, cell) \
    ((cell) ? ((uintptr_t)(cell) - (uintptr_t)(transport)->pointer_to_shared_memory) : 0)

#define MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, handle) \
    (MPIDI_POSIX_EAGER_IQUEUE_cell_t*)((handle) ? \
        ((char*)(transport)->pointer_to_shared_memory + (uintptr_t)(handle)) : NULL);

#endif /* POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED */
