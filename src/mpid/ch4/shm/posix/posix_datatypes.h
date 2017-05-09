/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_DATATYPES_H_INCLUDED
#define POSIX_DATATYPES_H_INCLUDED

/* ************************************************************************** */
/* from mpid/ch3/channels/nemesis/include/mpid_nem_datatypes.h                */
/* ************************************************************************** */

#define MPIDI_POSIX_OFFSETOF(struc, field) ((int)(&((struc *)0)->field))
#define MPIDI_POSIX_CACHE_LINE_LEN (64)
#define MPIDI_POSIX_NUM_CELLS      (64)
#define MPIDI_POSIX_CELL_LEN       (64*1024)

#if (SIZEOF_OPA_PTR_T > 8)
#if (SIZEOF_OPA_PTR_T > 16)
#error unexpected size for OPA_ptr_t
#endif
#define MPIDI_POSIX_CELL_HEAD_LEN  16   /* We use this to keep elements 64-bit aligned */
#else /* (SIZEOF_OPA_PTR_T <= 8) */
#define MPIDI_POSIX_CELL_HEAD_LEN  8    /* We use this to keep elements 64-bit aligned */
#endif

/* cell header with matching info now is cache line */
#define MPIDI_POSIX_CELL_PAYLOAD_LEN (MPIDI_POSIX_CELL_LEN - MPIDI_POSIX_CACHE_LINE_LEN + MPIDI_POSIX_MPICH_HEAD_LEN)

#define MPIDI_POSIX_CALC_CELL_LEN(cellp) (MPIDI_POSIX_CELL_HEAD_LEN + MPIDI_POSIX_MPICH_HEAD_LEN + MPIDI_POSIX_CELL_DLEN (cell))

#define MPIDI_POSIX_ALIGNED(addr, bytes) ((((unsigned long)addr) & (((unsigned long)bytes)-1)) == 0)

#define MPIDI_POSIX_PKT_UNKNOWN     0
#define MPIDI_POSIX_PKT_MPICH      1
#define MPIDI_POSIX_PKT_MPICH_HEAD 2

#define MPIDI_POSIX_FBOX_SOURCE(cell) (MPIDI_POSIX_mem_region.local_procs[(cell)->pkt.mpich.source])
#define MPIDI_POSIX_CELL_SOURCE(cell) ((cell)->pkt.mpich.source)
#define MPIDI_POSIX_CELL_DEST(cell)   ((cell)->pkt.mpich.dest)
#define MPIDI_POSIX_CELL_DLEN(cell)   ((cell)->pkt.mpich.datalen)
#define MPIDI_POSIX_CELL_SEQN(cell)   ((cell)->pkt.mpich.seqno)

#define MPIDI_POSIX_MPICH_HEAD_LEN sizeof(MPIDI_POSIX_pkt_header_t)
#define MPIDI_POSIX_DATA_LEN (MPIDI_POSIX_CELL_PAYLOAD_LEN - MPIDI_POSIX_MPICH_HEAD_LEN)

#define MPIDI_POSIX_PKT_HEADER_FIELDS          \
    int source;                             \
    int dest;                               \
    uintptr_t datalen;                      \
    unsigned short seqno;                   \
    unsigned short type;        /* currently used only with checkpointing */

typedef struct MPIDI_POSIX_pkt_header {
    MPIDI_POSIX_PKT_HEADER_FIELDS
} MPIDI_POSIX_pkt_header_t;

typedef struct MPIDI_POSIX_pkt_mpich {
    MPIDI_POSIX_PKT_HEADER_FIELDS
    union {
        char payload[MPIDI_POSIX_DATA_LEN];
        double dummy;           /* align paylod to double */
    } p;
} MPIDI_POSIX_pkt_mpich_t;

typedef union {
    MPIDI_POSIX_pkt_header_t header;
    MPIDI_POSIX_pkt_mpich_t mpich;
} MPIDI_POSIX_pkt_t;

/* Nemesis cells which are to be used in shared memory need to use
 * "relative pointers" because the absolute pointers to a cell from
 * different processes may be different.  Relative pointers are
 * offsets from the beginning of the mmapped region where they live.
 * We use different types for relative and absolute pointers to help
 * catch errors.  Use MPIDI_POSIX_REL_TO_ABS and MPIDI_POSIX_ABS_TO_REL to
 * convert between relative and absolute pointers. */

/* This should always be exactly the size of a pointer */
typedef struct MPIDI_POSIX_cell_rel_ptr {
    OPA_ptr_t p;
} MPIDI_POSIX_cell_rel_ptr_t;

/* MPIDI_POSIX_cell and MPIDI_POSIX_abs_cell must be kept in sync so that we
 * can cast between them.  MPIDI_POSIX_abs_cell should only be used when
 * a cell is enqueued on a queue local to a single process (e.g., a
 * queue in a network module) where relative pointers are not
 * needed. */

typedef struct MPIDI_POSIX_cell {
    MPIDI_POSIX_cell_rel_ptr_t next;
#if (MPIDI_POSIX_CELL_HEAD_LEN > SIZEOF_OPA_PTR_T)
    char padding[MPIDI_POSIX_CELL_HEAD_LEN - sizeof(MPIDI_POSIX_cell_rel_ptr_t)];
#endif
    int my_rank;
    int rank;
    int tag;
    int context_id;
    MPIR_Request *pending;
#if MPIDI_POSIX_CACHE_LINE_LEN != 0
    char padding[MPIDI_POSIX_CACHE_LINE_LEN - MPIDI_POSIX_CELL_HEAD_LEN - MPIDI_POSIX_MPICH_HEAD_LEN - 4 * sizeof(int) - sizeof(MPIR_Request *)];       /* should be 64-16-16-16-8 = 8 */
#endif
    volatile MPIDI_POSIX_pkt_t pkt;
} MPIDI_POSIX_cell_t;
typedef MPIDI_POSIX_cell_t *MPIDI_POSIX_cell_ptr_t;

