/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_CONTROL_H_INCLUDED
#define XPMEM_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_XPMEM_ctrl_send_lmt_ack_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_XPMEM_ctrl_send_lmt_req_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* XPMEM_CONTROL_H_INCLUDED */
