/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/**********************************************************************************************************************************
						 BEGIN REQUEST OBJECT MANAGEMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_request_destruct_fn(MPID_Request * req);

MPIG_STATIC const char * mpig_cm_xio_request_state_get_string(mpig_cm_xio_req_state_t req_state);

MPIG_STATIC const char * mpig_cm_xio_request_protocol_get_string(MPID_Request * req);

#define mpig_cm_xio_request_construct(req_)						\
{											\
    (req_)->cmu.xio.state = MPIG_CM_XIO_REQ_STATE_UNDEFINED;				\
    (req_)->cmu.xio.msg_type = MPIG_CM_XIO_MSG_TYPE_UNDEFINED;				\
    (req_)->cmu.xio.cc = 0;								\
    (req_)->cmu.xio.buf_type = MPIG_CM_XIO_APP_BUF_TYPE_UNDEFINED;			\
    (req_)->cmu.xio.stream_pos = 0;							\
    (req_)->cmu.xio.stream_size = 0;							\
    mpig_iov_construct((req_)->cmu.xio.iov, MPIG_CM_XIO_IOV_NUM_ENTRIES);		\
    (req_)->cmu.xio.gcb = (globus_xio_iovec_callback_t) NULL;				\
    (req_)->cmu.xio.sreq_type = MPIG_REQUEST_TYPE_UNDEFINED;				\
    mpig_databuf_construct((req_)->cmu.xio.msgbuf, MPIG_CM_XIO_REQUEST_MSGBUF_SIZE);	\
    (req_)->cmu.xio.df = NEXUS_DC_FORMAT_UNKNOWN;					\
    (req_)->cmu.xio.databuf = NULL;							\
    (req_)->cmu.xio.sendq_next = NULL;							\
											\
    (req_)->dev.cm_destruct = mpig_cm_xio_request_destruct_fn;				\
}

#define mpig_cm_xio_request_destruct(req_)					\
{										\
    (req_)->cmu.xio.state = MPIG_CM_XIO_REQ_STATE_UNDEFINED;			\
    (req_)->cmu.xio.msg_type = MPIG_CM_XIO_MSG_TYPE_UNDEFINED;			\
    (req_)->cmu.xio.cc = -1;							\
    (req_)->cmu.xio.buf_type = MPIG_CM_XIO_APP_BUF_TYPE_UNDEFINED;		\
    (req_)->cmu.xio.stream_pos = 0;						\
    (req_)->cmu.xio.stream_size = 0;						\
    mpig_iov_destruct((req_)->cmu.xio.iov);					\
    (req_)->cmu.xio.gcb = (globus_xio_iovec_callback_t) MPIG_INVALID_PTR;	\
    (req_)->cmu.xio.sreq_type = MPIG_REQUEST_TYPE_UNDEFINED;			\
    mpig_databuf_destruct((req_)->cmu.xio.msgbuf);				\
    (req_)->cmu.xio.df = NEXUS_DC_FORMAT_UNKNOWN;				\
    if ((req_)->cmu.xio.databuf != NULL)					\
    {										\
	mpig_databuf_destroy((req_)->cmu.xio.databuf);				\
	(req_)->cmu.xio.databuf = MPIG_INVALID_PTR;				\
    }										\
    (req_)->cmu.xio.sendq_next = MPIG_INVALID_PTR;				\
}

#define mpig_cm_xio_request_set_cc(req_, cc_)										\
{															\
    (req_)->cmu.xio.cc = (cc_);												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "request - set XIO completion count: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cmu.xio.cc));	\
}

#define mpig_cm_xio_request_inc_cc(req_, was_complete_p_)								\
{															\
    *(was_complete_p_) = !(((req_)->cmu.xio.cc)++);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "request - increment XIO completion count: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cmu.xio.cc));	\
}

#define mpig_cm_xio_request_dec_cc(req_, is_complete_p_)								\
{															\
    *(is_complete_p_) = !(--((req_)->cmu.xio.cc));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "request - decrement XIO completion count: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cmu.xio.cc));	\
}

#define mpig_cm_xio_request_set_state(req_, state_)										\
{																\
    (req_)->cmu.xio.state = (state_);												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set XIO state: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT		\
	", state=%s", (req_)->handle, MPIG_PTR_CAST(req_), mpig_cm_xio_request_state_get_string((req_)->cmu.xio.state)));	\
}

#define mpig_cm_xio_request_get_state(req_) ((req_)->cmu.xio.state)

#define mpig_cm_xio_request_set_msg_type(req_, msg_type_)									\
{																\
    (req_)->cmu.xio.msg_type = (msg_type_);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set XIO message type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", type=%s", (req_)->handle, MPIG_PTR_CAST(req_), mpig_cm_xio_msg_type_get_string((req_)->cmu.xio.msg_type)));		\
}

#define mpig_cm_xio_request_get_msg_type(req_) ((req_)->cmu.xio.msg_type)

#define mpig_cm_xio_request_set_sreq_type(req_, sreq_type_)									\
{																\
    (req_)->cmu.xio.sreq_type = (sreq_type_);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set XIO request sreq type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", type=%s", (req_)->handle, MPIG_PTR_CAST(req_), mpig_request_type_get_string((req_)->cmu.xio.sreq_type)));		\
}

#define mpig_cm_xio_request_get_sreq_type(req_) ((req_)->cmu.xio.sreq_type)

#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_request_destruct_fn([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_destruct_fn
MPIG_STATIC void mpig_cm_xio_request_destruct_fn(MPID_Request * const req)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);

    MPIG_UNUSED_VAR(fcname);

    mpig_cm_xio_request_destruct(req);
}
/* mpig_cm_xio_request_destruct_fn() */


