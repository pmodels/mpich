/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "mpidu_sock.h"
#include "utlist.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

static MPIDI_CH3_PktHandler_Fcn *pktArray[MPIDI_CH3_PKT_END_CH3+1];

static int ReadMoreData( MPIDI_CH3I_Connection_t *, MPIR_Request * );

static int MPIDI_CH3i_Progress_wait(MPID_Progress_state * );
static int MPIDI_CH3i_Progress_test(void);

/* FIXME: Move thread stuff into some set of abstractions in order to remove
   ifdefs */
volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

#ifdef MPICH_IS_THREADED
    volatile int MPIDI_CH3I_progress_blocked = FALSE;
    volatile int MPIDI_CH3I_progress_wakeup_signalled = FALSE;

    /* This value must be static so that it isn't an uninitialized
       common symbol */
    static MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond;

    static int MPIDI_CH3I_Progress_delay(unsigned int completion_count);
    static int MPIDI_CH3I_Progress_continue(unsigned int completion_count);
#endif


MPIDI_CH3I_Sock_set_t MPIDI_CH3I_sock_set = NULL;
static int MPIDI_CH3I_Progress_handle_sock_event(MPIDI_CH3I_Sock_event_t * event);

static inline int connection_pop_sendq_req(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);

static int adjust_iov(MPL_IOV ** iovp, int * countp, size_t nb);

#define MAX_PROGRESS_HOOKS 4
typedef int (*progress_func_ptr_t) (int* made_progress);

typedef struct progress_hook_slot {
    progress_func_ptr_t func_ptr;
    int active;
} progress_hook_slot_t;

static progress_hook_slot_t progress_hooks[MAX_PROGRESS_HOOKS];

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3i_Progress_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3i_Progress_test(void)
{
    MPIDI_CH3I_Sock_event_t event;
    int mpi_errno = MPI_SUCCESS;
    int made_progress;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_TEST);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_TEST);

#   ifdef MPICH_IS_THREADED
    {
	/* We don't bother testing whether threads are enabled in the 
	   runtime-checking case because this simple test will always be false
	   if threads are not enabled. */
	if (MPIDI_CH3I_progress_blocked == TRUE) 
	{
	    /*
	     * Another thread is already blocking in the progress engine.  
	     * We are not going to block waiting for progress, so we
	     * simply return.  It might make sense to yield before * returning,
	     * giving the PE thread a change to make progress.
	     *
	     * MT: Another thread is already blocking in poll.  Right now, 
	     * calls to the progress routines are effectively
	     * serialized by the device.  The only way another thread may 
	     * enter this function is if MPIDI_CH3I_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked 
	     * may be required to determine if another thread is in
	     * the progress engine.
	     */
	    
	    goto fn_exit;
	}
    }
#   endif
    
    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (progress_hooks[i].active == TRUE) {
            MPIR_Assert(progress_hooks[i].func_ptr != NULL);
            mpi_errno = progress_hooks[i].func_ptr(&made_progress);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    mpi_errno = MPIDI_CH3I_Sock_wait(MPIDI_CH3I_sock_set, 0, &event);

    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|sock|handle_sock_event");
	}
    }
    else if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDI_CH3I_SOCK_ERR_TIMEOUT)
    {
	mpi_errno = MPI_SUCCESS;
	goto fn_exit;
    }
    else {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**progress_sock_wait");
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_TEST);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Progress_test() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3i_Progress_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3i_Progress_wait(MPID_Progress_state * progress_state)
{
    MPIDI_CH3I_Sock_event_t event;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_WAIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_WAIT);

    /*
     * MT: the following code will be needed if progress can occur between 
     * MPIDI_CH3_Progress_start() and
     * MPIDI_CH3_Progress_wait(), or iterations of MPIDI_CH3_Progress_wait().
     *
     * This is presently not possible, and thus the code is commented out.
     */
#   if 0
    /* FIXME: Was (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED),
       which really meant not-using-global-mutex-thread model .  This
       was true for the single threaded case, but was probably not intended
       for that case*/
    {
	if (progress_state->ch.completion_count != MPIDI_CH3I_progress_completion_count)
	{
	    goto fn_exit;
	}
    }
#   endif
	
#   ifdef MPICH_IS_THREADED
    MPIR_THREAD_CHECK_BEGIN;
    {
	if (MPIDI_CH3I_progress_blocked == TRUE) 
	{
	    /*
	     * Another thread is already blocking in the progress engine.
	     *
	     * MT: Another thread is already blocking in poll.  Right now, 
	     * calls to MPIDI_CH3_Progress_wait() are effectively
	     * serialized by the device.  The only way another thread may 
	     * enter this function is if MPIDI_CH3I_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked 
	     * may be required to determine if another thread is in
	     * the progress engine.
	     */
	    MPIDI_CH3I_Progress_delay(MPIDI_CH3I_progress_completion_count);
		
	    goto fn_exit;
	}
    }
    MPIR_THREAD_CHECK_END;
#   endif
    
    do
    {
        int made_progress = FALSE;
        int i;

        for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
            if (progress_hooks[i].active == TRUE) {
                MPIR_Assert(progress_hooks[i].func_ptr != NULL);
                mpi_errno = progress_hooks[i].func_ptr(&made_progress);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                if (made_progress) {
                    MPIDI_CH3_Progress_signal_completion();
                    break; /* break the for loop */
                }
            }
        }
        if (made_progress) break; /* break the do loop */

#       ifdef MPICH_IS_THREADED

	/* The logic for this case is just complicated enough that
	   we write separate code for each possibility */
	if (MPIR_ThreadInfo.isThreaded) {
	    MPIDI_CH3I_progress_blocked = TRUE;
	    mpi_errno = MPIDI_CH3I_Sock_wait(MPIDI_CH3I_sock_set,
				    MPIDI_CH3I_SOCK_INFINITE_TIME, &event);
	    MPIDI_CH3I_progress_blocked = FALSE;
	    MPIDI_CH3I_progress_wakeup_signalled = FALSE;
	}
	else {
	    mpi_errno = MPIDI_CH3I_Sock_wait(MPIDI_CH3I_sock_set,
				    MPIDI_CH3I_SOCK_INFINITE_TIME, &event);
	}

#       else
	mpi_errno = MPIDI_CH3I_Sock_wait(MPIDI_CH3I_sock_set,
				    MPIDI_CH3I_SOCK_INFINITE_TIME, &event);
#	endif

	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_Assert(MPIR_ERR_GET_CLASS(mpi_errno) != MPIDI_CH3I_SOCK_ERR_TIMEOUT);
	    MPIR_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**progress_sock_wait");
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|sock|handle_sock_event");
	}
    }
    while (progress_state->ch.completion_count == MPIDI_CH3I_progress_completion_count);

    /*
     * We could continue to call MPIU_Sock_wait in a non-blocking fashion 
     * and process any other events; however, this would not
     * give the application a chance to post new receives, and thus could 
     * result in an increased number of unexpected messages
     * that would need to be buffered.
     */
    
#   if MPICH_IS_THREADED
    {
	/*
	 * Awaken any threads which are waiting for the progress that just 
	 * occurred
	 */
	MPIDI_CH3I_Progress_continue(MPIDI_CH3I_progress_completion_count);
    }
#   endif
    
 fn_exit:
    /*
     * Reset the progress state so it is fresh for the next iteration
     */
    progress_state->ch.completion_count = MPIDI_CH3I_progress_completion_count;
    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_WAIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Progress_wait() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = &vc->ch;

    MPL_DBG_CONNSTATECHANGE(vc,vcch->conn,CONN_STATE_CLOSING);
    vcch->conn->state = CONN_STATE_CLOSING;
    MPL_DBG_MSG(MPIDI_CH3_DBG_DISCONNECT,TYPICAL,"Closing sock (Post_close)");
    mpi_errno = MPIDI_CH3I_Sock_post_close(vcch->sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Connection_terminate() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init(void)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIR_THREAD_CHECK_BEGIN;
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
    {
        int err;
	MPID_Thread_cond_create(&MPIDI_CH3I_progress_completion_cond, &err);
        MPIR_Assert(err == 0);
    }
#   endif
    MPIR_THREAD_CHECK_END;
	
    mpi_errno = MPIDI_CH3I_Sock_init();
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    
    /* create sock set */
    mpi_errno = MPIDI_CH3I_Sock_create_set(&MPIDI_CH3I_sock_set);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    
    /* establish non-blocking listener */
    mpi_errno = MPIDU_CH3I_SetupListener( MPIDI_CH3I_sock_set );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

    /* Initialize the code to handle incoming packets */
    mpi_errno = MPIDI_CH3_PktHandler_Init( pktArray, MPIDI_CH3_PKT_END_CH3+1 );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

    /* Initialize progress hook slots */
    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        progress_hooks[i].func_ptr = NULL;
        progress_hooks[i].active = FALSE;
    }
    
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MIPDI_CH3I_Progress_init() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize(void)
{
    int mpi_errno;
    MPIDI_CH3I_Connection_t *conn = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    /* Shut down the listener */
    mpi_errno = MPIDU_CH3I_ShutdownListener();
    if (mpi_errno != MPI_SUCCESS) { MPIR_ERR_POP(mpi_errno); }
    

    /* Close open connections */
    MPIDI_CH3I_Sock_close_open_sockets(MPIDI_CH3I_sock_set,(void**) &conn);
    while (conn != NULL) {
        conn->state = CONN_STATE_CLOSING;
        mpi_errno = MPIDI_CH3_Sockconn_handle_close_event(conn);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
        MPIDI_CH3I_Sock_close_open_sockets(MPIDI_CH3I_sock_set,(void**) &conn);
    }



    /*
     * MT: in a multi-threaded environment, finalize() should signal any 
     * thread(s) blocking on MPIDI_CH3I_Sock_wait() and wait for
     * those * threads to complete before destroying the progress engine 
     * data structures.
     */

    MPIDI_CH3I_Sock_destroy_set(MPIDI_CH3I_sock_set);
    MPIDI_CH3I_Sock_finalize();

    MPIR_THREAD_CHECK_BEGIN;
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
    {
        int err;
	MPID_Thread_cond_destroy(&MPIDI_CH3I_progress_completion_cond, &err);
        MPIR_Assert(err == 0);
    }
#   endif
    MPIR_THREAD_CHECK_END;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3I_Progress_finalize() */


#ifdef MPICH_IS_THREADED
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_CH3I_Progress_wakeup(void)
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"progress_wakeup called");
    MPIDI_CH3I_Sock_wakeup(MPIDI_CH3I_sock_set);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_business_card
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_business_card(int myRank, char *value, int length)
{
    return MPIDI_CH3U_Get_business_card_sock(myRank, &value, &length);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_handle_sock_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_handle_sock_event(MPIDI_CH3I_Sock_event_t * event)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,"Socket event of type %d", event->op_type );

    switch (event->op_type)
    {
	case MPIDI_CH3I_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = 
		(MPIDI_CH3I_Connection_t *) event->user_ptr;
            /* If we have a READ event on a discarded connection, we probably have
               an error on this connection, if the remote side is closed due to
               MPI_Finalize. Since the connection is discareded (and therefore not needed)
               it can be closed and the error can be ignored */
            if(conn->state == CONN_STATE_DISCARD){
                MPIDI_CH3_Sockconn_handle_close_event(conn);
                break;
            }
		
	    MPIR_Request * rreq = conn->recv_active;

	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS)
	    {
		/* FIXME: the following should be handled by the close 
		   protocol */
		if (MPIR_ERR_GET_CLASS(event->error) != MPIDI_CH3I_SOCK_ERR_CONN_CLOSED) {
		    mpi_errno = event->error;
		    MPIR_ERR_POP(mpi_errno);
		}		    
		break;
	    }
	    /* --END ERROR HANDLING-- */
		
	    if (conn->state == CONN_STATE_CONNECTED)
	    {
		if (conn->recv_active == NULL)
		{
                    intptr_t buflen = 0;
		    MPIR_Assert(conn->pkt.type < MPIDI_CH3_PKT_END_CH3);
                    
		    /* conn->pkt is always aligned */
		    mpi_errno = pktArray[conn->pkt.type]( conn->vc, &conn->pkt, NULL,
							  &buflen, &rreq );
		    if (mpi_errno != MPI_SUCCESS) {
			MPIR_ERR_POP(mpi_errno);
		    }
                    MPIR_Assert(buflen == 0);

		    if (rreq == NULL)
		    {
			if (conn->state != CONN_STATE_CLOSING)
			{
			    /* conn->recv_active = NULL;  -- 
			       already set to NULL */
			    mpi_errno = connection_post_recv_pkt(conn);
			    if (mpi_errno != MPI_SUCCESS) {
				MPIR_ERR_POP(mpi_errno);
			    }
			}
		    }
		    else
		    {
			mpi_errno = ReadMoreData( conn, rreq );
			if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
		    }
		}
		else /* incoming data */
		{
		    int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
		    int complete;

		    reqFn = rreq->dev.OnDataAvail;
		    if (!reqFn) {
			MPIR_Assert(MPIDI_Request_get_type(rreq)!=MPIDI_REQUEST_TYPE_GET_RESP);
                        mpi_errno = MPID_Request_complete(rreq);
                        if (mpi_errno != MPI_SUCCESS) {
                            MPIR_ERR_POP(mpi_errno);
                        }
			complete = TRUE;
		    }
		    else {
			mpi_errno = reqFn( conn->vc, rreq, &complete );
			if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		    }
			
		    if (complete)
		    {
			conn->recv_active = NULL;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIR_ERR_POP(mpi_errno);
			}
		    }
		    else /* more data to be read */
		    {
			mpi_errno = ReadMoreData( conn, rreq );
			if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		mpi_errno = MPIDI_CH3_Sockconn_handle_connopen_event( conn );
		if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }
	    }
	    else /* Handling some internal connection establishment or 
		    tear down packet */
	    { 
		mpi_errno = MPIDI_CH3_Sockconn_handle_conn_event( conn );
		if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	    }
	    break;
	}

	/* END OF SOCK_OP_READ */

	case MPIDI_CH3I_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = 
		(MPIDI_CH3I_Connection_t *) event->user_ptr;
	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS) {
		mpi_errno = event->error;
		MPIR_ERR_POP(mpi_errno);
	    }
	    /* --END ERROR HANDLING-- */
		
	    if (conn->send_active)
	    {
		MPIR_Request * sreq = conn->send_active;
		int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
		int complete;

		reqFn = sreq->dev.OnDataAvail;
		if (!reqFn) {
		    MPIR_Assert(MPIDI_Request_get_type(sreq)!=MPIDI_REQUEST_TYPE_GET_RESP);
                    mpi_errno = MPID_Request_complete(sreq);
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_ERR_POP(mpi_errno);
                    }

		    complete = TRUE;
		}
		else {
		    mpi_errno = reqFn( conn->vc, sreq, &complete );
		    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		}
		    
		if (complete)
		{
		    mpi_errno = connection_pop_sendq_req(conn);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIR_ERR_POP(mpi_errno);
		    }
		}
		else /* more data to send */
		{
		    for(;;)
		    {
			MPL_IOV * iovp;
			size_t nb;
				
			iovp = sreq->dev.iov;
			    
			mpi_errno = MPIDI_CH3I_Sock_writev(conn->sock, iovp, sreq->dev.iov_count, &nb);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p",
							     sreq, conn, conn->vc);
			    goto fn_fail;
			}
			/* --END ERROR HANDLING-- */

			MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
       (MPL_DBG_FDEST,"immediate writev, vc=%p, sreq=0x%08x, nb=%" PRIdPTR,
	conn->vc, sreq->handle, nb));
			    
			if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb))
			{
			    reqFn = sreq->dev.OnDataAvail;
			    if (!reqFn) {
				MPIR_Assert(MPIDI_Request_get_type(sreq)!=MPIDI_REQUEST_TYPE_GET_RESP);
                                mpi_errno = MPID_Request_complete(sreq);
                                if (mpi_errno != MPI_SUCCESS) {
                                    MPIR_ERR_POP(mpi_errno);
                                }
				complete = TRUE;
			    }
			    else {
				mpi_errno = reqFn( conn->vc, sreq, &complete );
				if (mpi_errno) MPIR_ERR_POP(mpi_errno);
			    }
			    if (complete)
			    {
				mpi_errno = connection_pop_sendq_req(conn);
				if (mpi_errno != MPI_SUCCESS) {
				    MPIR_ERR_POP(mpi_errno);
				}
				break;
			    }
			}
			else
			{
			    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
       (MPL_DBG_FDEST,"posting writev, vc=%p, conn=%p, sreq=0x%08x",
	conn->vc, conn, sreq->handle));
			    mpi_errno = MPIDI_CH3I_Sock_post_writev(conn->sock, iovp, sreq->dev.iov_count, NULL);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite",
				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    break;
			}
		    }
		}
	    }
	    else /* finished writing internal packet header */
	    {
		/* the connection is not active yet */
		mpi_errno = MPIDI_CH3_Sockconn_handle_connwrite( conn );
		if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }
	    }
	    break;
	}
	/* END OF SOCK_OP_WRITE */

	case MPIDI_CH3I_SOCK_OP_ACCEPT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_accept_event();
	    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	    break;
	}
	    
	case MPIDI_CH3I_SOCK_OP_CONNECT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_connect_event( 
				(MPIDI_CH3I_Connection_t *) event->user_ptr,
				event->error );
	    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	    break;
	}
	    
	case MPIDI_CH3I_SOCK_OP_CLOSE:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_close_event( 
			      (MPIDI_CH3I_Connection_t *) event->user_ptr );
	    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	    break;
	}

	case MPIDI_CH3I_SOCK_OP_WAKEUP:
	{
	    MPIDI_CH3_Progress_signal_completion();
	    /* MPIDI_CH3I_progress_completion_count++; */
	    break;
	}
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3I_Progress_handle_sock_event() */


