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

#endif /* CH4R_RECV_H_INCLUDED */
