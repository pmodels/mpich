/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"

#define NUM_PREALLOC_SENDQ 10
#define MAX_SEND_IOV 10

#define SENDQ_EMPTY(q) GENERIC_Q_EMPTY (q)
#define SENDQ_HEAD(q) GENERIC_Q_HEAD (q)
#define SENDQ_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, dev.next)
#define SENDQ_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, dev.next)


typedef struct MPID_nem_newtcp_module_send_q_element
{
    struct MPID_nem_newtcp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to send */
    char *start;                       /* pointer to next byte to send */
    MPID_nem_cell_ptr_t cell;
    /*     char buf[MPID_NEM_MAX_PACKET_LEN];*/ /* data to be sent */
} MPID_nem_newtcp_module_send_q_element_t;

static struct {MPID_nem_newtcp_module_send_q_element_t *top;} free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                         \
        if (S_EMPTY (free_buffers))                                                                                                     \
        {                                                                                                                               \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_newtcp_module_send_q_element_t *, sizeof(MPID_nem_newtcp_module_send_q_element_t),      \
                                 mpi_errno, "send queue element");                                                                      \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            S_POP (&free_buffers, e);                                                                                                   \
        }                                                                                                                               \
    } while (0)

/* FREE_Q_ELEMENTS() frees a list if elements starting at e0 through e1 */
#define FREE_Q_ELEMENTS(e0, e1) S_PUSH_MULTIPLE (&free_buffers, e0, e1)
#define FREE_Q_ELEMENT(e) S_PUSH (&free_buffers, e)

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL (NUM_PREALLOC_SENDQ);
    
    /* preallocate sendq elements */
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        
        MPIU_CHKPMEM_MALLOC (e, MPID_nem_newtcp_module_send_q_element_t *,
                             sizeof(MPID_nem_newtcp_module_send_q_element_t), mpi_errno, "send queue element");
        S_PUSH (&free_buffers, e);
    }

    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

