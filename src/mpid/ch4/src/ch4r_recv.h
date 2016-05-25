/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4R_RECV_H_INCLUDED
#define CH4R_RECV_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_reply_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_CH4U_ssend_ack_msg_t ack_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REPLY_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REPLY_SSEND);
    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.sreq_ptr = MPIDI_CH4U_REQUEST(rreq, req->rreq.peer_req_ptr);

    mpi_errno = MPIDI_NM_am_isend_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(rreq, tag)),
                                        MPIDI_CH4U_REQUEST(rreq, rank),
                                        MPIDI_CH4U_SSEND_ACK, &ack_msg, sizeof(ack_msg),
                                        NULL, 0, MPI_DATATYPE_NULL, rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REPLY_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_handle_unexp_mrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_handle_unexp_mrecv(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t msg_tag;
    size_t message_sz;
    MPI_Aint last;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t data_sz, dt_sz;
    MPIR_Segment *segment_ptr;
    void *buf;
    int count;
    MPI_Datatype datatype;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_HANDLE_UNEXP_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_HANDLE_UNEXP_MRECV);

    msg_tag = MPIDI_CH4U_REQUEST(rreq, tag);
    rreq->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDI_CH4U_get_tag(msg_tag);

    buf = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_buffer);
    count = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_count);
    datatype = MPIDI_CH4U_REQUEST(rreq, req->rreq.mrcv_datatype);

    message_sz = MPIDI_CH4U_REQUEST(rreq, count);
    MPIR_Datatype_get_size_macro(datatype, dt_sz);

    if (message_sz > count * dt_sz) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
    }
    else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        count = message_sz / dt_sz;
    }

    MPIR_STATUS_SET_COUNT(rreq->status, count * dt_sz);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (!dt_contig) {
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv MPIR_Segment_alloc");
        MPIR_Segment_init(buf, count, datatype, segment_ptr, 0);

        last = count * dt_sz;
        MPIR_Segment_unpack(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, buffer));
        MPIR_Segment_free(segment_ptr);
        if (last != (MPI_Aint) (count * dt_sz)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            rreq->status.MPI_ERROR = mpi_errno;
        }
    }
    else {
        MPIR_Memcpy((char *) buf + dt_true_lb, MPIDI_CH4U_REQUEST(rreq, buffer), data_sz);
    }

    MPL_free(MPIDI_CH4U_REQUEST(rreq, buffer));
    rreq->kind = MPIR_REQUEST_KIND__RECV;

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_HANDLE_UNEXP_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* CH4R_RECV_H_INCLUDED */
