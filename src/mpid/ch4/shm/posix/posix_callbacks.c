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


#include "mpidimpl.h"
#include "posix_types.h"
#include "ch4_types.h"
#include "posix_eager.h"
#include "posix_noinline.h"

int MPIDI_POSIX_ctrl_send_lmt_rts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    return MPIDI_POSIX_lmt_ctrl_send_lmt_rts_cb(ctrl_hdr);
}

int MPIDI_POSIX_ctrl_send_lmt_cts_cb(MPIDI_SHM_ctrl_hdr_t * ctrl_hdr)
{
    return MPIDI_POSIX_lmt_ctrl_send_lmt_cts_cb(ctrl_hdr);
}
