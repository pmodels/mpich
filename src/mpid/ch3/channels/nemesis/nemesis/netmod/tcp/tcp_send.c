/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_impl.h"

#define NUM_PREALLOC_SENDQ 10
#define MAX_SEND_IOV 10

typedef struct MPID_nem_tcp_send_q_element
{
    struct MPID_nem_tcp_send_q_element *next;
    size_t len;                        /* number of bytes left to send */
    char *start;                       /* pointer to next byte to send */
    MPID_nem_cell_ptr_t cell;
    /*     char buf[MPID_NEM_MAX_PACKET_LEN];*/ /* data to be sent */
} MPID_nem_tcp_send_q_element_t;

static struct {MPID_nem_tcp_send_q_element_t *top;} free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                         \
        if (S_EMPTY (free_buffers))                                                                                                     \
        {                                                                                                                               \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_tcp_send_q_element_t *, sizeof(MPID_nem_tcp_send_q_element_t),      \
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
#define FUNCNAME MPID_nem_tcp_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL (NUM_PREALLOC_SENDQ);
    
    /* preallocate sendq elements */
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        MPID_nem_tcp_send_q_element_t *e;
        
        MPIU_CHKPMEM_MALLOC (e, MPID_nem_tcp_send_q_element_t *,
                             sizeof(MPID_nem_tcp_send_q_element_t), mpi_errno, "send queue element");
        S_PUSH (&free_buffers, e);
    }

    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_queued(MPIDI_VC_t *vc, MPIDI_nem_tcp_request_queue_t *send_queue)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq;
    MPIDI_msg_sz_t offset;
    MPID_IOV *iov;
    int complete;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);

    MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "vc = %p", vc);
    MPIU_Assert(vc != NULL);

    if (MPIDI_CH3I_Sendq_empty(*send_queue))
	goto fn_exit;

    while (!MPIDI_CH3I_Sendq_empty(*send_queue))
    {
        sreq = MPIDI_CH3I_Sendq_head(*send_queue);
        MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "Sending %p", sreq);

        iov = &sreq->dev.iov[sreq->dev.iov_offset];
        
        CHECK_EINTR(offset, writev(vc_tcp->sc->fd, iov, sreq->dev.iov_count));
        if (offset == 0) {
            int req_errno = MPI_SUCCESS;

            MPIU_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
            MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
            mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            goto fn_exit; /* this vc is closed now, just bail out */
        }
        if (offset == -1)
        {
            if (errno == EAGAIN)
            {
                offset = 0;
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "EAGAIN");
                break;
            } else {
                int req_errno = MPI_SUCCESS;
                MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIU_Strerror(errno));
                MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                goto fn_exit; /* this vc is closed now, just bail out */
            }
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write " MPIDI_MSG_SZ_FMT, offset);

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
            /* writev couldn't write the entire iov, give up for now */
            break;
        }
        else
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
                continue;
            }

            complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            if (complete)
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
                continue;
            }
            sreq->dev.iov_offset = 0;
        }
    }

    if (MPIDI_CH3I_Sendq_empty(*send_queue))
        UNSET_PLFD(vc_tcp);
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_send_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    while (!S_EMPTY (free_buffers))
    {
        MPID_nem_tcp_send_q_element_t *e;
        S_POP (&free_buffers, &e);
        MPIU_Free (e);
    }
    return mpi_errno;
}

