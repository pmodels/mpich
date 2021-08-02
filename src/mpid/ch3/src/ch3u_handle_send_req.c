/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidrma.h"

int MPIDI_CH3U_Handle_send_req(MPIDI_VC_t * vc, MPIR_Request * sreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn) (MPIDI_VC_t *, MPIR_Request *, int *);

    MPIR_FUNC_ENTER;

    /* Use the associated function rather than switching on the old ca field */
    /* Routines can call the attached function directly */
    reqFn = sreq->dev.OnDataAvail;
    if (!reqFn) {
        MPIR_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
        mpi_errno = MPID_Request_complete(sreq);
        *complete = 1;
    }
    else {
        mpi_errno = reqFn(vc, sreq, complete);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ----------------------------------------------------------------------- */
/* Here are the functions that implement the actions that are taken when
 * data is available for a send request (or other completion operations)
 * These include "send" requests that are part of the RMA implementation.
 */
/* ----------------------------------------------------------------------- */

int MPIDI_CH3_ReqHandler_GetSendComplete(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                         MPIR_Request * sreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr;
    int pkt_flags = sreq->dev.pkt_flags;

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPIR_Request_is_complete(sreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    MPIR_Win_get_ptr(sreq->dev.target_win_handle, win_ptr);

    /* here we decrement the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter--;
    MPIR_Assert(win_ptr->at_completion_counter >= 0);

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(sreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                    pkt_flags, MPI_WIN_NULL);
    MPIR_ERR_CHECK(mpi_errno);

    *complete = TRUE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_ReqHandler_GaccumSendComplete(MPIDI_VC_t * vc, MPIR_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr;
    int pkt_flags = rreq->dev.pkt_flags;

    MPIR_FUNC_ENTER;

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPIR_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    /* This function is triggered when sending back process of GACC/FOP/CAS
     * is finished. Only GACC used user_buf. FOP and CAS can fit all data
     * in response packet. */
    MPL_free(rreq->dev.user_buf);

    MPIR_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* here we decrement the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter--;
    MPIR_Assert(win_ptr->at_completion_counter >= 0);

    mpi_errno = MPID_Request_complete(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                    pkt_flags, MPI_WIN_NULL);
    MPIR_ERR_CHECK(mpi_errno);

    *complete = TRUE;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


int MPIDI_CH3_ReqHandler_CASSendComplete(MPIDI_VC_t * vc, MPIR_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr;
    int pkt_flags = rreq->dev.pkt_flags;

    MPIR_FUNC_ENTER;

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPIR_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    /* This function is triggered when sending back process of GACC/FOP/CAS
     * is finished. Only GACC used user_buf. FOP and CAS can fit all data
     * in response packet. */
    MPL_free(rreq->dev.user_buf);

    MPIR_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* here we decrement the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter--;
    MPIR_Assert(win_ptr->at_completion_counter >= 0);

    mpi_errno = MPID_Request_complete(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                    pkt_flags, MPI_WIN_NULL);
    MPIR_ERR_CHECK(mpi_errno);

    *complete = TRUE;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_ReqHandler_FOPSendComplete(MPIDI_VC_t * vc, MPIR_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Win *win_ptr;
    int pkt_flags = rreq->dev.pkt_flags;

    MPIR_FUNC_ENTER;

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPIR_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    /* This function is triggered when sending back process of GACC/FOP/CAS
     * is finished. Only GACC used user_buf. FOP and CAS can fit all data
     * in response packet. */
    MPL_free(rreq->dev.user_buf);

    MPIR_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* here we decrement the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter--;
    MPIR_Assert(win_ptr->at_completion_counter >= 0);

    mpi_errno = MPID_Request_complete(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                    pkt_flags, MPI_WIN_NULL);
    MPIR_ERR_CHECK(mpi_errno);

    *complete = TRUE;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


int MPIDI_CH3_ReqHandler_SendReloadIOV(MPIDI_VC_t * vc ATTRIBUTE((unused)), MPIR_Request * sreq,
                                       int *complete)
{
    int mpi_errno;

    /* setting the iov_offset to 0 here is critical, since it is intentionally
     * not set in the _load_send_iov function */
    sreq->dev.iov_offset = 0;
    sreq->dev.iov_count = MPL_IOV_LIMIT;
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, sreq->dev.iov, &sreq->dev.iov_count);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");
    }

    *complete = FALSE;

  fn_fail:
    return mpi_errno;
}
