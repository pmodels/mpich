/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_impl.h"

/* receive buffer */
typedef struct recv_buffer
{
    struct recv_buffer *next;
    volatile MPID_nem_pkt_t pkt;
} recv_buffer_t;

static recv_buffer_t *recv_buffers;

#define RECVBUF_TO_PKT(rbp) (&(rbp)->pkt)
#define PKT_TO_RECVBUF(pktp) ((recv_buffer_t *)((MPI_Aint)(pktp) - (MPI_Aint)(&((recv_buffer_t *)0)->pkt)))

#define RECVBUF_S_EMPTY() GENERIC_S_EMPTY(recvbuf_stack)
#define RECVBUF_S_PUSH(ep) GENERIC_S_PUSH(&recvbuf_stack, ep, next)
#define RECVBUF_S_POP(ep) GENERIC_S_POP(&recvbuf_stack, ep, next)
static struct {recv_buffer_t *top;} recvbuf_stack;


static int num_recv_tokens;
static int recv_tokens_hiwatermark;
#define HIWATERMARK_RATIO 0.75

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_recv_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_recv_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    gm_status_t status;
    MPIU_CHKPMEM_DECL (1);

    num_recv_tokens = gm_num_receive_tokens(MPID_nem_module_gm_port);

    recvbuf_stack.top = NULL;
    
    MPIU_CHKPMEM_MALLOC(recv_buffers, recv_buffer_t *, sizeof(recv_buffer_t) * num_recv_tokens, mpi_errno, "recvbuf");
    status = gm_register_memory(MPID_nem_module_gm_port, (void *)recv_buffers,
                                sizeof(recv_buffer_t) * num_recv_tokens);
    MPIU_ERR_CHKANDJUMP1(status != GM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gm_regmem", "**gm_regmem %d", status);

    recv_tokens_hiwatermark = num_recv_tokens * HIWATERMARK_RATIO;
    
    for (i = 0; i < num_recv_tokens; ++i)
    {
	gm_provide_receive_buffer_with_tag(MPID_nem_module_gm_port, (void *)RECVBUF_TO_PKT(&recv_buffers[i]), PACKET_SIZE,
                                           GM_LOW_PRIORITY, i);
    }
    num_recv_tokens = 0;
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int MPID_nem_gm_recv()
{
    int mpi_errno = MPI_SUCCESS;
    gm_recv_event_t *e;
    MPIDI_VC_t *vc;
    
    /*    printf_d ("MPID_nem_gm_recv()\n"); */
    
    while (!RECVBUF_S_EMPTY())
    {
        recv_buffer_t *recvbuf;

        RECVBUF_S_POP(&recvbuf);
        
	gm_provide_receive_buffer_with_tag(MPID_nem_module_gm_port, (void *)RECVBUF_TO_PKT(recvbuf), PACKET_SIZE, GM_LOW_PRIORITY, -1);
	--num_recv_tokens;
        MPIU_Assert(num_recv_tokens >= 0);
    }

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    e = gm_receive (MPID_nem_module_gm_port);
    while (gm_ntoh_u8 (e->recv.type) != GM_NO_RECV_EVENT)
    {
        volatile MPID_nem_pkt_t *pkt;
        int msg_len;
        
	switch (gm_ntoh_u8 (e->recv.type))
	{
	case GM_FAST_HIGH_PEER_RECV_EVENT:
	case GM_FAST_HIGH_RECV_EVENT:
	case GM_HIGH_PEER_RECV_EVENT:
	case GM_HIGH_RECV_EVENT:
	    printf ("Received unexpected high priority message\n");
	    gm_provide_receive_buffer_with_tag (MPID_nem_module_gm_port, gm_ntohp (e->recv.buffer), gm_ntoh_u8 (e->recv.size), GM_HIGH_PRIORITY,
						gm_ntoh_u8 (e->recv.tag));
	    break;
	case GM_FAST_PEER_RECV_EVENT:
	case GM_FAST_RECV_EVENT:
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
	    DO_PAPI (PAPI_reset (PAPI_EventSet));

            pkt = (volatile MPID_nem_pkt_t *)gm_ntohp(e->recv.message);
            msg_len = pkt->mpich2.datalen;
            MPIU_Assert(msg_len ==  gm_ntoh_u32(e->recv.length) - MPID_NEM_MPICH2_HEAD_LEN);
            MPIDI_PG_Get_vc_set_active (MPIDI_Process.my_pg, pkt->mpich2.source, &vc);

            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Recvd pkt src=%d len=%d\n", pkt->mpich2.source, msg_len));
            mpi_errno = MPID_nem_handle_pkt(vc, (char *)pkt->mpich2.payload, msg_len);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            RECVBUF_S_PUSH(PKT_TO_RECVBUF((volatile MPID_nem_pkt_t *)gm_ntohp(e->recv.buffer)));
            ++num_recv_tokens;

	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues7));
            break;
	case GM_PEER_RECV_EVENT:
	case GM_RECV_EVENT:
            pkt = (volatile MPID_nem_pkt_t *)gm_ntohp(e->recv.buffer);
            msg_len = pkt->mpich2.datalen;
            MPIU_Assert(msg_len ==  gm_ntoh_u32(e->recv.length) - MPID_NEM_MPICH2_HEAD_LEN);
            MPIDI_PG_Get_vc_set_active (MPIDI_Process.my_pg, pkt->mpich2.source, &vc);
            
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Recvd pkt src=%d len=%d\n", pkt->mpich2.source, msg_len));
            mpi_errno = MPID_nem_handle_pkt(vc, (char *)pkt->mpich2.payload, msg_len);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            RECVBUF_S_PUSH(PKT_TO_RECVBUF(pkt));
            ++num_recv_tokens;
	    break;
	default:
	    gm_unknown (MPID_nem_module_gm_port, e);
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues6));
	}

        if (num_recv_tokens > recv_tokens_hiwatermark)
        {
            while (!RECVBUF_S_EMPTY())
            {
                recv_buffer_t *recvbuf;
        
                RECVBUF_S_POP(&recvbuf);
                gm_provide_receive_buffer_with_tag(MPID_nem_module_gm_port, (void *)RECVBUF_TO_PKT(recvbuf), PACKET_SIZE,
                                                   GM_LOW_PRIORITY, -1);
                --num_recv_tokens;
                MPIU_Assert(num_recv_tokens >= 0);
            }
        }
	
	DO_PAPI (PAPI_reset (PAPI_EventSet));
	e = gm_receive (MPID_nem_module_gm_port);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static inline int