#ifdef MPICH_IS_THREADED

/* Note that this routine is only called if threads are enabled; 
   it does not need to check whether runtime threads are enabled */
/* FIXME: Test for runtime thread level (here or where used) */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_delay
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_delay(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS, err;
    
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
    {
	while (completion_count == MPIDI_CH3I_progress_completion_count)
	{
	    MPID_Thread_cond_wait(&MPIDI_CH3I_progress_completion_cond, 
				  &MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
	}
    }
#   endif
    
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_delay() */

/* FIXME: Test for runtime thread level */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_continue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS,err;

    MPIR_THREAD_CHECK_BEGIN;
    /* FIXME should be appropriately abstracted somehow */
#   if defined(MPICH_IS_THREADED) && (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
    {
        MPID_Thread_cond_broadcast(&MPIDI_CH3I_progress_completion_cond,&err);
    }
#   endif
    MPIR_THREAD_CHECK_END;

    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_continue() */

#endif /* MPICH_IS_THREADED */


/* FIXME: (a) what does this do and where is it used and (b) 
   we could replace it with a #define for the single-method case */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    return MPIDI_CH3I_VC_post_sockconnect( vc );
}
/* end MPIDI_CH3I_VC_post_connect() */

/* FIXME: This function also used in ch3u_connect_sock.c */
#undef FUNCNAME
#define FUNCNAME connection_pop_sendq_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int connection_pop_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = &conn->vc->ch;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONNECTION_POP_SENDQ_REQ);


    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONNECTION_POP_SENDQ_REQ);
    /* post send of next request on the send queue */

    /* FIXME: Is dequeue/get next the operation we really want? */
    MPIDI_CH3I_SendQ_dequeue(vcch);
    conn->send_active = MPIDI_CH3I_SendQ_head(vcch); /* MT */
    if (conn->send_active != NULL)
    {
	MPL_DBG_MSG_P(MPIDI_CH3_DBG_CONNECT,TYPICAL,"conn=%p: Posting message from connection send queue", conn );
	mpi_errno = MPIDI_CH3I_Sock_post_writev(conn->sock, conn->send_active->dev.iov, conn->send_active->dev.iov_count, NULL);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_POP(mpi_errno);
	}
    }
    
 fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONNECTION_POP_SENDQ_REQ);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    mpi_errno = MPIDI_CH3I_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}

