/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PIP_CONTROL_H_INCLUDED
#define PIP_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_PIP_ctrl_send_lmt_send_fin_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);
int MPIDI_PIP_ctrl_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr);

#endif /* PIP_CONTROL_H_INCLUDED */