/*
 * char * mpig_cm_xio_request_state_get_string([IN] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_state_get_string
MPIG_STATIC const char * mpig_cm_xio_request_state_get_string(mpig_cm_xio_req_state_t req_state)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * str;

    MPIG_UNUSED_VAR(fcname);

    switch(req_state)
    {
	case MPIG_CM_XIO_REQ_STATE_UNDEFINED:
	    str ="MPIG_CM_XIO_REQ_STATE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_INACTIVE:
	    str ="MPIG_CM_XIO_REQ_STATE_INACTIVE";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_RTS_RECVD_CTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_CTS:
	    str ="MPIG_CM_XIO_REQ_WAIT_RNDV_CTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG:
	    str ="MPIG_CM_XIO_REQ_STATE_CTRL_SEND_MSG";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_SREQ_CANCELLED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_SREQ_CANCELLED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RNDV_WAIT_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE:
	    str ="MPIG_CM_XIO_REQ_STATE_EAGER_RECV_COMPLETE";
	    break;
	default:
	    str = "(unrecognized request state)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_request_state_get_string() */


/*
 * char * mpig_cm_xio_request_protocol_get_string([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_protocol_get_string
MPIG_STATIC const char * mpig_cm_xio_request_protocol_get_string(MPID_Request * req)
{
    const char * str;
    
    switch(req->cmu.xio.msg_type)
    {
	case MPIG_CM_XIO_MSG_TYPE_EAGER_DATA:
	case MPIG_CM_XIO_MSG_TYPE_SSEND_ACK:
	    str ="eager";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_RTS:
	case MPIG_CM_XIO_MSG_TYPE_RNDV_CTS:
	case MPIG_CM_XIO_MSG_TYPE_RNDV_DATA:
	    str ="rndv";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND:
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP:
	    str ="cancel";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_REQ:
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_RESP:
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ:
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_RESP:
	case MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP:
	    str ="open";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CLOSE_PROC:
	    str ="close";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_UNDEFINED:
	case MPIG_CM_XIO_MSG_TYPE_FIRST:
	case MPIG_CM_XIO_MSG_TYPE_LAST:
	default:
	    str ="(invalid message type)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_request_protocol_get_string() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						  END REQUEST OBJECT MANAGEMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN REQUEST COMPLETION QUEUE
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC MPID_Request * mpig_cm_xio_rcq_head = NULL;
MPIG_STATIC MPID_Request * mpig_cm_xio_rcq_tail = NULL;
MPIG_STATIC mpig_mutex_t mpig_cm_xio_rcq_mutex;
MPIG_STATIC mpig_cond_t mpig_cm_xio_rcq_cond;
MPIG_STATIC int mpig_cm_xio_rcq_blocked = FALSE;
MPIG_STATIC int mpig_cm_xio_rcq_wakeup_pending = FALSE;

MPIG_STATIC int mpig_cm_xio_rcq_init(void);

MPIG_STATIC int mpig_cm_xio_rcq_finalize(void);

MPIG_STATIC void mpig_cm_xio_rcq_enq(MPID_Request * req);

MPIG_STATIC int mpig_cm_xio_rcq_deq_wait(MPID_Request ** req);

MPIG_STATIC int mpig_cm_xio_rcq_deq_test(MPID_Request ** req);

MPIG_STATIC void mpig_cm_xio_rcq_wakeup(void);

#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */

/*
 * <mpi_errno> mpig_cm_xio_rcq_init(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_init
MPIG_STATIC int mpig_cm_xio_rcq_init(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    rc = mpig_mutex_construct(&mpig_cm_xio_rcq_mutex);
    MPIU_ERR_CHKANDJUMP((rc), mpi_errno, MPI_ERR_OTHER, "**globus|mutex_init");
    
    rc = mpig_cond_construct(&mpig_cm_xio_rcq_cond);
    MPIU_ERR_CHKANDJUMP((rc), mpi_errno, MPI_ERR_OTHER, "**globus|cond_init");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_init);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_rcq_init() */


