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
    MPIDIU_request_complete(req);
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
                                            int vni_src, int vni_dst, int have_request, int is_sync)
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

    ep = MPIDI_UCX_AV_TO_EP(addr, vni_src, vni_dst);
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
            req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, vni_src, 2);
            req->u.send.dest_world_rank = MPIDIU_rank_to_lpid(rank, comm);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    int vni_src = MPIDI_UCX_get_vni(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag);
    int vni_dst = MPIDI_UCX_get_vni(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);

    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    mpi_errno = MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                               addr, request, vni_src, vni_dst, 1, 0);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);
    MPIR_FUNC_EXIT;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    int vni_src = MPIDI_UCX_get_vni(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag);
    int vni_dst = MPIDI_UCX_get_vni(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag);

    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
    mpi_errno = MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                               addr, request, vni_src, vni_dst, 1, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    int vni_src = MPIDI_UCX_get_vni(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag);
    int vni_dst = MPIDI_UCX_get_vni(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag);

    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
    mpi_errno = MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                               addr, request, vni_src, vni_dst, 1, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    MPIR_FUNC_ENTER;

    if (!MPIR_Request_is_complete(sreq)) {
        int vci = MPIDI_Request_get_vci(sreq);
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
        ucp_request_cancel(MPIDI_UCX_global.ctx[vci].worker, MPIDI_UCX_REQ(sreq).ucp_request);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#endif /* UCX_SEND_H_INCLUDED */
