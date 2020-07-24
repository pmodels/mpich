/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_TYPES_H_INCLUDED
#define UCX_TYPES_H_INCLUDED
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>
#include "mpiimpl.h"

#define __SHORT_FILE__                          \
  (strrchr(__FILE__,'/')                        \
   ? strrchr(__FILE__,'/')+1                    \
   : __FILE__                                   \
)

#define UCP_PEER_NAME_MAX         HOST_NAME_MAX

/* Active Message Stuff */
#define MPIDI_UCX_MAX_AM_EAGER_SZ      (16*1024)
#define MPIDI_UCX_AM_TAG               (1ULL << MPIR_Process.tag_bits)
#define MPIDI_UCX_AM_BUF_COUNT         8
#define MPIDI_UCX_AM_BUF_SIZE          MPIDI_UCX_MAX_AM_EAGER_SZ
#define MPIDI_UCX_AM_PACK_LIMIT        1024

/* Circular buffer for FIFO queue, to handle when multiple am msgs arrive
 * together.
 * For simplicity, we require size to be power of 2 and strictly greater than
 * MPIDI_UCX_AM_BUF_COUNT (to avoid boundary checking)
 */
#define MPIDI_UCX_AM_QUEUE_SIZE  16
#define MPIDI_UCX_AM_QUEUE_MASK  0xf

typedef struct {
    ucp_worker_h worker;
    ucp_address_t *if_address;
    size_t addrname_len;
} MPIDI_UCX_context_t;

typedef struct {
    ucp_context_h context;
    MPIDI_UCX_context_t ctx[MPIDI_CH4_MAX_VCIS];

    char am_bufs[MPIDI_UCX_AM_BUF_COUNT][MPIDI_UCX_AM_BUF_SIZE] MPL_ATTR_ALIGNED(8);
    MPIDI_UCX_ucp_request_t *am_ucp_reqs[MPIDI_UCX_AM_BUF_COUNT];
    /* simple stacks to track am messages that needs to be progressed */
    int am_ready_queue[MPIDI_UCX_AM_QUEUE_SIZE];
    int am_ready_head;
    int am_ready_tail;

    int num_vnis;
} MPIDI_UCX_global_t;

#define MPIDI_UCX_AV(av)     ((av)->netmod.ucx)

extern MPIDI_UCX_global_t MPIDI_UCX_global;
extern ucp_generic_dt_ops_t MPIDI_UCX_datatype_ops;

/* UCX TAG Layout */

/* 01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
 *  context_id (16) |source rank (16) | Message Tag (32)+ERROR BITS
 */

#define MPIDI_UCX_CONTEXT_TAG_BITS 16
#define MPIDI_UCX_CONTEXT_RANK_BITS 16

#define MPIDI_UCX_TAG_BITS      31
#define MPIDI_UCX_TAG_MASK      (0x00000000FFFFFFFFULL)
#define MPIDI_UCX_SOURCE_MASK   (0x0000FFFF00000000ULL)
#define MPIDI_UCX_TAG_SHIFT     (32)
#define MPIDI_UCX_SOURCE_SHIFT  (16)

#define MPIDI_UCX_AM_HDR_TAG    (0x0000000080000000ULL)
#define MPIDI_UCX_AM_DATA_TAG   (0x00000000C0000000ULL)
#define MPIDI_UCX_AM_HDR_MASK   (0xFFFF0000FFFFFFFFULL)
#define MPIDI_UCX_AM_DATA_MASK  (0xFFFFFFFFFFFFFFFFULL)

#endif /* UCX_TYPES_H_INCLUDED */
