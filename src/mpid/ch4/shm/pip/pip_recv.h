/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#ifndef PIP_RECV_H_INCLUDED
#define PIP_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_handle_lmt_rts_recv(uint64_t src_offset,
                                                           uint64_t src_data_sz, uint64_t sreq_ptr,
                                                           int src_lrank, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz, recv_data_sz;
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
    MPIDI_SHM_ctrl_pip_send_lmt_send_fin_t *slmt_fin_hdr = &ack_ctrl_hdr.pip_slmt_fin;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PIP_HANDLE_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PIP_HANDLE_LMT_RECV);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Copy data to receive buffer */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    mpi_errno = MPIR_Localcopy((void *) src_offset, recv_data_sz,
                               MPI_BYTE, (char *) MPIDIG_REQUEST(rreq, buffer),
                               MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype));

    PIP_TRACE("handle_lmt_recv: handle matched rreq %p [source %d, tag %d, context_id 0x%x],"
              " copy dst %p, src %p, bytes %ld\n", rreq, MPIDIG_REQUEST(rreq, rank),
              MPIDIG_REQUEST(rreq, tag), MPIDIG_REQUEST(rreq, context_id),
              (char *) MPIDIG_REQUEST(rreq, buffer), src_offset, recv_data_sz);

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* Send ack to sender */
    slmt_fin_hdr->req_ptr = sreq_ptr;
    mpi_errno =
        MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                               MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id)),
                               MPIDI_SHM_PIP_SEND_LMT_SEND_ACK, &ack_ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PIP_HANDLE_LMT_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif /* PIP_RECV_H_INCLUDED */
