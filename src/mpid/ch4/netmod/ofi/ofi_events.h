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
int MPIDI_OFI_recv_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_peek_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_rndv_cts_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
int MPIDI_OFI_pipeline_recv_datasize_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_pipeline_send_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_pipeline_recv_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvread_recv_mrs_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvread_read_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvread_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvwrite_recv_mrs_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvwrite_write_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
int MPIDI_OFI_rndvwrite_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);

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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* free the packing buffers and datatype */
    if (event_id == MPIDI_OFI_EVENT_SEND_PACK) {
        MPIDI_OFI_free_pack_buffer(sreq);
    } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_SEND_NOPACK)) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack.iovs));
    }

    if (MPIDI_OFI_REQUEST(sreq, am_req)) {
        MPIR_Request *am_sreq = MPIDI_OFI_REQUEST(sreq, am_req);
        int handler_id = MPIDI_OFI_REQUEST(sreq, am_handler_id);
        if (handler_id == -1) {
            /* native rndv direct */
            MPIDI_OFI_rndv_common_t *p = &MPIDI_OFI_AMREQ_COMMON(am_sreq);
            MPIR_Datatype_release_if_not_builtin(p->datatype);
            MPIDI_Request_complete_fast(am_sreq);
        } else {
            mpi_errno = MPIDIG_global.origin_cbs[handler_id] (am_sreq);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPIDI_Request_complete_fast(sreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_complete(MPIR_Request * rreq, int event_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_anysrc_free_partner(rreq);
#endif
    if ((event_id == MPIDI_OFI_EVENT_RECV_PACK) &&
        (MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer))) {
        MPI_Aint count = MPIR_STATUS_GET_COUNT(rreq->status);
        mpi_errno = MPIR_Localcopy_gpu(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer), count,
                                       MPIR_BYTE_INTERNAL, 0, NULL,
                                       MPIDI_OFI_REQUEST(rreq, buf),
                                       MPIDI_OFI_REQUEST(rreq, count),
                                       MPIDI_OFI_REQUEST(rreq, datatype), 0, NULL,
                                       MPL_GPU_COPY_H2D,
                                       MPIDI_OFI_gpu_get_recv_engine_type(), true);
        if (mpi_errno) {
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
        }
        MPIDI_OFI_free_pack_buffer(rreq);
    } else if (event_id == MPIDI_OFI_EVENT_RECV_NOPACK) {
#ifdef HAVE_ERROR_CHECKING
        MPI_Count elements;
        MPI_Count count_x = MPIR_STATUS_GET_COUNT(rreq->status);
        MPI_Datatype datatype = MPIDI_OFI_REQUEST(rreq, datatype);
        MPIR_Get_elements_x_impl(&count_x, datatype, &elements);
        if (count_x) {
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
        }
#endif
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack.iovs));
    }

    if (MPIDI_OFI_REQUEST(rreq, am_req) != NULL) {
        MPI_Status *status = &rreq->status;
        MPIR_Request *am_req = MPIDI_OFI_REQUEST(rreq, am_req);
        int am_recv_id = MPIDI_OFI_REQUEST(rreq, am_handler_id);
        if (am_recv_id == -1) {
            /* native rndv direct */

            /* copy COUNT and MPI_ERROR, but skip MPI_SOURCE and MPI_RANK */
            am_req->status.count_lo = rreq->status.count_lo;
            am_req->status.count_hi_and_cancelled = rreq->status.count_hi_and_cancelled;
            am_req->status.MPI_ERROR = rreq->status.MPI_ERROR;

#ifndef MPIDI_CH4_DIRECT_NETMOD
            MPIDI_anysrc_free_partner(am_req);
#endif

            MPIDI_OFI_rndv_common_t *p = &MPIDI_OFI_AMREQ_COMMON(am_req);
            MPIR_Datatype_release_if_not_builtin(p->datatype);
            MPIDI_Request_complete_fast(am_req);
        } else {
            mpi_errno = MPIDIG_global.tag_recv_cbs[am_recv_id] (am_req, status);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(rreq, datatype));
    MPIDI_Request_complete_fast(rreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_event(int vci, struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * rreq, int event_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* update status from matched information */
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    if (!rreq->status.MPI_ERROR) {
        rreq->status.MPI_ERROR = MPIDI_OFI_idata_get_error_bits(wc->data);
    }
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_cancelled;
    MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
    /* Cancel SHM partner is always successful */
    MPIR_Assert(is_cancelled);
#endif

    if (MPIDI_OFI_is_tag_rndv(wc->tag)) {
        mpi_errno = MPIDI_OFI_recv_rndv_event(vci, wc, rreq);
        goto fn_exit;
    }

    /* If synchronous, send ack */
    if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
        int context_id = MPIDI_OFI_REQUEST(rreq, context_id);
        mpi_errno = MPIDI_OFI_send_ack(rreq, context_id, NULL, 0);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_OFI_recv_complete(rreq, event_id);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
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
