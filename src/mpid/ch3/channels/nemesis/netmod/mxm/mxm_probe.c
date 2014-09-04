/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#include "mxm_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_probe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_probe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                       MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t err;
    mxm_recv_req_t mxm_req;
    MPID_nem_mxm_vc_area *vc_area = (vc ? VC_BASE(vc) : NULL);

    MPIDI_STATE_DECL(MPID_STATE_MXM_PROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_PROBE);

    mxm_req.base.state = MXM_REQ_NEW;
    mxm_req.base.mq = (mxm_mq_h) comm->dev.ch.netmod_priv;
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_iprobe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                        int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t err;
    mxm_recv_req_t mxm_req;
    MPID_nem_mxm_vc_area *vc_area = (vc ? VC_BASE(vc) : NULL);

    MPIDI_STATE_DECL(MPID_STATE_MXM_IPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_IPROBE);

    mxm_req.base.state = MXM_REQ_NEW;
    mxm_req.base.mq = (mxm_mq_h) comm->dev.ch.netmod_priv;
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_improbe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_IMPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_IMPROBE);

    MPIU_Assert(0 && "not currently implemented");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_IMPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                  MPI_Status * status)
{
    return MPID_nem_mxm_iprobe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, status);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_improbe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                   MPID_Request ** message, MPI_Status * status)
{
    return MPID_nem_mxm_improbe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, message,
                                status);
}
