/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_CONTROL_H_INCLUDED
#define XPMEM_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_IPC_xpmem_send_lmt_coop_cts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_IPC_xpmem_send_lmt_coop_send_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_IPC_xpmem_send_lmt_coop_recv_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_IPC_xpmem_send_lmt_coop_cnt_free_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* XPMEM_CONTROL_H_INCLUDED */
