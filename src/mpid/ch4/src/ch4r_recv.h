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

static inline int MPIDIG_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Context_id_t context_id = 0;
    MPIDIG_ssend_ack_msg_t ack_msg;
    MPIR_Comm *comm;
    MPIDI_av_entry_t *av;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REPLY_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REPLY_SSEND);
    MPIR_cc_incr(rreq->cc_ptr, &c);
    ack_msg.sreq_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);
    context_id = MPIDIG_REQUEST(rreq, context_id);
    comm = MPIDIG_context_id_to_comm(context_id);
    av = MPIDIU_comm_rank_to_av(comm, MPIDIG_REQUEST(rreq, rank));

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_isend_reply(MPIDIG_REQUEST(rreq, rank), comm, MPIDIG_SSEND_ACK, &ack_msg,
                                     sizeof(ack_msg), NULL, 0, MPI_DATATYPE_NULL, rreq, av);
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_isend_reply(MPIDIG_REQUEST(rreq, rank), comm, MPIDIG_SSEND_ACK, &ack_msg,
                                    sizeof(ack_msg), NULL, 0, MPI_DATATYPE_NULL, rreq, av);
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REPLY_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int MPIDIG_handle_unexp_mrecv(MPIR_Request * rreq)
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
        MPIR_Memcpy((char *) buf + dt_true_lb, MPIDIG_REQUEST(rreq, buffer), nbytes);
    }

    MPL_free(MPIDIG_REQUEST(rreq, buffer));
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
