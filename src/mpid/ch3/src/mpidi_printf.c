/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>
#include <stdarg.h>

/* style: allow:vprintf:1 sig:0 */
/* style: allow:printf:2 sig:0 */

/* FIXME: What are these routines for?  Who uses them?  Why are they different
   from the src/util/dbg routines? */

/*
 * Note on thread safety.  These routines originally used
 * MPID_Common_thread_lock/unlock, but that lock was not defined or used
 * consistently with the global mutex approach (now defined as
 * SINGLE_CS_ENTER/EXIT).  As these debugging routines should also
 * be withdrawn in favor of the general messaging utility, the
 * Common_thread_lock/unlock has been removed.
 */

/* --BEGIN DEBUG-- */
#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char *func, char *fmt, ...)
{
    /* FIXME: This "unreferenced_arg" is an example of a problem with the
     * API (unneeded level argument) or the code (failure to check the
     * level argument).  Inserting these "unreference_arg" macros erroneously
     * suggests that the code is correct with this ununsed argument, and thus
     * commits the grave harm of obscuring a real problem */
    MPIU_UNREFERENCED_ARG(level);
    {
        va_list list;

        if (MPIR_Process.comm_world) {
            MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
        }
        else {
            MPIU_dbglog_printf("[-1] %s(): ", func);
        }
        va_start(list, fmt);
        MPIU_dbglog_vprintf(fmt, list);
        va_end(list);
        MPIU_dbglog_printf("\n");
        fflush(stdout);
    }
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char *func, char *fmt, ...)
{
    {
        va_list list;

        if (MPIR_Process.comm_world) {
            printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
        }
        else {
            printf("[-1] ERROR - %s(): ", func);
        }
        va_start(list, fmt);
        vprintf(fmt, list);
        va_end(list);
        printf("\n");
        fflush(stdout);
    }
}

/* FIXME: It would be much better if the routine to print packets used
   routines defined by packet type, making it easier to modify the handling
   of packet types (and allowing channels to customize the printing
   of packets). For example, an array of function pointers, indexed by
   packet type, could be used.
   Also, these routines should not use MPIU_DBG_PRINTF, instead they should
   us a simple fprintf with a style allowance (so that the style checker
   won't flag the use as a possible problem).

   This should switch to using a table of functions

   MPIDI_PktPrintFunctions[pkt->type](stdout,pkt);

*/

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t * pkt)
{
    {
        MPIU_DBG_PRINTF(("MPIDI_CH3_Pkt_t:\n"));
        switch (pkt->type) {
        case MPIDI_CH3_PKT_EAGER_SEND:
            MPIDI_CH3_PktPrint_EagerSend(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_READY_SEND:
            MPIDI_CH3_PktPrint_ReadySend(stdout, pkt);
            break;

        case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
            MPIDI_CH3_PktPrint_EagerSyncSend(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
            MPIDI_CH3_PktPrint_EagerSyncAck(stdout, pkt);
            break;

        case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
            MPIDI_CH3_PktPrint_RndvReqToSend(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
            MPIDI_CH3_PktPrint_RndvClrToSend(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_RNDV_SEND:
            MPIDI_CH3_PktPrint_RndvSend(stdout, pkt);
            break;

        case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
            MPIDI_CH3_PktPrint_CancelSendReq(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
            MPIDI_CH3_PktPrint_CancelSendResp(stdout, pkt);
            break;

            /* FIXME: Move these RMA descriptions into the RMA code files */
        case MPIDI_CH3_PKT_PUT:
            MPIDI_CH3_PktPrint_Put(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_GET:
            MPIDI_CH3_PktPrint_Get(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_GET_RESP:
            MPIDI_CH3_PktPrint_GetResp(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_ACCUMULATE:
            MPIDI_CH3_PktPrint_Accumulate(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_LOCK:
            MPIDI_CH3_PktPrint_Lock(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_ACK:
            MPIDI_CH3_PktPrint_Ack(stdout, pkt);
            break;
        case MPIDI_CH3_PKT_LOCK_ACK:
            MPIDI_CH3_PktPrint_LockAck(stdout, pkt);
            break;
            /*
             * case MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE:
             * MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE\n"));
             * MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->shared_lock_ops_done.source_win_handle));
             * break;
             */
        case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
            MPIU_DBG_PRINTF((" FLOW_CNTRL_UPDATE\n"));
            break;

        case MPIDI_CH3_PKT_CLOSE:
            MPIDI_CH3_PktPrint_Close(stdout, pkt);
            break;

        default:
            MPIU_DBG_PRINTF((" INVALID PACKET\n"));
            MPIU_DBG_PRINTF((" unknown type ... %d\n", pkt->type));
            MPIU_DBG_PRINTF(("  type .......... EAGER_SEND\n"));
            MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
            MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->eager_send.match.parts.context_id));
            MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->eager_send.data_sz));
            MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->eager_send.match.parts.tag));
            MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->eager_send.match.parts.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
            MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
            MPIU_DBG_PRINTF(("  type .......... REQ_TO_SEND\n"));
            MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
            MPIU_DBG_PRINTF(("   context_id ... %d\n",
                             pkt->rndv_req_to_send.match.parts.context_id));
            MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
            MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->rndv_req_to_send.match.parts.tag));
            MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->rndv_req_to_send.match.parts.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
            MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
            MPIU_DBG_PRINTF(("  type .......... CLR_TO_SEND\n"));
            MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
            MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
            MPIU_DBG_PRINTF(("  type .......... RNDV_SEND\n"));
            MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
            MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND\n"));
            MPIU_DBG_PRINTF(("   context_id ... %d\n",
                             pkt->cancel_send_req.match.parts.context_id));
            MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->cancel_send_req.match.parts.tag));
            MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->cancel_send_req.match.parts.rank));
            MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
            MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND_RESP\n"));
            MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
            MPIU_DBG_PRINTF(("   ack .......... %d\n", pkt->cancel_send_resp.ack));
            break;
        }
    }
}
#endif


