/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*#include "mpidpre.h"*/
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif


#define PKTARRAY_SIZE (MPIDI_NEM_PKT_END+1)
static MPIDI_CH3_PktHandler_Fcn *pktArray[PKTARRAY_SIZE];

#ifndef MPIDI_POSTED_RECV_ENQUEUE_HOOK
#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(x) do{}while(0)
#endif
#ifndef MPIDI_POSTED_RECV_DEQUEUE_HOOK
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(x) 0
#endif

#ifdef BY_PASS_PROGRESS
extern MPID_Request ** const MPID_Recvq_posted_head_ptr;
extern MPID_Request ** const MPID_Recvq_unexpected_head_ptr;
extern MPID_Request ** const MPID_Recvq_posted_tail_ptr;
extern MPID_Request ** const MPID_Recvq_unexpected_tail_ptr;
#endif

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

/* NEMESIS MULTITHREADING: Extra Data Structures Added */
#ifdef MPICH_IS_THREADED
volatile int MPIDI_CH3I_progress_blocked = FALSE;
volatile int MPIDI_CH3I_progress_wakeup_signalled = FALSE;
static MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond;
static int MPIDI_CH3I_Progress_delay(unsigned int completion_count);
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count);
#endif /* MPICH_IS_THREADED */
/* NEMESIS MULTITHREADING - End block*/

struct MPID_Request *MPIDI_CH3I_sendq_head[CH3_NUM_QUEUES] = {0};
struct MPID_Request *MPIDI_CH3I_sendq_tail[CH3_NUM_QUEUES] = {0};
struct MPID_Request *MPIDI_CH3I_active_send[CH3_NUM_QUEUES] = {0};

static int pkt_NETMOD_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);

int (*MPID_nem_local_lmt_progress)(void) = NULL;
int MPID_nem_local_lmt_pending = FALSE;

/* qn_ent and friends are used to keep a list of notification
   callbacks for posted and matched anysources */
typedef struct qn_ent
{
    struct qn_ent *next;
    void (*enqueue_fn)(MPID_Request *rreq);
    int (*dequeue_fn)(MPID_Request *rreq);
} qn_ent_t;

static qn_ent_t *qn_head = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress (MPID_Progress_state *progress_state, int is_blocking)
{
    unsigned completions = MPIDI_CH3I_progress_completion_count;
    int mpi_errno = MPI_SUCCESS;
    int complete;
#if !defined(ENABLE_NO_YIELD) || defined(MPICH_IS_THREADED)
    int pollcount = 0;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);


    MPIU_DBG_MSG_D(CH3_PROGRESS,VERBOSE,"before outer while loop, completions=%d",completions);

