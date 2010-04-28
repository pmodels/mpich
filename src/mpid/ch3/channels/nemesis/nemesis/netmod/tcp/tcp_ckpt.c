/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define SOCKSM_H_EXTERNS_ 1
#include "tcp_impl.h"

MPIU_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef ENABLE_CHECKPOINTING

#define SENDQ_EMPTY(q) GENERIC_Q_EMPTY (q)
#define SENDQ_HEAD(q) GENERIC_Q_HEAD (q)
#define SENDQ_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, dev.next)
#define SENDQ_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, dev.next)
#define SENDQ_ENQUEUE_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_MULTIPLE(qp, ep0, ep1, dev.next)

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_ckpt_pause_send_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_ckpt_pause_send_vc(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CKPT_PAUSE_SEND_VC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CKPT_PAUSE_SEND_VC);

    vc_tcp->send_paused = TRUE;
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CKPT_PAUSE_SEND_VC);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_ckpt_unpause_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_pkt_unpause_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CKPT_UNPAUSE_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CKPT_UNPAUSE_HANDLER);

    vc_tcp->send_paused = FALSE;
    
    /* if anything is waiting to be sent on the paused queue, put it on the send queue and reconnect */
    if (!SENDQ_EMPTY(vc_tcp->paused_send_queue)) {
        MPIU_Assert(SENDQ_EMPTY(vc_tcp->send_queue));
        
        SENDQ_ENQUEUE_MULTIPLE(&vc_tcp->send_queue, vc_tcp->paused_send_queue.head, vc_tcp->paused_send_queue.tail);
        vc_tcp->paused_send_queue.head = vc_tcp->paused_send_queue.tail = NULL;

        mpi_errno = MPID_nem_tcp_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CKPT_UNPAUSE_HANDLER);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_ckpt_continue_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_ckpt_continue_vc(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_PKT_DECL_CAST(upkt, MPIDI_nem_tcp_pkt_unpause_t, unpause_pkt);
    MPID_Request *unpause_req;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CKPT_CONTINUE_VC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CKPT_CONTINUE_VC);

    unpause_pkt->type = MPIDI_NEM_PKT_NETMOD;
    unpause_pkt->subtype = MPIDI_NEM_TCP_PKT_UNPAUSE;

    mpi_errno = MPID_nem_tcp_iStartContigMsg_paused(vc, &upkt, sizeof(MPIDI_nem_tcp_pkt_unpause_t), NULL, 0, &unpause_req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    if (unpause_req) {
        if (unpause_req->status.MPI_ERROR) MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
        MPID_Request_release(unpause_req);
        if (mpi_errno) goto fn_fail;
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CKPT_CONTINUE_VC);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_ckpt_restart_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_ckpt_restart_vc(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_nem_tcp_pkt_unpause_t * const pkt = (MPIDI_nem_tcp_pkt_unpause_t *)&upkt;
    MPID_Request *sreq;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CKPT_RESTART_VC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CKPT_RESTART_VC);

    pkt->type = MPIDI_NEM_PKT_NETMOD;
    pkt->subtype = MPIDI_NEM_TCP_PKT_UNPAUSE;

    mpi_errno = MPID_nem_tcp_iStartContigMsg_paused(vc, pkt, sizeof(pkt), NULL, 0, &sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (sreq != NULL) {
        if (sreq->status.MPI_ERROR != MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
            MPID_Request_release(sreq);
            MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Failed to send checkpoint unpause pkt.");
        }
        MPID_Request_release(sreq);
    }
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CKPT_RESTART_VC);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#endif /* ENABLE_CHECKPOINTING */
