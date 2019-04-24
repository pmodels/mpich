/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
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
#define MPIDI_UCX_NUM_AM_BUFFERS       (64)
#define MPIDI_UCX_MAX_AM_EAGER_SZ      (16*1024)
#define MPIDI_UCX_AM_TAG               (1ULL << MPIR_Process.tag_bits)

typedef struct {
    int is_free;
    size_t addrname_len;
    ucp_address_t *if_address;
    ucp_worker_h worker;
} MPIDI_UCX_vni_t;

typedef struct {
    int next_free_vni;
    int total_vnis;
    MPIDI_UCX_vni_t vni[MPIDI_UCX_MAX_WORKERS];
} MPIDI_UCX_vni_pool_t;

typedef struct {
    int avtid;
    ucp_context_h context;
    char addrname[UCP_PEER_NAME_MAX];
    char *pmi_addr_table;
    char kvsname[MPIDI_UCX_KVSAPPSTRLEN];
    char pname[MPI_MAX_PROCESSOR_NAME];
    int max_addr_len;
    MPIDI_UCX_vni_pool_t vni_pool;
} MPIDI_UCX_global_t;

#define MPIDI_UCX_AV(av)     ((av)->netmod.ucx)
#define MPIDI_UCX_VNI_POOL(field) MPIDI_UCX_global.vni_pool.field
#define MPIDI_UCX_VNI(i) MPIDI_UCX_VNI_POOL(vni)[i]

extern MPIDI_UCX_global_t MPIDI_UCX_global;
extern ucp_generic_dt_ops_t MPIDI_UCX_datatype_ops;

/* UCX TAG Layout */

/* 01234567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
 *  context_id (16) |source rank (16) | Message Tag (32)+ERROR BITS
 */

#define MPIDI_UCX_CONTEXT_TAG_BITS 16
#define MPIDI_UCX_CONTEXT_RANK_BITS 16

#define MPIDI_UCX_TAG_MASK      (0x00000000FFFFFFFFULL)
#define MPIDI_UCX_SOURCE_MASK   (0x0000FFFF00000000ULL)
#define MPIDI_UCX_TAG_SHIFT     (32)
#define MPIDI_UCX_SOURCE_SHIFT  (16)

#endif /* UCX_TYPES_H_INCLUDED */
