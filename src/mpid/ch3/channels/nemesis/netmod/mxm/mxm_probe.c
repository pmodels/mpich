/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#include "mxm_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_probe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                       MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t err;
    mxm_recv_req_t mxm_req;
    mxm_mq_h *mq_h_v = (mxm_mq_h *) comm->dev.ch.netmod_priv;
    MPID_nem_mxm_vc_area *vc_area = (vc ? VC_BASE(vc) : NULL);

    MPIDI_STATE_DECL(MPID_STATE_MXM_PROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_PROBE);

    mxm_req.base.state = MXM_REQ_NEW;
    mxm_req.base.mq = mq_h_v[0];
    mxm_req.base.conn = (vc_area ? vc_area->mxm_ep->mxm_conn : 0);

    mxm_req.tag = _mxm_tag_mpi2mxm(tag, comm->context_id + context_offset);
    mxm_req.tag_mask = _mxm_tag_mask(tag);

    do {
        err = mxm_req_probe(&mxm_req);
        _mxm_progress_cb(NULL);
    } while (err != MXM_ERR_NO_MESSAGE);

    if (MXM_OK == err) {
        _mxm_to_mpi_status(mxm_req.base.error, status);
        status->MPI_SOURCE = mxm_req.completion.sender_imm;
        status->MPI_TAG = _mxm_tag_mxm2mpi(mxm_req.completion.sender_tag);
        MPIR_STATUS_SET_COUNT(*status, mxm_req.completion.sender_len);
    }
    else {
        mpi_errno = MPI_ERR_INTERN;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_PROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_iprobe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                        int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t err;
    mxm_recv_req_t mxm_req;
    mxm_mq_h *mq_h_v = (mxm_mq_h *) comm->dev.ch.netmod_priv;
    MPID_nem_mxm_vc_area *vc_area = (vc ? VC_BASE(vc) : NULL);

    MPIDI_STATE_DECL(MPID_STATE_MXM_IPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_IPROBE);

    mxm_req.base.state = MXM_REQ_NEW;
    mxm_req.base.mq = mq_h_v[0];
    mxm_req.base.conn = (vc_area ? vc_area->mxm_ep->mxm_conn : 0);

    mxm_req.tag = _mxm_tag_mpi2mxm(tag, comm->context_id + context_offset);
    mxm_req.tag_mask = _mxm_tag_mask(tag);

    err = mxm_req_probe(&mxm_req);
    if (MXM_OK == err) {
        *flag = 1;
        _mxm_to_mpi_status(mxm_req.base.error, status);
        status->MPI_SOURCE = mxm_req.completion.sender_imm;
        status->MPI_TAG = _mxm_tag_mxm2mpi(mxm_req.completion.sender_tag);
        MPIR_STATUS_SET_COUNT(*status, mxm_req.completion.sender_len);
    }
    else if (MXM_ERR_NO_MESSAGE == err) {
        *flag = 0;
    }
    else {
        mpi_errno = MPI_ERR_INTERN;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_improbe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t err;
    mxm_recv_req_t mxm_req;
    mxm_message_h mxm_msg;
    mxm_mq_h *mq_h_v = (mxm_mq_h *) comm->dev.ch.netmod_priv;
    MPID_nem_mxm_vc_area *vc_area = (vc ? VC_BASE(vc) : NULL);

    MPIDI_STATE_DECL(MPID_STATE_MXM_IMPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_IMPROBE);

    mxm_req.base.state = MXM_REQ_NEW;
    mxm_req.base.mq = mq_h_v[0];
    mxm_req.base.conn = (vc_area ? vc_area->mxm_ep->mxm_conn : 0);

    mxm_req.tag = _mxm_tag_mpi2mxm(tag, comm->context_id + context_offset);
    mxm_req.tag_mask = _mxm_tag_mask(tag);

    err = mxm_req_mprobe(&mxm_req, &mxm_msg);
    if (MXM_OK == err) {
        MPID_Request *req;

        *flag = 1;

        req = MPID_Request_create();
        MPIU_Object_set_ref(req, 2);
        req->kind = MPID_REQUEST_MPROBE;
        req->comm = comm;
        MPIR_Comm_add_ref(comm);
        req->ch.vc = vc;
//        MPIDI_Request_set_sync_send_flag(req, 1); /* set this flag in case MXM_REQ_OP_SEND_SYNC*/
        MPIDI_Request_set_msg_type(req, MPIDI_REQUEST_EAGER_MSG);
        req->dev.recv_pending_count = 1;

        _mxm_to_mpi_status(mxm_req.base.error, &req->status);
        req->status.MPI_TAG = _mxm_tag_mxm2mpi(mxm_req.completion.sender_tag);
        req->status.MPI_SOURCE = mxm_req.completion.sender_imm;
        req->dev.recv_data_sz = mxm_req.completion.sender_len;
        MPIR_STATUS_SET_COUNT(req->status, req->dev.recv_data_sz);
        req->dev.tmpbuf = MPIU_Malloc(req->dev.recv_data_sz);
        MPIU_Assert(req->dev.tmpbuf);

        mxm_req.base.completed_cb = NULL;
        mxm_req.base.context = req;
        mxm_req.base.data_type = MXM_REQ_DATA_BUFFER;
        mxm_req.base.data.buffer.ptr = req->dev.tmpbuf;
        mxm_req.base.data.buffer.length = req->dev.recv_data_sz;

        err = mxm_message_recv(&mxm_req, mxm_msg);
        _mxm_req_wait(&mxm_req.base);

        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        *message = req;

        /* TODO: Should we change status
         * _mxm_to_mpi_status(mxm_req.base.error, status);
         */
        status->MPI_SOURCE = req->status.MPI_SOURCE;
        status->MPI_TAG = req->status.MPI_TAG;
        MPIR_STATUS_SET_COUNT(*status, req->dev.recv_data_sz);

        _dbg_mxm_output(8,
                        "imProbe ========> Found USER msg (context %d from %d tag %d size %d) \n",
                        comm->context_id + context_offset, status->MPI_SOURCE, status->MPI_TAG,
                        MPIR_STATUS_GET_COUNT(*status));
    }
    else if (MXM_ERR_NO_MESSAGE == err) {
        *flag = 0;
        *message = NULL;
    }
    else {
        mpi_errno = MPI_ERR_INTERN;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_IMPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                  MPI_Status * status)
{
    return MPID_nem_mxm_iprobe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, status);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_improbe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                   MPID_Request ** message, MPI_Status * status)
{
    return MPID_nem_mxm_improbe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, message,
                                status);
}
