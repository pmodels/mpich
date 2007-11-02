/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */


/**********************************************************************************************************************************
						BEGIN COMMUNICATION HELPER MACROS
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

#define mpig_cm_xio_register_write_vc_msgbuf(vc_, gcb_, mpi_errno_p_)								\
{																\
    globus_result_t grc__;													\
    globus_xio_handle_t handle__ = (vc_)->cmu.xio.handle;									\
    globus_byte_t * buf__ = (globus_byte_t *) mpig_databuf_get_base_ptr((vc_)->cmu.xio.msgbuf);					\
    globus_size_t nbytes__= (globus_size_t) mpig_databuf_get_eod((vc_)->cmu.xio.msgbuf);					\
																\
    *(mpi_errno_p_) = MPI_SUCCESS;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "register write: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", buf="		\
	MPIG_PTR_FMT ", nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc_), MPIG_PTR_CAST(handle__),					\
    MPIG_PTR_CAST(buf__), nbytes__));												\
    grc__ = globus_xio_register_write(handle__, buf__, nbytes__, nbytes__, NULL, (gcb_), (void *) (vc_));			\
    if (grc__)															\
    {   /* --BEGIN ERROR HANDLING-- */												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT	\
	    ", msg=\"%s\"", "globus_xio_register_write", MPIG_PTR_CAST(vc_), mpig_get_globus_error_msg(grc__)));		\
	MPIU_ERR_SET1(*(mpi_errno_p_), MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_write",						\
	    "**globus|cm_xio|xio_reg_write %s", mpig_get_globus_error_msg(grc__));						\
    }   /* --END ERROR HANDLING-- */												\
}

#define mpig_cm_xio_register_write_sreq(vc_, sreq_, mpi_errno_p_)								\
{																\
    globus_result_t grc__;													\
    globus_xio_handle_t handle__ = (vc_)->cmu.xio.handle;									\
    globus_xio_iovec_t * iov__ = (globus_xio_iovec_t *) mpig_iov_get_current_entry_ptr((sreq_)->cmu.xio.iov);			\
    int iov_cnt__ = mpig_iov_get_num_inuse_entries((sreq_)->cmu.xio.iov);							\
    globus_size_t nbytes__ = (globus_size_t) mpig_iov_get_num_bytes((sreq_)->cmu.xio.iov);					\
																\
    *(mpi_errno_p_) = MPI_SUCCESS;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "register writev: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="		\
	MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc_), (sreq_)->handle,			\
	MPIG_PTR_CAST(sreq_), MPIG_PTR_CAST(handle__), nbytes__));								\
    grc__ = globus_xio_register_writev(handle__, iov__, iov_cnt__, nbytes__,  NULL, (sreq_)->cmu.xio.gcb, (void *) (vc_));	\
    if (grc__)															\
    {   /* --BEGIN ERROR HANDLING-- */												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT	\
	    ", msg=\"%s\"", "globus_xio_register_writev", MPIG_PTR_CAST(vc_), mpig_get_globus_error_msg(grc__)));		\
	MPIU_ERR_SET1(*(mpi_errno_p_), MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_writev",						\
	    "**globus|cm_xio|xio_reg_writev %s", mpig_get_globus_error_msg(grc__));						\
    }   /* --END ERROR HANDLING-- */												\
}

#define mpig_cm_xio_register_read_vc_msgbuf(vc_, nbytes_min_, nbytes_max_, gcb_, mpi_errno_p_)					\
{																\
    globus_result_t grc__;													\
    globus_xio_handle_t handle__ = (vc_)->cmu.xio.handle;									\
    globus_byte_t * buf__ = (globus_byte_t *) mpig_databuf_get_eod_ptr((vc_)->cmu.xio.msgbuf);					\
																\
    *(mpi_errno_p_) = MPI_SUCCESS;												\
    MPIU_Assert((nbytes_max_) <= mpig_databuf_get_free_bytes((vc_)->cmu.xio.msgbuf));						\
    MPIU_Assert((nbytes_min_) <= (nbytes_max_));										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "register read: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", buf="		\
	MPIG_PTR_FMT ", min=" MPIG_SIZE_FMT ", max=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc_), MPIG_PTR_CAST(handle__),		\
	MPIG_PTR_CAST(buf__), (nbytes_min_), (nbytes_max_)));									\
    grc__ = globus_xio_register_read(handle__, buf__, (nbytes_max_), (nbytes_min_), NULL, (gcb_), (void *) (vc_));		\
    if (grc__)															\
    {   /* --BEGIN ERROR HANDLING-- */												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT	\
	    ", msg=\"%s\"", "globus_xio_register_read", MPIG_PTR_CAST(vc_),mpig_get_globus_error_msg(grc__)));			\
	MPIU_ERR_SET1(*(mpi_errno_p_), MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_read",						\
	"**globus|cm_xio|xio_reg_read %s", mpig_get_globus_error_msg(grc__));							\
    }   /* --END ERROR HANDLING-- */												\
}

#define mpig_cm_xio_register_read_rreq(vc_, rreq_, mpi_errno_p_)								\
{																\
    globus_result_t grc__;													\
    globus_xio_handle_t handle__ = (vc_)->cmu.xio.handle;									\
    globus_xio_iovec_t * iov__ = (globus_xio_iovec_t *) mpig_iov_get_current_entry_ptr((rreq_)->cmu.xio.iov);			\
    int iov_cnt__ = mpig_iov_get_num_inuse_entries((rreq_)->cmu.xio.iov);							\
    globus_size_t nbytes__ = (globus_size_t) mpig_iov_get_num_bytes((rreq_)->cmu.xio.iov);					\
																\
    *(mpi_errno_p_) = MPI_SUCCESS;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "register writev: vc=" MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT ", rreqp="		\
	MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc_), (rreq_)->handle,			\
	MPIG_PTR_CAST(rreq_), MPIG_PTR_CAST(handle__), nbytes__));								\
    grc__ = globus_xio_register_readv(handle__, iov__, iov_cnt__, nbytes__, NULL, (rreq_)->cmu.xio.gcb, (void *) (vc_));	\
    if (grc__)															\
    {   /* --BEGIN ERROR HANDLING-- */												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT	\
	    ", msg=\"%s\"", "globus_xio_register_readv", MPIG_PTR_CAST(vc_), mpig_get_globus_error_msg(grc__)));		\
	MPIU_ERR_SET1(*(mpi_errno_p_), MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_readv",						\
	    "**globus|cm_xio|xio_reg_readv %s", mpig_get_globus_error_msg(grc__));						\
    }   /* --END ERROR HANDLING-- */												\
}

#endif
/**********************************************************************************************************************************
						 END COMMUNICATION HELPER MACROS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN SEND MESSAGE HEADERS AND DATA
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC int mpig_cm_xio_send_enq_sreq(mpig_vc_t * vc, MPID_Request * sreq);

MPIG_STATIC int mpig_cm_xio_send_next_sreq(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_send_enq_isend(mpig_vc_t * vc, MPID_Request * sreq);

MPIG_STATIC int mpig_cm_xio_send_enq_rndv_cts_msg( mpig_vc_t * vc, MPI_Request rreq_id, MPI_Request sreq_id);

MPIG_STATIC int mpig_cm_xio_send_enq_rndv_data_msg(mpig_vc_t * vc, MPID_Request * sreq, MPI_Request rreq_id);

MPIG_STATIC int mpig_cm_xio_send_enq_ssend_ack_msg(mpig_vc_t * vc, MPI_Request sreq_id);

MPIG_STATIC int mpig_cm_xio_send_enq_cancel_send_msg(mpig_vc_t * vc, int rank, int tag, int ctx, MPI_Request sreq_id);

MPIG_STATIC int mpig_cm_xio_send_enq_cancel_send_resp_msg(mpig_vc_t * vc, MPI_Request sreq_id, bool_t cancelled);

MPIG_STATIC void mpig_cm_xio_send_handle_write_msg_data(globus_xio_handle_t handle, globus_result_t op_grc,
    globus_xio_iovec_t * iov, int iov_cnt, globus_size_t nbytes_written, globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_send_handle_write_control_msg(globus_xio_handle_t handle, globus_result_t op_grc,
    globus_xio_iovec_t * iov, int iov_cnt, globus_size_t nbytes_written, globus_xio_data_descriptor_t dd, void * arg);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_sreq([IN/MOD] vc, [IN/MOD] sreq)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the send request is to be enqueued
 *
 * sreq [IN/MOD] - send request to be enqueued
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_sreq
MPIG_STATIC int mpig_cm_xio_send_enq_sreq(mpig_vc_t * const vc, MPID_Request * const sreq)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_sreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_sreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
    
    if (mpig_cm_xio_vc_is_connected(vc))
    {
	/* a connection to the remote process exists.  enqueue the send request.  if no other request is actively being sent,
	   then start sending the request at the head of the queue. */
	mpig_cm_xio_sendq_enq_tail(vc, sreq);
	if (vc->cmu.xio.active_sreq == NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC is connected and send engine is inactive; starting send; vc="
		MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle,
		MPIG_PTR_CAST(sreq)));
		    
	    MPIU_Assert(mpig_cm_xio_sendq_head(vc) == sreq);
	    mpi_errno = mpig_cm_xio_send_next_sreq(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC is connected but send engine is active; leaving request; vc="
		MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle,
		MPIG_PTR_CAST(sreq)));
	}
    }
    else if (mpig_cm_xio_vc_is_connecting(vc))
    {
	/* a connection to the remote process is being established.  enqueue the send request.  it will be sent once the
	   connection establishment is complete. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC connecting; enqueuing request; vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	    ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
	mpig_cm_xio_sendq_enq_tail(vc, sreq);
    }
    else if (mpig_cm_xio_vc_is_disconnecting(vc))
    {
	/* the connection associated with this VC is in the process of disconnecting.  enqueue the request.  if no other request
	   is actively being sent, the start sending the request at the head of the queue.  this may seem strange since the state
	   class is called "disconnecting"; however, communication still needs to proceed to perform the disconnect protocol.
	   once it is no longer safe to send messages, the VC state is switch to the "closing" class of states, and messages are
	   enqueued.  see below. */
	mpig_cm_xio_sendq_enq_tail(vc, sreq);
	if (vc->cmu.xio.active_sreq == NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC is disconnecting and send engine is inactive; starting send; vc="
		MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle,
		MPIG_PTR_CAST(sreq)));
		    
	    MPIU_Assert(mpig_cm_xio_sendq_head(vc) == sreq);
	    mpi_errno = mpig_cm_xio_send_next_sreq(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC is disconnecting but send engine is active; leaving request; vc="
		MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle,
		MPIG_PTR_CAST(sreq)));
	}
    }
    else if (mpig_cm_xio_vc_is_closing(vc))
    {
	/* the connection associated with this VC is in the process of closing.  enqueue the request.  it will be sent
	   after the disconnect completes and a new connection is formed. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC is closing; send state machine suspended; enqueuing request; vc="
	    MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="  MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle,
	    MPIG_PTR_CAST(sreq)));
	mpig_cm_xio_sendq_enq_tail(vc, sreq);
    }
    else if (mpig_cm_xio_vc_is_unconnected(vc))
    {
	/* no connection exists for this VC or the connecting is being closed.  enqueue the send request so that it may be sent
	   after a new connection is established */
	mpig_cm_xio_sendq_enq_tail(vc, sreq);

	if (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_UNCONNECTED)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC unconnected; enqueuing request and starting connect; vc=" MPIG_PTR_FMT
		", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
	    mpi_errno = mpig_cm_xio_client_connect_proc(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_connect_proc");
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "VC closing connection; enqueuing request: vc=" MPIG_PTR_FMT ", sreq="
		MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
	}
    }
    else if (mpig_cm_xio_vc_has_failed(vc))
    {
	/* connection failed.  mark request as failed and complete. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "VC in failed state; return error; vc=" MPIG_PTR_FMT
	    ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));

	/* MT-RC-NOTE: the mutex lock/unlock in the RCQ routines will insure that the sreq data is released and acquired */
	MPIU_ERR_SETANDSTMT3(sreq->status.MPI_ERROR, MPI_ERR_OTHER, {;}, "**globus|cm_xio|vc_connection",
	    "**globus|cm_xio|vc_connection %s %d %s", mpig_vc_get_pg_id(vc), mpig_vc_get_pg_rank(vc),
	    mpig_cm_xio_vc_get_contact_string(vc));
	mpig_cm_xio_request_set_cc(sreq, 0);
	mpig_cm_xio_rcq_enq(sreq);
    }
    else if (mpig_cm_xio_vc_is_undefined(vc))
    {   /* --BEGIN ERROR HANDLING-- */

	/* VC is unitialized.  this should never happen! */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: VC state is uninitialized; vc=" MPIG_PTR_FMT,
	    MPIG_PTR_CAST(vc)));
	MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|vc_uninit", "**globus|cm_xio|vc_uninit %p", vc);
    }   /* --END ERROR HANDLING-- */
    else
    {   /* --BEGIN ERROR HANDLING-- */
	/* VC state is not valid.  this should never happen! */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: VC state is corrupt; vc=" MPIG_PTR_FMT ", state=%d",
	    MPIG_PTR_CAST(vc), (int) mpig_cm_xio_vc_get_state(vc)));
	MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|vc_corrupt", "**globus|cm_xio|vc_corrupt %p", vc);
    }   /* --END ERROR HANDLING-- */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_sreq);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_sreq() */