#ifdef ENABLE_CHECKPOINTING
    if (MPIDI_nem_ckpt_start_checkpoint) {
        MPIDI_nem_ckpt_start_checkpoint = FALSE;
        mpi_errno = MPIDI_nem_ckpt_start();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (MPIDI_nem_ckpt_finish_checkpoint) {
        MPIDI_nem_ckpt_finish_checkpoint = FALSE;
        mpi_errno = MPIDI_nem_ckpt_finish();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
#endif
    
    do
    {
	MPID_Request        *sreq;
	MPID_Request        *rreq;
	MPID_nem_cell_ptr_t  cell;
	int                  in_fbox = 0;
	MPIDI_VC_t          *vc;

#ifdef MPICH_IS_THREADED
        MPIU_THREAD_CHECK_BEGIN;
        {
	    /* In the case of threads, we poll for lesser number of
	     * iterations than the case with only processes, as
	     * threads contend for CPU and the lock, while processes
	     * only contend for the CPU. */
            if (pollcount >= MPID_NEM_THREAD_POLLS_BEFORE_YIELD)
            {
                pollcount = 0;
                MPIDI_CH3I_progress_blocked = TRUE;
                MPIU_THREAD_CS_YIELD(ALLFUNC,);
                MPIDI_CH3I_progress_blocked = FALSE;
                MPIDI_CH3I_progress_wakeup_signalled = FALSE;
            }
            ++pollcount;
        }
        MPIU_THREAD_CHECK_END;
#elif !defined(ENABLE_NO_YIELD)
        if (pollcount >= MPID_NEM_POLLS_BEFORE_YIELD)
        {
            pollcount = 0;
            MPIU_PW_Sched_yield();
        }
        ++pollcount;
#endif
        do /* receive progress */
        {

#ifdef MPICH_IS_THREADED
            MPIU_THREAD_CHECK_BEGIN;
            {
                if (MPIDI_CH3I_progress_blocked == TRUE)
                {
                    /* another thread is already blocking in the progress engine.*/
                    break; /* break out of receive block */
                }
            }
            MPIU_THREAD_CHECK_END;
#endif

            /* make progress receiving */
            /* check queue */
            if (!MPID_nem_local_lmt_pending && !MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE]
                && !MPIDI_CH3I_SendQ_head(CH3_NORMAL_QUEUE) && is_blocking
#ifdef MPICH_IS_THREADED
#ifdef HAVE_RUNTIME_THREADCHECK
                && !MPIR_ThreadInfo.isThreaded
#else
                && 0
#endif
#endif
                )
            {
                mpi_errno = MPID_nem_mpich2_blocking_recv(&cell, &in_fbox);
            }
            else
            {
                mpi_errno = MPID_nem_mpich2_test_recv(&cell, &in_fbox, is_blocking);
            }
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            if (cell)
            {
                char            *cell_buf    = (char *)cell->pkt.mpich2.payload;
                MPIDI_msg_sz_t   payload_len = cell->pkt.mpich2.datalen;
                MPIDI_CH3_Pkt_t *pkt         = (MPIDI_CH3_Pkt_t *)cell_buf;

                /* Empty packets are not allowed */
                MPIU_Assert(payload_len >= 0);

                if (in_fbox)
                {
                    MPIDI_CH3I_VC *vc_ch;
                    MPIDI_msg_sz_t buflen = payload_len;

                    /* This packet must be the first packet of a new message */
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Recv pkt from fbox");
                    MPIU_Assert(payload_len >= sizeof (MPIDI_CH3_Pkt_t));

                    MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, MPID_NEM_FBOX_SOURCE(cell), &vc);
		   
		    MPIU_Assert(((MPIDI_CH3I_VC *)vc->channel_private)->recv_active == NULL &&
                                ((MPIDI_CH3I_VC *)vc->channel_private)->pending_pkt_len == 0);
                    vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;

                    /* invalid pkt data will result in unpredictable behavior */
                    MPIU_Assert(pkt->type >= 0 && pkt->type < MPIDI_NEM_PKT_END);

                    mpi_errno = pktArray[pkt->type](vc, pkt, &buflen, &rreq);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                    if (!rreq)
                    {
                        MPID_nem_mpich2_release_fbox(cell);
                        break; /* break out of recv progress block */
                    }

                    /* we received a truncated packet, handle it with handle_pkt */
                    vc_ch->recv_active = rreq;
                    cell_buf    += buflen;
                    payload_len -= buflen;

                    mpi_errno = MPID_nem_handle_pkt(vc, cell_buf, payload_len);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    MPID_nem_mpich2_release_fbox(cell);

                    /* the whole message should have been handled */
                    MPIU_Assert(!vc_ch->recv_active);

                    break; /* break out of recv progress block */
                }


                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Recv pkt from queue");

                MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, MPID_NEM_CELL_SOURCE(cell), &vc);

                mpi_errno = MPID_nem_handle_pkt(vc, cell_buf, payload_len);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                MPID_nem_mpich2_release_cell(cell, vc);

                break; /* break out of recv progress block */

            }
        }
        while(0);  /* do the loop exactly once.  Used so we can jump out of recv progress using break. */


	/* make progress sending */
	do
        {
            MPID_IOV *iov;
            int n_iov;
            int again = 0;

            if (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL && MPIDI_CH3I_SendQ_head(CH3_NORMAL_QUEUE) == NULL)
            {
#ifdef MPICH_IS_THREADED
                MPIU_THREAD_CHECK_BEGIN;
                {
                    if (MPIDI_CH3I_progress_blocked == TRUE && is_blocking && !MPID_nem_local_lmt_pending)
                    {
                        /* There's nothing to send and there's another thread already blocking in the progress engine.*/
                        MPIDI_CH3I_Progress_delay(MPIDI_CH3I_progress_completion_count);
                    }
                }
                MPIU_THREAD_CHECK_END;
#endif
                /* there are no pending sends */
                break; /* break out of send progress */

            }

            sreq = MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE];
            MPIU_DBG_STMT(CH3_CHANNEL, VERBOSE, {if (sreq) MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "Send: cont sreq");});
            if (sreq)
            {
                if (!sreq->ch.noncontig)
                {
                    MPIU_Assert(sreq->dev.iov_count > 0 && sreq->dev.iov[sreq->dev.iov_offset].MPID_IOV_LEN > 0);

                    iov = &sreq->dev.iov[sreq->dev.iov_offset];
                    n_iov = sreq->dev.iov_count;

                    do
                    {
                        mpi_errno = MPID_nem_mpich2_sendv(&iov, &n_iov, sreq->ch.vc, &again);
                        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                    }
                    while (!again && n_iov > 0);

                    if (again) /* not finished sending */
                    {
                        sreq->dev.iov_offset = iov - sreq->dev.iov;
                        sreq->dev.iov_count = n_iov;
                        break; /* break out of send progress */
                    }
                    else
                        sreq->dev.iov_offset = 0;
                }
                else
                {
                    do
                    {
                        MPID_nem_mpich2_send_seg(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size,
                                                 sreq->ch.vc, &again);
                    }
                    while (!again && sreq->dev.segment_first < sreq->dev.segment_size);

                    if (again) /* not finished sending */
                        break; /* break out of send progress */
                }
            }
            else
            {
                sreq = MPIDI_CH3I_SendQ_head (CH3_NORMAL_QUEUE);
                MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, {if (sreq) MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "Send: new sreq ");});

                if (!sreq->ch.noncontig)
                {
                    MPIU_Assert(sreq->dev.iov_count > 0 && sreq->dev.iov[sreq->dev.iov_offset].MPID_IOV_LEN > 0);

                    iov = &sreq->dev.iov[sreq->dev.iov_offset];
                    n_iov = sreq->dev.iov_count;

                    mpi_errno = MPID_nem_mpich2_sendv_header(&iov, &n_iov, sreq->ch.vc, &again);
                    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                    if (!again)
                    {
                        MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
                        while (!again && n_iov > 0)
                        {
                            mpi_errno = MPID_nem_mpich2_sendv(&iov, &n_iov, sreq->ch.vc, &again);
                            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                        }
                    }

                    if (again) /* not finished sending */
                    {
                        sreq->dev.iov_offset = iov - sreq->dev.iov;
                        sreq->dev.iov_count = n_iov;
                        break; /* break out of send progress */
                    }
                    else
                        sreq->dev.iov_offset = 0;
                }
                else
                {
                    MPID_nem_mpich2_send_seg_header(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size,
                                                    &sreq->dev.pending_pkt, sreq->ch.header_sz, sreq->ch.vc, &again);
                    if (!again)
                    {
                        MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
                        while (!again && sreq->dev.segment_first < sreq->dev.segment_size)
                        {
                            MPID_nem_mpich2_send_seg(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size,
                                                     sreq->ch.vc, &again);
                        }
                    }

                    if (again) /* not finished sending */
                        break; /* break out of send progress */
                }
            }

            /* finished sending sreq */
            MPIU_Assert(!again);

            if (!sreq->dev.OnDataAvail)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);

                /* MT - clear the current active send before dequeuing/destroying the current request */
                MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = NULL;
                MPIDI_CH3I_SendQ_dequeue(CH3_NORMAL_QUEUE);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            }
            else
            {
                complete = 0;
                mpi_errno = sreq->dev.OnDataAvail(sreq->ch.vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIDI_CH3I_SendQ_dequeue(CH3_NORMAL_QUEUE);
                    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = NULL;
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                }
            }
        }
        while (0); /* do the loop exactly once.  Used so we can jump out of send progress using break. */

        /* make progress on LMTs */
        if (MPID_nem_local_lmt_pending)
        {
            mpi_errno = MPID_nem_local_lmt_progress();
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        MPIU_DBG_MSG_FMT(CH3_PROGRESS,VERBOSE,(MPIU_DBG_FDEST,"end of outer while loop, completions=%d MPIDI_CH3I_progress_completion_count=%d",completions,MPIDI_CH3I_progress_completion_count));
    }
    while (completions == MPIDI_CH3I_progress_completion_count && is_blocking);