const char *MPIDI_VC_GetStateString(int state)
{
    switch (state) {
    case MPIDI_VC_STATE_INACTIVE:
        return "MPIDI_VC_STATE_INACTIVE";
    case MPIDI_VC_STATE_INACTIVE_CLOSED:
        return "MPIDI_VC_STATE_INACTIVE_CLOSED";
    case MPIDI_VC_STATE_ACTIVE:
        return "MPIDI_VC_STATE_ACTIVE";
    case MPIDI_VC_STATE_LOCAL_CLOSE:
        return "MPIDI_VC_STATE_LOCAL_CLOSE";
    case MPIDI_VC_STATE_REMOTE_CLOSE:
        return "MPIDI_VC_STATE_REMOTE_CLOSE";
    case MPIDI_VC_STATE_CLOSE_ACKED:
        return "MPIDI_VC_STATE_CLOSE_ACKED";
    case MPIDI_VC_STATE_CLOSED:
        return "MPIDI_VC_STATE_CLOSED";
    case MPIDI_VC_STATE_MORIBUND:
        return "MPIDI_VC_STATE_MORIBUND";
    default:
        return "unknown";
    }
}

/* This routine is not thread safe and should only be used while
   debugging.  It is used to encode a brief description of a message
   packet into a string to make it easy to include in the message log
   output (with no newlines to simplify extracting info from the log file)
*/
const char *MPIDI_Pkt_GetDescString(MPIDI_CH3_Pkt_t * pkt)
{
    static char pktmsg[256];

    /* For data messages, the string (...) is (context,tag,rank,size) */
    switch (pkt->type) {
    case MPIDI_CH3_PKT_EAGER_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "EAGER_SEND - (%d,%d,%d,)" MPIDI_MSG_SZ_FMT,
                     pkt->eager_send.match.parts.context_id,
                     (int) pkt->eager_send.match.parts.tag,
                     pkt->eager_send.match.parts.rank, pkt->eager_send.data_sz);
        break;
    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "EAGER_SYNC_SEND - (%d,%d,%d,)" MPIDI_MSG_SZ_FMT " req=%d",
                     pkt->eager_sync_send.match.parts.context_id,
                     (int) pkt->eager_sync_send.match.parts.tag,
                     pkt->eager_sync_send.match.parts.rank,
                     pkt->eager_sync_send.data_sz, pkt->eager_sync_send.sender_req_id);
        break;
    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "EAGER_SYNC_ACK - req=%d", pkt->eager_sync_ack.sender_req_id);
        break;
    case MPIDI_CH3_PKT_READY_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "READY_SEND - (%d,%d,%d,)" MPIDI_MSG_SZ_FMT,
                     pkt->ready_send.match.parts.context_id,
                     (int) pkt->ready_send.match.parts.tag,
                     pkt->ready_send.match.parts.rank, pkt->ready_send.data_sz);
        break;
    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "RNDV_REQ_TO_SEND - (%d,%d,%d,)" MPIDI_MSG_SZ_FMT " req=%d",
                     pkt->rndv_req_to_send.match.parts.context_id,
                     (int) pkt->rndv_req_to_send.match.parts.tag,
                     pkt->rndv_req_to_send.match.parts.rank,
                     pkt->rndv_req_to_send.data_sz, pkt->rndv_req_to_send.sender_req_id);
        break;
    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "RNDV_CLRTO_SEND - req=%d, recv req=%d",
                     pkt->rndv_clr_to_send.sender_req_id, pkt->rndv_clr_to_send.receiver_req_id);
        break;
    case MPIDI_CH3_PKT_RNDV_SEND:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "RNDV_SEND - recv req=%d", pkt->rndv_send.receiver_req_id);
        break;
    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "CANCEL_SEND_REQ - req=%d", pkt->cancel_send_req.sender_req_id);
        break;
    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "CANCEL_SEND_RESP - req=%d ack=%d",
                     pkt->cancel_send_resp.sender_req_id, pkt->cancel_send_resp.ack);
        break;
    case MPIDI_CH3_PKT_PUT:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "PUT - (%p,%d,0x%08X)",
                     pkt->put.addr, pkt->put.count, pkt->put.target_win_handle);
        break;
    case MPIDI_CH3_PKT_GET:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "GET - (%p,%d,0x%08X) req=%d",
                     pkt->get.addr,
                     pkt->get.count, pkt->get.target_win_handle, pkt->get.request_handle);
        break;
    case MPIDI_CH3_PKT_GET_RESP:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "GET_RESP - req=%d", pkt->get_resp.request_handle);
        break;
    case MPIDI_CH3_PKT_ACCUMULATE:
        MPL_snprintf(pktmsg, sizeof(pktmsg),
                     "ACCUMULATE - (%p,%d,0x%08X)",
                     pkt->accum.addr, pkt->accum.count, pkt->accum.target_win_handle);
        break;
    case MPIDI_CH3_PKT_LOCK:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "LOCK - %d", pkt->lock.target_win_handle);
        break;
    case MPIDI_CH3_PKT_ACK:
        /* There is no rma_done packet type */
        MPL_snprintf(pktmsg, sizeof(pktmsg), "RMA_DONE - 0x%08X", pkt->ack.source_win_handle);
        break;
    case MPIDI_CH3_PKT_LOCK_ACK:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "LOCK_ACK - 0x%08X", pkt->lock_ack.source_win_handle);
        break;
    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "FLOW_CNTL_UPDATE");
        break;
    case MPIDI_CH3_PKT_CLOSE:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "CLOSE ack=%d", pkt->close.ack);
        break;

    default:
        MPL_snprintf(pktmsg, sizeof(pktmsg), "INVALID PACKET type=%d", pkt->type);
        break;
    }

    return pktmsg;
}

/* --END DEBUG-- */
