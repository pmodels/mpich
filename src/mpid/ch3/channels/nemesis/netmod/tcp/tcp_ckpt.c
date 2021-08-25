/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#define SOCKSM_H_EXTERNS_ 1
#include "tcp_impl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef ENABLE_CHECKPOINTING

int MPID_nem_tcp_ckpt_pause_send_vc(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);

    MPIR_FUNC_ENTER;

    vc_tcp->send_paused = TRUE;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPID_nem_tcp_pkt_unpause_handler(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                     void *data ATTRIBUTE((unused)),
                                     intptr_t * buflen, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);

    MPIR_FUNC_ENTER;

    vc_tcp->send_paused = FALSE;

    /* There may be a unpause message in the send queue.  If so, just enqueue everything on the send queue. */
    if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
        mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->paused_send_queue);

    /* if anything is left on the paused queue, put it on the send queue and wait for the reconnect */
    if (!MPIDI_CH3I_Sendq_empty(vc_tcp->paused_send_queue)) {

        MPIDI_CH3I_Sendq_enqueue_multiple_no_refcount(&vc_tcp->send_queue,
                                                      vc_tcp->paused_send_queue.head,
                                                      vc_tcp->paused_send_queue.tail);
        vc_tcp->paused_send_queue.head = vc_tcp->paused_send_queue.tail = NULL;
    }

  fn_exit:
    *buflen = 0;

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPID_nem_tcp_ckpt_continue_vc(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_PKT_DECL_CAST(upkt, MPIDI_nem_tcp_pkt_unpause_t, unpause_pkt);
    MPIR_Request *unpause_req;

    MPIR_FUNC_ENTER;

    unpause_pkt->type = MPIDI_NEM_PKT_NETMOD;
    unpause_pkt->subtype = MPIDI_NEM_TCP_PKT_UNPAUSE;

    mpi_errno =
        MPID_nem_tcp_iStartContigMsg_paused(vc, &upkt, sizeof(MPIDI_nem_tcp_pkt_unpause_t), NULL, 0,
                                            &unpause_req);
    MPIR_ERR_CHECK(mpi_errno);
    if (unpause_req) {
        if (unpause_req->status.MPI_ERROR)
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
        MPIR_Request_free(unpause_req);
        if (mpi_errno)
            goto fn_fail;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}



int MPID_nem_tcp_ckpt_restart_vc(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_nem_tcp_pkt_unpause_t *const pkt = (MPIDI_nem_tcp_pkt_unpause_t *) & upkt;
    MPIR_Request *sreq;

    MPIR_FUNC_ENTER;

    pkt->type = MPIDI_NEM_PKT_NETMOD;
    pkt->subtype = MPIDI_NEM_TCP_PKT_UNPAUSE;

    mpi_errno = MPID_nem_tcp_iStartContigMsg_paused(vc, pkt, sizeof(pkt), NULL, 0, &sreq);
    MPIR_ERR_CHECK(mpi_errno);

    if (sreq != NULL) {
        if (sreq->status.MPI_ERROR != MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
            MPIR_Request_free(sreq);
            MPIR_ERR_INTERNALANDJUMP(mpi_errno, "Failed to send checkpoint unpause pkt.");
        }
        MPIR_Request_free(sreq);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#endif /* ENABLE_CHECKPOINTING */