/*
 * <mpi_errno> mpig_cm_xio_send_next_sreq([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object containing the connection over which to start sending the next enqueued send request
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is lccked is held by the current context.
 *
 * MT-RC-NOTE: on machines with release consistent memory semantics, the earlier lock of the VC mutex will force an previous
 * changes made to the queued send requests to be acquired.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_next_sreq
MPIG_STATIC int mpig_cm_xio_send_next_sreq(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_next_sreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_next_sreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    MPIU_Assert(vc_cmu->active_sreq == NULL);
    
    /* start sending any data in the send queue */
    mpig_cm_xio_sendq_deq(vc, &sreq);
    if (sreq != NULL)
    {
	vc_cmu->active_sreq = sreq;
	
	/* TODO: try an immediate send first; only queue if necessary */

	mpig_cm_xio_register_write_sreq(vc, sreq, &mpi_errno);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_write_sreq");
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_next_sreq);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_next_sreq() */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_isend([IN/MOD] vc, [IN/MOD] sreq)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the application's nonblocking send request is to be enqueued
 *
 * sreq [IN/MOD] - the request to be sent
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_isend
MPIG_STATIC int mpig_cm_xio_send_enq_isend(mpig_vc_t * const vc, MPID_Request * const sreq)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request_cmu * sreq_cmu = &sreq->cmu.xio;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
	sreq->handle, MPIG_PTR_CAST(sreq)));

    /* allocate space for the header before packing data by setting the initial iovec count to one */
    mpig_iov_reset(sreq_cmu->iov, 1);

    /* pack the data (or generate and I/O vector describing the application buffer).  the header includes the amount of data
       being sent with this message.  in the case of a rendezvous request-to-send message, the exact count cannot be determined
       before the data is packed by the dataloop engine, so the data must be packed before the header is generated. */
    mpi_errno = mpig_cm_xio_stream_sreq_init(sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_sreq_init");

    if (mpig_request_get_type(sreq) != MPIG_REQUEST_TYPE_RSEND)
    {
	mpig_cm_xio_stream_set_max_pos(sreq, MPIG_CM_XIO_EAGER_MAX_SIZE);
    }
    mpi_errno = mpig_cm_xio_stream_sreq_pack(sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_sreq_pack");
    
    if (mpig_cm_xio_stream_get_size(sreq) == 0)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "sending zero-byte message; vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	    ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "sending data: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
	    MPIG_PTR_FMT ", nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq),
	    mpig_iov_get_num_bytes(sreq_cmu->iov)));
    }

    /* set state and pack the message header according to the protocol being used */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    if (mpig_cm_xio_stream_get_size(sreq) <= MPIG_CM_XIO_EAGER_MAX_SIZE || mpig_request_get_type(sreq) == MPIG_REQUEST_TYPE_RSEND)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "eager protocol selected: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
	    MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));

	mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_DATA);
	mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_EAGER_DATA);
    }
    else
    {
	/* Rendezvous request-to-send includes some data to help hide the overhead of the RTS/CTS handshake */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "rndv protocol selected: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
	    MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));

	/*
	 * NOTE: the size of the RTS message payload is determined by the first call to mpig_cm_xio_stream_sreq_pack().  as a
	 * result, the size of the payload is limited to the minimum of the following.
	 *
	 *    (a) MPIG_CM_XIO_EAGER_MAX_SIZE
	 *    (b) MPIG_CM_XIO_IOV_NUM_ENTRIES * MPIG_CM_XIO_DATA_DENSITY_THRESHOLD
	 *    (c) MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE
	 *
	 * To increase the maximum RTS payload size to always be MPIG_CM_XIO_EAGER_MAX would require another routine that could
	 * predetermine the size of the RTS payload and then reset the segment.  This routine could use MPID_Segment_pack(), but
	 * it would be more efficient to write a customized segment routine specifically for this purpose.
	 */
	mpig_cm_xio_stream_set_max_pos(sreq, mpig_iov_get_num_bytes(sreq_cmu->iov));
	
	mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS);
	mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
    }
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_type(vc, sreq_cmu->msgbuf, mpig_request_get_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, sreq->handle);
    {
	int rank;
	int tag;
	int ctx;

	mpig_request_get_envelope(sreq, &rank, &tag, &ctx);
	mpig_cm_xio_msg_hdr_put_envelope(vc, sreq_cmu->msgbuf, sreq->comm->rank, tag, ctx);
    }
    /* XXX: still need to handle, and thus identify, MPI_PACKED messages since they will have a special header */
    if (mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS)
    {
	mpig_cm_xio_msg_hdr_put_data_size(vc, sreq_cmu->msgbuf, mpig_iov_get_num_bytes(sreq_cmu->iov));
    }
    mpig_cm_xio_msg_hdr_put_data_size(vc, sreq_cmu->msgbuf, mpig_cm_xio_stream_get_size(sreq));
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);
    
    /* iovec count was set to one earlier to leave room for the header, so we put the message header in the first iovec entry
       instead of adding it to the end */
    mpig_iov_set_entry(sreq_cmu->iov, 0, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* set the completion handler and add the send request to the VC's send queue */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_msg_data;
    mpig_vc_mutex_lock(vc);
    {
	mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    }
    mpig_vc_mutex_unlock(vc);
    /* MT-RC-NOTE: the unlock of the VC mutex will cause changes made to the sreq to be released as well */
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
 
    /* add to usage bytes sent counter */
    MPIG_USAGE_INC_NBYTES_SENT(mpig_cm_xio_stream_get_size(sreq));
       
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_isend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_isend() */


/*
 * <mpi_errno> voidmpig_cm_xio_send_enq_rndv_cts_msg([IN/MOD] vc, [IN] local_rreq_id, [IN] remote_sreq_id)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the clear-to-send message is to be enqueued
 *
 * local_rreq_id [IN] - handle of local receive request waiting to receive data
 *
 * remote_sreq_id [IN] - handle of remote send request waiting to send data
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_rndv_cts_msg
MPIG_STATIC int mpig_cm_xio_send_enq_rndv_cts_msg(mpig_vc_t * const vc, const MPI_Request local_rreq_id,
    const MPI_Request remote_sreq_id)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request_cmu * sreq_cmu = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_rndv_cts_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_rndv_cts_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", local_rreq_id="
	MPIG_HANDLE_FMT ", remote_sreq_id=" MPIG_HANDLE_FMT, MPIG_PTR_CAST(vc), local_rreq_id, remote_sreq_id));

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
	    "ERROR: malloc failed when attempting to allocate a send request for a rendezvous CTS message"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for rndv cts msg");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    sreq_cmu = &sreq->cmu.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
			    MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_cc(sreq, 1);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_RNDV_CTS);
    
    /* pack the message header */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, remote_sreq_id);
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, local_rreq_id);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);

    mpig_iov_reset(sreq_cmu->iov, 0);
    mpig_iov_add_entry(sreq_cmu->iov, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* MT-RC-NOTE: the request data will be released with the unlock of the VC mutex performed later by one of the routines on
       the call stack. */

    /* send the rendezvous clear-to-send message */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_control_msg;
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT ", local_rreq_id="
	MPIG_HANDLE_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc),
	local_rreq_id, MPIG_HANDLE_VAL(sreq), MPIG_PTR_CAST(sreq)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_rndv_cts_msg);
    return mpi_errno;;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_rndv_cts_msg() */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_rndv_data_msg([IN/MOD] vc, [IN/MOD] sreq)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the rendezvous data message is to be enqueued
 *
 * sreq [IN/MOD] - send request associated with the whole rendezvous message (created when the RTS was sent)
 *
 * remote_rreq_id [IN] - handle of remote receive request waiting to receive data
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 *
 * MT-NOTE: this routine assumes that the send request's mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_rndv_data_msg
