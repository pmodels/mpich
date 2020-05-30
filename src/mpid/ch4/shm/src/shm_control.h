/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_CONTROL_H_INCLUDED
#define SHM_CONTROL_H_INCLUDED

#include "shm_types.h"
#include "../posix/posix_am.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_do_ctrl_send(int rank, MPIR_Comm * comm,
                                                    int ctrl_id, void *ctrl_hdr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);

    ret = MPIDI_POSIX_am_send_hdr(rank, comm, MPIDI_POSIX_AM_HDR_SHM,
                                  ctrl_id, ctrl_hdr, sizeof(MPIDI_SHMI_ctrl_hdr_t));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);
    return ret;
}

void MPIDI_SHMI_ctrl_reg_cb(int ctrl_id, MPIDI_SHMI_ctrl_cb cb);
int MPIDI_SHMI_ctrl_dispatch(int ctrl_id, void *ctrl_hdr);

#endif /* SHM_CONTROL_H_INCLUDED */
