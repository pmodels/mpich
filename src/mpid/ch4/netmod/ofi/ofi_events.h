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

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE
      category    : CH4_OFI
      type        : enum
      default     : copy_low_latency
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : |-
        Specifies GPU engine type for GPU pt2pt on the receiver side.
        compute - use a compute engine
        copy_high_bandwidth - use a high-bandwidth copy engine
        copy_low_latency - use a low-latency copy engine
        yaksa - use Yaksa

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIDI_OFI_rma_done_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * in_req);
int MPIDI_OFI_dispatch_function(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
int MPIDI_OFI_recv_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_peek_rndv_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
int MPIDI_OFI_rndv_cts_event(int vci, struct fi_cq_tagged_entry *wc, MPIR_Request * req);

MPL_STATIC_INLINE_PREFIX MPL_gpu_engine_type_t MPIDI_OFI_gpu_get_recv_engine_type(void)
{
    if (MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE ==
        MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE_compute) {
        return MPL_GPU_ENGINE_TYPE_COMPUTE;
    } else if (MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE ==
               MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE_copy_high_bandwidth) {
        return MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;
    } else if (MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE ==
               MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE_copy_low_latency) {
        return MPL_GPU_ENGINE_TYPE_COPY_LOW_LATENCY;
    } else {
        return MPL_GPU_ENGINE_TYPE_LAST;
    }
}

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
    if ((event_id == MPIDI_OFI_EVENT_SEND_PACK) &&
        (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer))) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
    } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_SEND_NOPACK)) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack.iovs));
    }

    if (MPIDI_OFI_REQUEST(sreq, am_req)) {
        MPIR_Request *am_sreq = MPIDI_OFI_REQUEST(sreq, am_req);
        int handler_id = MPIDI_OFI_REQUEST(sreq, am_handler_id);
        mpi_errno = MPIDIG_global.origin_cbs[handler_id] (am_sreq);
    }

    MPIDI_Request_complete_fast(sreq);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_event(int vci, struct fi_cq_tagged_entry *wc,
                                                  MPIR_Request * rreq, int event_id)
{
    int mpi_errno = MPI_SUCCESS;
    size_t count;
    MPIR_FUNC_ENTER;

    /* update status from matched information */
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);

    if (MPIDI_OFI_is_tag_rndv(wc->tag)) {
        mpi_errno = MPIDI_OFI_recv_rndv_event(vci, wc, rreq);
        goto fn_exit;
    } else if (MPIDI_OFI_is_tag_huge(wc->tag)) {
        mpi_errno = MPIDI_OFI_recv_huge_event(vci, wc, rreq);
        goto fn_exit;
    }
    if (!rreq->status.MPI_ERROR) {
        rreq->status.MPI_ERROR = MPIDI_OFI_idata_get_error_bits(wc->data);
    }
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
        mpi_errno = MPIR_Localcopy_gpu(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer), count,
                                       MPI_BYTE, 0, NULL,
                                       MPIDI_OFI_REQUEST(rreq, buf),
                                       MPIDI_OFI_REQUEST(rreq, count),
                                       MPIDI_OFI_REQUEST(rreq, datatype), 0, NULL,
                                       MPL_GPU_COPY_H2D,
                                       MPIDI_OFI_gpu_get_recv_engine_type(), true);
        if (mpi_errno) {
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
        }
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer));
    } else if (event_id == MPIDI_OFI_EVENT_RECV_NOPACK) {
        MPI_Count elements;
        MPI_Count count_x = count;
        MPI_Datatype datatype = MPIDI_OFI_REQUEST(rreq, datatype);
        MPIR_Get_elements_x_impl(&count_x, datatype, &elements);
        if (count_x) {
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
        }

        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack.iovs));
    }

    /* If synchronous, send ack */
    if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
        int context_id = MPIDI_OFI_REQUEST(rreq, context_id);
        mpi_errno = MPIDI_OFI_send_ack(rreq, context_id, NULL, 0);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIDI_OFI_REQUEST(rreq, am_req) != NULL) {
        MPI_Status *status = &rreq->status;
        MPIR_Request *am_req = MPIDI_OFI_REQUEST(rreq, am_req);
        int am_recv_id = MPIDI_OFI_REQUEST(rreq, am_handler_id);
        mpi_errno = MPIDIG_global.tag_recv_cbs[am_recv_id] (am_req, status);
        MPIR_ERR_CHECK(mpi_errno);
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
