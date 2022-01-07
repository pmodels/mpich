/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_PROGRESS_H_INCLUDED
#define OFI_PROGRESS_H_INCLUDED

#include "ofi_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_deferred_ops(int vni)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_deferred_am_isend_req_t *dreq = MPIDI_OFI_global.per_vni[vni].deferred_am_isend_q;

    MPIR_FUNC_ENTER;

    if (dreq) {
        switch (dreq->op) {
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER:
                mpi_errno = MPIDI_OFI_do_am_isend_eager(dreq->rank, dreq->comm, dreq->handler_id,
                                                        NULL, 0, dreq->buf, dreq->count,
                                                        dreq->datatype, dreq->sreq, true, vni,
                                                        dreq->vni_dst);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE:
                mpi_errno = MPIDI_OFI_do_am_isend_pipeline(dreq->rank, dreq->comm, dreq->handler_id,
                                                           NULL, 0, dreq->buf, dreq->count,
                                                           dreq->datatype, dreq->sreq,
                                                           dreq->data_sz, true, vni, dreq->vni_dst);
                break;
            case MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ:
                mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(dreq->rank, dreq->comm,
                                                            dreq->handler_id, NULL, 0, dreq->buf,
                                                            dreq->count, dreq->datatype, dreq->sreq,
                                                            true, vni, dreq->vni_dst);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_cq_entries(int vni, struct fi_cq_tagged_entry *wc,
                                                         ssize_t num)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIR_FUNC_ENTER;

    for (i = 0; i < num; i++) {
        req = MPIDI_OFI_context_to_request(wc[i].op_context);
        mpi_errno = MPIDI_OFI_dispatch_optimized(vni, &wc[i], req);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    ssize_t ret;
    MPIR_FUNC_ENTER;

    int vni = MPIDI_OFI_vci_to_vni(vci);
    if (vni >= MPIDI_OFI_global.num_vnis) {
        /* The requests generated from ofi will have vni/vci within our range.
         * All requests that have vci beyond num_vnis are not from us, nothing
         * to do, so simply return.
         * NOTE: it is not an error since global progress will poll every vci.
         */
        return MPI_SUCCESS;
    }

    if (unlikely(MPIDI_OFI_has_cq_buffered(vni))) {
        int num = MPIDI_OFI_get_buffered(vni, wc);
        mpi_errno = MPIDI_OFI_handle_cq_entries(vni, wc, num);
    } else if (likely(1)) {
        for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
            int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);
            ret = fi_cq_read(MPIDI_OFI_global.ctx[ctx_idx].cq, (void *) wc,
                             MPIDI_OFI_NUM_CQ_ENTRIES);

            if (likely(ret > 0))
                mpi_errno = MPIDI_OFI_handle_cq_entries(vni, wc, ret);
            else if (ret == -FI_EAGAIN)
                mpi_errno = MPI_SUCCESS;
            else
                mpi_errno = MPIDI_OFI_handle_cq_error(vni, nic, ret);
        }

        if (unlikely(mpi_errno == MPI_SUCCESS && MPIDI_OFI_global.per_vni[vni].deferred_am_isend_q)) {
            mpi_errno = MPIDI_OFI_handle_deferred_ops(vni);
        }
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

#endif /* OFI_PROGRESS_H_INCLUDED */
