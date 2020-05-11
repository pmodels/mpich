/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_PRE_H_INCLUDED
#define IPC_PRE_H_INCLUDED

#include "../xpmem/xpmem_pre.h"
#include "ipc_types.h"

/* request extension */
typedef struct MPIDI_IPC_am_unexp_rreq {
    MPIDI_IPCI_mem_handle_t mem_handle;
    uint64_t data_sz;
    uint64_t sreq_ptr;
    int src_lrank;
} MPIDI_IPC_am_unexp_rreq_t;

typedef struct MPIDI_IPC_am_request {
    MPIDI_IPCI_type_t ipc_type;
    union {
        MPIDI_XPMEM_am_request_t xpmem;
    } u;
    MPIDI_IPC_am_unexp_rreq_t unexp_rreq;
} MPIDI_IPC_am_request_t;

/* window extension */
typedef struct MPIDI_IPC_win {
    MPIDI_IPCI_mem_seg_t *mem_segs;     /* store opened memory handles
                                         * for all local processes in the window. */
} MPIDI_IPC_win_t;

/* ctrl packet header types */
typedef struct MPIDI_SHM_ctrl_ipc_send_contig_lmt_rts {
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_mem_handle_t mem_handle;
    uint64_t data_sz;           /* data size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    int src_lrank;              /* sender rank on local node */

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_SHM_ctrl_ipc_send_contig_lmt_rts_t;

typedef struct MPIDI_SHM_ctrl_ipc_send_contig_lmt_fin {
    MPIDI_IPCI_type_t ipc_type;
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_ipc_send_contig_lmt_fin_t;

#endif /* IPC_PRE_H_INCLUDED */
