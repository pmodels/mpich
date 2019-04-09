/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED
#define POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_init_shm.h"

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL 0
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR 1
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA 2

typedef struct MPIDI_POSIX_eager_iqueue_cell MPIDI_POSIX_eager_iqueue_cell_t;

/* Each cell contains some data being communicated from one process to another. */
struct MPIDI_POSIX_eager_iqueue_cell {
    uint16_t type;              /* Type of cell (head/tail/etc.) */
    uint16_t from;              /* Who is the message in the cell from */
    uint32_t payload_size;      /* Size of the message in the cell */
    uintptr_t prev;             /* Internal pointer when finding the previous active cell. This is
                                 * the absolute offset of a cell in the shared memory segment and is
                                 * the same over all ranks. It is also visible to all ranks. */
    MPIDI_POSIX_eager_iqueue_cell_t *next;      /* Internal pointer when finding the next active
                                                 * cell. The pointer is a virtual memory address
                                                 * (private to the process). */
    MPIDI_POSIX_am_header_t am_header;  /* If this cell is the beginning of a message, it will have
                                         * an active message header and this will point to it. */
};

/* The terminal describes each of the iqueues, mostly by tracking where the head of the queue is at
 * any given time. */
typedef union {
    MPL_atomic_ptr_t head;      /* head of inverted queue of cells */
    uint8_t pad[MPIDU_SHM_CACHE_LINE_LEN];      /* Padding to make sure the terminals are cache
                                                 * aligned */
} MPIDI_POSIX_eager_iqueue_terminal_t;

typedef struct MPIDI_POSIX_eager_iqueue_transport {
    int num_cells;              /* The number of cells allocated to each terminal in this transport */
    int size_of_cell;           /* The size of each of the cells in this transport */
    void *pointer_to_shared_memory;     /* The entire shared memory region used by both the terminal
                                         * and the cells in this transport */
    void *cells;                /* Pointer to the memory containing all of the cells used by this
                                 * transport */
    MPIDI_POSIX_eager_iqueue_terminal_t *terminals;     /* The list of all the terminals that
                                                         * describe each of the cells */
    MPIDI_POSIX_eager_iqueue_cell_t *first_cell;        /* Internal variable to track the first cell
                                                         * to be received when receiving multiple
                                                         * cells. */
    MPIDI_POSIX_eager_iqueue_cell_t *last_cell; /* Internal variable to track the last cell to be
                                                 * received when receiving multiple cells. */
    MPIDI_POSIX_eager_iqueue_cell_t *last_read_cell;    /* Internal variable to track the last cell to
                                                         * be read, which is probably still in cache. */
} MPIDI_POSIX_eager_iqueue_transport_t;

extern MPIDI_POSIX_eager_iqueue_transport_t MPIDI_POSIX_eager_iqueue_transport_global;

static inline MPIDI_POSIX_eager_iqueue_transport_t *MPIDI_POSIX_eager_iqueue_get_transport()
{
    return &MPIDI_POSIX_eager_iqueue_transport_global;
}

#define MPIDI_POSIX_EAGER_IQUEUE_THIS_CELL(transport, index) \
    (MPIDI_POSIX_eager_iqueue_cell_t*)((char*)(transport)->cells + (size_t)(transport)->size_of_cell * (index))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell) \
    ((char*)(cell) + sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport) \
    ((transport)->size_of_cell - sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

/* The offset of memory to the beginning of the cell. */
#define MPIDI_POSIX_EAGER_IQUEUE_GET_HANDLE(transport, cell) \
    ((cell) ? ((uintptr_t)(cell) - (uintptr_t)(transport)->pointer_to_shared_memory) : 0)

/* Grab a specific cell by using the offset value (from the beginning of the memory used to store
 * all of the cells) of the handle. */
#define MPIDI_POSIX_EAGER_IQUEUE_GET_CELL(transport, handle) \
    (MPIDI_POSIX_eager_iqueue_cell_t*)((handle) ? \
        ((char*)(transport)->pointer_to_shared_memory + (uintptr_t)(handle)) : NULL);

#endif /* POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED */
