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
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

typedef struct vc_term_element
{
    struct vc_term_element *next;
    MPIDI_VC_t *vc;
    MPID_Request *req;
} vc_term_element_t;

static struct { vc_term_element_t *head, *tail; } vc_term_queue;
#define TERMQ_EMPTY() GENERIC_Q_EMPTY(vc_term_queue)
#define TERMQ_HEAD() GENERIC_Q_HEAD(vc_term_queue)
#define TERMQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&vc_term_queue, ep, next)
#define TERMQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&vc_term_queue, epp, next)

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

OPA_int_t MPIDI_CH3I_progress_completion_count = OPA_INT_T_INITIALIZER(0);

/* NEMESIS MULTITHREADING: Extra Data Structures Added */
#ifdef MPICH_IS_THREADED
volatile int MPIDI_CH3I_progress_blocked = FALSE;
volatile int MPIDI_CH3I_progress_wakeup_signalled = FALSE;
static MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond;
static int MPIDI_CH3I_Progress_delay(unsigned int completion_count);
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count);
#endif /* MPICH_IS_THREADED */
/* NEMESIS MULTITHREADING - End block*/

#ifdef HAVE_SIGNAL
static void (*prev_sighandler) (int);
#endif
static volatile int sigusr1_count = 0;
static int my_sigusr1_count = 0;

MPIDI_CH3I_shm_sendq_t MPIDI_CH3I_shm_sendq = {NULL, NULL};
struct MPID_Request *MPIDI_CH3I_shm_active_send = NULL;

static int pkt_NETMOD_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);
static int shm_connection_terminated(MPIDI_VC_t * vc);
static int check_terminating_vcs(void);

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

#ifdef HAVE_SIGNAL
static void sigusr1_handler(int sig)
{
    ++sigusr1_count;
    /* poke the progress engine in case we're waiting in a blocking recv */
    MPIDI_CH3_Progress_signal_completion();
    if (prev_sighandler)
        prev_sighandler(sig);
}
#endif

#undef FUNCNAME
#define FUNCNAME check_terminating_vcs
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int check_terminating_vcs(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_TERMINATING_VCS);

    MPIDI_FUNC_ENTER(MPID_STATE_CHECK_TERMINATING_VCS);

    while (!TERMQ_EMPTY() && MPID_Request_is_complete(TERMQ_HEAD()->req)) {
        vc_term_element_t *ep;
        TERMQ_DEQUEUE(&ep);
        MPID_Request_release(ep->req);
        mpi_errno = shm_connection_terminated(ep->vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_Free(ep);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CHECK_TERMINATING_VCS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* MPIDI_CH3I_Shm_send_progress() this function makes progress sending
   queued messages on the shared memory queues.  This function is
   nonblocking and does not call netmod functions..*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_send_progress
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_send_progress(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV *iov;
    int n_iov;
    MPID_Request *sreq;
    int again = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_SEND_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_SEND_PROGRESS);

    sreq = MPIDI_CH3I_shm_active_send;
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
                goto fn_exit;
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
                goto fn_exit;
        }
    }
    else
    {
        sreq = MPIDI_CH3I_Sendq_head(MPIDI_CH3I_shm_sendq);
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
                MPIDI_CH3I_shm_active_send = sreq;
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
                goto fn_exit;
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
                MPIDI_CH3I_shm_active_send = sreq;
                while (!again && sreq->dev.segment_first < sreq->dev.segment_size)
                {
                    MPID_nem_mpich2_send_seg(sreq->dev.segment_ptr, &sreq->dev.segment_first, sreq->dev.segment_size,
                                             sreq->ch.vc, &again);
                }
            }

            if (again) /* not finished sending */
                goto fn_exit;
        }
    }

    /* finished sending sreq */
    MPIU_Assert(!again);

    if (!sreq->dev.OnDataAvail)
    {
        /* MT FIXME-N1 race under per-object, harmless to disable here but
         * its a symptom of a bigger problem... */
#if !(defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT))
        MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
#endif

        MPIDI_CH3U_Request_complete(sreq);

        /* MT - clear the current active send before dequeuing/destroying the current request */
        MPIDI_CH3I_shm_active_send = NULL;
        MPIDI_CH3I_Sendq_dequeue(&MPIDI_CH3I_shm_sendq, &sreq);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        mpi_errno = check_terminating_vcs();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        int complete = 0;
        mpi_errno = sreq->dev.OnDataAvail(sreq->ch.vc, sreq, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (complete)
        {
            MPIDI_CH3I_shm_active_send = NULL;
            MPIDI_CH3I_Sendq_dequeue(&MPIDI_CH3I_shm_sendq, &sreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            mpi_errno = check_terminating_vcs();
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
        
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_SEND_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* NOTE: it appears that this function is sometimes (inadvertently?) recursive.
 * Some packet handlers, such as MPIDI_CH3_PktHandler_Close, call iStartMsg,
 * which calls MPID_Progress_test. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress (MPID_Progress_state *progress_state, int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
#if !defined(ENABLE_NO_YIELD) || defined(MPICH_IS_THREADED)
    int pollcount = 0;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIU_THREAD_CS_ENTER(MPIDCOMM,);

    /* sanity: if this doesn't hold, we can't track our local view of completion safely */
    if (is_blocking) {
        MPIU_Assert(progress_state != NULL);
    }

    if (sigusr1_count > my_sigusr1_count) {
        my_sigusr1_count = sigusr1_count;
        mpi_errno = MPIDI_CH3U_Check_for_failed_procs();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
#ifdef ENABLE_CHECKPOINTING
    if (MPIR_PARAM_ENABLE_CKPOINT) {
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
    }
#endif

    do
    {
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
                /* MPIDCOMM yield is needed because at least the send functions
                 * acquire MPIDCOMM to put things into the send queues.  Failure
                 * to yield could result in a deadlock.  This thread needs the
                 * send from another thread to be posted, but the other thread
                 * can't post it while this CS is held. */
                /* assertion: we currently do not hold any other critical
                 * sections besides the MPIDCOMM CS at this point.  Violating
                 * this will probably lead to lock-ordering deadlocks. */
                MPIU_THREAD_CS_YIELD(MPIDCOMM,);
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
            if (!MPID_nem_local_lmt_pending && !MPIDI_CH3I_shm_active_send
                && !MPIDI_CH3I_Sendq_head(MPIDI_CH3I_shm_sendq) && is_blocking
#ifdef MPICH_IS_THREADED
#ifdef HAVE_RUNTIME_THREADCHECK
                && !MPIR_ThreadInfo.isThreaded
#else
                && 0
#endif
#endif
                )
            {
                mpi_errno = MPID_nem_mpich2_blocking_recv(&cell, &in_fbox, progress_state->ch.completion_count);
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
		   
		    MPIU_Assert(VC_CH(vc)->recv_active == NULL &&
                                VC_CH(vc)->pending_pkt_len == 0);
                    vc_ch = VC_CH(vc);

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
        if (MPIDI_CH3I_shm_active_send || MPIDI_CH3I_Sendq_head(MPIDI_CH3I_shm_sendq)) {
            mpi_errno = MPIDI_CH3I_Shm_send_progress();
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        } else {
            /* there are no pending sends */
#ifdef MPICH_IS_THREADED
            MPIU_THREAD_CHECK_BEGIN;
            {
                if (MPIDI_CH3I_progress_blocked == TRUE && is_blocking && !MPID_nem_local_lmt_pending)
                {
                    /* There's nothing to send and there's another thread already blocking in the progress engine.*/
                    MPIDI_CH3I_Progress_delay(progress_state->ch.completion_count);
                    /* the progress_state count will be updated below at the
                     * bottom of the outermost loop (see CC-1) */
                }
            }
            MPIU_THREAD_CHECK_END;
#endif
        }
        
        /* make progress on LMTs */
        if (MPID_nem_local_lmt_pending)
        {
            mpi_errno = MPID_nem_local_lmt_progress();
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        /* in the case of progress_wait, bail out if anything completed (CC-1) */
        if (is_blocking) {
            int completion_count = OPA_load_int(&MPIDI_CH3I_progress_completion_count);
            if (progress_state->ch.completion_count != completion_count) {
                /* Read barrier to make sure no reads get values before the
                   completion counter was incremented  */
                OPA_read_barrier();
                /* reset for the next iteration */
                progress_state->ch.completion_count = completion_count;
                break;
            }
        }
    }
    while (is_blocking);

    
#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        if (is_blocking)
        {
            MPIDI_CH3I_Progress_continue(0/*unused*/);
        }
    }
    MPIU_THREAD_CHECK_END;
#endif

 fn_exit:
    MPIU_THREAD_CS_EXIT(MPIDCOMM,);
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
	while (1)
	{
            if (completion_count != OPA_load_int(&MPIDI_CH3I_progress_completion_count) ||
                MPIDI_CH3I_progress_blocked != TRUE)
                break;
	    MPID_Thread_cond_wait(&MPIDI_CH3I_progress_completion_cond, &MPIR_ThreadInfo.global_mutex/*MPIDCOMM*/);
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
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count/*unused*/)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_CONTINUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_CONTINUE);
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL)
    {
        /* we currently hold the MPIDCOMM CS */
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
    MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
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
		MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "        remaining: " MPIDI_MSG_SZ_FMT " bytes + %d iov entries\n", iov->MPID_IOV_LEN, n_iov));
            }
            else
            {
                int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

                reqFn = rreq->dev.OnDataAvail;
                if (!reqFn)
                {
                    /* MT FIXME-N1 */
#if !(defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT))
                    MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
#endif
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

    MPIDI_CH3I_shm_sendq.head = NULL;
    MPIDI_CH3I_shm_sendq.tail = NULL;
    MPIDI_CH3I_shm_active_send = NULL;
    
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
   
#ifdef HAVE_SIGNAL
    /* install signal handler for process failure notifications from hydra */
    prev_sighandler = signal(SIGUSR1, sigusr1_handler);
    MPIU_ERR_CHKANDJUMP1(prev_sighandler == SIG_ERR, mpi_errno, MPI_ERR_OTHER, "**signal", "**signal %s", MPIU_Strerror(errno));
    if (prev_sighandler == SIG_IGN || prev_sighandler == SIG_DFL)
        prev_sighandler = NULL;
#endif

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
    int mpi_errno = MPI_SUCCESS;
    qn_ent_t *ent;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    while(qn_head) {
        ent = qn_head->next;
        MPIU_Free(qn_head);
        qn_head = ent;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME shm_connection_terminated
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int shm_connection_terminated(MPIDI_VC_t * vc)
{
    /* This function is called after all sends have completed */
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SHM_CONNECTION_TERMINATED);

    MPIDI_FUNC_ENTER(MPID_STATE_SHM_CONNECTION_TERMINATED);

    mpi_errno = VC_CH(vc)->lmt_vc_terminated(vc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIU_SHMW_Hnd_finalize(&(VC_CH(vc)->lmt_copy_buf_handle));
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_SHMW_Hnd_finalize(&(VC_CH(vc)->lmt_recv_copy_buf_handle));
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_DBG_MSG_D(CH3_DISCONNECT, TYPICAL, "Terminated VC %d", vc->pg_rank);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_CONNECTION_TERMINATED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);

    MPIU_DBG_MSG_D(CH3_DISCONNECT, TYPICAL, "Terminating VC %d", vc->pg_rank);

    /* if this is already closed, exit */
    if (vc->state == MPIDI_VC_STATE_MORIBUND ||
        vc->state == MPIDI_VC_STATE_INACTIVE_CLOSED)
        goto fn_exit;

    if (VC_CH(vc)->is_local) {
        MPIU_DBG_MSG(CH3_DISCONNECT, TYPICAL, "VC is local");

        if (vc->state != MPIDI_VC_STATE_CLOSED) {
            /* VC is terminated as a result of a fault.  Complete
               outstanding sends with an error and terminate
               connection immediately. */
            MPIU_DBG_MSG(CH3_DISCONNECT, TYPICAL, "VC terminated due to fault");
            mpi_errno = MPIDI_CH3I_Complete_sendq_with_error(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = shm_connection_terminated(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        } else {
            /* VC is terminated as a result of the close protocol.
               Wait for sends to complete, then terminate. */

            if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq)) {
                /* The sendq is empty, so we can immediately terminate
                   the connection. */
                MPIU_DBG_MSG(CH3_DISCONNECT, TYPICAL, "Shm send queue empty, terminating immediately");
                mpi_errno = shm_connection_terminated(vc);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            } else {
                /* There may be sends from this VC on the send queue.
                   Since there is one send queue, we don't want to
                   search the queue to find the last send from this
                   VC.  Instead, we use the last send in the queue,
                   regardless of which VC it's from.  When that send
                   completes, (since no new messages are sent on this
                   VC anymore) we know that all sends on this VC must
                   have completed.  */
                vc_term_element_t *ep;
                MPIU_DBG_MSG(CH3_DISCONNECT, TYPICAL, "Shm send queue not empty, waiting to terminate");
                MPIU_CHKPMEM_MALLOC(ep, vc_term_element_t *, sizeof(vc_term_element_t), mpi_errno, "vc_term_element");
                ep->vc = vc;
                ep->req = MPIDI_CH3I_shm_sendq.tail;
                MPIR_Request_add_ref(ep->req); /* make sure this doesn't get released before we can check it */
                TERMQ_ENQUEUE(ep);
            }
        }
    
    } else {
        MPIU_DBG_MSG(CH3_DISCONNECT, TYPICAL, "VC is remote");
        mpi_errno = MPID_nem_netmod_func->vc_terminate(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CONNECTION_TERMINATE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
/* end MPIDI_CH3_Connection_terminate() */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Complete_sendq_with_error
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_Complete_sendq_with_error(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req, *prev;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMPLETE_SENDQ_WITH_ERROR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_COMPLETE_SENDQ_WITH_ERROR);

    req = MPIDI_CH3I_shm_sendq.head;
    prev = NULL;
    while (req) {
        if (req->ch.vc == vc) {
            MPID_Request *next = req->dev.next;
            if (prev)
                prev->dev.next = next;
            else
                MPIDI_CH3I_shm_sendq.head = next;
            if (MPIDI_CH3I_shm_sendq.tail == req)
                MPIDI_CH3I_shm_sendq.tail = prev;

            req->status.MPI_ERROR = MPI_SUCCESS;
            MPIU_ERR_SET1(req->status.MPI_ERROR, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
            
            MPID_Request_release(req); /* ref count was incremented when added to queue */
            MPIDI_CH3U_Request_complete(req);
            req = next;
        } else {
            prev = req;
            req = req->dev.next;
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_COMPLETE_SENDQ_WITH_ERROR);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME pkt_NETMOD_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_NETMOD_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_netmod_t * const netmod_pkt = (MPID_nem_pkt_netmod_t *)pkt;
    MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
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

    /* MT FIXME acquiring MPIDCOMM here violates lock ordering rules (see
     * mpiimplthread.h comments), easily causes deadlock */

    if ((rreq)->dev.match.parts.rank == MPI_ANY_SOURCE)
        /* call anysource handler */
	anysource_posted(rreq);
    else
    {
        int local_rank = -1;
	MPIDI_VC_t *vc;

        /* MT FIXME does this macro need some sort of synchronization too? */
	MPIDI_Comm_get_vc((rreq)->comm, (rreq)->dev.match.parts.rank, &vc);

#ifdef ENABLE_COMM_OVERRIDES
        /* MT FIXME causes deadlock b/c of the MSGQUEUE/CH3COMM ordering (acquired
         * in reverse in some pkt handlers?) */
        MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        /* call vc-specific handler */
	if (vc->comm_ops && vc->comm_ops->recv_posted)
            vc->comm_ops->recv_posted(vc, rreq);
        MPIU_THREAD_CS_EXIT(CH3COMM,vc);
#endif

        /* MT FIXME we unfortunately must disable this optimization for now in
         * per_object mode. There are possibly other ways to synchronize the
         * fboxes that won't cause lock-ordering deadlocks.  There might also be
         * ways to do this that don't require a hook on every request post, but
         * instead do some sort of caching or something analogous to branch
         * prediction. */
#if !(defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT))
        /* enqueue fastbox */

        /* don't enqueue a fastbox for yourself */
        MPIU_Assert(rreq->comm != NULL);
        if (rreq->dev.match.parts.rank == rreq->comm->rank)
            goto fn_exit;

        /* don't enqueue non-local processes */
        if (!VC_CH(vc)->is_local)
            goto fn_exit;

        /* Translate the communicator rank to a local rank.  Note that there is an
           implicit assumption here that because is_local is true above, that these
           processes are in the same PG. */
        local_rank = MPID_NEM_LOCAL_RANK(vc->pg_rank);

        MPID_nem_mpich2_enqueue_fastbox(local_rank);
#endif
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
    /* MT FIXME we unfortunately must disable this optimization for now in
     * per_object mode. There are possibly other ways to synchronize the
     * fboxes that won't cause lock-ordering deadlocks */
#if !(defined(MPICH_IS_THREADED) && (MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT))
    else
    {
        if (rreq->dev.match.parts.rank == rreq->comm->rank)
            goto fn_exit;

        /* don't use MPID_NEM_IS_LOCAL, it doesn't handle dynamic processes */
        MPIDI_Comm_get_vc(rreq->comm, rreq->dev.match.parts.rank, &vc);
        MPIU_Assert(vc != NULL);
        if (!VC_CH(vc)->is_local)
            goto fn_exit;

        /* Translate the communicator rank to a local rank.  Note that there is an
           implicit assumption here that because is_local is true above, that these
           processes are in the same PG. */
        local_rank = MPID_NEM_LOCAL_RANK(vc->pg_rank);

        MPID_nem_mpich2_dequeue_fastbox(local_rank);
    }
#endif

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_POSTED_RECV_DEQUEUED);
    return matched;
}

