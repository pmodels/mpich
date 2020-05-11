/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_CONTROL_H_INCLUDED
#define IPC_CONTROL_H_INCLUDED

#include "shm_types.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_control.h"
#endif

int MPIDI_IPC_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_IPC_send_lmt_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* IPC_CONTROL_H_INCLUDED */
