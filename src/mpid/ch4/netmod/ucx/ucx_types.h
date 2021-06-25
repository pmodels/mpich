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
#define MPIDI_UCX_MAX_AM_EAGER_SZ      (8000)   /* internal max RTS 8256 */
#define MPIDI_UCX_AM_HANDLER_ID        (0)
#define MPIDI_UCX_AM_NBX_HANDLER_ID    (1)

typedef struct {
    ucp_worker_h worker;
    ucp_address_t *if_address;
    size_t addrname_len;
    char pad[MPL_CACHELINE_SIZE];
} MPIDI_UCX_context_t;

typedef struct {
    ucp_context_h context;
    MPIDI_UCX_context_t ctx[MPIDI_CH4_MAX_VCIS];
    int num_vcis;
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
