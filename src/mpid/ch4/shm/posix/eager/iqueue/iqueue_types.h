/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED
#define POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "mpidu_genq.h"

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR 0
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA 1

typedef struct MPIDI_POSIX_eager_iqueue_cell MPIDI_POSIX_eager_iqueue_cell_t;

/* Each cell contains some data being communicated from one process to another. */
struct MPIDI_POSIX_eager_iqueue_cell {
    uint16_t type;              /* Type of cell (head/tail/etc.) */
    uint16_t from;              /* Who is the message in the cell from */
    uint32_t payload_size;      /* Size of the message in the cell */
    MPIDI_POSIX_am_header_t am_header;  /* If this cell is the beginning of a message, it will have
                                         * an active message header and this will point to it. */
};

typedef struct MPIDI_POSIX_eager_iqueue_transport {
    int num_cells;              /* The number of cells allocated to each terminal in this transport */
    int size_of_cell;           /* The size of each of the cells in this transport */
    MPIDU_genq_shmem_queue_u *terminals;        /* The list of all the terminals that
                                                 * describe each of the cells */
    MPIDU_genq_shmem_queue_t my_terminal;
    MPIDU_genq_shmem_pool_t cell_pool;
} MPIDI_POSIX_eager_iqueue_transport_t;

extern MPIDI_POSIX_eager_iqueue_transport_t MPIDI_POSIX_eager_iqueue_transport_global;

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_eager_iqueue_transport_t
    * MPIDI_POSIX_eager_iqueue_get_transport(void)
{
    return &MPIDI_POSIX_eager_iqueue_transport_global;
}

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell) \
    ((char*)(cell) + sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport) \
    ((transport)->size_of_cell - sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

#endif /* POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED */
