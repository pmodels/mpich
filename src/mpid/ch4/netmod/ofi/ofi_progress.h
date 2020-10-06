/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_PROGRESS_H_INCLUDED
#define OFI_PROGRESS_H_INCLUDED

#include "ofi_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_cq_entries(struct fi_cq_tagged_entry *wc, ssize_t num)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);

    for (i = 0; i < num; i++) {
        req = MPIDI_OFI_context_to_request(wc[i].op_context);
        mpi_errno = MPIDI_OFI_dispatch_optimized(&wc[i], req);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vci, int blocking)
{
    int mpi_errno;
    struct fi_cq_tagged_entry wc[MPIDI_OFI_NUM_CQ_ENTRIES];
    ssize_t ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS);

    int vni = MPIDI_OFI_vci_to_vni(vci);
    if (vni >= MPIDI_OFI_global.num_vnis) {
        /* The requests generated from ofi will have vni/vci within our range.
         * All requests that have vci beyond num_vnis are not from us, nothing
         * to do, so simply return.
         * NOTE: it is not an error since global progress will poll every vci.
         */
        return MPI_SUCCESS;
    }

    if (unlikely(MPIDI_OFI_get_buffered(wc, 1)))
        mpi_errno = MPIDI_OFI_handle_cq_entries(wc, 1);
    else if (likely(1)) {
        ret = fi_cq_read(MPIDI_OFI_global.ctx[vni].cq, (void *) wc, MPIDI_OFI_NUM_CQ_ENTRIES);

        if (likely(ret > 0))
            mpi_errno = MPIDI_OFI_handle_cq_entries(wc, ret);
        else if (ret == -FI_EAGAIN)
            mpi_errno = MPI_SUCCESS;
        else
            mpi_errno = MPIDI_OFI_handle_cq_error(vni, ret);
    }

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = MPIDI_OFI_handle_deferred_ops();
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS);

    return mpi_errno;
}

#endif /* OFI_PROGRESS_H_INCLUDED */