/* MPID_nem_tcp_conn_est -- this function is called when the
   connection is finally established to send any pending sends */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_conn_est
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_conn_est (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CONN_EST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CONN_EST);

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if (!MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
    {
        SET_PLFD(vc_tcp);
        mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CONN_EST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                 MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_msg_sz_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "tcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                MPID_IOV iov[2];
                
                iov[0].MPID_IOV_BUF = hdr;
                iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
                iov[1].MPID_IOV_BUF = data;
                iov[1].MPID_IOV_LEN = data_sz;
                
                CHECK_EINTR(offset, writev(sc->fd, iov, 2));
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIU_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIU_Strerror(errno));
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write " MPIDI_MSG_SZ_FMT, offset);
                
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
            /* state may be DISCONNECTED or ERROR.  Calling tcp_connect in an ERROR state will return an
               appropriate error code. */
            mpi_errno = MPID_nem_tcp_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

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

    if (MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        MPIDI_CH3I_Sendq_enqueue(&vc_tcp->paused_send_queue, sreq);
    } else {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp)) {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
                /* this will be the first send on the queue: queue it and set the write flag on the pollfd */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                SET_PLFD(vc_tcp);
            } else {
                /* there are other sends in the queue before this one: try to send from the queue */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
    *sreq_ptr = sreq;
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This sends the message even if the vc is in a paused state */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iStartContigMsg_paused
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_iStartContigMsg_paused(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                        MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_msg_sz_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "tcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (MPID_nem_tcp_vc_is_connected(vc_tcp))
    {
        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
        {
            MPID_IOV iov[2];
                
            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[1].MPID_IOV_LEN = data_sz;
                
            CHECK_EINTR(offset, writev(sc->fd, iov, 2));
            if (offset == 0) {
                int req_errno = MPI_SUCCESS;

                MPIU_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                goto fn_fail;
            }
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else {
                    int req_errno = MPI_SUCCESS;
                    MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIU_Strerror(errno));
                    MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);

                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
            }
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write " MPIDI_MSG_SZ_FMT, offset);
                
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
        /* state may be DISCONNECTED or ERROR.  Calling tcp_connect in an ERROR state will return an
           appropriate error code. */
        mpi_errno = MPID_nem_tcp_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

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

    if (MPID_nem_tcp_vc_is_connected(vc_tcp)) {
        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
            /* this will be the first send on the queue: queue it and set the write flag on the pollfd */
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
            SET_PLFD(vc_tcp);
        } else {
            /* there are other sends in the queue before this one: try to send from the queue */
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
            mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    } else {
        MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
    }
    
    *sreq_ptr = sreq;
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                             void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "tcp_iSendContig");

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                MPID_IOV iov[2];
                
                iov[0].MPID_IOV_BUF = hdr;
                iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
                iov[1].MPID_IOV_BUF = data;
                iov[1].MPID_IOV_LEN = data_sz;
                
                CHECK_EINTR(offset, writev(sc->fd, iov, 2));
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIU_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIU_Strerror(errno));
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write " MPIDI_MSG_SZ_FMT, offset);
                
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
        }
        else
        {
            /* state may be DISCONNECTED or ERROR.  Calling tcp_connect in an ERROR state will return an
               appropriate error code. */
            mpi_errno = MPID_nem_tcp_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
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

    if (MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        MPIDI_CH3I_Sendq_enqueue(&vc_tcp->paused_send_queue, sreq);
    } else {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp)) {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
                /* this will be the first send on the queue: queue it and set the write flag on the pollfd */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                SET_PLFD(vc_tcp);
            } else {
                /* there are other sends in the queue before this one: try to send from the queue */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_SendNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_IOV *iov_p;
    MPIDI_msg_sz_t offset;
    int complete;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "tcp_SendNoncontig");
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));
    
    iov[0].MPID_IOV_BUF = header;
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    iov_n = MPID_IOV_LIMIT - 1;

    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");

    iov_n += 1;
    offset = 0;

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                CHECK_EINTR(offset, writev(vc_tcp->sc->fd, iov, iov_n));
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIU_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIU_Strerror(errno));
                        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write noncontig " MPIDI_MSG_SZ_FMT, offset);
            }
        }
        else
        {
            /* state may be DISCONNECTED or ERROR.  Calling tcp_connect in an ERROR state will return an
               appropriate error code. */
            mpi_errno = MPID_nem_tcp_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    
    if (offset < iov[0].MPID_IOV_LEN)
    {
        /* header was not yet sent, save it in req */
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.pending_pkt;
        iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
    }

    /* check if whole iov was sent, and save any unsent portion of iov */
    sreq->dev.iov_count = 0;
    complete = 1;
    for (iov_p = &iov[0]; iov_p < &iov[iov_n]; ++iov_p)
    {
        if (offset < iov_p->MPID_IOV_LEN)
        {
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *)iov_p->MPID_IOV_BUF + offset);
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_LEN = iov_p->MPID_IOV_LEN - offset;
            offset = 0;
            ++sreq->dev.iov_count;
            complete = 0;
        }
        else
            offset -= iov_p->MPID_IOV_LEN;
    }
        
    if (complete)
    {
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
        
    if (MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        MPIDI_CH3I_Sendq_enqueue(&vc_tcp->paused_send_queue, sreq);
    } else {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp)) {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
                /* this will be the first send on the queue: queue it and set the write flag on the pollfd */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                SET_PLFD(vc_tcp);
            } else {
                /* there are other sends in the queue before this one: try to send from the queue */
                MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
                mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    return mpi_errno;
fn_fail:
    MPIU_Object_set_ref(sreq, 0);
    MPIDI_CH3_Request_destroy(sreq);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_error_out_send_queue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_tcp_error_out_send_queue(struct MPIDI_VC *const vc, int req_errno)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req;
    MPID_nem_tcp_vc_area *const vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);

    /* we don't call onDataAvail or onFinal handlers because this is
       an error condition and we just want to mark them as complete */

    /* send queue */
    while (!MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
        MPIDI_CH3I_Sendq_dequeue(&vc_tcp->send_queue, &req);
        req->status.MPI_ERROR = req_errno;

        MPIDI_CH3U_Request_complete(req);
    }

    /* paused send queue */
    while (!MPIDI_CH3I_Sendq_empty(vc_tcp->paused_send_queue)) {
        MPIDI_CH3I_Sendq_dequeue(&vc_tcp->paused_send_queue, &req);
        req->status.MPI_ERROR = req_errno;

        MPIDI_CH3U_Request_complete(req);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