MPIG_STATIC int mpig_cm_xio_send_enq_rndv_data_msg(mpig_vc_t * const vc, MPID_Request * const sreq,
    const MPI_Request remote_rreq_id)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request_cmu * sreq_cmu = &sreq->cmu.xio;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_rndv_data_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_rndv_data_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", remote_rreq_id=" MPIG_HANDLE_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq),
	remote_rreq_id));

    /* pack the message header */
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_DATA);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_RNDV_DATA);
    
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, remote_rreq_id);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);

    mpig_iov_reset(sreq_cmu->iov, 0);
    mpig_iov_add_entry(sreq_cmu->iov, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* pack the first (and possible final) set of data */
    mpig_cm_xio_stream_set_max_pos(sreq, 0);
    mpi_errno = mpig_cm_xio_stream_sreq_pack(sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_sreq_pack");
    MPIU_Assert(mpig_iov_get_num_bytes(sreq_cmu->iov) > 0);

    /* send the rendezvous data message */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_msg_data;
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_rndv_data_msg);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_rndv_data_msg() */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_ssend_ack_msg([IN/MOD] vc, [IN] remote_sreq_id)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection on which the synchronous send acknowledgement message is to be enqueued
 *
 * remote_sreq_id [IN] - handle of remote send request to be acknowledge
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_ssend_ack_msg
MPIG_STATIC int mpig_cm_xio_send_enq_ssend_ack_msg(mpig_vc_t * const vc, const MPI_Request remote_sreq_id)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request_cmu * sreq_cmu = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_ssend_ack_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_ssend_ack_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", remote_sreq="
	MPIG_HANDLE_FMT, MPIG_PTR_CAST(vc), remote_sreq_id));

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
	    "ERROR: malloc failed when attempting to allocate a send request for a ssend ACK message"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for ssend ack msg");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    sreq_cmu = &sreq->cmu.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
	MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_SSEND_ACK);
    
    /* pack the message header */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, remote_sreq_id);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);

    mpig_iov_reset(sreq_cmu->iov, 0);
    mpig_iov_add_entry(sreq_cmu->iov, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* MT-RC-NOTE: the request data will be released with the unlock of the VC mutex performed later by one of the routines on
       the call stack. */

    /* send the ssend acknowledgement message */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_control_msg;
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: remote_sreq_id=" MPIG_HANDLE_FMT ", sreq="
	MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, remote_sreq_id, MPIG_HANDLE_VAL(sreq),
	MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_ssend_ack_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_ssend_ack_msg() */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_cancel_send_msg([IN/MOD] vc, [IN] remote_sreq_id)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the synchronous send acknowledgement message is to be enqueued
 *
 * remote_sreq_id [IN] - handle of remote send request
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_cancel_send_msg
MPIG_STATIC int mpig_cm_xio_send_enq_cancel_send_msg(mpig_vc_t * const vc, const int rank, const int tag, const int ctx,
    const MPI_Request remote_sreq_id)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request_cmu * sreq_cmu = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", remote_sreq="
	MPIG_HANDLE_FMT, MPIG_PTR_CAST(vc), remote_sreq_id));

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
	    "ERROR: malloc failed when attempting to allocate a send request for a cancel send request message"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for cancel send req msg");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    sreq_cmu = &sreq->cmu.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
	MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND);
    
    /* pack the message header */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_envelope(vc, sreq_cmu->msgbuf, rank, tag, ctx);
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, remote_sreq_id);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);

    mpig_iov_reset(sreq_cmu->iov, 0);
    mpig_iov_add_entry(sreq_cmu->iov, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* MT-RC-NOTE: the request data will be released with the unlock of the VC mutex performed later by one of the routines on
       the call stack. */

    /* send the cancel send message */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_control_msg;
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: remote_sreq_id=" MPIG_HANDLE_FMT ", sreq="
	MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, remote_sreq_id, MPIG_HANDLE_VAL(sreq),
	MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_cancel_send_msg() */


/*
 * <mpi_errno> mpig_cm_xio_send_enq_cancel_send_resp_msg([IN/MOD] vc, [IN] remote_sreq_id)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object on which the synchronous send acknowledgement messsage is to be enqueued
 *
 * remote_sreq_id [IN] - handle of remote send request
 *
 * cancelled [IN] - TRUE if the incoming message was successfully squashed; FALSE otherwise
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_enq_cancel_send_resp_msg
MPIG_STATIC int mpig_cm_xio_send_enq_cancel_send_resp_msg(mpig_vc_t * const vc, const MPI_Request remote_sreq_id,
    const bool_t cancelled)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request_cmu * sreq_cmu = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_resp_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_resp_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", remote_sreq="
	MPIG_HANDLE_FMT ", cancelled=%s", MPIG_PTR_CAST(vc), remote_sreq_id, MPIG_BOOL_STR(cancelled)));

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
	    "ERROR: malloc failed when attempting to allocate a send request for a cancel send response message"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for cancel send resp msg");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    sreq_cmu = &sreq->cmu.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
	MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP);
    
    /* pack the message header */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cmu->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cmu->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_req_id(vc, sreq_cmu->msgbuf, remote_sreq_id);
    mpig_cm_xio_msg_hdr_put_bool(vc, sreq_cmu->msgbuf, cancelled);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cmu->msgbuf);

    mpig_iov_reset(sreq_cmu->iov, 0);
    mpig_iov_add_entry(sreq_cmu->iov, mpig_databuf_get_base_ptr(sreq_cmu->msgbuf), mpig_databuf_get_eod(sreq_cmu->msgbuf));

    /* MT-RC-NOTE: the request data will be released with the unlock of the VC mutex performed later by one of the routines on
       the call stack. */

    /* send the cancel send message */
    sreq_cmu->gcb = mpig_cm_xio_send_handle_write_control_msg;
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: remote_sreq_id=" MPIG_HANDLE_FMT ", sreq="
	MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, remote_sreq_id, MPIG_HANDLE_VAL(sreq),
	MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_enq_cancel_send_resp_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_enq_cancel_send_resp_msg() */


/*
 * <void> mpig_cm_xio_send_handle_write_msg_data([IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes_written, [IN] dd,
 *     [IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_handle_write_msg_data
MPIG_STATIC void mpig_cm_xio_send_handle_write_msg_data(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_xio_iovec_t * const iov, const int iov_cnt, const globus_size_t nbytes_written, const globus_xio_data_descriptor_t dd,
    void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc_cmu * const vc_cmu = &vc->cmu.xio;
    bool_t vc_locked = FALSE;
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request_cmu * sreq_cmu = NULL;
    bool_t sreq_locked = FALSE;
    bool_t sreq_complete = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_handle_write_msg_data);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_handle_write_msg_data);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, iov=" MPIG_PTR_FMT ", iov_cnt=%d, nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle),
	op_grc, MPIG_PTR_CAST(iov), iov_cnt, nbytes_written));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	sreq = vc_cmu->active_sreq;
	sreq_cmu = &sreq->cmu.xio;
	
	mpig_request_mutex_lock(sreq);
	sreq_locked = TRUE;
	{
	    if (op_grc)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		    ", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle),
		    globus_error_print_chain(globus_error_peek(op_grc))));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_handle_msg_data",
		    "**globus|cm_xio|send_handle_write_msg_data %s", globus_error_print_chain(globus_error_peek(op_grc)));
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
    
	    MPIU_Assert(nbytes_written == mpig_iov_get_num_bytes(sreq_cmu->iov));
	    MPIU_Assert(
		(mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_DATA &&
		    (mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA ||
			mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_DATA)) ||
		((mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS ||
		    mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS) &&
		    mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS));

	    /* get the next set of data to send */
	    mpig_iov_reset(sreq_cmu->iov, 0);
	    mpi_errno = mpig_cm_xio_stream_sreq_pack(sreq);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_sreq_pack");

	    if (mpig_iov_get_num_bytes(sreq_cmu->iov) == 0)
	    {
		/* if all of the message has been sent, then start sending the the next message on the send queue and update the
		   state of the current request */
		
		/* if the send queue contains any messages, then start sending the one at the head of the queue */
		vc_cmu->active_sreq = NULL;
		mpi_errno = mpig_cm_xio_send_next_sreq(vc);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
		
		if (mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS)
		{
		    /* if the message was a rendezvous RTS message... */
		    
		    if (mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS)
		    {
			/* if a clear-to-send message arrived while the request-to-send was still in progress, then we need to
			   start transfering the remaining data */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished sending rts message; cts received; sending remaining "
			    "data: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc),
			    sreq->handle, MPIG_PTR_CAST(sreq)));
			
			/* MT-NOTE: the enq routine assumes the send request's mutex is being held by the current context */
			mpi_errno = mpig_cm_xio_send_enq_rndv_data_msg(vc, sreq, mpig_request_get_remote_req_id(sreq));
			MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_rndv_data_msg");
		    }
		    else
		    {
			/* otherwise, we are still waiting for the clear-to-send message, so change the state of the request to
			   waiting for the CTS message. */
			MPIU_Assert(mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS);
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished sending rts message; waiting for cts to arrive: "
			    "vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc),
			    sreq->handle, MPIG_PTR_CAST(sreq)));
			
			mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_CTS);
		    }
		}
		else
		{
		    /* otherwise, an eager or rendezvous message just finished being sent, so change the request's state to
		       complete and decrement the cm_xio completion counter */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished sending %s message: vc=" MPIG_PTR_FMT ", sreq="
			MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, mpig_cm_xio_request_protocol_get_string(sreq), MPIG_PTR_CAST(vc),
			sreq->handle, MPIG_PTR_CAST(sreq)));
		
		    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE);
		    mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
		}
	    }
	    else
	    {
		/* otherwise, register an XIO write operation to send the next set of data */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "sending more %s data: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
		    ", sreqp=" MPIG_PTR_FMT, mpig_cm_xio_request_protocol_get_string(sreq), MPIG_PTR_CAST(vc), sreq->handle,
		    MPIG_PTR_CAST(sreq)));
	    
		mpig_cm_xio_register_write_sreq(vc, sreq, &mpi_errno);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_write_sreq");
	    }
	}
	mpig_request_mutex_unlock(sreq);
	sreq_locked = FALSE;

    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

  fn_return:
    {
	/* if all tasks associated with the request have completed, then added it to the completion queue */
	if (sreq_complete) mpig_cm_xio_rcq_enq(sreq);
    
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	    ", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq),
	    mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_handle_write_msg_data);
	return;
    }
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpig_request_mutex_unlock_conditional(sreq, (sreq_locked));
	/* XXX: attach an error to the request and add it to the completion queue */

	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_proc_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);
	
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_handle_write_msg_data() */


/*
 * void mpig_cm_xio_send_handle_write_control_msg([IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes_written, [IN] dd,
 *     [IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_handle_write_control_msg
MPIG_STATIC void mpig_cm_xio_send_handle_write_control_msg(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_xio_iovec_t * const iov, const int iov_cnt, const globus_size_t nbytes_written, const globus_xio_data_descriptor_t dd,
    void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    bool_t vc_locked = FALSE;
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_handle_write_control_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_handle_write_control_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, iov=" MPIG_PTR_FMT ", iov_cnt=%d, nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle),
	op_grc, MPIG_PTR_CAST(iov), iov_cnt, nbytes_written));

    /* get the send request from the VC; start sending the next message, if one is present */
    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	sreq = vc_cmu->active_sreq;
	mpig_request_destroy(sreq);
	vc_cmu->active_sreq = NULL;
    
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle),
		globus_error_print_chain(globus_error_peek(op_grc))));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_handle_write_control_msg",
		"**globus|cm_xio|send_handle_write_contro_msg %s", globus_error_print_chain(globus_error_peek(op_grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpi_errno = mpig_cm_xio_send_next_sreq(vc);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send control message completed: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq)));
	
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq),
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_handle_write_control_msg);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_proc_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_send_handle_write_control_msg() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						END SEND MESSAGE HEADERS AND DATA
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					     BEGIN RECEIVE MESSAGE HEADERS AND DATA
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

typedef int (*mpig_cm_xio_msghan_fn_t)(mpig_vc_t * vc);


