/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_PRE_H_INCLUDED
#define IPC_PRE_H_INCLUDED

#include "../xpmem/xpmem_pre.h"
#include "../gpu/gpu_pre.h"
#include "ipc_types.h"

/* request extension */
typedef struct MPIDI_IPC_am_unexp_rreq {
    MPIDI_IPCI_ipc_handle_t ipc_handle;
    uint64_t data_sz;
    MPIR_Request *sreq_ptr;
    int src_lrank;
} MPIDI_IPC_am_unexp_rreq_t;

typedef struct MPIDI_IPC_am_request {
    MPIDI_IPCI_type_t ipc_type;
    union {
        MPIDI_XPMEM_am_request_t xpmem;
    } u;
    MPIDI_IPC_am_unexp_rreq_t unexp_rreq;
} MPIDI_IPC_am_request_t;

/* ctrl packet header types */
typedef struct MPIDI_IPC_ctrl_send_contig_lmt_rts {
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_ipc_handle_t ipc_handle;
    uint64_t data_sz;           /* data size in bytes */
    MPIR_Request *sreq_ptr;     /* send request pointer */
    int src_lrank;              /* sender rank on local node */

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_IPC_ctrl_send_contig_lmt_rts_t;

typedef struct MPIDI_IPC_ctrl_send_contig_lmt_fin {
    MPIDI_IPCI_type_t ipc_type;
    MPIR_Request *req_ptr;
} MPIDI_IPC_ctrl_send_contig_lmt_fin_t;

#endif /* IPC_PRE_H_INCLUDED */