lmt_poll()
{
    int ret;
    MPID_nem_gm_lmt_queue_t *e;
    
    MPID_nem_gm_queue_dequeue (lmt, &e);
    
    while (e && MPID_nem_module_gm_num_send_tokens)
    {
	ret = MPID_nem_gm_lmt_do_get (e->node_id, e->port_id, &e->r_iov, &e->r_n_iov, &e->r_offset, &e->s_iov, &e->s_n_iov, &e->s_offset,
                                             e->compl_ctr);
	if (ret == LMT_AGAIN)
	{
	    MPID_nem_gm_queue_free (lmt, e);
	    MPID_nem_gm_queue_dequeue (lmt, &e);
	}
	else if (ret == LMT_FAILURE)
	{
	    printf ("error: MPID_nem_gm_lmt_do_get failed.  Dequeuing.\n");
	    MPID_nem_gm_queue_free (lmt, e);
	    MPID_nem_gm_queue_dequeue (lmt, &e);
	}
    }
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_send_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_gm_send_poll( void )
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPID_nem_send_from_queue();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    /*lmt_poll(); */
    mpi_errno = MPID_nem_gm_recv();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_recv_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_gm_recv_poll( void )
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPID_nem_gm_recv();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    mpi_errno = MPID_nem_send_from_queue();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    /*lmt_poll(); */
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_poll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPID_nem_gm_recv();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    mpi_errno = MPID_nem_send_from_queue();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