/* FIXME: What is this routine for? */
#undef FUNCNAME
#define FUNCNAME adjust_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int adjust_iov(MPL_IOV ** iovp, int * countp, size_t nb)
{
    MPL_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;
    
    while (offset < count)
    {
	if (iov[offset].MPL_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPL_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPL_IOV_BUF = 
		(MPL_IOV_BUF_CAST)((char *) iov[offset].MPL_IOV_BUF + nb);
	    iov[offset].MPL_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}
/* end adjust_iov() */


static int ReadMoreData( MPIDI_CH3I_Connection_t * conn, MPIR_Request *rreq )
{
    int mpi_errno = MPI_SUCCESS;
    
    while (1) {
	MPL_IOV * iovp;
	size_t nb;
	
	iovp = rreq->dev.iov;
			    
	mpi_errno = MPIDI_CH3I_Sock_readv(conn->sock, iovp,
				     rreq->dev.iov_count, &nb);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS) {
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
					     rreq, conn, conn->vc);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
		 (MPL_DBG_FDEST,"immediate readv, vc=%p nb=%" PRIdPTR ", rreq=0x%08x",
		  conn->vc, nb, rreq->handle));
				
	if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb)) {
	    int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
	    int complete;
	    
	    reqFn = rreq->dev.OnDataAvail;
	    if (!reqFn) {
		MPIR_Assert(MPIDI_Request_get_type(rreq)!=MPIDI_REQUEST_TYPE_GET_RESP);
                mpi_errno = MPID_Request_complete(rreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
		complete = TRUE;
	    }
	    else {
		mpi_errno = reqFn( conn->vc, rreq, &complete );
		if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	    }
	    
	    if (complete) {
		conn->recv_active = NULL; /* -- already set to NULL */
		mpi_errno = connection_post_recv_pkt(conn);
		if (mpi_errno != MPI_SUCCESS) {
		    MPIR_ERR_POP(mpi_errno);
		}
		
		break;
	    }
	}
	else {
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
        (MPL_DBG_FDEST,"posting readv, vc=%p, rreq=0x%08x",
	 conn->vc, rreq->handle));
	    conn->recv_active = rreq;
	    mpi_errno = MPIDI_CH3I_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) {
		mpi_errno = MPIR_Err_create_code(
		 mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
		"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    break;
	}
    }

 fn_fail:
    return mpi_errno;
}