typedef struct MPIDI_POSIX_abs_cell {
    struct MPIDI_POSIX_abs_cell *next;
#if (MPIDI_POSIX_CELL_HEAD_LEN > SIZEOF_VOID_P)
    char padding[MPIDI_POSIX_CELL_HEAD_LEN - sizeof(struct MPIDI_POSIX_abs_cell *)];
#endif
    volatile MPIDI_POSIX_pkt_t pkt;
} MPIDI_POSIX_abs_cell_t;
typedef MPIDI_POSIX_abs_cell_t *MPIDI_POSIX_abs_cell_ptr_t;

#define MPIDI_POSIX_CELL_TO_PACKET(cellp) (&(cellp)->pkt)
#define MPIDI_POSIX_PACKET_TO_CELL(packetp) \
    ((MPIDI_POSIX_cell_ptr_t) ((char*)(packetp) - (char *)MPIDI_POSIX_CELL_TO_PACKET((MPIDI_POSIX_cell_ptr_t)0)))
#define MPIDI_POSIX_MIN_PACKET_LEN (sizeof (MPIDI_POSIX_pkt_header_t))
#define MPIDI_POSIX_MAX_PACKET_LEN (sizeof (MPIDI_POSIX_pkt_t))
#define MPIDI_POSIX_PACKET_LEN(pkt) ((pkt)->mpich.datalen + MPIDI_POSIX_MPICH_HEAD_LEN)

#define MPIDI_POSIX_OPT_LOAD     16
#define MPIDI_POSIX_OPT_SIZE     ((sizeof(MPIDI_CH3_Pkt_t)) + (MPIDI_POSIX_OPT_LOAD))
#define MPIDI_POSIX_OPT_HEAD_LEN ((MPIDI_POSIX_MPICH_HEAD_LEN) + (MPIDI_POSIX_OPT_SIZE))

#define MPIDI_POSIX_PACKET_OPT_LEN(pkt) \
    (((pkt)->mpich.datalen < MPIDI_POSIX_OPT_SIZE) ? (MPIDI_POSIX_OPT_HEAD_LEN) : (MPIDI_POSIX_PACKET_LEN(pkt)))

#define MPIDI_POSIX_PACKET_PAYLOAD(pkt) ((pkt)->mpich.payload)

typedef struct MPIDI_POSIX_queue {
    MPIDI_POSIX_cell_rel_ptr_t head;
    MPIDI_POSIX_cell_rel_ptr_t tail;
#if (MPIDI_POSIX_CACHE_LINE_LEN > (2 * SIZEOF_OPA_PTR_T))
    char padding1[MPIDI_POSIX_CACHE_LINE_LEN - 2 * sizeof(MPIDI_POSIX_cell_rel_ptr_t)];
#endif
    MPIDI_POSIX_cell_rel_ptr_t my_head;
#if (MPIDI_POSIX_CACHE_LINE_LEN > SIZEOF_OPA_PTR_T)
    char padding2[MPIDI_POSIX_CACHE_LINE_LEN - sizeof(MPIDI_POSIX_cell_rel_ptr_t)];
#endif
#ifndef MPIDI_POSIX_USE_LOCK_FREE_QUEUES
    /* see FIXME in mpid_nem_queue.h */
#define MPIDI_POSIX_queue_mutex_t MPID_Thread_mutex_t
    MPIDI_POSIX_queue_mutex_t lock;
    char padding3[MPIDI_POSIX_CACHE_LINE_LEN - sizeof(MPID_Thread_mutex_t)];
#endif
}
MPIDI_POSIX_queue_t, *MPIDI_POSIX_queue_ptr_t;

/* Fast Boxes*/
typedef union {
    OPA_int_t value;
#if MPIDI_POSIX_CACHE_LINE_LEN != 0
    char padding[MPIDI_POSIX_CACHE_LINE_LEN];
#endif
} MPIDI_POSIX_opt_volint_t;

typedef struct MPIDI_POSIX_fbox_common {
    MPIDI_POSIX_opt_volint_t flag;
} MPIDI_POSIX_fbox_common_t, *MPIDI_POSIX_fbox_common_ptr_t;

typedef struct MPIDI_POSIX_fbox_mpich {
    MPIDI_POSIX_opt_volint_t flag;
    MPIDI_POSIX_cell_t cell;
} MPIDI_POSIX_fbox_mpich_t;

#define MPIDI_POSIX_FBOX_DATALEN MPIDI_POSIX_DATA_LEN

typedef union {
    MPIDI_POSIX_fbox_common_t common;
    MPIDI_POSIX_fbox_mpich_t mpich;
} MPIDI_POSIX_fastbox_t;

typedef struct MPIDI_POSIX_fbox_arrays {
    MPIDI_POSIX_fastbox_t **in;
    MPIDI_POSIX_fastbox_t **out;
} MPIDI_POSIX_fbox_arrays_t;

#endif /* POSIX_DATATYPES_H_INCLUDED */