#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        if (is_blocking)
        {
            MPIDI_CH3I_Progress_continue(MPIDI_CH3I_progress_completion_count);
        }
    }
    MPIU_THREAD_CHECK_END;
#endif

 fn_exit:
    /* Reset the progress state so it is fresh for the next iteration */
    if (progress_state)
        progress_state->ch.completion_count = MPIDI_CH3I_progress_completion_count;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#ifdef MPICH_IS_THREADED

/* Note that this routine is only called if threads are enabled;
   it does not need to check whether runtime threads are enabled */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_delay
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_delay(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_DELAY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_DELAY);
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL)
    {
	while (completion_count == MPIDI_CH3I_progress_completion_count && MPIDI_CH3I_progress_blocked == TRUE)
	{
	    MPID_Thread_cond_wait(&MPIDI_CH3I_progress_completion_cond, &MPIR_ThreadInfo.global_mutex);
	}
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_DELAY);
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_delay() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_continue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_CONTINUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_CONTINUE);
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL)
    {
	MPID_Thread_cond_broadcast(&MPIDI_CH3I_progress_completion_cond);
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_CONTINUE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_continue() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_wakeup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_Progress_wakeup(void)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_WAKEUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_WAKEUP);

    /* no processes sleep in nemesis progress */
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_WAKEUP);
    return;
}
#endif /* MPICH_IS_THREADED */

