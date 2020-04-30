/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef GPU_CONTROL_H_INCLUDED
#define GPU_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_GPU_ctrl_send_ipc_recv_req_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_GPU_ctrl_send_ipc_recv_ack_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* GPU_CONTROL_H_INCLUDED */
