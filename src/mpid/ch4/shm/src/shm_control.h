/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SHM_CONTROL_H_INCLUDED
#define SHM_CONTROL_H_INCLUDED

#include "shm_types.h"
#include "../posix/posix_am.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_do_ctrl_send(int rank, MPIR_Comm * comm,
                                                    int ctrl_id, void *ctrl_hdr)
{
    int ret;

    ret = MPIDI_POSIX_am_send_hdr(rank, comm, MPIDI_POSIX_AM_HDR_SHM,
                                  ctrl_id, ctrl_hdr, sizeof(MPIDI_SHM_ctrl_hdr_t));

    return ret;
}

int MPIDI_SHM_ctrl_dispatch(int ctrl_id, void *ctrl_hdr);

#endif /* SHM_CONTROL_H_INCLUDED */