/*
 * The dynamic-library interface requires a unified Progress routine.
 * This is that routine.
 */
int MPIDI_CH3I_Progress( int blocking, MPID_Progress_state *state )
{
    int mpi_errno;
    if (blocking) mpi_errno = MPIDI_CH3i_Progress_wait(state);
    else          mpi_errno = MPIDI_CH3i_Progress_test();

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_register_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_register_hook(int (*progress_fn)(int*), int *id)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_REGISTER_HOOK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_REGISTER_HOOK);
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (progress_hooks[i].func_ptr == NULL) {
            progress_hooks[i].func_ptr = progress_fn;
            progress_hooks[i].active = FALSE;
            break;
        }
    }

    if (i >= MAX_PROGRESS_HOOKS) {
        return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
				     "MPIDI_CH3I_Progress_register_hook", __LINE__,
				     MPI_ERR_INTERN, "**progresshookstoomany", 0 );
    }

    (*id) = i;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_REGISTER_HOOK);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_deregister_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_deregister_hook(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_DEREGISTER_HOOK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_DEREGISTER_HOOK);
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPIR_Assert(id >= 0 && id < MAX_PROGRESS_HOOKS && progress_hooks[id].func_ptr != NULL);

    progress_hooks[id].func_ptr = NULL;
    progress_hooks[id].active = FALSE;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_DEREGISTER_HOOK);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_activate_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_activate_hook(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_ACTIVATE_HOOK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_ACTIVATE_HOOK);
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPIR_Assert(id >= 0 && id < MAX_PROGRESS_HOOKS &&
                progress_hooks[id].active == FALSE && progress_hooks[id].func_ptr != NULL);
    progress_hooks[id].active = TRUE;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_ACTIVATE_HOOK);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_deactivate_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_deactivate_hook(int id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_DEACTIVATE_HOOK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_DEACTIVATE_HOOK);
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPIR_Assert(id >= 0 && id < MAX_PROGRESS_HOOKS &&
                progress_hooks[id].active == TRUE && progress_hooks[id].func_ptr != NULL);
    progress_hooks[id].active = FALSE;

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_DEACTIVATE_HOOK);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* A convenience dummy symbol so that the PETSc folks can configure test to
 * ensure that they have a working version of MPICH ch3:sock.  Please don't
 * delete it without consulting them. */
int MPIDI_CH3I_sock_fixed_nbc_progress = TRUE;
