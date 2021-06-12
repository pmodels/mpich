/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PROGRESS_H_INCLUDED
#define UCX_PROGRESS_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_handle_deferred_ops(void)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_deferred_am_isend_req_t *dreq = MPIDI_UCX_global.deferred_am_isend_q;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_HANDLE_DEFERRED_OPS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_HANDLE_DEFERRED_OPS);

    if (dreq) {
        switch (dreq->ucx_handler_id) {
            case MPIDI_UCX_AM_HANDLER_ID__SHORT:
                mpi_errno = MPIDI_UCX_do_am_isend_short(dreq->rank, dreq->comm, dreq->handler_id,
                                                        dreq->am_hdr, dreq->am_hdr_sz, dreq->buf,
                                                        dreq->count, dreq->datatype, dreq->sreq,
                                                        true);
                break;
            default:
                MPIR_Assert(0);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_HANDLE_DEFERRED_OPS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;

    int vni = MPIDI_UCX_vci_to_vni(vci);
    if (vni < 0) {
        /* skip if the vci is not for us */
        return MPI_SUCCESS;
    }

    ucp_worker_progress(MPIDI_UCX_global.ctx[vni].worker);

    if (unlikely(MPIDI_UCX_global.deferred_am_isend_q)) {
        mpi_errno = MPIDI_UCX_handle_deferred_ops();
    }

    return mpi_errno;
}

#endif /* UCX_PROGRESS_H_INCLUDED */
