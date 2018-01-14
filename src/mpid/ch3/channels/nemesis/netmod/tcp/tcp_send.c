/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
            MPIR_CHKPMEM_MALLOC (*(e), MPID_nem_tcp_send_q_element_t *, sizeof(MPID_nem_tcp_send_q_element_t),      \
                                 mpi_errno, "send queue element", MPL_MEM_BUFFER);                                                      \
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_CHKPMEM_DECL (NUM_PREALLOC_SENDQ);
    
    /* preallocate sendq elements */
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        MPID_nem_tcp_send_q_element_t *e;
        
        MPIR_CHKPMEM_MALLOC (e, MPID_nem_tcp_send_q_element_t *,
                             sizeof(MPID_nem_tcp_send_q_element_t), mpi_errno, "send queue element", MPL_MEM_BUFFER);
        S_PUSH (&free_buffers, e);
    }

    MPIR_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_send_queued
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_queued(MPIDI_VC_t *vc, MPIDI_nem_tcp_request_queue_t *send_queue)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    intptr_t offset;
    MPL_IOV *iov;
    int complete;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);

    MPL_DBG_MSG_P(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "vc = %p", vc);
    MPIR_Assert(vc != NULL);

    if (MPIDI_CH3I_Sendq_empty(*send_queue))
	goto fn_exit;

    while (!MPIDI_CH3I_Sendq_empty(*send_queue))
    {
        sreq = MPIDI_CH3I_Sendq_head(*send_queue);
        MPL_DBG_MSG_P(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "Sending %p", sreq);

        iov = &sreq->dev.iov[sreq->dev.iov_offset];
        
        offset = MPL_large_writev(vc_tcp->sc->fd, iov, sreq->dev.iov_count);
        if (offset == 0) {
            int req_errno = MPI_SUCCESS;

            MPIR_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
            MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
            mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            goto fn_exit; /* this vc is closed now, just bail out */
        }
        if (offset == -1)
        {
            if (errno == EAGAIN)
            {
                offset = 0;
                MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "EAGAIN");
                break;
            } else {
                int req_errno = MPI_SUCCESS;
                MPIR_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIR_Strerror(errno));
                MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                goto fn_exit; /* this vc is closed now, just bail out */
            }
        }
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "write %" PRIdPTR, offset);

        complete = 1;
        for (iov = &sreq->dev.iov[sreq->dev.iov_offset]; iov < &sreq->dev.iov[sreq->dev.iov_offset + sreq->dev.iov_count]; ++iov)
        {
            if (offset < iov->MPL_IOV_LEN)
            {
                iov->MPL_IOV_BUF = (char *)iov->MPL_IOV_BUF + offset;
                iov->MPL_IOV_LEN -= offset;
                /* iov_count should be equal to the number of iov's remaining */
                sreq->dev.iov_count -= ((iov - sreq->dev.iov) - sreq->dev.iov_offset);
                sreq->dev.iov_offset = iov - sreq->dev.iov;
                complete = 0;
                break;
            }
            offset -= iov->MPL_IOV_LEN;
        }
        if (!complete)
        {
            /* writev couldn't write the entire iov, give up for now */
            break;
        }
        else
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIR_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                mpi_errno = MPID_Request_complete(sreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
                MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
                MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
                continue;
            }

            complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
            if (complete)
            {
                MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
                MPIDI_CH3I_Sendq_dequeue(send_queue, &sreq);
                continue;
            }
            sreq->dev.iov_offset = 0;
        }
    }

    if (MPIDI_CH3I_Sendq_empty(*send_queue))
        UNSET_PLFD(vc_tcp);
    
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_SEND_QUEUED);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_send_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_send_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    while (!S_EMPTY (free_buffers))
    {
        MPID_nem_tcp_send_q_element_t *e;
        S_POP (&free_buffers, &e);
        MPL_free (e);
    }
    return mpi_errno;
}