MPIG_STATIC int mpig_cm_xio_recv_next_msg(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_recv_handle_incoming_msg(globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf,
    globus_size_t rbuf_len, globus_size_t nbytes_read, globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC int mpig_cm_xio_recv_handle_eager_data_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_rts_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_cts_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_data_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_ssend_ack_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_cancel_send_msg(mpig_vc_t * vc);

MPIG_STATIC int mpig_cm_xio_recv_handle_cancel_send_resp_msg(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_recv_handle_read_msg_data(globus_xio_handle_t handle, globus_result_t op_grc,
    globus_xio_iovec_t * iov, int iov_cnt, globus_size_t nbytes_read, globus_xio_data_descriptor_t dd, void * arg);


/* table message reception handler functions */
MPIG_STATIC mpig_cm_xio_msghan_fn_t mpig_cm_xio_msghan_funcs[] =
{
    NULL, /* MPIG_CM_XIO_MSG_TYPE_FIRST */
    NULL, /* MPIG_CM_XIO_MSG_TYPE_UNDEFINED */
    mpig_cm_xio_recv_handle_eager_data_msg,
    mpig_cm_xio_recv_handle_rndv_rts_msg,
    mpig_cm_xio_recv_handle_rndv_cts_msg,
    mpig_cm_xio_recv_handle_rndv_data_msg,
    mpig_cm_xio_recv_handle_ssend_ack_msg,
    mpig_cm_xio_recv_handle_cancel_send_msg,
    mpig_cm_xio_recv_handle_cancel_send_resp_msg,
    NULL, /* MPIG_CM_XIO_MSG_TYPE_OPEN_VC_REQ */
    NULL, /* MPIG_CM_XIO_MSG_TYPE_OPEN_VC_RESP */
    NULL, /* MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ */
    NULL, /* MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_RESP */
    NULL, /* MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP */
    mpig_cm_xio_disconnect_handle_recv_close_proc_msg,
    NULL  /* MPIG_CM_XIO_MSG_TYPE_LAST */
};


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_recv_next_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object containing the connection over which to start receiving the next message
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_next_msg
MPIG_STATIC int mpig_cm_xio_recv_next_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_next_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_next_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
    
    /* If the routine is being called from a routine that handles message headers, then return; otherwise register a read for the
       next message header.  mpig_cm_xio_recv_handle_incoming_msg() will process the message header, registering a new receive
       when necessary. */
    if (vc_cmu->msg_hdr_size == 0)
    {
	MPIU_Size_t min_nbytes;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "registering receive for new message: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
	    
	MPIU_Assert(vc_cmu->active_rreq == NULL);

	min_nbytes = (mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) < mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc)) ?
	    mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc) - mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) : 0;
	
	mpig_cm_xio_register_read_vc_msgbuf(vc, min_nbytes, mpig_databuf_get_free_bytes(vc_cmu->msgbuf),
					    mpig_cm_xio_recv_handle_incoming_msg, &mpi_errno);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_vc_msgbuf");
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "in message handler; skipping register recv: vc=" MPIG_PTR_FMT,
	    MPIG_PTR_CAST(vc)));
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_next_msg);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_next_msg() */


