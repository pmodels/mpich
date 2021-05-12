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
#define MPIDI_UCX_DEFAULT_SHORT_SEND_SIZE      (8*1024)
#define MPIDI_UCX_AM_HDR_POOL_CELL_SIZE            (1024)
#define MPIDI_UCX_AM_HDR_POOL_NUM_CELLS_PER_CHUNK   (1024)
#define MPIDI_UCX_AM_HDR_POOL_MAX_NUM_CELLS         (256 * 1024)

/*
 *  UCX level am handler IDs are for differentiating AM message transferred in different modes:
 *  1. Bulk mode. Send data in one tag send regardless message size.
 *  2. Short mode. Send data in one tag send if message (with headers) is smaller than a predefined
 *  threshold.
 *  3. Pipeline mode. Send data in multiple tag sends.
 */
enum MPIDI_UCX_am_handler_ids {
    MPIDI_UCX_AM_HANDLER_ID__SHORT = 0,
    MPIDI_UCX_AM_HANDLER_ID__PIPELINE
};

typedef struct {
    ucp_worker_h worker;
    ucp_address_t *if_address;
    size_t addrname_len;
    char pad[MPL_CACHELINE_SIZE];
} MPIDI_UCX_context_t;

typedef struct {
    ucp_context_h context;
    MPIDI_UCX_context_t ctx[MPIDI_CH4_MAX_VCIS];
    int num_vnis;
    MPIDU_genq_private_pool_t am_hdr_buf_pool;
    MPIDU_genq_private_pool_t pack_buf_pool;
    MPIDI_UCX_deferred_am_isend_req_t *deferred_am_isend_q;
    void *req_map;
} MPIDI_UCX_global_t;

#define MPIDI_UCX_AV(av)     ((av)->netmod.ucx)

extern MPIDI_UCX_global_t MPIDI_UCX_global;
extern ucp_generic_dt_ops_t MPIDI_UCX_datatype_ops;

/* Default UCX TAG Layout */

/* 01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
 *  context_id (16) |source rank (16) | Message Tag (32)+ERROR BITS
 */

#define MPIDI_UCX_CONTEXT_ID_BITS 16
#define MPIDI_UCX_RANK_BITS CH4_UCX_RANKBITS
#define MPIDI_UCX_TAG_BITS  (64 - MPIDI_UCX_CONTEXT_ID_BITS - MPIDI_UCX_RANK_BITS)

#define MPIDI_UCX_RANK_SHIFT       MPIDI_UCX_TAG_BITS
#define MPIDI_UCX_CONTEXT_ID_SHIFT (MPIDI_UCX_TAG_BITS + MPIDI_UCX_RANK_BITS)

#define MPIDI_UCX_TAG_MASK  ((uint64_t) ((1ULL << MPIDI_UCX_TAG_BITS) - 1))
#define MPIDI_UCX_RANK_MASK ((uint64_t) (((1ULL << MPIDI_UCX_RANK_BITS) - 1) << MPIDI_UCX_RANK_SHIFT))
#define MPIDI_UCX_CONTEXT_ID_MASK \
    ((uint64_t) (((1ULL << MPIDI_UCX_CONTEXT_ID_BITS) - 1) << MPIDI_UCX_CONTEXT_ID_SHIFT))

#endif /* UCX_TYPES_H_INCLUDED */