/* MPID_nem_tcp_conn_est -- this function is called when the
   connection is finally established to send any pending sends */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_conn_est
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_conn_est (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CONN_EST);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_CONN_EST);

    /* only update VC state when it is not being closed.
     * Note that we still need change state here if the VC is passively
     * connected (i.e., server in dynamic process connection) */
    if (vc->state == MPIDI_VC_STATE_INACTIVE)
        MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if (!MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
    {
        SET_PLFD(vc_tcp);
        mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

 fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_CONN_EST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iStartContigMsg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, intptr_t hdr_sz, void *data, intptr_t data_sz,
                                 MPIR_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request * sreq = NULL;
    intptr_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);
    
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "tcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                MPL_IOV iov[2];
                
                iov[0].MPL_IOV_BUF = hdr;
                iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
                iov[1].MPL_IOV_BUF = data;
                iov[1].MPL_IOV_LEN = data_sz;
                
                offset = MPL_large_writev(sc->fd, iov, 2);
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIR_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIR_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIR_Strerror(errno));
                        MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "write %" PRIdPTR, offset);
                
                if (offset == sizeof(MPIDI_CH3_Pkt_t) + data_sz)
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
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    /* create and enqueue request */
    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_Assert (sreq != NULL);
    MPIR_Object_set_ref (sreq, 2);

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    if (offset < sizeof(MPIDI_CH3_Pkt_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)hdr;
        sreq->dev.iov[0].MPL_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPL_IOV_BUF = data;
            sreq->dev.iov[1].MPL_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPL_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_Pkt_t));
        sreq->dev.iov[0].MPL_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_Pkt_t));
        sreq->dev.iov_count = 1;
    }
    
    MPIR_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPL_IOV_LEN > 0);

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
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
    *sreq_ptr = sreq;
    
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This sends the message even if the vc is in a paused state */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iStartContigMsg_paused
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_iStartContigMsg_paused(MPIDI_VC_t *vc, void *hdr, intptr_t hdr_sz, void *data, intptr_t data_sz,
                                        MPIR_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request * sreq = NULL;
    intptr_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);
    
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "tcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (MPID_nem_tcp_vc_is_connected(vc_tcp))
    {
        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
        {
            MPL_IOV iov[2];
                
            iov[0].MPL_IOV_BUF = hdr;
            iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
            iov[1].MPL_IOV_BUF = data;
            iov[1].MPL_IOV_LEN = data_sz;
                
            offset = MPL_large_writev(sc->fd, iov, 2);
            if (offset == 0) {
                int req_errno = MPI_SUCCESS;

                MPIR_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                goto fn_fail;
            }
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else {
                    int req_errno = MPI_SUCCESS;
                    MPIR_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIR_Strerror(errno));
                    MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);

                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
            }
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "write %" PRIdPTR, offset);
                
            if (offset == sizeof(MPIDI_CH3_Pkt_t) + data_sz)
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
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* create and enqueue request */
    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_Assert (sreq != NULL);
    MPIR_Object_set_ref (sreq, 2);

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    if (offset < sizeof(MPIDI_CH3_Pkt_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)hdr;
        sreq->dev.iov[0].MPL_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPL_IOV_BUF = data;
            sreq->dev.iov[1].MPL_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPL_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_Pkt_t));
        sreq->dev.iov[0].MPL_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_Pkt_t));
        sreq->dev.iov_count = 1;
    }
    
    MPIR_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPL_IOV_LEN > 0);

    if (MPID_nem_tcp_vc_is_connected(vc_tcp)) {
        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
            /* this will be the first send on the queue: queue it and set the write flag on the pollfd */
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
            SET_PLFD(vc_tcp);
        } else {
            /* there are other sends in the queue before this one: try to send from the queue */
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
            mpi_errno = MPID_nem_tcp_send_queued(vc, &vc_tcp->send_queue);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    } else {
        MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
    }
    
    *sreq_ptr = sreq;
    
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_ISTARTCONTIGMSG_PAUSED);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_iSendContig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_iSendContig(MPIDI_VC_t *vc, MPIR_Request *sreq, void *hdr, intptr_t hdr_sz,
                             void *data, intptr_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    intptr_t offset = 0;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    sockconn_t *sc = vc_tcp->sc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);
    
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "tcp_iSendContig");

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                MPL_IOV iov[3];
                int iov_n = 0;

                iov[iov_n].MPL_IOV_BUF = hdr;
                iov[iov_n].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
                iov_n++;

                if (sreq->dev.ext_hdr_sz != 0) {
                    iov[iov_n].MPL_IOV_BUF = sreq->dev.ext_hdr_ptr;
                    iov[iov_n].MPL_IOV_LEN = sreq->dev.ext_hdr_sz;
                    iov_n++;
                }

                iov[iov_n].MPL_IOV_BUF = data;
                iov[iov_n].MPL_IOV_LEN = data_sz;
                iov_n++;
                
                offset = MPL_large_writev(sc->fd, iov, iov_n);
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIR_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIR_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIR_Strerror(errno));
                        MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "write %" PRIdPTR, offset);
                
                if (offset == sizeof(MPIDI_CH3_Pkt_t) + sreq->dev.ext_hdr_sz + data_sz)
                {
                    /* sent whole message */
                    int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
                    
                    reqFn = sreq->dev.OnDataAvail;
                    if (!reqFn)
                    {
                        MPIR_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                        mpi_errno = MPID_Request_complete(sreq);
                        if (mpi_errno != MPI_SUCCESS) {
                            MPIR_ERR_POP(mpi_errno);
                        }
                        MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
                        goto fn_exit;
                    }
                    else
                    {
                        int complete = 0;
                        
                        mpi_errno = reqFn(vc, sreq, &complete);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        
                        if (complete)
                        {
                            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
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
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }


    /* save iov */
    sreq->dev.iov_count = 0;
    if (offset < sizeof(MPIDI_CH3_Pkt_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)hdr;

        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t) - offset;
        sreq->dev.iov_count++;

        if (sreq->dev.ext_hdr_sz > 0) {
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = sreq->dev.ext_hdr_ptr;
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = sreq->dev.ext_hdr_sz;
            sreq->dev.iov_count++;
        }

        if (data_sz)
        {
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = data;
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = data_sz;
            sreq->dev.iov_count++;
        }
    }
    else if (offset < sizeof(MPIDI_CH3_Pkt_t) + sreq->dev.ext_hdr_sz) {
        MPIR_Assert(sreq->dev.ext_hdr_sz > 0);
        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = sreq->dev.ext_hdr_ptr;
        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = sreq->dev.ext_hdr_sz;
        sreq->dev.iov_count++;

        if (data_sz) {
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = data;
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = data_sz;
            sreq->dev.iov_count++;
        }
    }
    else
    {
        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_Pkt_t) - sreq->dev.ext_hdr_sz);
        sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_Pkt_t) - sreq->dev.ext_hdr_sz);
        sreq->dev.iov_count++;
    }

