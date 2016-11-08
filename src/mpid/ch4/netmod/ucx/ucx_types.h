/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef NETMOD_UCX_TYPES_H_INCLUDED
#define NETMOD_UCX_TYPES_H_INCLUDED
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>
#include "mpiimpl.h"

#define __SHORT_FILE__                          \
  (strrchr(__FILE__,'/')                        \
   ? strrchr(__FILE__,'/')+1                    \
   : __FILE__                                   \
)

#define UCP_PEER_NAME_MAX         HOST_NAME_MAX

#define MPIDI_MAP_NOT_FOUND      ((void*)(-1UL))

/* Active Message Stuff */
#define MPIDI_UCX_NUM_AM_BUFFERS       (64)
#define MPIDI_UCX_MAX_AM_EAGER_SZ      (16*1024)
#define MPIDI_UCX_AM_TAG               (1 << 28)
#define MPIDI_UCX_MAX_AM_HANDLERS      (64)

typedef struct {
    int avtid;
    ucp_context_h context;
    ucp_worker_h worker;
    char addrname[UCP_PEER_NAME_MAX];
    char *pmi_addr_table;
    size_t addrname_len;
    ucp_address_t *if_address;
    char kvsname[MPIDI_UCX_KVSAPPSTRLEN];
    char pname[MPI_MAX_PROCESSOR_NAME];
    int max_addr_len;
    MPIDI_NM_am_target_msg_cb target_msg_cbs[MPIDI_UCX_MAX_AM_HANDLERS];
    MPIDI_NM_am_origin_cb origin_cbs[MPIDI_UCX_MAX_AM_HANDLERS];
} MPIDI_UCX_global_t;

#define MPIDI_UCX_AV(av)     ((av)->netmod.ucx)

extern MPIDI_UCX_global_t MPIDI_UCX_global;

/* UCX TAG Layout */

/* 01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
 *  context_id (16) |source rank (16) | Message Tag (32)+ERROR BITS
 */

#define MPIDI_UCX_CONTEXT_TAG_BITS 16
#define MPIDI_UCX_CONTEXT_RANK_BITS 16
#define UCX_TAG_BITS 32

#define MPIDI_UCX_TAG_MASK      (0x00000000FFFFFFFFULL)
#define MPIDI_UCX_SOURCE_MASK   (0x0000FFFF00000000ULL)
#define MPIDI_UCX_TAG_SHIFT     (32)
#define MPIDI_UCX_SOURCE_SHIFT  (16)

#endif /* NETMOD_UCX_TYPES_H_INCLUDED */
