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
    MPIDI_SHM_CTRL_IDS_MAX
} MPIDI_SHM_ctrl_id_t;

typedef union {
    MPIDI_SHM_ctrl_ipc_send_contig_lmt_rts_t ipc_contig_slmt_rts;
    MPIDI_SHM_ctrl_ipc_send_contig_lmt_fin_t ipc_contig_slmt_fin;
} MPIDI_SHM_ctrl_hdr_t;

typedef int (*MPIDI_SHM_ctrl_cb) (MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* SHM_TYPES_H_INCLUDED */
