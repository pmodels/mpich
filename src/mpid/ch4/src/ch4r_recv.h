/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_RECV_H_INCLUDED
#define CH4R_RECV_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDIG_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_ssend_ack_msg_t ack_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REPLY_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REPLY_SSEND);
    ack_msg.sreq_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(rreq->comm,
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                        (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(rreq->comm,
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                       (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REPLY_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_handle_unexp_mrecv(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);

    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    buf = MPIDIG_REQUEST(rreq, req->rreq.mrcv_buffer);
    count = MPIDIG_REQUEST(rreq, req->rreq.mrcv_count);
    datatype = MPIDIG_REQUEST(rreq, req->rreq.mrcv_datatype);

    MPIDIG_copy_from_unexp_req(rreq, buf, datatype, count);

    rreq->kind = MPIR_REQUEST_KIND__RECV;

    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDIG_reply_ssend(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIR_Datatype_release_if_not_builtin(datatype);
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* CH4R_RECV_H_INCLUDED */
