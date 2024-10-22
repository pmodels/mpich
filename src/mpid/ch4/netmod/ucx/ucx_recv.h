/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_RECV_H_INCLUDED
#define UCX_RECV_H_INCLUDED

#include "ucx_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_UCX_DT_RECV
      category    : CH4_UCX
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select method for receiving noncontiguous data
        true                - Use UCX datatype with pack/unpack callbacks
        false               - MPICH will decide to pack/unpack at completion or use IOVs
                              based on the datatype
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define MPIDI_UCX_ENABLE_IOV (UCP_VERSION(UCP_API_MAJOR, UCP_API_MINOR) >= UCP_VERSION(1, 16))

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_recv_cmpl_cb(void *request, ucs_status_t status,
                                                     const ucp_tag_recv_info_t * info,
                                                     void *user_data)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = user_data;

    MPIR_FUNC_ENTER;

    if (unlikely(status == UCS_ERR_CANCELED)) {
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
    } else {
        if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
            rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
        } else {
            MPI_Aint count = info->length;
            rreq->status.MPI_ERROR = MPI_SUCCESS;
            rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
            rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
            MPIR_STATUS_SET_COUNT(rreq->status, count);
        }

        if (MPIDI_UCX_REQ(rreq).s.type == MPIDI_UCX_RECV_UNPACK) {
            void *pack_buf = MPIDI_UCX_REQ(rreq).s.u.pack.pack_buf;
            void *user_buf = MPIDI_UCX_REQ(rreq).s.u.pack.user_buf;
            MPI_Aint user_count = MPIDI_UCX_REQ(rreq).s.u.pack.count;
            MPI_Datatype datatype = MPIDI_UCX_REQ(rreq).s.u.pack.datatype;
            MPI_Aint actual_unpack_bytes;

            MPIR_Typerep_unpack(pack_buf, info->length, user_buf, user_count, datatype,
                                0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
            if (actual_unpack_bytes < info->length) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            }
            MPIR_Datatype_release_if_not_builtin(datatype);
            MPL_free(MPIDI_UCX_REQ(rreq).s.u.pack.pack_buf);
        } else if (MPIDI_UCX_REQ(rreq).s.type == MPIDI_UCX_RECV_IOV) {
            MPL_free(MPIDI_UCX_REQ(rreq).s.u.iov.iov);
        }
#ifndef MPIDI_CH4_DIRECT_NETMOD
        int is_cancelled;
        MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
        /* Cancel SHM partner is always successful */
        MPIR_Assert(is_cancelled);
        MPIDI_anysrc_free_partner(rreq);
#endif
    }

    MPIDI_Request_complete_fast(rreq);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_mrecv_cmpl_cb(void *request, ucs_status_t status,
                                                      const ucp_tag_recv_info_t * info,
                                                      void *user_data)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = user_data;

    MPIR_FUNC_ENTER;

    /* populate status fields */
    if (unlikely(status == UCS_ERR_MESSAGE_TRUNCATED)) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
    } else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(rreq->status, info->length);
    }
    rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
    rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);

    /* complete the request */
    MPIDI_Request_complete_fast(rreq);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_recv(void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag, MPIR_Comm * comm,
                                            int context_offset,
                                            MPIDI_av_entry_t * addr,
                                            int vci_dst, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    uint64_t ucp_tag, tag_mask;
    MPIR_Request *req = *request;
    MPIDI_UCX_ucp_request_t *ucp_request;

    MPIR_FUNC_ENTER;

    if (req == NULL) {
        req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RECV, vci_dst, 2);
        MPIR_ERR_CHKANDSTMT(req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        req->comm = comm;
        MPIR_Comm_add_ref(comm);
    } else {
        MPIR_Request_add_ref(req);
    }

    ucp_request_param_t param = {
        .op_attr_mask =
            UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA | UCP_OP_ATTR_FLAG_NO_IMM_CMPL,
        .cb.recv = MPIDI_UCX_recv_cmpl_cb,
        .user_data = req,
    };

    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    void *recv_buf;
    size_t recv_count;
    if (dt_contig) {
        recv_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
        recv_count = data_sz;
        MPIDI_UCX_REQ(req).s.type = MPIDI_UCX_RECV_CONTIG;
    } else {
        if (MPIR_CVAR_UCX_DT_RECV) {
            recv_buf = buf;
            recv_count = count;
            param.op_attr_mask |= UCP_OP_ATTR_FIELD_DATATYPE;
            param.datatype = dt_ptr->dev.netmod.ucx.ucp_datatype;
            MPIR_Datatype_ptr_add_ref(dt_ptr);
            MPIDI_UCX_REQ(req).s.type = MPIDI_UCX_RECV_UCX_DT;
        } else {
            MPI_Aint density;
            MPIR_Datatype_get_density(datatype, density);

            if (MPIDI_UCX_ENABLE_IOV && density >= MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
                MPI_Aint iov_len, actual_iov_len;
                MPIR_Typerep_get_iov_len(count, datatype, &iov_len);
                ucp_dt_iov_t *iov = MPL_malloc(sizeof(ucp_dt_iov_t) * iov_len, MPL_MEM_BUFFER);
                MPIR_Typerep_to_iov_offset(buf, count, datatype, 0, (struct iovec *) iov,
                                           iov_len, &actual_iov_len);
                MPIR_Assert(iov_len == actual_iov_len);

                recv_buf = iov;
                recv_count = iov_len;
                param.op_attr_mask |= UCP_OP_ATTR_FIELD_DATATYPE;
                param.datatype = ucp_dt_make_iov();
                MPIDI_UCX_REQ(req).s.u.iov.iov = iov;
                MPIDI_UCX_REQ(req).s.type = MPIDI_UCX_RECV_IOV;
            } else {
                MPIDI_UCX_REQ(req).s.u.pack.datatype = datatype;
                MPIR_Datatype_add_ref_if_not_builtin(datatype);
                MPIDI_UCX_REQ(req).s.u.pack.user_buf = buf;
                MPIDI_UCX_REQ(req).s.u.pack.count = count;
                MPIDI_UCX_REQ(req).s.u.pack.pack_buf = MPL_malloc(data_sz, MPL_MEM_BUFFER);
                recv_buf = MPIDI_UCX_REQ(req).s.u.pack.pack_buf;
                recv_count = data_sz;
                MPIDI_UCX_REQ(req).s.type = MPIDI_UCX_RECV_UNPACK;
            }
        }
    }

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_recv_nbx(MPIDI_UCX_global.ctx[vci_dst].worker,
                                                     recv_buf, recv_count,
                                                     ucp_tag, tag_mask, &param);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    MPIDI_UCX_REQ(req).s.ucp_request = ucp_request;
    *request = req;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define MPIDI_UCX_RECV_VNIS(vci_dst_) \
    do { \
        int vci_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, rank, comm->rank, vci_src_tmp, vci_dst_); \
        if (vci_src_tmp == 0 && vci_dst_ == 0) { \
            vci_dst_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, rank, comm->rank, tag); \
        } \
    } while (0)