/*
 * <void> mpig_cm_xio_recv_handle_incoming_msg([IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes_read, [IN] dd,
 *     [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_incoming_msg
MPIG_STATIC void mpig_cm_xio_recv_handle_incoming_msg(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_byte_t * const rbuf, const globus_size_t rbuf_len, const globus_size_t nbytes_read,
    const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    bool_t vc_locked = FALSE;
    bool_t vc_inuse;
    bool_t vc_was_inuse;
    mpig_pg_t * pg = NULL;
    MPIU_Size_t bytes_needed = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_incoming_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_incoming_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	/* some receive message handlers, like those involved in the disconnect process, may need to release the VC mutex or
	   decrement its reference count when an error occurs, so we increment the VC reference here to keep the VC intact until
	   this routine is ready to exit. */
	mpig_vc_inc_ref_count(vc, &vc_was_inuse);
	pg = mpig_vc_get_pg(vc);
	
	/* if the VC is disconnecting or has failed, and the registered read operation was cancelled or EOF was reached, then
	   ignore the callback */
	if (globus_xio_error_is_eof(op_grc) && (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING ||
	    mpig_cm_xio_vc_has_failed(vc)))
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "EOF reached; terminating receive state machine: vc=" MPIG_PTR_FMT
		", vc_state=%s", MPIG_PTR_CAST(vc), mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    goto vc_lock_exit;
	}

	if (globus_xio_error_is_canceled(op_grc) && (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING ||
	    mpig_cm_xio_vc_has_failed(vc)))
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "read operation cancelled; terminating receive state machine: vc="
		MPIG_PTR_FMT ", vc_state=%s", MPIG_PTR_CAST(vc), mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    goto vc_lock_exit;
	}

	/* if any other error occurs, report it */
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT "vc_state=%s, msg=%s", "globus_xio_register_write", MPIG_PTR_CAST(vc),
		MPIG_PTR_CAST(handle),	mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)),
		globus_error_print_chain(globus_error_peek(op_grc))));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|recv_handle_incoming_msg",
		"**globus|cm_xio|recv_handle_incoming_msg %s", globus_error_print_chain(globus_error_peek(op_grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* adjust number of bytes in the receive buffer */
	mpig_databuf_inc_eod(vc_cmu->msgbuf, nbytes_read);
    
	/* process message(s) */
	while (1)
	{
	    /* if the message header size is not known, then we are at the start of a new message; otherwise, we are in the
	       middle of processing a message header */
	    if (vc_cmu->msg_hdr_size == 0)
	    {
		/* if the message buffer does not contain enough bytes to get the message header size, then receive more data */
		if (mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) < mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc))
		{
		    bytes_needed = mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc) -
			mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf);
		    MPIU_Assert(bytes_needed > 0 && bytes_needed <= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc));
		    break;
		}

		/* extract the message header size from the message header */
		mpig_cm_xio_msg_hdr_get_msg_size(vc, vc_cmu->msgbuf, &vc_cmu->msg_hdr_size);
		vc_cmu->msg_hdr_size -= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(vc);

		/* if the message buffer does not contain the entire message header, then receive remainder of header */
		if (mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) < vc_cmu->msg_hdr_size)
		{
		    bytes_needed = vc_cmu->msg_hdr_size - mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf);
		    MPIU_Assert(bytes_needed > 0 && bytes_needed <= vc_cmu->msg_hdr_size);
		    break;
		}
	    }
	    else
	    {
		mpig_cm_xio_msg_type_t msg_type = MPIG_CM_XIO_MSG_TYPE_UNDEFINED;
		
		MPIU_Assert(mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) >= vc_cmu->msg_hdr_size);
		
		/* extract the message type from the message header */
		mpig_cm_xio_msg_hdr_get_msg_type(vc, vc_cmu->msgbuf, &msg_type);
		vc_cmu->msg_hdr_size -= mpig_cm_xio_msg_hdr_remote_sizeof_msg_type(vc);

		/*
		 * call the message handler associated with the extracted message type.  after the handler returns, set the
		 * message header size to zero to indicate that the message header has been consumed.
		 *
		 * NOTE: if a handler changes the state of the VC to a state not in the connected or disconnecting state classes,
		 * it responsible for for reporting any error(s) to the application.  mpig_cm_xio_recv_handle_incoming_msg() only
		 * reports errors if the VC is still in the connected or disconnecting state (even if that connection is now
		 * broken).
		 *
		 * XXX: Are there any handlers left, besides those that are part of the disconnect process, that either release
		 * the mutex allowing other threads to change the VC state or explicitly change the state themselves?
		 */
		mpi_errno = mpig_cm_xio_msghan_funcs[msg_type](vc);
		vc_cmu->msg_hdr_size = 0;
		if (mpig_cm_xio_vc_is_connected(vc) || mpig_cm_xio_vc_is_disconnecting(vc))
		{
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|msg_handler");
		}
		else
		{
		    /* the VC is no longer connected.  this routine should exit without any further processing of data.  we
		       should only reach this state if the receive messsage handler changed the state, or the message handler
		       released the VC mutex allowing other threads to change the state.  see notes above. */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			"VC is not longer connected; terminating receive state machine: vc=" MPIG_PTR_FMT ", vc_state=%s",
			MPIG_PTR_CAST(vc), mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
		    goto vc_lock_exit;
		}
		
		if (vc_cmu->active_rreq != NULL)
		{
		    /* a receive request is active and awaiting message data.  the message data handler(s) are responsible for
		       registering a new read operation to receive the next message header once the message data associated with
		       the current message has been consumed. */
		    bytes_needed = 0;
		    break;
		}
		else
		{
		    /* no receive request is active, so loop back to the top and process the next message header */
		}
	    }
	}
	
	if (bytes_needed > 0)
	{
	    size_t bytes_remaining = mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf);
	    
	    /* if there are unconsumed bytes, then move an remaining data to the beginning of the buffer and update the current
	       position and end-of-data counters; otherwise reset the databuf to the beginning of the buffer.  one could argue
	       that the move should not be unconditional; however, the number of bytes remaining in the buffer should be very
	       small (since message headers are small) and thus the cost of the move should be minimal.  the extra bytes
	       available in the buffer as a result of the move could eliminate one or more read operations, which are costly in
	       comparison. */
	    if (bytes_remaining > 0)
	    {
		memmove(mpig_databuf_get_base_ptr(vc_cmu->msgbuf), mpig_databuf_get_pos_ptr(vc_cmu->msgbuf), bytes_remaining);
		mpig_databuf_set_pos(vc_cmu->msgbuf, 0);
		mpig_databuf_set_eod(vc_cmu->msgbuf, bytes_remaining);
	    }
	    else
	    {
		mpig_databuf_reset(vc_cmu->msgbuf);
	    }

	    /* register a receive to receive, at a minimum, the number of bytes specified by "bytes_needed" */
	    mpig_cm_xio_register_read_vc_msgbuf(vc, bytes_needed, mpig_databuf_get_free_bytes(vc_cmu->msgbuf),
		mpig_cm_xio_recv_handle_incoming_msg, &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_vc_msgbuf");
	}
	else
	{
	    /* the reception of a message data is in progress.  the handler consuming this data will register a read for the next
	       message header by calling mpig_cm_xio_recv_next_msg() once it has finished with the current message. */
	    MPIU_Assert(mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf) == 0);
	    mpig_databuf_reset(vc_cmu->msgbuf);
	}
	
      vc_lock_exit:
	mpig_vc_dec_ref_count(vc, &vc_inuse);
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

    if (vc_inuse == FALSE)
    {
	mpig_pg_release_ref(pg);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_incoming_msg);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_proc_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	mpig_vc_release_ref(vc);
	
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_incoming_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_eager_data_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the eager data message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_eager_data_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_eager_data_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    MPID_Request * rreq = NULL;
    struct mpig_cm_xio_request_cmu * rreq_cmu = NULL;
    bool_t rreq_found;
    bool_t rreq_complete = FALSE;
    mpig_request_type_t sreq_type;
    int sreq_id;
    int rank;
    int tag;
    int ctx;
    MPIU_Size_t stream_size;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_eager_data_msg);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_eager_data_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    MPIG_UNUSED_VAR(fcname);

    /* unpack message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_req_type(vc) +
	mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc) + mpig_cm_xio_msg_hdr_remote_sizeof_envelope(vc) +
	mpig_cm_xio_msg_hdr_remote_sizeof_data_size(vc));
    mpig_cm_xio_msg_hdr_get_req_type(vc, vc_cmu->msgbuf, &sreq_type);
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);
    mpig_cm_xio_msg_hdr_get_envelope(vc, vc_cmu->msgbuf, &rank, &tag, &ctx);
    /* XXX: still need to handle, and thus identify, MPI_PACKED messages since they will have a special header */
    mpig_cm_xio_msg_hdr_get_data_size(vc, vc_cmu->msgbuf, &stream_size);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
	"eager message received: rank=%d, tag=%d, ctx=%d, sreq_type=%s, stream_size=" MPIG_SIZE_FMT,
	rank, tag, ctx, mpig_request_type_get_string(sreq_type), stream_size));

    /* get receive request.  when using vendor MPI, mpig_recvq_deq_posted_or_enq_unexp() may block if the matching request is
       found and the source is MPI_ANY_SOURCE.  to prevent deadlock, the VC is passed to mpig_recvq_deq_posted_or_enq_unexp() so
       that it may release the VC mutex before block.  this is icky, but eliminates releasing and reacquiring the VC mutex when
       the matching request's source is not MPI_ANY_SOURCE. */
    mpi_errno = mpig_recvq_deq_posted_or_enq_unexp(vc, rank, tag, ctx, &rreq_found, &rreq);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: receive queue operation failed; dequeue "
	    "posted or enqueue unexpected: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
	MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|recvq_deq_posted_or_enq_unexp",
	    "**globus|recvq_deq_posted_or_enq_unexp %d %d %d", rank, tag, ctx);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    rreq_cmu = &rreq->cmu.xio;

    /* if a matching request was not found, then initialize the one allocated for the unexpected queue; otherwise, if the request
       was posted with a rank of MPI_ANY_SOURCE, set the request's VC field to the VC on which the message arrived and intialize
       the XIO portion of the request */
    if (!rreq_found)
    {
	mpig_request_construct_rreq(rreq, 2, 1, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, vc);
	mpig_cm_xio_request_construct(rreq);
	mpig_cm_xio_request_set_cc(rreq, 1);
    }
    else if (mpig_request_get_rank(rreq) == MPI_ANY_SOURCE)
    {
	mpig_request_set_vc(rreq, vc);
	mpig_cm_xio_request_construct(rreq);
	mpig_cm_xio_request_set_cc(rreq, 1);
	mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED);
    }

    /* set request parameters common to both situations (found or not) */
    mpig_cm_xio_request_set_msg_type(rreq, MPIG_CM_XIO_MSG_TYPE_EAGER_DATA);
    mpig_cm_xio_request_set_sreq_type(rreq, sreq_type);
    mpig_request_set_remote_req_id(rreq, sreq_id);
    mpig_cm_xio_stream_set_size(rreq, stream_size);
    rreq_cmu->df = vc_cmu->df; /* XXX: NEED TO DEAL WITH DATA THAT WAS SENT AS MPI_PACKED! */
    
    /* fill in the status fields */
    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    rreq->status.mpig_dc_format = vc_cmu->df;  /* XXX: may be different that local format if data was sent using MPI_PACKED */
    /* rreq->status.count is set by the stream unpack routines if the message was posted */
    
    if (rreq_found)
    {
	/* a matching request was found in the posted queue */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
	    "request found in posted queue: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

	MPIU_Assertp(mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED);

	/* set the envelope information in the request */
	/* mpig_request_set_envelope(rreq, rank, tag, ctx); */
    
	/* if this is a synchronous send, then we need to send an acknowledgement back to the sender. */
	if (mpig_cm_xio_request_get_sreq_type(rreq) == MPIG_REQUEST_TYPE_SSEND)
	{
	    mpi_errno = mpig_cm_xio_send_enq_ssend_ack_msg(vc, mpig_request_get_remote_req_id(rreq));
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_ssend_ack_msg");
	}

	/* initialize the stream, and start the process of unpacking and receiving the data.  if the message is not a zero-byte
	   message, the unpack routine will set the IOV with the locations to place the data. */
	mpi_errno = mpig_cm_xio_stream_rreq_init(rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_init");

	mpig_iov_reset(rreq_cmu->iov, 0);
	mpi_errno = mpig_cm_xio_stream_rreq_unpack(rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack");
	
	/* if the message was not a zero-byte message, then first try to get the bytes from the message buffer */
	if (mpig_iov_get_num_bytes(rreq_cmu->iov) > 0)
	{
	    mpi_errno = mpig_cm_xio_stream_rreq_unpack_vc_msgbuf(vc, rreq);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack_vc_msgbuf");
	}

	if (mpig_iov_get_num_bytes(rreq_cmu->iov) == 0)
	{
	    /* if all of the data has already been received and placed in the applicaton buffer, the change the request state to
	       complete and decrement the internal (CM XIO) completion counter */
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
	    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	}
	else
	{
	    /* otherwise, register a read operation for more data */
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA);
	    rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
	    MPIU_Assert(vc_cmu->active_rreq == NULL);
	    mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
	    vc_cmu->active_rreq = rreq;
	}
    }
    else
    {
	/* the application code has yet to post a matching MPI_RECV.  a new request has been allocated and inserted into the
	   unexpected queue. */
	MPIU_Size_t nbytes;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
	    "request added to the unexpected queue: req=" MPIG_HANDLE_FMT ", ptr=" MPIG_PTR_FMT,
	    rreq->handle, MPIG_PTR_CAST(rreq)));

	if (mpig_cm_xio_request_get_sreq_type(rreq) == MPIG_REQUEST_TYPE_RSEND)
	{
	    /* if data was sent using a ready-send and the envelope did not match a posted request, then attach an error to the
	       request.  note: any data associated with the message must still be received. */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
		"message was a ready-send; adding error: req=" MPIG_HANDLE_FMT ", ptr=" MPIG_PTR_FMT,
		rreq->handle, MPIG_PTR_CAST(rreq)));
	    
	    MPIU_ERR_SET2(rreq->status.MPI_ERROR, MPI_ERR_OTHER, "**rsendnomatch", "**rsendnomatch %d %d", rank, tag);

	    /* if some of the data for this message is in the VC message bufffer, then remove it */
	    nbytes = MPIG_MIN(mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf), mpig_cm_xio_stream_get_size(rreq));
	    mpig_databuf_inc_pos(vc_cmu->msgbuf, nbytes);

	    if(nbytes < mpig_cm_xio_stream_get_size(rreq))
	    {
		/* allocate a temporary data buffer for the unexpected data */
		mpi_errno = mpig_databuf_create((MPIU_Size_t) MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE, &rreq_cmu->databuf);
		if (mpi_errno)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
			"ERROR: malloc failed when attempting to allocate a unexpected receive buffer"));
		    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "unexpected receive bufffer");
		    goto fn_fail;
		}   /* --END ERROR HANDLING-- */

		/* use the stream fields to assist in draining data off the network */
		mpig_cm_xio_stream_set_cur_pos(rreq, nbytes);
		mpig_cm_xio_stream_set_max_pos(rreq, 0);
		rreq->status.count = 0;

		/* set up an IOV to pointing at the temporary buffer */
		nbytes = MPIG_MIN((mpig_cm_xio_stream_get_size(rreq) - mpig_cm_xio_stream_get_cur_pos(rreq)),
				  mpig_databuf_get_size(rreq_cmu->databuf));
		mpig_iov_reset(rreq_cmu->iov, 0);
		mpig_iov_add_entry(rreq_cmu->iov, mpig_databuf_get_base_ptr(rreq_cmu->databuf), nbytes);
		mpig_databuf_set_eod(rreq_cmu->databuf, nbytes);
		mpig_cm_xio_stream_inc_cur_pos(rreq, nbytes);

		/* register a read operation to start draining the data from the network and place into the temporary buffer */
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_RSEND_DATA);
		rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
		MPIU_Assert(vc_cmu->active_rreq == NULL);
		mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
		{
		    bool_t was_complete;
		    mpig_cm_xio_request_inc_cc(rreq, &was_complete);
		}
		vc_cmu->active_rreq = rreq;
	    }
	    else
	    {
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
	    }
	}
	else if (mpig_cm_xio_stream_get_size(rreq) > 0)
	{
	    /* otherwise, any data associated with the message must be buffered until the application supplies a matching
	       receive. */

	    /* allocate a temporary data buffer for the unexpected data */
            mpi_errno = mpig_databuf_create(mpig_cm_xio_stream_get_size(rreq), &rreq_cmu->databuf);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
		    "ERROR: malloc failed when attempting to allocate a unexpected receive buffer"));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "unexpected receive bufffer");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    /* set up an IOV to pointing at the temporary buffer */
	    mpig_iov_reset(rreq_cmu->iov, 0);
	    mpig_iov_add_entry(rreq_cmu->iov, mpig_databuf_get_base_ptr(rreq_cmu->databuf), mpig_cm_xio_stream_get_size(rreq));
	    mpig_databuf_set_eod(rreq_cmu->databuf, mpig_cm_xio_stream_get_size(rreq));

	    /* if some of the data for this message is in the VC message bufffer, then copy it into the data buffer */
	    nbytes = mpig_iov_unpack(mpig_databuf_get_pos_ptr(vc_cmu->msgbuf), mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf),
		rreq_cmu->iov);
	    mpig_databuf_inc_pos(vc_cmu->msgbuf, nbytes);
	    
	    /* if any of the unexpected data remains in the network, the register a read operation to drain the data from the
	       network and place into the temporary buffer */
	    if (mpig_iov_get_num_bytes(rreq_cmu->iov) > 0)
	    {
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA);
		rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
		MPIU_Assert(vc_cmu->active_rreq == NULL);
		mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
		vc_cmu->active_rreq = rreq;
	    }
	    else
	    {
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
	    }

	    rreq->status.count = mpig_cm_xio_stream_get_size(rreq);
	}
	else
	{
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
	    rreq->status.count = 0;
	}
    }

  fn_return:
    if (rreq != NULL)
    {
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);

	/* if all tasks associated with the request have completed, then added it to the completion queue; otherwise, if the
	   message was unexpected then tell the receive queue to return.  the latter is necessary because the application may be
	   waiting in MPI_Probe() :-((. */
	if (rreq_complete)
	{
	    mpig_cm_xio_rcq_enq(rreq);
	}
	else if (!rreq_found)
	{
	    /* if the CM is blocking in a adi3_probe(), then wake it up */
	    mpig_cm_xio_probe_wakeup();
	}
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_eager_data_msg);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* FIXME: need better error handling here.  any extraneous data should be drained from the network. */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_eager_data_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_rndv_rts_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the rendezvous request-to-send message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_rndv_rts_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_rts_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    MPID_Request * rreq = NULL;
    struct mpig_cm_xio_request_cmu * rreq_cmu = NULL;
    bool_t rreq_found;
    mpig_request_type_t sreq_type;
    int sreq_id;
    int rank;
    int tag;
    int ctx;
    MPIU_Size_t rts_size;
    MPIU_Size_t stream_size;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_rndv_rts_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_rndv_rts_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
    
    MPIG_UNUSED_VAR(fcname);

    /* unpack message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_req_type(vc) +
	mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc) + mpig_cm_xio_msg_hdr_remote_sizeof_envelope(vc) +
	2 * mpig_cm_xio_msg_hdr_remote_sizeof_data_size(vc));
    
    mpig_cm_xio_msg_hdr_get_req_type(vc, vc_cmu->msgbuf, &sreq_type);
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);
    mpig_cm_xio_msg_hdr_get_envelope(vc, vc_cmu->msgbuf, &rank, &tag, &ctx);
    /* XXX: still need to handle, and thus identify, MPI_PACKED messages since they will have a special header */
    mpig_cm_xio_msg_hdr_get_data_size(vc, vc_cmu->msgbuf, &rts_size);
    mpig_cm_xio_msg_hdr_get_data_size(vc, vc_cmu->msgbuf, &stream_size);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "eager message received: rank=%d, tag=%d, ctx=%d, sreq_type=%s, stream_size="
	MPIG_SIZE_FMT, rank, tag, ctx, mpig_request_type_get_string(sreq_type), stream_size));

    /* get receive request.  when using vendor MPI, mpig_recvq_deq_posted_or_enq_unexp() may block if the matching request is
       found and the source is MPI_ANY_SOURCE.  to prevent deadlock, the VC is passed to mpig_recvq_deq_posted_or_enq_unexp() so
       that it may release the VC mutex before block.  this is icky, but eliminates releasing and reacquiring the VC mutex when
       the matching request's source is not MPI_ANY_SOURCE. */
    mpi_errno = mpig_recvq_deq_posted_or_enq_unexp(vc, rank, tag, ctx, &rreq_found, &rreq);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: receive queue operation failed; dequeue "
	    "posted or enqueue unexpected: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
	MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|recvq_deq_posted_or_enq_unexp",
	    "**globus|recvq_deq_posted_or_enq_unexp %d %d %d", rank, tag, ctx);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    rreq_cmu = &rreq->cmu.xio;

    /* if a matching request was not found, then initialize the one allocated for the unexpected queue; otherwise, if the request
       was posted with a rank of MPI_ANY_SOURCE, set the request's VC field to the VC on which the message arrived and intialize
       the XIO portion of the request */
    if (!rreq_found)
    {
	mpig_request_construct_rreq(rreq, 2, 1, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, vc);
	mpig_cm_xio_request_construct(rreq);
	mpig_cm_xio_request_set_cc(rreq, 1);
    }
    else if (mpig_request_get_rank(rreq) == MPI_ANY_SOURCE)
    {
	mpig_request_set_vc(rreq, vc);
	mpig_cm_xio_request_construct(rreq);
	mpig_cm_xio_request_set_cc(rreq, 1);
	mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED);
    }

    /* set request parameters common to both situations (found or not) */
    mpig_cm_xio_request_set_msg_type(rreq, MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
    mpig_cm_xio_request_set_sreq_type(rreq, sreq_type);
    mpig_request_set_remote_req_id(rreq, sreq_id);
    mpig_cm_xio_stream_set_size(rreq, stream_size);
    rreq_cmu->df = vc_cmu->df; /* XXX: NEED TO DEAL WITH DATA THAT WAS SENT AS MPI_PACKED! */
    
    /* fill in the status fields */
    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    rreq->status.mpig_dc_format = vc_cmu->df;  /* XXX: may be different that local format if data was sent using MPI_PACKED */
    /* rreq->status.count is set by the stream unpack routines if the message was posted */
    
    if (rreq_found)
    {
	/* a matching request was found in the posted queue */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request found in posted queue: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
	    rreq->handle, MPIG_PTR_CAST(rreq)));

	/* set the envelope information in the request */
	/* mpig_request_set_envelope(rreq, rank, tag, ctx); */
    
	/* send the clear-to-send message */
	mpi_errno = mpig_cm_xio_send_enq_rndv_cts_msg(vc, rreq->handle, sreq_id);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_rndv_cts_msg");
	
	MPIU_Assertp(mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED);

	/* initialize the stream, and start the process of unpacking and receiving the data.  if the message is not a zero-byte
	   message, the unpack routine will set the IOV with the locations to place the data. */
	mpi_errno = mpig_cm_xio_stream_rreq_init(rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_init");

	mpig_iov_reset(rreq_cmu->iov, 0);
	mpig_cm_xio_stream_set_max_pos(rreq, rts_size);
	mpi_errno = mpig_cm_xio_stream_rreq_unpack(rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack");
	
	/* first try to get the bytes from the message buffer */
	MPIU_Assert(mpig_iov_get_num_bytes(rreq_cmu->iov) > 0);
	mpi_errno = mpig_cm_xio_stream_rreq_unpack_vc_msgbuf(vc, rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack_vc_msgbuf");

	if (mpig_iov_get_num_bytes(rreq_cmu->iov) == 0)
	{
	    /* if all of the data sent with the request-to-send has already been received and placed in the applicaton buffer,
	       then change the request state to "wait for rendezvous data message" */
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
	}
	else
	{
	    /*  otherwise, register a read operation for more data */
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA);
	    rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
	    MPIU_Assert(vc_cmu->active_rreq == NULL);
	    mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
	    vc_cmu->active_rreq = rreq;
	}
    }
    else
    {
	/* the application code has yet to post a matching MPI_RECV.  a new request has been allocated and inserted into the
	   unexpected queue.  the data arriving with the request-to-send message must be buffered until the application supplies a
	   matching receive. */
	MPIU_Size_t nbytes;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request added to the unexpected queue: req=" MPIG_HANDLE_FMT ", ptr="
	    MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

	/* allocate a temporary data buffer for the unexpected data */
	mpi_errno = mpig_databuf_create(rts_size, &rreq_cmu->databuf);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
		"ERROR: malloc failed when attempting to allocate a unexpected receive buffer"));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "unexpected receive bufffer");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* set up an IOV to pointing at the temporary buffer */
	mpig_iov_reset(rreq_cmu->iov, 0);
	mpig_iov_add_entry(rreq_cmu->iov, mpig_databuf_get_base_ptr(rreq_cmu->databuf), rts_size);
	mpig_databuf_set_eod(rreq_cmu->databuf, rts_size);

	/* if some of the data for this message is in the VC message bufffer, then copy it into the data buffer */
	nbytes = mpig_iov_unpack(mpig_databuf_get_pos_ptr(vc_cmu->msgbuf), mpig_databuf_get_remaining_bytes(vc_cmu->msgbuf),
				 rreq_cmu->iov);
	mpig_databuf_inc_pos(vc_cmu->msgbuf, nbytes);
	    
	/* if any of the unexpected data remains in the network, the register a read operation to drain the data from the
	   network and place into the temporary buffer */
	if (mpig_iov_get_num_bytes(rreq_cmu->iov) > 0)
	{
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA);
	    rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
	    MPIU_Assert(vc_cmu->active_rreq == NULL);
	    mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
	    vc_cmu->active_rreq = rreq;
	}
	else
	{
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
	}
	
	rreq->status.count = mpig_cm_xio_stream_get_size(rreq);
    }

  fn_return:
    if (rreq != NULL)
    {
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);
    }

    /* if the CM is blocking in a adi3_probe(), then wake it up */
    if (!rreq_found)
    {
	mpig_cm_xio_probe_wakeup();
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_rndv_rts_msg);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	/* FIXME: need better error handling here.  any extraneous data should be drained from the network. */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_rndv_rts_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_rndv_cts_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the rendezvous clear-to-send message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_rndv_cts_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_cts_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    int sreq_id;
    int rreq_id;
    MPID_Request * sreq;
    bool_t sreq_locked = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_rndv_cts_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_rndv_cts_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* get the request handles from the message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == 2 * mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc));
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &rreq_id);
    MPID_Request_get_ptr(sreq_id, sreq);

    mpig_request_mutex_lock(sreq);
    sreq_locked = TRUE;
    {
	if (mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_CTS)
	{
	    /* enqueue the remainder of the data */
	    mpi_errno = mpig_cm_xio_send_enq_rndv_data_msg(vc, sreq, rreq_id);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_rndv_data_msg");
	}
	else
	{
	    MPIU_Assert(mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS);
	    mpig_request_set_remote_req_id(sreq, rreq_id);
	    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS);
	}

	/* if this was an ssend request, then decrement the completion counter */
	if (mpig_request_get_type(sreq) == MPIG_REQUEST_TYPE_SSEND)
	{
	    bool_t sreq_complete;
	    mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
	    MPIU_Assert(sreq_complete == FALSE);
	}
    }
    mpig_request_mutex_unlock(sreq);
    sreq_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_rndv_cts_msg);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_request_mutex_unlock_conditional(sreq, sreq_locked);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_rndv_cts_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_rndv_data_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the rendezvous data message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_rndv_data_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_rndv_data_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    int rreq_id;
    MPID_Request * rreq;
    struct mpig_cm_xio_request_cmu * rreq_cmu;
    bool_t rreq_locked = FALSE;
    bool_t rreq_complete = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_rndv_data_msg);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_rndv_data_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* get the receive request handle from the message header and turn it into a pointer to the request object */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc));
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &rreq_id);
    MPID_Request_get_ptr(rreq_id, rreq);
    rreq_cmu = &rreq->cmu.xio;

    mpig_request_mutex_lock(rreq);
    rreq_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
    
	/* prepare to start reading the data */
	mpig_cm_xio_stream_set_max_pos(rreq, 0);
	mpi_errno = mpig_cm_xio_stream_rreq_unpack(rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack");
	MPIU_Assert(mpig_iov_get_num_bytes(rreq_cmu->iov) > 0);

	/* first try to get the bytes from the message buffer */
	mpi_errno = mpig_cm_xio_stream_rreq_unpack_vc_msgbuf(vc, rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack_vc_msgbuf");

	/* if there is more data to receive, then register a read; otherwise, mark the request as complete and enqueue it on the
	   completion queue */
	if (mpig_iov_get_num_bytes(rreq_cmu->iov) > 0)
	{
	    mpig_cm_xio_request_set_msg_type(rreq, MPIG_CM_XIO_MSG_TYPE_RNDV_DATA);
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA);
	    rreq_cmu->gcb = mpig_cm_xio_recv_handle_read_msg_data;
	    MPIU_Assert(vc_cmu->active_rreq == NULL);
	    mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
	    vc_cmu->active_rreq = rreq;
	}
	else
	{
	    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
	    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
	}
    }
    mpig_request_mutex_unlock(rreq);
    rreq_locked = FALSE;

    /* if all tasks associated with the request have completed, then added it to the completion queue */
    if (rreq_complete) mpig_cm_xio_rcq_enq(rreq);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_rndv_data_msg);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_request_mutex_unlock_conditional(rreq, rreq_locked);
	/* FIXME: need better error handling here.  any extraneous data should be drained from the network. */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_rndv_data_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_ssend_ack_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the ssend acknowledgement message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_ssend_ack_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_ssend_ack_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    MPI_Request sreq_id;
    MPID_Request * sreq = NULL;
    bool_t sreq_complete = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_ssend_ack_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_ssend_ack_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* unpack message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc));
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);

    /* get the send request object */
    MPID_Request_get_ptr(sreq_id, sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "failed to locate send request: sreq_id=" MPIG_HANDLE_FMT, sreq_id));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|request_get_ptr", "**globus|request_get_ptr %R", sreq_id);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    /* decrement the cm_xio completion counter and enqueue the request on the request completion queue (RCQ) if the counter
       reaches zero  */
    mpig_request_mutex_lock(sreq);
    {
	MPIU_Assert(mpig_request_get_type(sreq) == MPIG_REQUEST_TYPE_SSEND);

	mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
    }
    mpig_request_mutex_unlock(sreq);
    
    /* if all tasks associated with the request have completed, then added it to the completion queue */
    if (sreq_complete) mpig_cm_xio_rcq_enq(sreq);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting; vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_ssend_ack_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_ssend_ack_msg() */


