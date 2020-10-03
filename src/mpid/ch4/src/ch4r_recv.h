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
            MPIDI_SHM_am_send_hdr_reply(MPIDIG_REQUEST(rreq, context_id),
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                        sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(MPIDIG_REQUEST(rreq, context_id),
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                       sizeof(ack_msg));
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
    size_t message_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t data_sz ATTRIBUTE((unused)), dt_sz, nbytes;
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

    message_sz = MPIDIG_REQUEST(rreq, count);
    MPIR_Datatype_get_size_macro(datatype, dt_sz);

    if (message_sz > count * dt_sz) {
        rreq->status.MPI_ERROR = MPIR_Err_create_code(rreq->status.MPI_ERROR,
                                                      MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__,
                                                      MPI_ERR_TRUNCATE, "**truncate",
                                                      "**truncate %d %d %d %d",
                                                      rreq->status.MPI_SOURCE, rreq->status.MPI_TAG,
                                                      dt_sz * count, message_sz);
        nbytes = dt_sz * count;
    } else {
        nbytes = message_sz;
    }

    MPIR_STATUS_SET_COUNT(rreq->status, nbytes);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (!dt_contig) {
        MPI_Aint actual_unpack_bytes;
        mpi_errno = MPIR_Typerep_unpack(MPIDIG_REQUEST(rreq, buffer), nbytes, buf,
                                        count, datatype, 0, &actual_unpack_bytes);
        MPIR_ERR_CHECK(mpi_errno);

        if (actual_unpack_bytes != (MPI_Aint) nbytes) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            rreq->status.MPI_ERROR = mpi_errno;
        }
    } else {
        MPIR_Typerep_copy((char *) buf + dt_true_lb, MPIDIG_REQUEST(rreq, buffer), nbytes);
    }

    MPIDU_genq_private_pool_free_cell(MPIDI_global.unexp_pack_buf_pool,
                                      MPIDIG_REQUEST(rreq, buffer));
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
