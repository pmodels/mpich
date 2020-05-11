/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_TYPES_H_INCLUDED
#define SHM_TYPES_H_INCLUDED

#include "../ipc/src/ipc_pre.h"

typedef enum {
    MPIDI_SHM_IPC_SEND_LMT_RTS, /* issued by sender to initialize single-copy based IPC with sbuf info */
    MPIDI_SHM_IPC_SEND_LMT_FIN, /* issued by receiver to notify completion of sender-initialized single-copy based IPC */

    /* submodule specific protocols.
     * TODO: can these protocols be generalized for all IPC modules? */
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_IPC_XPMEM_SEND_LMT_CTS,   /* issued by receiver with rbuf info after receiving RTS if it performs coop copy. */
    MPIDI_SHM_IPC_XPMEM_SEND_LMT_SEND_FIN,      /* issued by sender to notify completion of coop copy */
    MPIDI_SHM_IPC_XPMEM_SEND_LMT_RECV_FIN,      /* issued by receiver to notify completion of coop copy */
    MPIDI_SHM_IPC_XPMEM_SEND_LMT_CNT_FREE,      /* issued by sender to notify free counter obj in coop copy */
#endif
    MPIDI_SHM_CTRL_IDS_MAX
} MPIDI_SHM_ctrl_id_t;

typedef struct MPIDI_SHM_ctrl_ipc_send_lmt_rts {
    MPIDI_SHM_IPC_type_t ipc_type;
    uint64_t src_offset;        /* send data starting address (buffer + true_lb) */
    uint64_t data_sz;           /* data size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    int src_lrank;              /* sender rank on local node */

    /* matching info */
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_SHM_ctrl_ipc_send_lmt_rts_t;

typedef struct MPIDI_SHM_ctrl_ipc_send_lmt_fin {
    MPIDI_SHM_IPC_type_t ipc_type;
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_ipc_send_lmt_fin_t;

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
typedef struct MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cts {
    uint64_t dest_offset;       /* receiver buffer starting address (buffer + true_lb) */
    uint64_t data_sz;           /* receiver buffer size in bytes */
    uint64_t sreq_ptr;          /* send request pointer */
    uint64_t rreq_ptr;          /* recv request pointer */
    int dest_lrank;             /* receiver rank on local node */

    /* counter info */
    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
} MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cts_t;

typedef struct MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_send_fin {
    MPIDI_SHM_IPC_type_t ipc_type;
    uint64_t req_ptr;
} MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_send_fin_t;

typedef struct MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cnt_free {
    int coop_counter_direct_flag;
    uint64_t coop_counter_offset;
} MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cnt_free_t;

typedef MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_send_fin_t MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_recv_fin_t;
#endif

typedef union {
    MPIDI_SHM_ctrl_ipc_send_lmt_rts_t ipc_slmt_rts;
    MPIDI_SHM_ctrl_ipc_send_lmt_fin_t ipc_slmt_fin;
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cts_t ipc_xpmem_slmt_cts;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_send_fin_t ipc_xpmem_slmt_send_fin;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_recv_fin_t ipc_xpmem_slmt_recv_fin;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cnt_free_t ipc_xpmem_slmt_cnt_free;
#endif
    char dummy;                 /* some compilers (suncc) does not like empty struct */
} MPIDI_SHM_ctrl_hdr_t;

#endif /* SHM_TYPES_H_INCLUDED */
