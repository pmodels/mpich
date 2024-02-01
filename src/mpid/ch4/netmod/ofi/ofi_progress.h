/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_PROGRESS_H_INCLUDED
#define OFI_PROGRESS_H_INCLUDED

#include "ofi_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_deferred_ops(int vci)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_deferred_am_isend_req_t *dreq = MPIDI_OFI_global.per_vci[vci].deferred_am_isend_q;

    MPIR_FUNC_ENTER;

    if (dreq) {
        switch (dreq->op) {
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER:
                mpi_errno = MPIDI_OFI_do_am_isend_eager(dreq->rank, dreq->comm, dreq->handler_id,
                                                        NULL, 0, dreq->buf, dreq->count,
                                                        dreq->datatype, dreq->sreq, true, vci,
                                                        dreq->vci_dst);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE:
                mpi_errno = MPIDI_OFI_do_am_isend_pipeline(dreq->rank, dreq->comm, dreq->handler_id,
                                                           NULL, 0, dreq->buf, dreq->count,
                                                           dreq->datatype, dreq->sreq,
                                                           dreq->data_sz, true, vci, dreq->vci_dst);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ:
                mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(dreq->rank, dreq->comm,
                                                            dreq->handler_id, NULL, 0, dreq->buf,
                                                            dreq->count, dreq->datatype, dreq->sreq,
                                                            true, vci, dreq->vci_dst);
                break;
            default:
                MPIR_Assert(0);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_cq_entries(int vci, struct fi_cq_tagged_entry *wc,
                                                         ssize_t num)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIR_FUNC_ENTER;

    for (i = 0; i < num; i++) {
        req = MPIDI_OFI_context_to_request(wc[i].op_context);
        mpi_errno = MPIDI_OFI_dispatch_optimized(vci, &wc[i], req);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    ssize_t ret;
    MPIR_FUNC_ENTER;

    if (vci >= MPIDI_OFI_global.num_vcis) {
        /* The requests generated from ofi will have vci within our range.
         * All requests that have vci beyond num_vcis are not from us, nothing
         * to do, so simply return.
         * NOTE: it is not an error since global progress will poll every vci.
         */
        goto fn_exit;
    }

    if (unlikely(MPIDI_OFI_has_cq_buffered(vci))) {
        int num = MPIDI_OFI_get_buffered(vci, wc);
        mpi_errno = MPIDI_OFI_handle_cq_entries(vci, wc, num);
    } else if (likely(1)) {
        for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
            int ctx_idx = MPIDI_OFI_get_ctx_index(vci, nic);
            ret = fi_cq_read(MPIDI_OFI_global.ctx[ctx_idx].cq, (void *) wc,
                             MPIDI_OFI_NUM_CQ_ENTRIES);

            if (likely(ret > 0))
                mpi_errno = MPIDI_OFI_handle_cq_entries(vci, wc, ret);
            else if (ret == -FI_EAGAIN)
                mpi_errno = MPI_SUCCESS;
            else
                mpi_errno = MPIDI_OFI_handle_cq_error(vci, nic, ret);
        }

        if (unlikely(mpi_errno == MPI_SUCCESS && MPIDI_OFI_global.per_vci[vci].deferred_am_isend_q)) {
            mpi_errno = MPIDI_OFI_handle_deferred_ops(vci);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* OFI_PROGRESS_H_INCLUDED */