/* Handle the request at the head of vc's sendq */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_handle_sendq_head_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_handle_sendq_head_req(MPIDI_VC_t *vc, int *req_complete)
{
    int (*req_handler)(MPIDI_VC_t *, MPID_Request *, int *);
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_HANDLE_SENDQ_HEAD_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_HANDLE_SENDQ_HEAD_REQ);

    MPIU_Assert(req_complete != NULL);

    MPIU_Assert(!SENDQ_EMPTY(VC_FIELD(vc, send_queue)));
    req = SENDQ_HEAD(VC_FIELD(vc, send_queue));
    MPIU_Assert(req != NULL);

    req_handler = req->dev.OnDataAvail;
    if (!req_handler){
        MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);
        MPIDI_CH3U_Request_complete(req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        SENDQ_DEQUEUE(&VC_FIELD(vc, send_queue), &req);
        *req_complete = 1;
    }
    else{
        *req_complete = 0;
        mpi_errno = req_handler(vc, req, req_complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (*req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            SENDQ_DEQUEUE(&VC_FIELD(vc, send_queue), &req);
        }
    }
    req->dev.iov_offset = 0;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_HANDLE_SENDQ_HEAD_REQ);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_queued (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq;
    MPIDI_msg_sz_t offset;
    MPID_IOV *iov;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);

    MPIU_Assert(vc != NULL);

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
	goto fn_exit;

    while (!SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
    {
        sreq = SENDQ_HEAD(VC_FIELD(vc, send_queue));
        
        iov = &sreq->dev.iov[sreq->dev.iov_offset];

/*         printf("sreq = %p sreq->dev.iov = %p iov = %p\n", sreq, sreq->dev.iov, iov); */
/*         printf("iov[0].MPID_IOV_BUF = %p iov[0].MPID_IOV_LEN = %d iov_count = %d\n", iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN, sreq->dev.iov_count);//DARIUS */
/*         printf("&iov[0].MPID_IOV_LEN = %p sreq->dev.iov_offset = %d\n", &iov[0].MPID_IOV_LEN, sreq->dev.iov_offset);//DARIUS */
        
        MPIU_OSW_RETRYON_INTR((offset < 0), 
            (mpi_errno = MPIU_SOCKW_Writev(VC_FIELD(vc, sc)->fd, iov, sreq->dev.iov_count, &offset)));
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        if(offset < 0){
            offset = 0;
        }

        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

        complete = 1;
        for (iov = &sreq->dev.iov[sreq->dev.iov_offset]; iov < &sreq->dev.iov[sreq->dev.iov_offset + sreq->dev.iov_count]; ++iov)
        {
            if (offset < iov->MPID_IOV_LEN)
            {
                iov->MPID_IOV_BUF = (char *)iov->MPID_IOV_BUF + offset;
                iov->MPID_IOV_LEN -= offset;
                /* iov_count should be equal to the number of iov's remaining */
                sreq->dev.iov_count -= ((iov - sreq->dev.iov) - sreq->dev.iov_offset);
                sreq->dev.iov_offset = iov - sreq->dev.iov;
                complete = 0;
                break;
            }
            offset -= iov->MPID_IOV_LEN;
        }
        if (!complete)
        {
            MPID_IOV *iov = &sreq->dev.iov[sreq->dev.iov_offset];
            int n_iov = sreq->dev.iov_count;
            /* writev couldn't write the entire iov,
             * post a write for the rem data and exit */
            mpi_errno = MPID_nem_newtcp_module_post_writev_ex(VC_FIELD(vc, sc), iov, n_iov);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            break;
        }
        else
        {
            /* sent whole message */
            complete = 0;
            mpi_errno = MPID_nem_newtcp_module_handle_sendq_head_req(vc, &complete);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    while (!S_EMPTY (free_buffers))
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        S_POP (&free_buffers, &e);
        MPIU_Free (e);
    }
    return mpi_errno;
}

/* MPID_nem_newtcp_module_conn_est -- this function is called when the
 * connection is finally extablished to send any pending sends 
 *  & post the first recv
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_conn_est
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_conn_est (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if (!SENDQ_EMPTY (VC_FIELD(vc, send_queue)))
    {
        mpi_errno = MPID_nem_newtcp_module_send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

    /* Post a read on the module scrap recv buf */
    mpi_errno = MPID_nem_newtcp_module_post_read_ex(VC_FIELD(vc, sc),
                        VC_FIELD(vc, sc)->tmp_buf, VC_FIELD(vc, sc)->tmp_buf_len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        
 fn_fail:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[1].MPID_IOV_LEN = data_sz;
        
            MPIU_OSW_RETRYON_INTR((offset < 0),
                (mpi_errno = MPIU_SOCKW_Writev(VC_FIELD(vc, sc)->fd, iov, 2, &offset)));
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            if(offset < 0){
                offset = 0;
            }

            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

            if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
            {
                /* sent whole message */
                *sreq_ptr = NULL;
                goto fn_exit;
            }
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Data is partially written OR vc is still connecting...
     * create and enqueue request
     */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

/*     printf("&sreq->dev.pending_pkt = %p sizeof(MPIDI_CH3_PktGeneric_t) = %d\n", &sreq->dev.pending_pkt, sizeof(MPIDI_CH3_PktGeneric_t));//DARIUS */
/*     printf("offset = %d\n", offset);//DARIUS */

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }
    
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

/*     printf("sreq = %p sreq->dev.iov = %p\n", sreq, sreq->dev.iov); */
/*     printf("sreq->dev.iov[0].MPID_IOV_BUF = %p\n", sreq->dev.iov[0].MPID_IOV_BUF);//DARIUS */
/*     printf("sreq->dev.iov[0].MPID_IOV_LEN = %d\n", sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */
/*     printf("&sreq->dev.iov[0].MPID_IOV_LEN = %p\n", &sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)) && MPID_nem_newtcp_module_vc_is_connected(vc)){
        /* Data was partially written... Post a write for the rem data */
        mpi_errno = MPID_nem_newtcp_module_post_writev_ex(VC_FIELD(vc, sc),
                        sreq->dev.iov, sreq->dev.iov_count);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    
    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);

    *sreq_ptr = sreq;
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iSendContig");

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];
            int iov_cnt;

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            if(data_sz > 0){
                iov[1].MPID_IOV_BUF = data;
                iov[1].MPID_IOV_LEN = data_sz;
                iov_cnt = 2;
            }
            else{
                iov_cnt = 1;
            }
        
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "writing hdr (sz = %d) and data (sz = %d)", sizeof(MPIDI_CH3_PktGeneric_t), data_sz));

            MPIU_OSW_RETRYON_INTR((offset < 0),
                (mpi_errno = MPIU_SOCKW_Writev(VC_FIELD(vc, sc)->fd, iov, iov_cnt, &offset)));
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            if(offset < 0){
                offset = 0;
            }

            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

            if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
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

                    /* not completed: more to send */
                    goto enqueue_request;
                }
            }
        }
        /* If sendq is not empty we default to enqueueing the request */
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* save iov */
    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

 enqueue_request:
    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)) && MPID_nem_newtcp_module_vc_is_connected(vc)){
        /* Post a write for the iov */
        mpi_errno = MPID_nem_newtcp_module_post_writev_ex(VC_FIELD(vc, sc),
                        sreq->dev.iov, sreq->dev.iov_count);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_IOV *iov_p;
    MPIDI_msg_sz_t offset;
    int complete;

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_SendNoncontig");
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));
    
    iov[0].MPID_IOV_BUF = header;
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    iov_n = MPID_IOV_LIMIT - 1;
    /* On the initial load of a send iov req, set the OnFinal action (null
       for point-to-point) */
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");

    iov_n += 1;
    offset = 0;

    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPIU_OSW_RETRYON_INTR((offset < 0),
                (mpi_errno = MPIU_SOCKW_Writev(VC_FIELD(vc, sc)->fd, iov, iov_n, &offset)));
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            if(offset < 0){
                offset = 0;
            }

            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write noncontig %d", offset);
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    if (offset < iov[0].MPID_IOV_LEN)
    {
        /* header was not yet sent, save it in req */
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
        iov[0].MPID_IOV_BUF = &sreq->dev.pending_pkt;
        iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
    }

    /* check if whole iov was sent, and save any unsent portion of iov */
    sreq->dev.iov_count = 0;
    complete = 1;
    for (iov_p = &iov[0]; iov_p < &iov[iov_n]; ++iov_p)
    {
        if (offset < iov_p->MPID_IOV_LEN)
        {
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_BUF = (char *)iov_p->MPID_IOV_BUF + offset;
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_LEN = iov_p->MPID_IOV_LEN - offset;
            offset = 0;
            ++sreq->dev.iov_count;
            complete = 0;
        }
        else
            offset -= iov_p->MPID_IOV_LEN;
    }
        
    if (complete){
        /* sent whole iov */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn)
        {
            MPIDI_CH3U_Request_complete(sreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }

        complete = 0;
        mpi_errno = reqFn(vc, sreq, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (complete)
        {
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }
    }
        
    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);
        
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;
        
    if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)) && MPID_nem_newtcp_module_vc_is_connected(vc)){
        /* Post a write for the iov */
        mpi_errno = MPID_nem_newtcp_module_post_writev_ex(VC_FIELD(vc, sc),
            sreq->dev.iov, sreq->dev.iov_count);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
 
    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);
 
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_Object_set_ref(sreq, 0);
    MPIDI_CH3_Request_destroy(sreq);
    goto fn_exit;
}
