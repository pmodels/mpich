/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>
#include <stdarg.h>

/* style: allow:vprintf:1 sig:0 */
/* style: allow:printf:2 sig:0 */

#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char * func, char * fmt, ...)
{
    MPID_Common_thread_lock();
    {
	va_list list;
    
	MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
	va_start(list, fmt);
	MPIU_dbglog_vprintf(fmt, list);
	va_end(list);
	MPIU_dbglog_printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char * func, char * fmt, ...)
{
    MPID_Common_thread_lock();
    {
	va_list list;
    
	printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
	va_start(list, fmt);
	vprintf(fmt, list);
	va_end(list);
	printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt)
{
    MPID_Common_thread_lock();
    {
	MPIU_DBG_PRINTF(("MPIDI_CH3_Pkt_t:\n"));
	switch(pkt->type)
	{
	    case MPIDI_CH3_PKT_EAGER_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_send.data_sz));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_SEND\n"));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_sync_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_sync_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_sync_send.match.rank));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_send.sender_req_id));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_sync_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_sync_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_ACK\n"));
		MPIU_DBG_PRINTF((" sender_reqid . %d\n", pkt->eager_sync_ack.sender_req_id));
		break;
	    case MPIDI_CH3_PKT_READY_SEND:
		MPIU_DBG_PRINTF((" type ......... READY_SEND\n"));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->ready_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->ready_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->ready_send.match.rank));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->ready_send.sender_req_id));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->ready_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->ready_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_RNDV_SEND:
		MPIU_DBG_PRINTF((" type ......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->cancel_send_req.match.rank));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF((" ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	    case MPIDI_CH3_PKT_PUT:
		MPIU_DBG_PRINTF((" PUT\n"));
		break;
	    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
		MPIU_DBG_PRINTF((" FLOW_CNTRL_UPDATE\n"));
		break;
	    default:
		MPIU_DBG_PRINTF((" INVALID PACKET\n"));
		MPIU_DBG_PRINTF((" unknown type ... %d\n", pkt->type));
		MPIU_DBG_PRINTF(("  type .......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->eager_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->eager_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->cancel_send_req.match.rank));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF(("   ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	}
    }
    MPID_Common_thread_unlock();
}
#endif