#undef FUNCNAME
#define FUNCNAME MPID_nem_handle_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_handle_pkt(MPIDI_VC_t *vc, char *buf, MPIDI_msg_sz_t buflen)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rreq = NULL;
    int complete;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_HANDLE_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_HANDLE_PKT);

    do
    {
        if (!vc_ch->recv_active && vc_ch->pending_pkt_len == 0 && buflen >= sizeof(MPIDI_CH3_Pkt_t))
        {
            /* handle fast-path first: received a new whole message */
            do
            {
                MPIDI_msg_sz_t len = buflen;
                MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t *)buf;

                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "received new message");

                /* invalid pkt data will result in unpredictable behavior */
                MPIU_Assert(pkt->type >= 0 && pkt->type < MPIDI_NEM_PKT_END);

                mpi_errno = pktArray[pkt->type](vc, pkt, &len, &rreq);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                buflen -= len;
                buf    += len;
                MPIU_DBG_STMT(CH3_CHANNEL, VERBOSE, if (!rreq) MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...completed immediately"));
            }
            while (!rreq && buflen >= sizeof(MPIDI_CH3_Pkt_t));

            if (!rreq)
                continue;

            /* Channel fields don't get initialized on request creation, init them here */
            if (rreq)
                rreq->dev.iov_offset = 0;
        }
        else if (vc_ch->recv_active)
        {
            MPIU_Assert(vc_ch->pending_pkt_len == 0);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "continuing recv");
            rreq = vc_ch->recv_active;
        }
        else
        {
            /* collect header fragments in vc's pending_pkt */
            MPIDI_msg_sz_t copylen;
            MPIDI_msg_sz_t pktlen;
            MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t *)vc_ch->pending_pkt;

            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "received header fragment");

            copylen = ((vc_ch->pending_pkt_len + buflen <= sizeof(MPIDI_CH3_Pkt_t))
                       ? buflen
                       : sizeof(MPIDI_CH3_Pkt_t) - vc_ch->pending_pkt_len);
            MPIU_Memcpy((char *)vc_ch->pending_pkt + vc_ch->pending_pkt_len, buf, copylen);
            vc_ch->pending_pkt_len += copylen;
            if (vc_ch->pending_pkt_len < sizeof(MPIDI_CH3_Pkt_t))
                goto fn_exit;

            /* we have a whole header */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    completed header");
            MPIU_Assert(vc_ch->pending_pkt_len == sizeof(MPIDI_CH3_Pkt_t));

            buflen -= copylen;
            buf    += copylen;

            /* invalid pkt data will result in unpredictable behavior */
            MPIU_Assert(pkt->type >= 0 && pkt->type < MPIDI_NEM_PKT_END);

            pktlen = sizeof(MPIDI_CH3_Pkt_t);
            mpi_errno = pktArray[pkt->type](vc, pkt, &pktlen, &rreq);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIU_Assert(pktlen == sizeof(MPIDI_CH3_Pkt_t));

            vc_ch->pending_pkt_len = 0;

            if (!rreq)
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...completed immediately");
                continue;
            }
            /* Channel fields don't get initialized on request creation, init them here */
            rreq->dev.iov_offset = 0;
        }

        /* copy data into user buffer described by iov in rreq */
        MPIU_Assert(rreq);
        MPIU_Assert(rreq->dev.iov_count > 0 && rreq->dev.iov[rreq->dev.iov_offset].MPID_IOV_LEN > 0);

        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "    copying into user buffer from IOV");

        if (buflen == 0)
        {
            vc_ch->recv_active = rreq;
            goto fn_exit;
        }

        complete = 0;

        while (buflen && !complete)
        {
            MPID_IOV *iov;
            int n_iov;

            iov = &rreq->dev.iov[rreq->dev.iov_offset];
            n_iov = rreq->dev.iov_count;
		
            while (n_iov && buflen >= iov->MPID_IOV_LEN)
            {
                int iov_len = iov->MPID_IOV_LEN;
		MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "        %d\n", iov_len);
                MPIU_Memcpy (iov->MPID_IOV_BUF, buf, iov_len);

                buflen -= iov_len;
                buf    += iov_len;
                --n_iov;
                ++iov;
            }
		
            if (n_iov)
            {
                if (buflen > 0)
                {
		    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "        " MPIDI_MSG_SZ_FMT "\n", buflen);
                    MPIU_Memcpy (iov->MPID_IOV_BUF, buf, buflen);
                    iov->MPID_IOV_BUF = (void *)((char *)iov->MPID_IOV_BUF + buflen);
                    iov->MPID_IOV_LEN -= buflen;
                    buflen = 0;
                }

                rreq->dev.iov_offset = iov - rreq->dev.iov;
                rreq->dev.iov_count = n_iov;
                vc_ch->recv_active = rreq;
		MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "        remaining: " MPIDI_MSG_SZ_FMT " bytes + %d iov entries\n", iov->MPID_IOV_LEN, n_iov - rreq->dev.iov_offset - 1));
            }
            else
            {
                int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

                reqFn = rreq->dev.OnDataAvail;
                if (!reqFn)
                {
                    MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                    MPIDI_CH3U_Request_complete(rreq);
                    complete = TRUE;
                }
                else
                {
                    mpi_errno = reqFn(vc, rreq, &complete);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }

                if (!complete)
                {
                    rreq->dev.iov_offset = 0;
                    MPIU_Assert(rreq->dev.iov_count > 0 && rreq->dev.iov[rreq->dev.iov_offset].MPID_IOV_LEN > 0);
                    vc_ch->recv_active = rreq;
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...not complete");
                }
                else
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
                    vc_ch->recv_active = NULL;
                }
            }
        }
    }
    while (buflen);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_HANDLE_PKT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#define set_request_info(rreq_, pkt_, msg_type_)                \
{                                                               \
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.rank;            \
    (rreq_)->status.MPI_TAG = (pkt_)->match.tag;                \
    (rreq_)->status.count = (pkt_)->data_sz;                    \
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;         \
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;                \
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);          \
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));           \
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIU_THREAD_CHECK_BEGIN
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL)
    {
	MPID_Thread_cond_create(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
    MPIU_THREAD_CHECK_END

    for (i = 0; i < CH3_NUM_QUEUES; ++i)
    {
	MPIDI_CH3I_sendq_head[i] = NULL;
	MPIDI_CH3I_sendq_tail[i] = NULL;
    }

    /* Initialize the code to handle incoming packets */
    if (PKTARRAY_SIZE <= MPIDI_NEM_PKT_END) {
        MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }
    /* pkt handlers from CH3 */
    mpi_errno = MPIDI_CH3_PktHandler_Init(pktArray, PKTARRAY_SIZE);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* pkt handlers for LMT */
    mpi_errno = MPID_nem_lmt_pkthandler_init(pktArray, PKTARRAY_SIZE);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
#ifdef ENABLE_CHECKPOINTING
    mpi_errno = MPIDI_nem_ckpt_pkthandler_init(pktArray, PKTARRAY_SIZE);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
#endif

    /* other pkt handlers */
    pktArray[MPIDI_NEM_PKT_NETMOD] = pkt_NETMOD_handler;
   
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate (MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);
    if (((MPIDI_CH3I_VC *)vc->channel_private)->is_local)
        mpi_errno = MPID_nem_vc_terminate(vc);
    else
        mpi_errno = MPID_nem_netmod_func->vc_terminate(vc);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH3U_Handle_connection (vc, MPIDI_VC_EVENT_TERMINATED);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Connection_terminate() */


#undef FUNCNAME
#define FUNCNAME pkt_NETMOD_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_NETMOD_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_netmod_t * const netmod_pkt = (MPID_nem_pkt_netmod_t *)pkt;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_PKT_NETMOD_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_PKT_NETMOD_HANDLER);

    MPIU_Assert_fmt_msg(vc_ch->pkt_handler && netmod_pkt->subtype < vc_ch->num_pkt_handlers, ("no handler defined for netmod-local packet"));

    mpi_errno = vc_ch->pkt_handler[netmod_pkt->subtype](vc, pkt, buflen, rreqp);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_NETMOD_HANDLER);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Register_anysource_notification
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Register_anysource_notification(void (*enqueue_fn)(MPID_Request *rreq), int (*dequeue_fn)(MPID_Request *rreq))
{
    int mpi_errno = MPI_SUCCESS;
    qn_ent_t *ent;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(ent, qn_ent_t *, sizeof(qn_ent_t), mpi_errno, "queue entry");

    ent->enqueue_fn = enqueue_fn;
    ent->dequeue_fn = dequeue_fn;
    ent->next = qn_head;
    qn_head = ent;

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Anysource_posted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void anysource_posted(MPID_Request *rreq)
{
    qn_ent_t *ent = qn_head;

    /* call all of the registered handlers */
    while (ent)
    {
        if (ent->enqueue_fn)
        {
            ent->enqueue_fn(rreq);
        }
        ent = ent->next;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Anysource_matched
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int anysource_matched(MPID_Request *rreq)
{
    int matched = FALSE;
    qn_ent_t *ent = qn_head;

    /* call all of the registered handlers */
    while(ent) {
        if (ent->dequeue_fn)
        {
            int m;
            
            m = ent->dequeue_fn(rreq);
            
            /* this is a crude check to check if the req has been
               matched by more than one netmod.  When MPIU_Assert() is
               defined to empty, the extra matched=m is optimized
               away. */
            MPIU_Assert(!m || !matched);
            matched = m;
        }
        ent = ent->next;
    }

    return matched;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Posted_recv_enqueued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_Posted_recv_enqueued(MPID_Request *rreq)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_POSTED_RECV_ENQUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_POSTED_RECV_ENQUEUED);

    if ((rreq)->dev.match.parts.rank == MPI_ANY_SOURCE)
        /* call anysource handler */
	anysource_posted(rreq);
    else
    {
        int local_rank = -1;
	MPIDI_VC_t *vc;

	MPIDI_Comm_get_vc_set_active((rreq)->comm, (rreq)->dev.match.parts.rank, &vc);
#ifdef ENABLE_COMM_OVERRIDES
        /* call vc-specific handler */
	if (vc->comm_ops && vc->comm_ops->recv_posted)
            vc->comm_ops->recv_posted(vc, rreq);
#endif
        
        /* enqueue fastbox */
        
        /* don't enqueue a fastbox for yourself */
        MPIU_Assert(rreq->comm != NULL);
        if (rreq->dev.match.parts.rank == rreq->comm->rank)
            goto fn_exit;

        /* don't enqueue non-local processes */
        if (!((MPIDI_CH3I_VC *)vc->channel_private)->is_local)
            goto fn_exit;

        /* Translate the communicator rank to a local rank.  Note that there is an
           implicit assumption here that because is_local is true above, that these
           processes are in the same PG. */
        local_rank = MPID_NEM_LOCAL_RANK(vc->pg_rank);

        MPID_nem_mpich2_enqueue_fastbox(local_rank);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_POSTED_RECV_ENQUEUED);
}

/* returns non-zero when req has been matched by channel */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Posted_recv_dequeued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Posted_recv_dequeued(MPID_Request *rreq)
{
    int local_rank = -1;
    MPIDI_VC_t *vc;
    int matched = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_POSTED_RECV_DEQUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_POSTED_RECV_DEQUEUED);
    
    if (rreq->dev.match.parts.rank == MPI_ANY_SOURCE)
    {
	matched = anysource_matched(rreq);
    }
    else
    {
        if (rreq->dev.match.parts.rank == rreq->comm->rank)
            goto fn_exit;
        
        /* don't use MPID_NEM_IS_LOCAL, it doesn't handle dynamic processes */
        MPIDI_Comm_get_vc_set_active(rreq->comm, rreq->dev.match.parts.rank, &vc);
        MPIU_Assert(vc != NULL);
        if (!((MPIDI_CH3I_VC *)vc->channel_private)->is_local)
            goto fn_exit;

        /* Translate the communicator rank to a local rank.  Note that there is an
           implicit assumption here that because is_local is true above, that these
           processes are in the same PG. */
        local_rank = MPID_NEM_LOCAL_RANK(vc->pg_rank);

        MPID_nem_mpich2_dequeue_fastbox(local_rank);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_POSTED_RECV_DEQUEUED);
    return matched;
}