/*
 * void mpig_cm_xio_recv_handle_cancel_send_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the cancel send message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_cancel_send_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_cancel_send_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    int rank;
    int tag;
    int ctx;
    MPI_Request sreq_id;
    MPID_Request * rreq = NULL;
    bool_t rreq_complete = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* unpack message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_envelope(vc) +
	mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc));
    mpig_cm_xio_msg_hdr_get_envelope(vc, vc_cmu->msgbuf, &rank, &tag, &ctx);
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "cancel send message received: rank=%d, tag=%d, ctx=%d, sreq_id=" MPIG_HANDLE_FMT,
	rank, tag, ctx, sreq_id));
	
    /* attempt to remove the request from the unexpected receive queue */
    rreq = mpig_recvq_find_unexp_and_deq(rank, tag, ctx, sreq_id);
    /* request mutex is locked by recvq routine */
    {
	const bool_t cancelled = (rreq != NULL);

	if (cancelled)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "matching request found in the unexpected queue; sending ack: rreq="
		MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "matching request NOT found in the unexpected queue; sending nak"));
	}
	
	/* send a reply to the sender inidicating if the request was cancelled */
	mpi_errno = mpig_cm_xio_send_enq_cancel_send_resp_msg(vc, sreq_id, cancelled);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|send_enq_cancel_send_resp_msg");
	
	if (cancelled)
	{
	    /* if the send request was cancelled, then we have either received all of the data or at least the first part of the
	       data with the rendezvous RTS message */
	    if (mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE ||
		mpig_cm_xio_request_get_state(rreq) == MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "unexpected data received; completing request: rreq=" MPIG_HANDLE_FMT
		    ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

		mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
		MPIU_Assert(rreq_complete);
		mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
		mpig_request_set_cc(rreq, 0);
		mpig_request_set_ref_count(rreq, 0);
	    }
	    else
	    {
		/* all other request states are erroneous.  NOTE: given that the XIO model is stream based, we cannot still be
		   draining data from the network and have received the cancel send message, so the RECV_UNEXP_* states
		   are not possible. */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "INTERNAL ERROR - receive request state not valid for operation: "
		    "state=%s", mpig_cm_xio_request_state_get_string(mpig_cm_xio_request_get_state(rreq))));
		MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {;},
		    "**globus|cm_xio|req_recv_state", "**globus|cm_xio|req_recv_state %s",
		    mpig_cm_xio_request_state_get_string(mpig_cm_xio_request_get_state(rreq)));
	    }
	}
    }
    mpig_request_mutex_unlock_conditional(rreq, (rreq != NULL));

    /* if all tasks associated with the request have completed, then destroy the request (as though it never existed).  MT-NOTE:
       in most cases, destroying the request from a thread other than the main thread is unsafe because the request may contain
       references to objects that are not thread safe, such as a communicator or datatype.  however, in this case, the request
       has yet to be populated with such objects and thus may be safely destroyed. */
    if (rreq_complete) mpig_request_destroy(rreq);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_msg);
    return mpi_errno;
}
/* mpig_cm_xio_recv_handle_cancel_send_msg() */


