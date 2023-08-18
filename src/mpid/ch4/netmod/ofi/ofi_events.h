/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_EVENTS_H_INCLUDED
#define OFI_EVENTS_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "utlist.h"

int MPIDI_OFI_rma_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * in_req);
int MPIDI_OFI_dispatch_function(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_cqe_get_source(struct fi_cq_tagged_entry *wc, bool has_err)
{
    if (MPIDI_OFI_ENABLE_DATA) {
        if (unlikely(has_err)) {
            return wc->data & ((1 << MPIDI_OFI_IDATA_SRC_BITS) - 1);
        }
        return wc->data;
    } else {
        return MPIDI_OFI_init_get_source(wc->tag);
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_event(int vci,
                                                  struct fi_cq_tagged_entry *wc /* unused */ ,
                                                  MPIR_Request * sreq, int event_id)
{
    MPIR_FUNC_ENTER;

    /* free the packing buffers and datatype */
    if ((event_id == MPIDI_OFI_EVENT_SEND_PACK) &&
        (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer))) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
    } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_SEND_NOPACK)) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));
    }
    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));

    MPIDI_Request_complete_fast(sreq);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_event(int vci, struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * rreq, int event_id)
{
    int mpi_errno = MPI_SUCCESS;
    size_t count;
    MPIR_FUNC_ENTER;

    if (wc->tag & MPIDI_OFI_HUGE_SEND) {
        mpi_errno = MPIDI_OFI_recv_huge_event(vci, wc, rreq);
        goto fn_exit;
    }
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, true);
    if (!rreq->status.MPI_ERROR) {
        rreq->status.MPI_ERROR = MPIDI_OFI_idata_get_error_bits(wc->data);
    }
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    count = wc->len;
    MPIR_STATUS_SET_COUNT(rreq->status, count);

    /* If striping is enabled, this data will be counted elsewhere. */
    if (MPIDI_OFI_REQUEST(rreq, event_id) != MPIDI_OFI_EVENT_RECV_HUGE ||
        !MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[MPIDI_OFI_REQUEST(rreq, nic_num)],
                                wc->len);
    }
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_cancelled;
    MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
    /* Cancel SHM partner is always successful */
    MPIR_Assert(is_cancelled);
    MPIDI_anysrc_free_partner(rreq);
#endif
    if ((event_id == MPIDI_OFI_EVENT_RECV_PACK || event_id == MPIDI_OFI_EVENT_GET_HUGE) &&
        (MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer))) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer), count,
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.buf),
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.count),
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.datatype), 0,
                            &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer));
        if (actual_unpack_bytes != (MPI_Aint) count) {
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS,
                                     MPIR_ERR_RECOVERABLE,
                                     __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
    } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_RECV_NOPACK) &&
               (MPIDI_OFI_REQUEST(rreq, noncontig.nopack))) {
        MPI_Count elements;

        /* Check to see if there are any bytes that don't fit into the datatype basic elements */
        MPI_Count count_x = count;      /* need a MPI_Count variable (consider 32-bit OS) */
        MPIR_Get_elements_x_impl(&count_x, MPIDI_OFI_REQUEST(rreq, datatype), &elements);
        if (count_x)
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");

        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
    }

    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(rreq, datatype));

    /* If synchronous, ack and complete when the ack is done */
    if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
        /* Read ordering unnecessary for context_id stored in util_id here, so use relaxed load */
        MPIR_Comm *c = rreq->comm;
        uint64_t ss_bits =
            MPIDI_OFI_init_sendtag(MPL_atomic_relaxed_load_int(&MPIDI_OFI_REQUEST(rreq, util_id)),
                                   MPIR_Comm_rank(c), rreq->status.MPI_TAG,
                                   MPIDI_OFI_SYNC_SEND_ACK);
        int r = rreq->status.MPI_SOURCE;
        /* NOTE: use target rank, reply to src */
        int vci_src = MPIDI_get_vci(SRC_VCI_FROM_RECVER, c, r, c->rank, rreq->status.MPI_TAG);
        int vci_dst = MPIDI_get_vci(DST_VCI_FROM_RECVER, c, r, c->rank, rreq->status.MPI_TAG);
        int vci_local = vci_dst;
        int vci_remote = vci_src;
        MPIR_Assert(vci_local == vci);
        int nic = 0;
        int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic);
        fi_addr_t dest_addr = MPIDI_OFI_comm_to_phys(c, r, nic, vci_remote);
        if (MPIDI_OFI_ENABLE_DATA) {
            MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx, NULL, 0,
                                                MPIR_Comm_rank(c), dest_addr, ss_bits),
                                 vci_local, tinjectdata);
        } else {
            MPIDI_OFI_CALL_RETRY(fi_tinject(MPIDI_OFI_global.ctx[ctx_idx].tx, NULL, 0,
                                            dest_addr, ss_bits), vci_local, tinject);
        }
    }

    MPIDI_Request_complete_fast(rreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_dispatch_optimized(int vci, struct fi_cq_tagged_entry *wc,
                                                          MPIR_Request * req)
{
    /* fast path */
    if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_SEND) {
        return MPIDI_OFI_send_event(vci, wc, req, MPIDI_OFI_EVENT_SEND);
    } else if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RECV) {
        return MPIDI_OFI_recv_event(vci, wc, req, MPIDI_OFI_EVENT_RECV);
    }

    /* slow path */
    return MPIDI_OFI_dispatch_function(vci, wc, req);
}

#endif /* OFI_EVENTS_H_INCLUDED */
