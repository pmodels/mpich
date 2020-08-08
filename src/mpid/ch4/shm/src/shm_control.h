/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_CONTROL_H_INCLUDED
#define SHM_CONTROL_H_INCLUDED

#include "shm_types.h"
#include "../posix/posix_am.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_do_ctrl_send(int rank, MPIR_Comm * comm,
                                                    int ctrl_id, MPI_Aint ctrl_hdr_sz,
                                                    void *ctrl_hdr, MPIR_Request * request)
{
    int ret;
    int protocol;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIDIG_IPC_hdr_t ipc_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);

    ipc_hdr.req_ptr = request;
    mpi_errno =
        MPIDIG_isend_impl_new(&ctrl_hdr, ctrl_hdr_sz, MPI_BYTE, rank, tag, comm,
                              context_offset, addr, &request, errflag, ctrl_id, &ipc_hdr,
                              sizeof(MPIDIG_IPC_hdr_t), protocol);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_DO_CTRL_SEND);
    return ret;
}

/* This function return bool-type fallback flag to tell caller whether it needs
 * to fallback to ch4 am based on whether the size can fit in with one cell of posix */
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_query_size_fallback(MPI_Aint size, int handle_id,
                                                           bool * fallback_flag)
{

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_QUERY_FALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_QUERY_FALLBACK);

    /* FIXME: MPIDIG_IPC_DATATYPE_REQ should be added in MPIDIG_am_check_size_le_eager_limit switch list */
    if (!MPIDIG_am_check_size_le_eager_limit(size, handle_id, MPIDI_POSIX_am_eager_limit())) {
        *fallback_flag = false;
    } else {
        *fallback_flag = true;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_QUERY_FALLBACK);
    return MPI_SUCCESS;
}

void MPIDI_SHMI_ctrl_reg_cb(int ctrl_id, MPIDI_SHMI_ctrl_cb cb);
int MPIDI_SHMI_ctrl_dispatch(int ctrl_id, void *ctrl_hdr);

#endif /* SHM_CONTROL_H_INCLUDED */