enqueue_request:
    /* enqueue request */
    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "enqueuing");
    MPIR_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPL_IOV_LEN > 0);

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
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_ISENDCONTIGMSG);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_SendNoncontig
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_SendNoncontig(MPIDI_VC_t *vc, MPIR_Request *sreq, void *header, intptr_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPL_IOV iov[MPL_IOV_LIMIT];
    MPL_IOV *iov_p;
    intptr_t offset;
    int complete;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    int iov_offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "tcp_SendNoncontig");
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    iov_n = 0;

    iov[iov_n].MPL_IOV_BUF = header;
    iov[iov_n].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    iov_n++;

    if (sreq->dev.ext_hdr_ptr != NULL) {
        iov[iov_n].MPL_IOV_BUF = sreq->dev.ext_hdr_ptr;
        iov[iov_n].MPL_IOV_LEN = sreq->dev.ext_hdr_sz;
        iov_n++;
    }

    iov_offset = iov_n;

    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[iov_offset], &iov_n);
    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");

    iov_n += iov_offset;
    offset = 0;

    if (!MPID_nem_tcp_vc_send_paused(vc_tcp)) {
        if (MPID_nem_tcp_vc_is_connected(vc_tcp))
        {
            if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue))
            {
                offset = MPL_large_writev(vc_tcp->sc->fd, iov, iov_n);
                if (offset == 0) {
                    int req_errno = MPI_SUCCESS;

                    MPIR_ERR_SET(req_errno, MPI_ERR_OTHER, "**sock_closed");
                    MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    goto fn_fail;
                }
                if (offset == -1)
                {
                    if (errno == EAGAIN)
                        offset = 0;
                    else {
                        int req_errno = MPI_SUCCESS;
                        MPIR_ERR_SET1(req_errno, MPI_ERR_OTHER, "**writev", "**writev %s", MPIR_Strerror(errno));
                        MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                        mpi_errno = MPID_nem_tcp_cleanup_on_error(vc, req_errno);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        goto fn_fail;
                    }
                }
                
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "write noncontig %" PRIdPTR, offset);
            }
        }
        else
        {
            /* state may be DISCONNECTED or ERROR.  Calling tcp_connect in an ERROR state will return an
               appropriate error code. */
            mpi_errno = MPID_nem_tcp_connect(vc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }
    
    if (offset < iov[0].MPL_IOV_LEN)
    {
        /* header was not yet sent, save it in req */
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)header;
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)&sreq->dev.pending_pkt;
        iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    }

    /* check if whole iov was sent, and save any unsent portion of iov */
    sreq->dev.iov_count = 0;
    complete = 1;
    for (iov_p = &iov[0]; iov_p < &iov[iov_n]; ++iov_p)
    {
        if (offset < iov_p->MPL_IOV_LEN)
        {
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)((char *)iov_p->MPL_IOV_BUF + offset);
            sreq->dev.iov[sreq->dev.iov_count].MPL_IOV_LEN = iov_p->MPL_IOV_LEN - offset;
            offset = 0;
            ++sreq->dev.iov_count;
            complete = 0;
        }
        else
            offset -= iov_p->MPL_IOV_LEN;
    }
        
    if (complete)
    {
        /* sent whole iov */
        int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);

        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn)
        {
            mpi_errno = MPID_Request_complete(sreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }

        complete = 0;
        mpi_errno = reqFn(vc, sreq, &complete);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
        if (complete)
        {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }
    }
        
    /* enqueue request */
    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "enqueuing");
    MPIR_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPL_IOV_LEN > 0);
        
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
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        } else {
            MPIDI_CH3I_Sendq_enqueue(&vc_tcp->send_queue, sreq);
        }
    }
    
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_SENDNONCONTIG);
    return mpi_errno;
fn_fail:
    MPIR_Request_free(sreq);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_error_out_send_queue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_error_out_send_queue(struct MPIDI_VC *const vc, int req_errno)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPID_nem_tcp_vc_area *const vc_tcp = VC_TCP(vc);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);

    /* we don't call onDataAvail or onFinal handlers because this is
       an error condition and we just want to mark them as complete */

    /* send queue */
    while (!MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
        MPIDI_CH3I_Sendq_dequeue(&vc_tcp->send_queue, &req);
        req->status.MPI_ERROR = req_errno;

        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    /* paused send queue */
    while (!MPIDI_CH3I_Sendq_empty(vc_tcp->paused_send_queue)) {
        MPIDI_CH3I_Sendq_dequeue(&vc_tcp->paused_send_queue, &req);
        req->status.MPI_ERROR = req_errno;

        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_TCP_ERROR_OUT_SEND_QUEUE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
