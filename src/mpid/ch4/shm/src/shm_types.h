/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_TYPES_H_INCLUDED
#define SHM_TYPES_H_INCLUDED

#include "../ipc/src/ipc_pre.h"

typedef enum {
    MPIDI_SHM_IPC_SEND_CONTIG_LMT_RTS,  /* issued by sender to initialize IPC with contig sbuf */
    MPIDI_SHM_IPC_SEND_CONTIG_LMT_FIN,  /* issued by receiver to notify completion of sender-initialized contig IPC */
    MPIDI_SHM_IPC_SEND_FLATTEN_DTYPE_LMT_RTS,   /* issued by sender to initialize IPC with noncontig sbuf and flatten datatype */
    MPIDI_SHM_IPC_SEND_FLATTEN_DTYPE_LMT_FIN,   /* issued by receiver to notify completion of sender-initialized
                                                 * flatten dataype-based IPC */

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

typedef union {
    MPIDI_SHM_ctrl_ipc_send_contig_lmt_rts_t ipc_slmt_contig_rts;
    MPIDI_SHM_ctrl_ipc_send_contig_lmt_fin_t ipc_slmt_contig_fin;
    MPIDI_SHM_ctrl_ipc_send_flatten_dtype_lmt_rts_t ipc_slmt_flatten_rts;
    MPIDI_SHM_ctrl_ipc_send_flatten_dtype_lmt_fin_t ipc_slmt_flatten_fin;
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cts_t ipc_xpmem_slmt_cts;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_send_fin_t ipc_xpmem_slmt_send_fin;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_recv_fin_t ipc_xpmem_slmt_recv_fin;
    MPIDI_SHM_ctrl_ipc_xpmem_send_lmt_cnt_free_t ipc_xpmem_slmt_cnt_free;
#endif
    char dummy;                 /* some compilers (suncc) does not like empty struct */
} MPIDI_SHM_ctrl_hdr_t;

typedef int (*MPIDI_SHM_ctrl_cb) (MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* SHM_TYPES_H_INCLUDED */
