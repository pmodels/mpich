/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_module_impl.h"

typedef struct sendbuf
{
    struct sendbuf *next;
    int node_id;
    int port_id;
    MPIDI_msg_sz_t datalen;
    struct 
    {
        int source_id;
        char buf[SENDPKT_DATALEN];
    } pkt;
} sendbuf_t;

static struct {sendbuf_t *top;} sendbuf_stack;
static struct {MPID_Request *head, *tail;} send_queue;
static MPID_Request *active_send;
static sendbuf_t *sendbufs;

#define SENDBUF_S_EMPTY() GENERIC_S_EMPTY(sendbuf_stack)
#define SENDBUF_S_PUSH(sbp) GENERIC_S_PUSH(&sendbuf_stack, sbp, next)
#define SENDBUF_S_POP(sbpp) GENERIC_S_POP(&sendbuf_stack, sbpp, next)

#define SEND_Q_EMPTY() GENERIC_Q_EMPTY(send_queue)
#define SEND_Q_ENQUEUE(reqp) GENERIC_Q_ENQUEUE(&send_queue, reqp, dev.next)
#define SEND_Q_DEQUEUE(reqpp) GENERIC_Q_DEQUEUE(&send_queue, reqpp, dev.next)

int MPID_nem_gm_module_send(MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    MPIU_Assert(0);
    return -1;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    gm_status_t status;
    MPIU_CHKPMEM_DECL(1);

    active_send = NULL;
    send_queue.head = send_queue.tail = NULL;
    sendbuf_stack.top = NULL;
    
    MPIU_CHKPMEM_MALLOC(sendbufs, sendbuf_t *, MPID_nem_module_gm_num_send_tokens * sizeof(sendbuf_t), mpi_errno, "sendbufs");

    status = gm_register_memory(MPID_nem_module_gm_port, (void *)sendbufs, MPID_nem_module_gm_num_send_tokens * sizeof (sendbuf_t));
    MPIU_ERR_CHKANDJUMP1(status != GM_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gm_regmem", "**gm_regmem %d", status);

    for (i = 0; i < MPID_nem_module_gm_num_send_tokens; ++i)
    {
        sendbufs[i].next = NULL;
        SENDBUF_S_PUSH(&sendbufs[i]);
    }
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

static void send_callback(struct gm_port *p, void *context, gm_status_t status)
{
    sendbuf_t *sb = (sendbuf_t *)context;

    printf_d ("send_callback()\n");

    if (status != GM_SUCCESS)
    {
	gm_perror ("Send error", status);
    }

    ++MPID_nem_module_gm_num_send_tokens;

    SENDBUF_S_PUSH(sb);
}


/* sends a packet consisting of a header and some of the data pointed
   to by *dataptr.  *dataptr and *dataleft will be updated to reflect
   the portion of the data not yet sent.  */
#undef FUNCNAME
#define FUNCNAME send_header_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void send_header_pkt(int node_id, int port_id, int source_id, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                   char **dataptr, MPIDI_msg_sz_t *dataleft)
{
    sendbuf_t *sb;
    MPIDI_msg_sz_t payload_len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_HEADER_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_HEADER_PKT);
    MPIU_Assert(MPID_nem_module_gm_num_send_tokens);
    MPIU_Assert(!SENDBUF_S_EMPTY());
    MPIU_Assert(SENDPKT_DATALEN > sizeof(MPIDI_CH3_Pkt_t));

    SENDBUF_S_POP(&sb);
        
    payload_len = (*dataleft > SENDPKT_DATALEN - sizeof(MPIDI_CH3_Pkt_t)) ? SENDPKT_DATALEN - sizeof(MPIDI_CH3_Pkt_t) : *dataleft;

    sb->node_id = node_id;
    sb->port_id = port_id;
    sb->datalen = PKT_HEADER_LEN + sizeof(MPIDI_CH3_Pkt_t) + payload_len;
    sb->pkt.source_id = source_id;

    /* copy header, then copy data starting at max header length */
    MPID_NEM_MEMCPY(&sb->pkt.buf, hdr, hdr_sz);
    MPID_NEM_MEMCPY(((char *)&sb->pkt.buf) + sizeof(MPIDI_CH3_Pkt_t), *dataptr, payload_len);
    
    gm_send_with_callback(MPID_nem_module_gm_port, &sb->pkt, PACKET_SIZE, sb->datalen, GM_LOW_PRIORITY, node_id, port_id,
                          send_callback, (void *)sb);
    --MPID_nem_module_gm_num_send_tokens;
    
    *dataleft -= payload_len;
    *dataptr  += payload_len;

    MPIDI_FUNC_EXIT(MPID_STATE_SEND_HEADER_PKT);
}

/* sends a packet consisting of some of the data pointed to by
   *dataptr.  *dataptr and *dataleft will be updated to reflect the
   portion of the data not yet sent.  */
#undef FUNCNAME
#define FUNCNAME send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void send_pkt(int node_id, int port_id, int source_id, char **dataptr, MPIDI_msg_sz_t *dataleft)
{
    sendbuf_t *sb;
    MPIDI_msg_sz_t payload_len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PKT);
    
    MPIU_Assert(!SENDBUF_S_EMPTY());

    if (*dataleft == 0)
        goto fn_exit;

    payload_len = ((*dataleft > SENDPKT_DATALEN) ? SENDPKT_DATALEN : *dataleft);
                
    SENDBUF_S_POP(&sb);

    sb->node_id = node_id;
    sb->port_id = port_id;
    sb->datalen = PKT_HEADER_LEN + payload_len;
    sb->pkt.source_id = source_id;
            
    MPID_NEM_MEMCPY(&sb->pkt.buf, *dataptr, payload_len);
           
    gm_send_with_callback (MPID_nem_module_gm_port, &sb->pkt, PACKET_SIZE, sb->datalen, GM_LOW_PRIORITY, sb->node_id,
                           sb->port_id, send_callback, (void *)sb);
    --MPID_nem_module_gm_num_send_tokens;

    *dataleft -= payload_len;
    *dataptr  += payload_len;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_PKT);
}

