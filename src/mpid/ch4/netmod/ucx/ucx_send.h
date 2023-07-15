/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_SEND_H_INCLUDED
#define UCX_SEND_H_INCLUDED
#include <ucp/api/ucp.h>
#include "mpir_func.h"
#include "ucx_impl.h"
#include "ucx_types.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_send_cmpl_cb(void *request, ucs_status_t status,
                                                     void *user_data)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;

    MPIR_FUNC_ENTER;

    if (unlikely(status == UCS_ERR_CANCELED))
        MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
    MPIDI_Request_complete_fast(req);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_send(const void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm,
                                            int context_offset,
                                            MPIDI_av_entry_t * addr,
                                            MPIR_Request ** request,
                                            int vci_src, int vci_dst, int have_request, int is_sync)
{
    int dt_contig;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = *request;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_ENTER;

    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK,
        .cb.send = MPIDI_UCX_send_cmpl_cb,
    };

    ep = MPIDI_UCX_AV_TO_EP(addr, vci_src, vci_dst);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    const void *send_buf;
    size_t send_count;
    if (dt_contig) {
        send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
        send_count = data_sz;
    } else {
        send_buf = buf;
        send_count = count;
        param.op_attr_mask |= UCP_OP_ATTR_FIELD_DATATYPE;
        param.datatype = dt_ptr->dev.netmod.ucx.ucp_datatype;
        MPIR_Datatype_ptr_add_ref(dt_ptr);
    }

    if (is_sync) {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nbx(ep, send_buf, send_count,
                                                              ucx_tag, &param);
    } else {
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nbx(ep, send_buf, send_count, ucx_tag, &param);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request) {
        if (req == NULL) {
            req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, vci_src, 2);
            MPIR_ERR_CHKANDSTMT(!req, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
            req->comm = comm;
            MPIR_Comm_add_ref(comm);
        } else {
            MPIR_Request_add_ref(req);
        }
        ucp_request->req = req;
        MPIDI_UCX_REQ(req).ucp_request = ucp_request;
    } else if (req != NULL) {
        MPIR_cc_set(&req->cc, 0);
    } else if (have_request) {
        req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
    }
    *request = req;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define MPIDI_UCX_SEND_VNIS(vci_src_, vci_dst_) \
    do { \
        MPIDI_EXPLICIT_VCIS(comm, attr, comm->rank, rank, vci_src_, vci_dst_); \
        if (vci_src_ == 0 && vci_dst_ == 0) { \
            vci_src = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
            vci_dst = MPIDI_get_vci(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int is_sync = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr) ? 1 : 0;

    int vci_src, vci_dst;
    MPIDI_UCX_SEND_VNIS(vci_src, vci_dst);

    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    switch (errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    MPIDI_UCX_THREAD_CS_ENTER_VCI(vci_src);
    mpi_errno = MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                               addr, request, vci_src, vci_dst, 1, is_sync);
    MPIDI_UCX_THREAD_CS_EXIT_VCI(vci_src);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    MPIR_FUNC_ENTER;

    if (!MPIR_Request_is_complete(sreq)) {
        int vci = MPIDI_Request_get_vci(sreq);
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vci);
        ucp_request_cancel(MPIDI_UCX_global.ctx[vci].worker, MPIDI_UCX_REQ(sreq).ucp_request);
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vci);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#endif /* UCX_SEND_H_INCLUDED */