/*
 * <mpi_errno> mpig_cm_xio_recv_handle_cancel_send_resp_msg([IN/MOD] vc)
 *
 * Parameters:
 *
 * vc [IN/MOD] - virtual connection object over which the cancel send response message arrived
 *
 *
 * Returns: an MPI error code
 *
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_cancel_send_resp_msg
MPIG_STATIC int mpig_cm_xio_recv_handle_cancel_send_resp_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    MPI_Request sreq_id;
    MPID_Request * sreq = NULL;
    bool_t sreq_complete = FALSE;
    bool_t cancelled;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_resp_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_resp_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* unpack message header */
    MPIU_Assert(vc_cmu->msg_hdr_size == mpig_cm_xio_msg_hdr_remote_sizeof_req_id(vc) +
	mpig_cm_xio_msg_hdr_remote_sizeof_bool(vc));
    mpig_cm_xio_msg_hdr_get_req_id(vc, vc_cmu->msgbuf, &sreq_id);
    mpig_cm_xio_msg_hdr_get_bool(vc, vc_cmu->msgbuf, &cancelled);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "received cancel send request: vc=" MPIG_PTR_FMT ", sreq_id=" MPIG_HANDLE_FMT
	", cancelled=%s", MPIG_PTR_CAST(vc), sreq_id, MPIG_BOOL_STR(cancelled)));
    
    /* get the send request object */
    MPID_Request_get_ptr(sreq_id, sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "failed to locate send request: sreq_id=" MPIG_HANDLE_FMT, sreq_id));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|request_get_ptr", "**globus|request_get_ptr %R", sreq_id);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_request_mutex_lock(sreq);
    {
	sreq->status.cancelled = cancelled;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "before adjusting CC: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
	    MPIG_PTR_FMT ", cc=%d", MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), sreq->cmu.xio.cc));
	
	/* if this was an eager ssend message, then decrement the completion counter to cover ssend ack that will never arrive */
	if (mpig_request_get_type(sreq) == MPIG_REQUEST_TYPE_SSEND)
	{
	    mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
	    MPIU_Assert(sreq_complete == FALSE);

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "after ssend CC adjustment: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
		", sreqp=" MPIG_PTR_FMT ", cc=%d", MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), sreq->cmu.xio.cc));
	}

	/* if the request was cancelled and it was sent using the rendezvous protocol, then decrement the completion counter to
	   compensate for the send being terminated prematurely.  (message send using the eager protocol will always finish
	   sending; those sent using the rendezvous protocol can only finish if the CTS is received, which won't happen if the
	   request is cancelled) */
	if (cancelled && mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS)
	{
	    mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
	    MPIU_Assert(sreq_complete == FALSE);

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "after cancelled CC adjustment: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
		", sreqp=" MPIG_PTR_FMT ", cc=%d", MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), sreq->cmu.xio.cc));
	}
   
	/* finally, decrement the completion counter for the outstanding cancel send response message we just finished
	   processing.  enqueue the request on the request completion queue (RCQ) if request is complete. */
	mpig_cm_xio_request_dec_cc(sreq, &sreq_complete);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "after adjusting CC: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
	    MPIG_PTR_FMT ", cc=%d, sreq_complete=%s", MPIG_PTR_CAST(vc), sreq->handle, MPIG_PTR_CAST(sreq), sreq->cmu.xio.cc,
	    MPIG_BOOL_STR(sreq_complete)));
    }
    mpig_request_mutex_unlock(sreq);
    
    /* if all tasks associated with the request have completed, then added it to the completion queue */
    if (sreq_complete) mpig_cm_xio_rcq_enq(sreq);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_cancel_send_resp_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_cancel_send_resp_msg() */


