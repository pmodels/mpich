/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef POSIX_LMT_H_INCLUDED
#define POSIX_LMT_H_INCLUDED

#include "mpidimpl.h"
#include "posix_impl.h"
#include "shm_types.h"

int MPIDI_POSIX_lmt_init(int rank, int size);
int MPIDI_POSIX_lmt_finalize(void);

int MPIDI_POSIX_lmt_send_rts(void *buf, int grank, int tag, int error_bits, MPIR_Comm * comm,
                             size_t data_sz, int handler_id, MPIR_Request * sreq);
int MPIDI_POSIX_lmt_recv(MPIR_Request * req);

int MPIDI_POSIX_lmt_progress(void);

void MPIDI_POSIX_lmt_recv_posted_hook(int grank);
void MPIDI_POSIX_lmt_recv_completed_hook(int grank);

int MPIDI_POSIX_lmt_ctrl_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_POSIX_lmt_ctrl_send_lmt_cts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* POSIX_LMT_H_INCLUDED */
