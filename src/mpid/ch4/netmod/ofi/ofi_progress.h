/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_PROGRESS_H_INCLUDED
#define OFI_PROGRESS_H_INCLUDED

#include "ofi_impl.h"

#define COND_HAS_CQ_BUFFERED ((MPIDI_OFI_global.cq_buffered_static_head != MPIDI_OFI_global.cq_buffered_static_tail) || (NULL != MPIDI_OFI_global.cq_buffered_dynamic_head))

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_buffered(struct fi_cq_tagged_entry *wc, ssize_t num)
{
    int rc = 0;

    if (1) {
        /* If the static list isn't empty, do so first */
        if (MPIDI_OFI_global.cq_buffered_static_head != MPIDI_OFI_global.cq_buffered_static_tail) {
            wc[0] =
                MPIDI_OFI_global.cq_buffered_static_list[MPIDI_OFI_global.
                                                         cq_buffered_static_tail].cq_entry;
            MPIDI_OFI_global.cq_buffered_static_tail =
                (MPIDI_OFI_global.cq_buffered_static_tail + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
        /* If there's anything in the dynamic list, it goes second. */
        else if (NULL != MPIDI_OFI_global.cq_buffered_dynamic_head) {
            MPIDI_OFI_cq_list_t *cq_list_entry = MPIDI_OFI_global.cq_buffered_dynamic_head;
            LL_DELETE(MPIDI_OFI_global.cq_buffered_dynamic_head,
                      MPIDI_OFI_global.cq_buffered_dynamic_tail, cq_list_entry);
            wc[0] = cq_list_entry->cq_entry;
            MPL_free(cq_list_entry);
        }

        rc = 1;
    }

    return rc;
}

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

    if (unlikely(COND_HAS_CQ_BUFFERED)) {
        ret = MPIDI_OFI_get_buffered(wc, 1);
        mpi_errno = MPIDI_OFI_handle_cq_entries(wc, 1);
    } else if (likely(1)) {
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
