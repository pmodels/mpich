/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_control.h"

void MPIDI_SHMI_ctrl_reg_cb(int ctrl_id, MPIDI_SHMI_ctrl_cb cb)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_CTRL_REG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_CTRL_REG_CB);

    MPIDI_global.shm.ctrl_cbs[ctrl_id] = cb;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_CTRL_REG_CB);
}

int MPIDI_SHMI_ctrl_dispatch(int ctrl_id, void *ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_CTRL_DISPATCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_CTRL_DISPATCH);

    MPIR_Assert(ctrl_id >= 0 && ctrl_id < MPIDI_SHMI_CTRL_IDS_MAX);

    mpi_errno = MPIDI_global.shm.ctrl_cbs[ctrl_id] ((MPIDI_SHMI_ctrl_hdr_t *) ctrl_hdr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_CTRL_DISPATCH);

    return mpi_errno;
}
