/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_RECV_H_INCLUDED
#define XPMEM_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "xpmem_pre.h"
#include "xpmem_seg.h"
#include "xpmem_impl.h"

/* Handle and complete a matched XPMEM LMT receive request. Input parameters
 * include send buffer info (see definition in MPIDI_SHM_ctrl_xpmem_send_lmt_req_t)
 * and receive request. */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_handle_lmt_recv(uint64_t src_offset, uint64_t src_data_sz,
                                                         uint64_t sreq_ptr, int src_lrank,
                                                         MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_XPMEM_seg_t *seg_ptr = NULL;
    void *attached_sbuf = NULL;
    size_t data_sz, recv_data_sz;
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
    MPIDI_SHM_ctrl_xpmem_send_lmt_ack_t *slmt_ack_hdr = &ack_ctrl_hdr.xpmem_slmt_ack;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_XPMEM_HANDLE_LMT_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_XPMEM_HANDLE_LMT_RECV);

    mpi_errno = MPIDI_XPMEM_seg_regist(src_lrank, src_data_sz, (void *) src_offset,
                                       &seg_ptr, &attached_sbuf,
                                       &MPIDI_XPMEM_global.segmaps[src_lrank].segcache);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Copy data to receive buffer */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    mpi_errno = MPIR_Localcopy(attached_sbuf, recv_data_sz,
                               MPI_BYTE, (char *) MPIDIG_REQUEST(rreq, buffer),
                               MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype));

    XPMEM_TRACE("handle_lmt_recv: handle matched rreq %p [source %d, tag %d, context_id 0x%x],"
                " copy dst %p, src %p, bytes %ld\n", rreq, MPIDIG_REQUEST(rreq, rank),
                MPIDIG_REQUEST(rreq, tag), MPIDIG_REQUEST(rreq, context_id),
                (char *) MPIDIG_REQUEST(rreq, buffer), attached_sbuf, recv_data_sz);

    mpi_errno = MPIDI_XPMEM_seg_deregist(seg_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Set receive status */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* Send ack to sender */
    slmt_ack_hdr->sreq_ptr = sreq_ptr;
    mpi_errno =
        MPIDI_SHM_do_ctrl_send(MPIDIG_REQUEST(rreq, rank),
                               MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id)),
                               MPIDI_SHM_XPMEM_SEND_LMT_ACK, &ack_ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_XPMEM_HANDLE_LMT_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif /* XPMEM_RECV_H_INCLUDED */
