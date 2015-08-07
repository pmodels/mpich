/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#include "mxm_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_cancel_send(MPIDI_VC_t * vc, MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    MPID_nem_mxm_vc_area *vc_area = VC_BASE(req->ch.vc);
    MPID_nem_mxm_req_area *req_area = REQ_BASE(req);

    _dbg_mxm_output(5, "========> Canceling SEND req %p\n", req);

    if (likely(!_mxm_req_test(&req_area->mxm_req->item.base))) {
        ret = mxm_req_cancel_send(&req_area->mxm_req->item.send);
        if ((MXM_OK == ret) || (MXM_ERR_NO_PROGRESS == ret)) {
            _mxm_req_wait(&req_area->mxm_req->item.base);
            if (MPIR_STATUS_GET_CANCEL_BIT(req->status)) {
                vc_area->pending_sends -= 1;
            }
        }
        else {
            mpi_errno = MPI_ERR_INTERN;
        }
    }

    _dbg_mxm_out_req(req);

  fn_exit:
    return mpi_errno;
  fn_fail:ATTRIBUTE((unused))
        goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_cancel_recv(MPIDI_VC_t * vc, MPID_Request * req)
{
    int mpi_errno ATTRIBUTE((unused)) = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    MPID_nem_mxm_req_area *req_area = REQ_BASE(req);

    _dbg_mxm_output(5, "========> Canceling RECV req %p\n", req);

    if (likely(!_mxm_req_test(&req_area->mxm_req->item.base))) {
        ret = mxm_req_cancel_recv(&req_area->mxm_req->item.recv);
        if ((MXM_OK == ret) || (MXM_ERR_NO_PROGRESS == ret)) {
            _mxm_req_wait(&req_area->mxm_req->item.base);
        }
        else {
            mpi_errno = MPI_ERR_INTERN;
        }
    }

    _dbg_mxm_out_req(req);

  fn_exit:
    /* This function returns zero in case request is canceled
     * and nonzero otherwise
     */
    return (!MPIR_STATUS_GET_CANCEL_BIT(req->status));
  fn_fail:ATTRIBUTE((unused))
        goto fn_exit;
}
