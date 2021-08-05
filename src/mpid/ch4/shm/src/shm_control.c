/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_control.h"

static MPIDI_SHMI_ctrl_cb ctrl_cbs[MPIDI_SHMI_CTRL_IDS_MAX];

void MPIDI_SHMI_ctrl_reg_cb(int ctrl_id, MPIDI_SHMI_ctrl_cb cb)
{
    MPIR_FUNC_ENTER;

    ctrl_cbs[ctrl_id] = cb;

    MPIR_FUNC_EXIT;
}

int MPIDI_SHMI_ctrl_dispatch(int ctrl_id, void *ctrl_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(ctrl_id >= 0 && ctrl_id < MPIDI_SHMI_CTRL_IDS_MAX);

    mpi_errno = ctrl_cbs[ctrl_id] ((MPIDI_SHMI_ctrl_hdr_t *) ctrl_hdr);

    MPIR_FUNC_EXIT;

    return mpi_errno;
}
