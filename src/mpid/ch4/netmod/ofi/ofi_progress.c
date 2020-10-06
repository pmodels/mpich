/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_events.h"

int MPIDI_OFI_handle_deferred_ops(void)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_deferred_am_isend_req_t *dreq = MPIDI_OFI_global.deferred_am_isend_q;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_DEFERRED_OPS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_DEFERRED_OPS);

    if (dreq) {
        switch (dreq->op) {
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER:
                mpi_errno = MPIDI_OFI_do_am_isend_eager(dreq->rank, dreq->comm, dreq->handler_id,
                                                        NULL, 0, dreq->buf, dreq->count,
                                                        dreq->datatype, dreq->sreq, true);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE:
                mpi_errno = MPIDI_OFI_do_am_isend_pipeline(dreq->rank, dreq->comm, dreq->handler_id,
                                                           NULL, 0, dreq->buf, dreq->count,
                                                           dreq->datatype, dreq->sreq,
                                                           dreq->data_sz, true);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ:
                mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(dreq->rank, dreq->comm,
                                                            dreq->handler_id, NULL, 0, dreq->buf,
                                                            dreq->count, dreq->datatype, dreq->sreq,
                                                            true);
                break;
            default:
                MPIR_Assert(0);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_DEFERRED_OPS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_progress_uninlined(int vni)
{
    return MPIDI_NM_progress(vni, 0);
}
