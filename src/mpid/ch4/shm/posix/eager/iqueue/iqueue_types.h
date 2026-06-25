/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED
#define POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "mpidu_genq.h"

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR 0x1
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA 0x2
#define MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_BUF 0x4

/* Each cell contains some data being communicated from one process to another.
 * The struct will be packed by default, occuping 16 bytes. Used in regular
 * queue and fast box */
typedef struct MPIDI_POSIX_eager_iqueue_cell {
    uint16_t type;              /* Type of cell (head/tail/etc.) */
    uint16_t from;              /* Who is the message in the cell from */
    uint32_t payload_size;      /* Size of the message in the cell */
    MPIDI_POSIX_am_header_t am_header;  /* If this cell is the beginning of a message, it will have
                                         * an active message header and this will point to it. */
} MPIDI_POSIX_eager_iqueue_cell_t;

typedef struct MPIDI_POSIX_eager_iqueue_cell_ext {
    MPIDI_POSIX_eager_iqueue_cell_t base;
    uint64_t buf_handle;
} MPIDI_POSIX_eager_iqueue_cell_ext_t;

/* Note we deliberately not handling or checking the counter wrap around case
 * because 64bit counters would take way too long to overflow. Generally, two 2
 * GHz CPU cores can sustain abount 200 million enqueue/dequeue per second with
 * ring buffer. That's >90 billion seconds (>2900 years) just updating atomic
 * counters without actually send any data. In reality, read/write data or having
 * multiple processes communicating will strech the time to overflow even further.
 */
typedef struct {
    /* *INDENT-OFF* */
    _Alignas(MPL_CACHELINE_SIZE) MPL_atomic_uint64_t seq;
    _Alignas(MPL_CACHELINE_SIZE) MPL_atomic_uint64_t ack;
    /* *INDENT-ON* */
} MPIDI_POSIX_eager_iqueue_rb_cntr_t;

typedef struct {
    int cell_size;
    int num_cells;
    struct {
        MPIDI_POSIX_eager_iqueue_rb_cntr_t *cntr;
        char *base;
        uint64_t next_seq;
        uint64_t last_ack;
    } send;
    struct {
        MPIDI_POSIX_eager_iqueue_rb_cntr_t *cntr;
        char *base;
        uint64_t next_seq;
        uint64_t last_ack;
    } recv;
    /* nice to have stuff, not really needed */
    int peer_rank;
    MPIDU_genq_shmem_pool_t cell_pool;
} MPIDI_POSIX_eager_iqueue_qp_t;

typedef struct MPIDI_POSIX_eager_iqueue_transport {
    int num_cells;              /* The number of cells allocated to each terminal in this transport */
    int size_of_cell;           /* The size of each of the cells in this transport */
    MPIDU_genq_shmem_queue_u *terminals;        /* The list of all the terminals that
                                                 * describe each of the cells */
    MPIDU_genq_shmem_queue_t my_terminal;
    MPIDU_genq_shmem_pool_t cell_pool;
    MPIDI_POSIX_eager_iqueue_qp_t **qp;
} MPIDI_POSIX_eager_iqueue_transport_t;

typedef struct MPIDI_POSIX_eager_iqueue_global {
    int max_vcis;
    /* sizes for shmem slabs */
    int slab_size;
    int terminal_offset;
    int qp_offset;
    /* shmem slabs */
    void *root_slab;
    void *all_vci_slab;
    /* 2d array indexed with [src_vci][dst_vci] */
    MPIDI_POSIX_eager_iqueue_transport_t transports[MPIDI_CH4_MAX_VCIS][MPIDI_CH4_MAX_VCIS];
} MPIDI_POSIX_eager_iqueue_global_t;

extern MPIDI_POSIX_eager_iqueue_global_t MPIDI_POSIX_eager_iqueue_global;

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_eager_iqueue_transport_t
    * MPIDI_POSIX_eager_iqueue_get_transport(int vci_src, int vci_dst)
{
    return &MPIDI_POSIX_eager_iqueue_global.transports[vci_src][vci_dst];
}

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell) \
    ((char*)(cell) + sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

#define MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport) \
    ((transport)->size_of_cell - sizeof(MPIDI_POSIX_eager_iqueue_cell_t))

#endif /* POSIX_EAGER_IQUEUE_TYPES_H_INCLUDED */