/*
 * <mpi_errno> mpig_cm_xio_rcq_finalize(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_finalize
MPIG_STATIC int mpig_cm_xio_rcq_finalize(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    
    rc = mpig_mutex_destruct(&mpig_cm_xio_rcq_mutex);
    MPIU_ERR_CHKANDSTMT((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|mutex_destroy")
    
    rc = mpig_cond_destruct(&mpig_cm_xio_rcq_cond);
    MPIU_ERR_CHKANDSTMT((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cond_destroy")

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_finalize);
    return mpi_errno;
}
/* mpig_cm_xio_rcq_finalize() */


/*
 * <void> mpig_cm_xio_rcq_enq([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_enq
MPIG_STATIC void mpig_cm_xio_rcq_enq(MPID_Request * const req)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_enq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_enq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
	req->handle, MPIG_PTR_CAST(req)));
    
    mpig_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	if (mpig_cm_xio_rcq_head == NULL)
	{
	    mpig_cm_xio_rcq_head = req;
	}
	else
	{
	    mpig_cm_xio_rcq_tail->dev.next = req;
	}
	mpig_cm_xio_rcq_tail = req;

	/* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes made
	   to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	req->dev.next = NULL;

	if (mpig_cm_xio_rcq_blocked)
	{
	    mpig_cm_xio_rcq_wakeup_pending = FALSE;
	    mpig_cond_signal(&mpig_cm_xio_rcq_cond);
	}
    }
    mpig_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, "req enqueued on the completion queue: req="
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, req->handle, MPIG_PTR_CAST(req)));
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
	req->handle, MPIG_PTR_CAST(req)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_enq);
    return;
}
/* mpig_cm_xio_rcq_enq() */


/*
 * <mpi_errno> mpig_cm_xio_rcq_deq_wait([OUT] reqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_deq_wait
MPIG_STATIC int mpig_cm_xio_rcq_deq_wait(MPID_Request ** const reqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_deq_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_deq_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    /* globus_poll_nonblocking(); */
    
    mpig_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	while (mpig_cm_xio_rcq_head == NULL && !mpig_cm_xio_rcq_wakeup_pending)
	{
	    /* XXX: check for registered asynchronous errors.  if any, return them.  should this be done before calling poll()? */
	    
	    mpig_cm_xio_rcq_blocked = TRUE;
	    mpig_cond_wait(&mpig_cm_xio_rcq_cond, &mpig_cm_xio_rcq_mutex);
	    mpig_cm_xio_rcq_blocked = FALSE;
	}
	mpig_cm_xio_rcq_wakeup_pending = FALSE;
	
	req = mpig_cm_xio_rcq_head;
	if (req != NULL)
	{
	    mpig_cm_xio_rcq_head = mpig_cm_xio_rcq_head->dev.next;
	    if (mpig_cm_xio_rcq_head == NULL)
	    {
		mpig_cm_xio_rcq_tail = NULL;
	    }
	    req->dev.next = NULL;
	    
	    /* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes
	       made to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	}
    }
    mpig_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    *reqp = req;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, "req dequeued from the completion queue, req="
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, MPIG_HANDLE_VAL(*reqp), MPIG_PTR_CAST(*reqp)));

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: reqp=" MPIG_PTR_FMT "mpi_errno="
	MPIG_ERRNO_FMT,MPIG_PTR_CAST(req), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_deq_wait);
    return mpi_errno;
}


/*
 * <mpi_errno> mpig_cm_xio_rcq_deq_test([OUT] reqp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_deq_test
MPIG_STATIC int mpig_cm_xio_rcq_deq_test(MPID_Request ** reqp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_deq_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_deq_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    mpig_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	/* XXX: check for registered asynchronous errors.  if any, return them.  should the be done before calling poll()? */
    
	*reqp = mpig_cm_xio_rcq_head;
	
	if (mpig_cm_xio_rcq_head != NULL)
	{
	    mpig_cm_xio_rcq_head = mpig_cm_xio_rcq_head->dev.next;
	    if (mpig_cm_xio_rcq_head == NULL)
	    {
		mpig_cm_xio_rcq_tail = NULL;
	    }

	    /* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes
	       made to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	    (*reqp)->dev.next = NULL;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, "req dequeued from the completion queue, req="
		MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (*reqp)->handle, MPIG_PTR_CAST(*reqp)));
	}

	mpig_cm_xio_rcq_wakeup_pending = FALSE;
    }
    mpig_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: reqp=" MPIG_PTR_FMT "mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(*reqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_deq_test);
    return mpi_errno;
}
/* mpig_cm_xio_rcq_deq_test() */


/*
 * void mpig_cm_xio_rcq_wakeup([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_wakeup
MPIG_STATIC void mpig_cm_xio_rcq_wakeup(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_wakeup);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_wakeup);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    mpig_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	mpig_cm_xio_rcq_wakeup_pending = TRUE;
	if (mpig_cm_xio_rcq_blocked)
	{
	    mpig_cond_signal(&mpig_cm_xio_rcq_cond);
	}
    }
    mpig_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_wakeup);
    return;
}
/* mpig_cm_xio_rcq_wakeup() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						 END REQUEST COMPLETION QUEUE
**********************************************************************************************************************************/