/*
 * void mpig_cm_xio_recv_handle_read_msg_data([IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes_written, [IN] dd,
 *     [IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_recv_handle_read_msg_data
MPIG_STATIC void mpig_cm_xio_recv_handle_read_msg_data(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_xio_iovec_t * const iov, const int iov_cnt, const globus_size_t nbytes_read, const globus_xio_data_descriptor_t dd,
    void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc_cmu * vc_cmu = &vc->cmu.xio;
    bool_t vc_locked = FALSE;
    MPID_Request * rreq = NULL;
    struct mpig_cm_xio_request_cmu * rreq_cmu = NULL;
    bool_t rreq_locked = FALSE;
    bool_t rreq_complete = FALSE;
    bool_t recv_complete = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_recv_handle_read_msg_data);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_recv_handle_read_msg_data);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "enter state: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, iov=" MPIG_PTR_FMT ", iov_cnt=%d, nbytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle), op_grc,
	MPIG_PTR_CAST(iov), iov_cnt, nbytes_read));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	rreq = vc_cmu->active_rreq;
	rreq_cmu = &rreq->cmu.xio;

	mpig_request_mutex_lock(rreq);
	rreq_locked = TRUE;
	{
	    if (op_grc)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		    ", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", MPIG_PTR_CAST(vc), MPIG_PTR_CAST(handle),
		    globus_error_print_chain(globus_error_peek(op_grc))));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|recv_handle_xio_read_msg_data",
		    "**globus|cm_xio|recv_handle_read_msg_data %s", globus_error_print_chain(globus_error_peek(op_grc)));
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
    
	    MPIU_Assert(nbytes_read == mpig_iov_get_num_bytes(rreq_cmu->iov));
	    MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA ||
			mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS ||
			mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_DATA);

	    switch(mpig_cm_xio_request_get_state(rreq))
	    {
		/* if we were receiving data for a message that the application has already posted... */
		case MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA:
		{
		    /* unpack any data in the request's databuf, and get the next set of data to be read */
		    mpig_iov_reset(rreq_cmu->iov, 0);
		    mpi_errno = mpig_cm_xio_stream_rreq_unpack(rreq);
		    MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|stream_rreq_unpack");
		    /* FIXME: better error handling is needed here.  the network should be drained of any remaining data from the
		       current message.  is this covered by the error handling in stream_rreq_unpack() ??? */

		    if (mpig_iov_get_num_bytes(rreq_cmu->iov) == 0)
		    {
			/* if all of the message data has been received, then update the state of the receive request... */
			
			if (mpig_cm_xio_stream_is_complete(rreq))
			{
			    /* if the entire stream of data has been sent, then decrement the internal (cm_xio) completion counter
			       and set the request state to complete */
			    MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA ||
					mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_DATA);
			    
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished receiving posted %s data: vc=" MPIG_PTR_FMT
				", rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, mpig_cm_xio_request_protocol_get_string(rreq),
				MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));

			    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
			    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
			}
			else
			{
			    /* otherwise, only the rendezvous RTS message has been sent, so set the request state to "waiting for
			       rendezvous data" */
			    MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
			    
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished receiving posted rts message; waiting for rndv "
				"data: vc=" MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc),
				rreq->handle, MPIG_PTR_CAST(rreq)));
			
			    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
			}
			
			recv_complete = TRUE;
		    }
		    else
		    {
			/* otherwise, more data needs to be received, so register and XIO read operation to receive the next set
			   of data */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "receiving more posted %s data: vc=" MPIG_PTR_FMT ", rreq="
			    MPIG_HANDLE_FMT ", rreqp="  MPIG_PTR_FMT, mpig_cm_xio_request_protocol_get_string(rreq),
			    MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
	    
			mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
			MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
		    }
		    break;
		}

		/* if we were receiving data for an unexpected message... */
		case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA:
		{
		    /* if this was an eager message, then set the request state to complete; otherwise set the state to "waiting
		       for rendezvous data" */
		    if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA)
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			     "finished receiving unexp eager data: vc=" MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT
			     ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));

			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
		    }
		    else
		    {
			MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);

			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			     "finished receiving unexp rts data; waiting for rndv data msg: vc=" MPIG_PTR_FMT
			     ", rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle,
			     MPIG_PTR_CAST(rreq)));
			
			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
		    }
		    
		    recv_complete = TRUE;
		    break;
		}
		
		/* if we are receiving data for unexpected message that the application has since posted ... */
		case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED:
		{
		    int mrc;
		    
		    mrc = mpig_cm_xio_stream_rreq_init(rreq);
		    if (mrc == MPI_SUCCESS)
		    {
			/* unpack the data send with the unexpected eager or RTS message */
			mpig_iov_reset(rreq_cmu->iov, 0);
			mpig_cm_xio_stream_set_max_pos(rreq, mpig_databuf_get_remaining_bytes(rreq_cmu->databuf));
			mrc = mpig_cm_xio_stream_rreq_unpack(rreq);
			MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(rreq->status.MPI_ERROR, mrc);},
			    "**globus|cm_xio|stream_rreq_unpack");
		    }
		    else
		    {
			MPIU_ERR_SETANDSTMT(mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(rreq->status.MPI_ERROR, mrc);},
			    "**globus|cm_xio|stream_req_init");
		    }

		    /* if this was an eager message, then decrement the internal (cm_xio) completion counter and set the request
		       state to complete; otherwise set the state to "waiting for rendezvous data" */
		    if (mrc)
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			    "ERROR: finished receiving unexp-posted data, but an error occurred while unpacking the data: vc="
			    MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle,
			    MPIG_PTR_CAST(rreq)));
			
			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
			mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
		    }
		    else if (mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA)
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			     "finished receiving unexp-posted eager data; data unpacked: vc=" MPIG_PTR_FMT ", rreq="
			     MPIG_HANDLE_FMT", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
			
			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
			mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
		    }
		    else
		    {
			MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_RNDV_RTS);
			
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			     "finished receiving unexp-posted rts data; waiting for rndv data msg: vc=" MPIG_PTR_FMT ", rreq="
			     MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
			
			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA);
		    }
		    
		    recv_complete = TRUE;
		    break;
		}

		/* if we are receiving data for unexpected message that the application has since posted a matching receive
		   request, but an error occurred during the posting process, then complete the request regardless of the message
		   type. */
		case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_ERROR:
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			"ERROR: finished receiving unexp-posted data, but an error occurred when the request was posted: vc="
			MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle,
			MPIG_PTR_CAST(rreq)));
			
		    mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
		    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
		    recv_complete = TRUE;
		    break;
		}

		/* if we are receiving data for an unexpected message that the application has since cancelled, then destroy the
		   request.  MT-NOTE: in most cases, destroying the request from a thread other than the main thread is unsafe
		   because the request may contain references to objects that are not thread safe, such as a communicator or
		   datatype.  however, in this case, the request has yet to be populated with such objects and thus may be safely
		   destroyed.. */
		case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_SREQ_CANCELLED:
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			 "finished receiving unexpected eager data from cancelled send; completing request: vc="
			 MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle,
			 MPIG_PTR_CAST(rreq)));

		    mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
		    MPIU_Assert(rreq_complete);
		    mpig_request_destroy(rreq);
		    rreq_complete = FALSE; /* reset to avoid attempt to complete below */
		    recv_complete = TRUE;
		    break;
		}
		/* if we are draining data from the network because of an unexpected rsend message... */
		case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_RSEND_DATA:
		{
		    MPIU_Size_t nbytes;
		    
		    MPIU_Assert(mpig_cm_xio_request_get_msg_type(rreq) == MPIG_CM_XIO_MSG_TYPE_EAGER_DATA);
		    
		    nbytes = MPIG_MIN((mpig_cm_xio_stream_get_size(rreq) - mpig_cm_xio_stream_get_cur_pos(rreq)),
				      mpig_databuf_get_size(rreq_cmu->databuf));
		    if (nbytes > 0)
		    {
			mpig_iov_reset(rreq_cmu->iov, 0);
			mpig_iov_add_entry(rreq_cmu->iov, mpig_databuf_get_base_ptr(rreq_cmu->databuf), nbytes);
			mpig_databuf_set_eod(rreq_cmu->databuf, nbytes);
			mpig_cm_xio_stream_inc_cur_pos(rreq, nbytes);

			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "draining more rsend data: vc=" MPIG_PTR_FMT ", rreq="
			    MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
	    
			mpig_cm_xio_register_read_rreq(vc, rreq, &mpi_errno);
			MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|reg_read_rreq");
		    }
		    else
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "finished draining more rsend data: vc=" MPIG_PTR_FMT ", rreq="
			    MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
	    
			mpig_cm_xio_request_dec_cc(rreq, &rreq_complete);
			mpig_cm_xio_request_set_state(rreq, MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE);
			recv_complete = TRUE;
		    }
		    break;
		}
		
		default:
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
			"INTERNAL ERROR - receive request state not valid for operation: %s",
			mpig_cm_xio_request_state_get_string(mpig_cm_xio_request_get_state(rreq))));
		    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN,
			"**globus|cm_xio|req_recv_state", "**globus|cm_xio|req_recv_state %s",
			mpig_cm_xio_request_state_get_string(mpig_cm_xio_request_get_state(rreq)));
		    break;
		}
	    }
	}
	mpig_request_mutex_unlock(rreq);
	rreq_locked = FALSE;

	/* if all of the data for this request has been received, then regiser a read to receive the next incoming message */
	if (recv_complete)
	{
	    vc_cmu->active_rreq = NULL;
	    mpi_errno = mpig_cm_xio_recv_next_msg(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|recv_next_msg");
	}
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

  fn_return:
    /* if all tasks associated with the request have completed, then added it to the completion queue */
    if (rreq_complete) mpig_cm_xio_rcq_enq(rreq);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exit state: vc=" MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle, MPIG_PTR_CAST(rreq)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_recv_handle_read_msg_data);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpig_request_mutex_unlock_conditional(rreq, (rreq_locked));
	/* FIXME: attach an error to the request and add it to the completion queue */
	
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_proc_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_recv_handle_read_msg_data() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					      END RECEIVE MESSAGE HEADERS AND DATA
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN FAULT HANDLING AND REPORTING
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_fault_handle_async_error(int mpi_errno);

MPIG_STATIC void mpig_cm_xio_fault_handle_async_proc_vc_error(mpig_vc_t * vc, int mpi_errno);

MPIG_STATIC int mpig_cm_xio_fault_handle_sync_proc_vc_error(mpig_vc_t * vc, int mpi_errno);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <void> mpig_cm_xio_fault_handle_async_error([IN/OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_fault_handle_async_error
MPIG_STATIC void mpig_cm_xio_fault_handle_async_error(const int mpi_errno)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_fault_handle_async_error);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_fault_handle_async_error);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "entering: mpi_errno="
	MPIG_ERRNO_FMT, mpi_errno));

    /* FIXME: tell the progress engine that an error has occurred, and have it return the error to the application during the
       next call to MPI_Test/Wait.  The error could also be returned by other MPI calls. */
    MPID_Abort(NULL, mpi_errno, 13, "ERROR: Asynchronous communication failure");

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_fault_handle_async_error);
    return;
}
/* mpig_cm_xio_fault_handle_async_error() */


/*
 * <void> mpig_cm_xio_fault_handle_async_proc_vc_error([IN/OUT] proc_vc, [IN] mpi_errno)
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_fault_handle_async_proc_vc_error
MPIG_STATIC void mpig_cm_xio_fault_handle_async_proc_vc_error(mpig_vc_t * const proc_vc, const int mpi_errno_in)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = mpi_errno_in;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_fault_handle_async_proc_vc_error);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_fault_handle_async_proc_vc_error);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "entering: proc_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(proc_vc), mpi_errno));

    /* if the VC has already failed, then just return */
    if (mpig_cm_xio_vc_has_failed(proc_vc))
    {
	char * msg;

        msg = (char *) MPIU_Malloc(MPIG_ERR_STRING_SIZE);
	if (msg)
        {
	    MPIR_Err_get_string(mpi_errno, msg, MPIG_ERR_STRING_SIZE, NULL);
	}

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "WARNING: VC fault handler called again: proc_vc="
	    MPIG_PTR_FMT ",\nmsg=\"%s\"", MPIG_PTR_CAST(proc_vc), msg ? msg : "memory allocation error"));

	MPIU_Free(msg);
	
	goto fn_return;
    }
    
    mpi_errno = mpig_cm_xio_fault_handle_sync_proc_vc_error(proc_vc, mpi_errno);

    /* if the error has not been cleared up by the synchronous VC fault handler, then report the error(s) using the asynchronous
       fault handler */
    if (mpi_errno)
    {
	mpig_vc_mutex_unlock(proc_vc);
	mpig_cm_xio_fault_handle_async_error(mpi_errno);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "exiting: proc_vc= " MPIG_PTR_FMT,
	MPIG_PTR_CAST(proc_vc)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_fault_handle_async_proc_vc_error);
    return;
}
/* mpig_cm_xio_fault_handle_async_proc_vc_error() */


/*
 * <mpi_errno> mpig_cm_xio_fault_handle_sync_proc_vc_error([IN/OUT] proc_vc, [IN] mpi_errno)
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_fault_handle_sync_proc_vc_error
MPIG_STATIC int mpig_cm_xio_fault_handle_sync_proc_vc_error(mpig_vc_t * proc_vc, int mpi_errno_in)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mrc;
    int mpi_errno = mpi_errno_in;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_fault_handle_sync_proc_vc_error);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_fault_handle_sync_proc_vc_error);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "entering: proc_vc=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(proc_vc)));

    /* if the VC has already failed, then just return */
    if (mpig_cm_xio_vc_has_failed(proc_vc))
    {
	char * msg;

        msg = (char *) MPIU_Malloc(MPIG_ERR_STRING_SIZE);
	if (msg)
        {
	    MPIR_Err_get_string(mpi_errno, msg, MPIG_ERR_STRING_SIZE, NULL);
	}

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "WARNING: proc VC fault handler called again: proc_vc="
	    MPIG_PTR_FMT ",\nmsg=\"%s\"", MPIG_PTR_CAST(proc_vc), msg ? msg : "memory allocation error"));

	MPIU_Free(msg);
	
	goto fn_return;
    }
    
    if (mpig_cm_xio_vc_is_connecting(proc_vc))
    {
	mpig_cm_xio_vc_set_state(proc_vc, MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING);
    }
    else if (mpig_cm_xio_vc_is_connected(proc_vc))
    {
	mpig_cm_xio_vc_set_state(proc_vc, MPIG_CM_XIO_VC_STATE_FAILED_CONNECTION);
    }
    else if (mpig_cm_xio_vc_is_disconnecting(proc_vc))
    {
	mpig_cm_xio_vc_set_state(proc_vc, MPIG_CM_XIO_VC_STATE_FAILED_DISCONNECTING);
    }
    else if (mpig_cm_xio_vc_is_closing(proc_vc))
    {
	mpig_cm_xio_vc_set_state(proc_vc, MPIG_CM_XIO_VC_STATE_FAILED_CLOSING);
    }
    else
    {
	MPIU_Assertp(FALSE && "unhandled VC state");
    }

    /* close the XIO handle associated with the VC */
    if (proc_vc->cmu.xio.handle != NULL)
    {
	mrc = mpig_cm_xio_disconnect_close_proc(proc_vc);
	if (mrc)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: attempt to close the proc VC failed: "
		"proc_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(proc_vc)));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_close_proc");
	}
    }

    /* FIXME: mark the VC and any associated communicators as bad.  terminate any requests associated with those
       communicators. */
    
    /* XXX: attempt reconnect */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "exiting: proc_vc= " MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(proc_vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_fault_handle_sync_proc_vc_error);
    return mpi_errno;
}
/* mpig_cm_xio_fault_handle_sync_proc_vc_error() */


#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						END FAULT HANDLING AND REPORTING
**********************************************************************************************************************************/