/* sends a packet consisting of an optional header and noncontiguous
   data described by a segment.  */
#undef FUNCNAME
#define FUNCNAME send_noncontig_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void send_noncontig_pkt(int node_id, int port_id, int source_id, void *hdr, MPIDI_msg_sz_t hdr_sz,
				      MPID_Segment *segment, MPIDI_msg_sz_t *segment_first, MPIDI_msg_sz_t segment_size)
{
    sendbuf_t *sb;
    MPIDI_msg_sz_t payload_len;
    MPIDI_msg_sz_t last;
    MPIDI_msg_sz_t h_len;
    
    MPIDI_STATE_DECL(MPID_STATE_SEND_NONCONTIG_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_NONCONTIG_PKT);
    MPIU_Assert(MPID_nem_module_gm_num_send_tokens);
    MPIU_Assert(!SENDBUF_S_EMPTY());
    MPIU_Assert(SENDPKT_DATALEN > sizeof(MPIDI_CH3_Pkt_t));

    SENDBUF_S_POP(&sb);

    if (hdr)
    {
	/* copy header */
	MPID_NEM_MEMCPY(&sb->pkt.buf, hdr, hdr_sz);
	h_len = sizeof(MPIDI_CH3_Pkt_t);
    }
    else
	h_len = 0;
   
    /* copy data */
    if (h_len + segment_size - *segment_first <= SENDPKT_DATALEN)
        last = segment_size;
    else
        last = *segment_first + SENDPKT_DATALEN - h_len;
    
    MPID_Segment_pack(segment, *segment_first, &last, ((char *)&sb->pkt.buf) + h_len);
    payload_len = h_len + last - *segment_first;
    *segment_first = last;
    
    sb->node_id = node_id;
    sb->port_id = port_id;
    sb->datalen = PKT_HEADER_LEN + payload_len;
    sb->pkt.source_id = source_id;
    
    gm_send_with_callback(MPID_nem_module_gm_port, &sb->pkt, PACKET_SIZE, sb->datalen, GM_LOW_PRIORITY, node_id, port_id,
                          send_callback, (void *)sb);
    --MPID_nem_module_gm_num_send_tokens;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_NONCONTIG_PKT);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_send_from_queue()
{
    int mpi_errno = MPI_SUCCESS;
    char *dataptr;
    int datalen;
    int complete;  

    while (active_send || !SEND_Q_EMPTY())
    {
        /* handle the partially sent requests */
        while (active_send)
        {
            MPIDI_VC_t *vc = active_send->ch.vc;

            if (MPID_nem_module_gm_num_send_tokens == 0)
                goto fn_exit;

	    if (active_send->ch.noncontig)
	    {
		/* send only if there's something left to send */
		if (active_send->dev.segment_first != active_send->dev.segment_size)
		    send_noncontig_pkt(VC_FIELD(vc, gm_node_id), VC_FIELD(vc, gm_port_id), VC_FIELD(vc, source_id),  NULL, 0,
				       active_send->dev.segment_ptr, &active_send->dev.segment_first, active_send->dev.segment_size);
		/* have we finished sending the whole message? */
		complete = active_send->dev.segment_first == active_send->dev.segment_size;
	    }
	    else
	    {

		dataptr = active_send->dev.iov[0].MPID_IOV_BUF;
		datalen = active_send->dev.iov[0].MPID_IOV_LEN;
		/* send only if there's something left to send */
		if (datalen != 0)
		    send_pkt(VC_FIELD(vc, gm_node_id), VC_FIELD(vc, gm_port_id), VC_FIELD(vc, source_id), &dataptr, &datalen);
		
		active_send->dev.iov[0].MPID_IOV_BUF = dataptr;
		active_send->dev.iov[0].MPID_IOV_LEN = datalen;

		/* have we finished sending the whole message? */
		complete = datalen == 0;
	    }
	    
            if (complete)
            {
                /* finished sending message */
                int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
            
                reqFn = active_send->dev.OnDataAvail;
                if (!reqFn)
                {
                    MPIDI_CH3U_Request_complete(active_send);
                    //MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    active_send = NULL;
                }
                else
                {
		    complete = 0;
                    mpi_errno = reqFn(vc, active_send, &complete);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                    if (complete)
                    {
                        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                        active_send = NULL;
                    }
                }
            }
        }
        
        
        if (!SEND_Q_EMPTY())
        {
            MPID_Request *sreq;
            MPIDI_VC_t *vc;
            
            if (MPID_nem_module_gm_num_send_tokens == 0)
                goto fn_exit;

            SEND_Q_DEQUEUE(&sreq);
            active_send = sreq;            
            vc = sreq->ch.vc;

	    if (active_send->ch.noncontig)
	    {
		send_noncontig_pkt(VC_FIELD(vc, gm_node_id), VC_FIELD(vc, gm_port_id), VC_FIELD(vc, source_id), &sreq->dev.pending_pkt,
				   sreq->ch.header_sz, sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size);
	    }
	    else
	    {
		
		dataptr = sreq->dev.iov[0].MPID_IOV_BUF;
		datalen = sreq->dev.iov[0].MPID_IOV_LEN;
            
		send_header_pkt(VC_FIELD(vc, gm_node_id), VC_FIELD(vc, gm_port_id), VC_FIELD(vc, source_id), &sreq->dev.pending_pkt,
				sreq->ch.header_sz, &dataptr, &datalen);
            
		sreq->dev.iov[0].MPID_IOV_BUF = dataptr;
		sreq->dev.iov[0].MPID_IOV_LEN = datalen;
	    }
	}
    }    
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int dataleft = data_sz;
    char *dataptr = (char *)data;
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_GM_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_GM_ISTARTCONTIGMSG);

    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (SEND_Q_EMPTY() && MPID_nem_module_gm_num_send_tokens)
        /* MT */
    {
        int node_id = VC_FIELD(vc, gm_node_id);
        int port_id = VC_FIELD(vc, gm_port_id);
        int source_id = VC_FIELD(vc, source_id);

        MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "gm_iStartContigMsg");

        /* send first packet with header */
        send_header_pkt(node_id, port_id, source_id, hdr, hdr_sz, &dataptr, &dataleft);

        /* send additional packets of data */
        while (dataleft && MPID_nem_module_gm_num_send_tokens)
        {
            send_pkt(node_id, port_id, source_id, &dataptr, &dataleft);
        }

        if (dataleft == 0)
        {
            /* sent whole message */
            *sreq_ptr = NULL;
            goto fn_exit;
        }
    }

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
    sreq->ch.noncontig = FALSE;
    sreq->ch.header_sz = hdr_sz;
    sreq->dev.iov[0].MPID_IOV_BUF = dataptr;
    sreq->dev.iov[0].MPID_IOV_LEN = dataleft;
    sreq->ch.vc = vc;

    /* if we sent anything, then the queue must have been empty */
    MPIU_Assert(dataleft == data_sz || SEND_Q_EMPTY());

    /* if we started sending this message, make it the active send */
    if (dataleft != data_sz)
        active_send = sreq;
    else
        SEND_Q_ENQUEUE(sreq);
    
    *sreq_ptr = sreq;
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_GM_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                            void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int dataleft = data_sz;
    char *dataptr = (char *)data;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_GM_ISENDCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_GM_ISENDCONTIGMSG);

    if (SEND_Q_EMPTY() && MPID_nem_module_gm_num_send_tokens)
        /* MT */
    {
        sendbuf_t *sb;
        int node_id = VC_FIELD(vc, gm_node_id);
        int port_id = VC_FIELD(vc, gm_port_id);
        int source_id = VC_FIELD(vc, source_id);

        MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "gm_iSendContig");

        /* send first packet with header */
        send_header_pkt(node_id, port_id, source_id, hdr, hdr_sz, &dataptr, &dataleft);

        /* send additional packets of data */
        while (dataleft && MPID_nem_module_gm_num_send_tokens)
        {
            send_pkt(node_id, port_id, source_id, &dataptr, &dataleft);
        }

        if (dataleft == 0)
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
            
            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                goto fn_exit;
            }
            else
            {
                int complete = 0;
                
                mpi_errno = reqFn(vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    goto fn_exit;
                }

                MPIU_Assert(0); /* FIXME:  I don't think we should get here with contig messages */
                
                sreq->ch.vc = vc;
                active_send = sreq;
                goto fn_exit;
            }
        }
    }

    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
    sreq->ch.noncontig = FALSE;
    sreq->ch.header_sz = hdr_sz;
    sreq->dev.iov[0].MPID_IOV_BUF = dataptr;
    sreq->dev.iov[0].MPID_IOV_LEN = dataleft;
    sreq->ch.vc = vc;

    /* if we sent anything, then the queue must have been empty */
    MPIU_Assert(dataleft == data_sz || SEND_Q_EMPTY());

    /* if we started sending this message, make it the active send */
    if (dataleft != data_sz)
        active_send = sreq;
    else
        SEND_Q_ENQUEUE(sreq);
  
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_GM_ISENDCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_gm_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_GM_SENDNONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_GM_SENDNONCONTIG);

    if (SEND_Q_EMPTY() && MPID_nem_module_gm_num_send_tokens)
        /* MT */
    {
        sendbuf_t *sb;
        int node_id = VC_FIELD(vc, gm_node_id);
        int port_id = VC_FIELD(vc, gm_port_id);
        int source_id = VC_FIELD(vc, source_id);

        MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "gm_SendNoncontig");

        /* send first packet with header */
	send_noncontig_pkt(node_id, port_id, source_id, header, hdr_sz,
			   sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size);

        /* send additional packets of data */
        while (sreq->dev.segment_first != sreq->dev.segment_size && MPID_nem_module_gm_num_send_tokens)
        {
            send_noncontig_pkt(node_id, port_id, source_id, NULL, 0, sreq->dev.segment_ptr,
			       &sreq->dev.segment_first, sreq->dev.segment_size);
        }

        if (sreq->dev.segment_first == sreq->dev.segment_size)
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
            
            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                goto fn_exit;
            }
            else
            {
                int complete = 0;
                
                mpi_errno = reqFn(vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    goto fn_exit;
                }

                sreq->ch.vc = vc;
                active_send = sreq;
                goto fn_exit;
            }
        }
    }

    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
    sreq->ch.noncontig = TRUE;
    sreq->ch.header_sz = hdr_sz;
    sreq->ch.vc = vc;

    /* if we sent anything, then the queue must have been empty */
    MPIU_Assert(sreq->dev.segment_first == 0 || SEND_Q_EMPTY());

    /* if we started sending this message, make it the active send */
    if (sreq->dev.segment_first != 0)
        active_send = sreq;
    else
        SEND_Q_ENQUEUE(sreq);
  
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_GM_SENDNONCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