#ifndef MPIDI_ENABLE_AM_ONLY
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_UCX_ucp_request_t *ucp_request;
    MPIR_Datatype *dt_ptr;

    int vci = MPIDI_Request_get_vci(message);

    MPIR_FUNC_ENTER;

    MPIDI_UCX_THREAD_CS_ENTER_VCI(vci);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    ucp_request_param_t param = {
        .op_attr_mask =
            UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA | UCP_OP_ATTR_FLAG_NO_IMM_CMPL,
        .cb.recv = MPIDI_UCX_mrecv_cmpl_cb,
        .user_data = message,
    };

    void *recv_buf;
    size_t recv_count;
    if (dt_contig) {
        recv_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
        recv_count = data_sz;
    } else {
        recv_buf = buf;
        recv_count = count;
        param.op_attr_mask |= UCP_OP_ATTR_FIELD_DATATYPE;
        param.datatype = dt_ptr->dev.netmod.ucx.ucp_datatype;
        MPIR_Datatype_ptr_add_ref(dt_ptr);
    }

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nbx(MPIDI_UCX_global.ctx[vci].worker,
                                                         recv_buf, recv_count,
                                                         MPIDI_UCX_REQ(message).message_handler,
                                                         &param);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    MPIDI_UCX_REQ(message).s.ucp_request = ucp_request;

  fn_exit:
    MPIDI_UCX_THREAD_CS_EXIT_VCI(vci);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                MPIR_Request * partner)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vci_dst;
    MPIDI_UCX_RECV_VNIS(vci_dst);

    int need_cs;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    need_cs = true;
#else
    need_cs = (rank != MPI_ANY_SOURCE);
#endif

    if (need_cs) {
        MPIDI_UCX_THREAD_CS_ENTER_VCI(vci_dst);
    }
    mpi_errno =
        MPIDI_UCX_recv(buf, count, datatype, rank, tag, comm, context_offset, addr, vci_dst,
                       request);
    MPIDI_REQUEST_SET_LOCAL(*request, 0, partner);
    if (need_cs) {
        MPIDI_UCX_THREAD_CS_EXIT_VCI(vci_dst);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq, bool is_blocking)
{
    MPIR_FUNC_ENTER;

    if (!MPIR_Request_is_complete(rreq)) {
        int vci = MPIDI_Request_get_vci(rreq);
        ucp_request_cancel(MPIDI_UCX_global.ctx[vci].worker, MPIDI_UCX_REQ(rreq).s.ucp_request);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}
#endif /* ifndef MPIDI_ENABLE_AM_ONLY */

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_tag_recv(int rank, MPIR_Comm * comm,
                                                  int handler_id, int tag,
                                                  void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                  int vci_src, int vci_dst, MPIR_Request * rreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* UCX_RECV_H_INCLUDED */
