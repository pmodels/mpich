/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Make sure that we can properly ensure atomic access to the poll routine */

static int SMPDU_Socki_handle_pollhup(struct pollfd * const pollfd, 
				      struct pollinfo * const pollinfo);
static int SMPDU_Socki_handle_pollerr(struct pollfd * const pollfd, 
				      struct pollinfo * const pollinfo);
static int SMPDU_Socki_handle_read(struct pollfd * const pollfd, 
				   struct pollinfo * const pollinfo);
static int SMPDU_Socki_handle_write(struct pollfd * const pollfd, 
				    struct pollinfo * const pollinfo);
static int SMPDU_Socki_handle_connect(struct pollfd * const pollfd, 
				      struct pollinfo * const pollinfo);

/*
 * SMPDU_Sock_wait()
 *
 * NOTES:
 *
 * For fatal errors, the state of the connection progresses directly to the 
 * failed state and the connection is marked inactive in
 * the poll array.  Under normal conditions, the fatal error should result in 
 * the termination of the process; but, if that
 * doesn't happen, we try to leave the implementation in a somewhat sane state.
 *
 * In the multithreaded case, only one routine at a time may call this routine
 * To permit progress by other threads, it will release any global lock or
 * coarse-grain critical section.
 */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_wait
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_wait(struct SMPDU_Sock_set * sock_set, int millisecond_timeout,
		    struct SMPDU_Sock_event * eventp)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    for (;;)
    { 
	int elem=0;   /* Keep compiler happy */
	int n_fds;
	int n_elems;
	int found_active_elem = FALSE;

	result = SMPDU_Socki_event_dequeue(sock_set, &elem, eventp);
	if (result == SMPD_SUCCESS) {
	    struct pollinfo * pollinfo;
	    int flags;
	    
	    if (eventp->op_type != SMPDU_SOCK_OP_CLOSE)
	    {
		break;
	    }

	    pollinfo = &sock_set->pollinfos[elem];

	    /*
	     * Attempt to set socket back to blocking.  This *should* prevent 
	     * any data in the socket send buffer from being
	     * discarded.  Instead close() will block until the buffer is 
	     * flushed or the connection timeouts and is considered
	     * lost.  Theoretically, this could cause the SMPDU_Sock_wait() to
	     * hang indefinitely; however, the calling code
	     * should ensure this will not happen by going through a shutdown 
	     * protocol before posting a close operation.
	     *
	     * FIXME: If the attempt to set the socket back to blocking fails, 
	     * we presently ignore it.  Should we return an
	     * error?  We need to define acceptible data loss at close time.  
	     * MS Windows has worse problems with this, so it
	     * may not be possible to make any guarantees.
	     */
	    flags = fcntl(pollinfo->fd, F_GETFL, 0);
	    if (flags != -1)
	    { 
		fcntl(pollinfo->fd, F_SETFL, flags & ~O_NONBLOCK);
	    }

	    /* FIXME: return code?  If an error occurs do we return it 
	       instead of the error specified in the event? */
	    close(pollinfo->fd);

	    SMPDU_Socki_sock_free(pollinfo->sock);

	    break;
	}

	for(;;)
	{
	    {
		smpd_enter_fn(FCNAME);
		n_fds = poll(sock_set->pollfds, sock_set->poll_array_elems, 
			     millisecond_timeout);
		smpd_exit_fn(FCNAME);
	    }

	    if (n_fds > 0)
	    {
		break;
	    }
	    else if (n_fds == 0)
	    {
		result = SMPD_FAIL;
		goto fn_exit;
	    }
	    else if (errno == EINTR)
	    {
		if (millisecond_timeout != SMPDU_SOCK_INFINITE_TIME)
		{
		    result = SMPD_FAIL;
		    goto fn_exit;
		}

		continue;
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
                result = SMPD_FAIL;
                smpd_err_printf("poll() failed, errno =%d (%s)\n", errno, strerror(errno));
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	elem = sock_set->starting_elem;
	n_elems = sock_set->poll_array_elems;
	while (n_fds > 0 && n_elems > 0)
	{
	    /*
	     * Acquire pointers to the pollfd and pollinfo structures for the next element
	     *
	     * NOTE: These pointers could become stale, if a new sock were to be allocated during the processing of the element.
	     * At present, none of the handler routines allocate a sock, so the issue does not arise.
	     */
	    struct pollfd * const pollfd = &sock_set->pollfds[elem];
	    struct pollinfo * const pollinfo = &sock_set->pollinfos[elem];
	
	    MPIU_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);
	    MPIU_Assert(pollfd->fd >= 0 || pollfd->fd == -1);
			
	    if (pollfd->fd < 0 || pollfd->revents == 0)
	    {
		/* This optimization assumes that most FDs will not have a pending event. */
		n_elems -= 1;
		elem = (elem + 1 < sock_set->poll_array_elems) ? elem + 1 : 0;
		continue;
	    }

	    if (found_active_elem == FALSE)
	    {
		found_active_elem = TRUE;
		sock_set->starting_elem = (elem + 1 < sock_set->poll_array_elems) ? elem + 1 : 0;
	    }

	    if (pollfd->revents & POLLNVAL)
	    {
                result = SMPD_FAIL;
                smpd_err_printf("Invalid socket passed to poll()\n");
		goto fn_exit;
	    }

	    /* --BEGIN ERROR HANDLING-- */
	    if (pollfd->revents & POLLHUP)
	    {
		result = SMPDU_Socki_handle_pollhup(pollfd, pollinfo);
		if (result != SMPD_SUCCESS)
		{
		    goto fn_exit;
		}
	    }

	    /* According to Stevens, some errors are reported as normal data 
	       (POLLIN) and some are reported with POLLERR. */
	    if (pollfd->revents & POLLERR)
	    {
		result = SMPDU_Socki_handle_pollerr(pollfd, pollinfo);
		if (result != SMPD_SUCCESS)
		{
		    goto fn_exit;
		}
	    }
	    /* --END ERROR HANDLING-- */
	    
	    if (pollfd->revents & POLLIN)
	    {
		if (pollinfo->type == SMPDU_SOCKI_TYPE_COMMUNICATION)
		{ 
		    if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RW || 
			pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RO)
		    {
			result = SMPDU_Socki_handle_read(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (result != SMPD_SUCCESS)
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    /* --BEGIN ERROR HANDLING-- */
		    else
		    {
                        result = SMPD_FAIL;
                        smpd_err_printf("Received data in socket on an unhandled state (%d)\n", pollinfo->state);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */

		}
		else if (pollinfo->type == SMPDU_SOCKI_TYPE_LISTENER)
		{
		    pollfd->events &= ~POLLIN;
		    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_ACCEPT, 0, pollinfo->user_ptr,
					      SMPD_SUCCESS, result, fn_exit);
		}
		/* --BEGIN ERROR HANDLING-- */
		else
		{
                    result = SMPD_FAIL;
                    smpd_err_printf("Received data in socket on an unhandled state (%d) \n", pollinfo->state);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->type == SMPDU_SOCKI_TYPE_COMMUNICATION)
		{
		    if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RW)
		    {
			result = SMPDU_Socki_handle_write(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (result != SMPD_SUCCESS)
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    else if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTING)
		    {
			result = SMPDU_Socki_handle_connect(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (result != SMPD_SUCCESS)
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    /* --BEGIN ERROR HANDLING-- */
		    else
		    {
                        result = SMPD_FAIL;
                        smpd_err_printf("Unable to write data on socket in state = %d\n", pollinfo->state);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */
		}
		/* --BEGIN ERROR HANDLING-- */
		else
		{
                    result = SMPD_FAIL;
                    smpd_err_printf("Unable to write data on socket in state = %d\n", pollinfo->state);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }

	    n_fds -= 1;
	    n_elems -= 1;
	    elem = (elem + 1 < sock_set->poll_array_elems) ? elem + 1 : 0;
	}
    }
    
  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_handle_pollhup
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_handle_pollhup(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RW)
    {
	/*
	 * If a write was posted then cancel it and generate an connection closed event.  If a read is posted, it will be handled
	 * by the POLLIN handler.
	 */
	/* --BEGIN ERROR HANDLING-- */
	if (pollfd->events & POLLOUT)
	{
	    int event_result;
	    
            smpd_err_printf("Connection closed, cancelling pending writes, %d bytes (user_ptr = %d)\n",
                                pollinfo->write_nb, pollinfo->user_ptr);

	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_result, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RO;
	}
	/* --END ERROR HANDLING-- */
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLHUP event.
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIU_Assert(pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_DISCONNECTED)
    {
	/*
	 * We should never reach this state because pollfd->fd should be set to -1 if we are in the disconnected state.
	 */
	MPIU_Assert(pollinfo->state == SMPDU_SOCKI_STATE_DISCONNECTED && pollfd->fd == -1);
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIU_Assert(pollinfo->state == SMPDU_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
        result = SMPD_FAIL;
        smpd_err_printf("pollhup on sock in invalid state (%d)\n", pollinfo->state);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_handle_pollhup() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_handle_pollerr
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_handle_pollerr(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->type != SMPDU_SOCKI_TYPE_COMMUNICATION)
    { 
        result = SMPD_FAIL;
        smpd_err_printf("poll err on sock of unhandled type (%d)\n", pollinfo->type);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
		
    if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RW)
    { 
	/*
	 * Stevens suggests that some older version of UNIX did not properly reset so_error, which could allow POLLERR to be
	 * continuously triggered.  We remove the socket from the poll list (pollfd->fd = 1) in order to prevent this issue.
	 * Here, we simple check that things are as we expect them to be.
	 */
	MPIU_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);

	/* If a write was posted then cancel it and generate an write completion event */
	if (pollfd->events & POLLOUT)
	{
	    int disconnected;
	    int os_errno;
	    int event_result;
	    
	    SMPDU_SOCKI_GET_SOCKET_ERROR(pollinfo, os_errno, result, fn_exit);
		
	    event_result = SMPDU_Socki_os_to_result(pollinfo, os_errno, FCNAME, __LINE__, &disconnected);
	    /* --BEGIN ERROR HANDLING-- */
	    if (event_result != SMPD_SUCCESS)
	    {
		result = event_result;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
			
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_result, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLERR event.  
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIU_Assert(pollinfo->state == SMPDU_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIU_Assert(pollinfo->state == SMPDU_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    else if (pollinfo->state == SMPDU_SOCKI_STATE_DISCONNECTED)
    {
	/* We are already disconnected!  Why are we handling an error? */
	MPIU_Assert(pollfd->fd == -1);
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
        result = SMPD_FAIL;
        smpd_err_printf("Received poll err on sock in unhandled state (%d)\n", pollinfo->state);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_handle_pollerr() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_handle_read
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    do
    {
	if (pollinfo->read_iov_flag)
	{ 
	    smpd_enter_fn(FCNAME);
	    nb = readv(pollinfo->fd, pollinfo->read.iov.ptr + pollinfo->read.iov.offset,
		       pollinfo->read.iov.count - pollinfo->read.iov.offset);
	    smpd_exit_fn(FCNAME);
	}
	else
	{
	    smpd_enter_fn(FCNAME);
	    nb = read(pollinfo->fd, pollinfo->read.buf.ptr + pollinfo->read_nb,
		      pollinfo->read.buf.max - pollinfo->read_nb);
	    smpd_exit_fn(FCNAME);
	}
    }
    while (nb < 0 && errno == EINTR);
	
    if (nb > 0)
    {
	int done;
	    
	pollinfo->read_nb += nb;

	done = pollinfo->read_iov_flag ?
	    SMPDU_Socki_adjust_iov(nb, pollinfo->read.iov.ptr, pollinfo->read.iov.count, &pollinfo->read.iov.offset) :
	    (pollinfo->read_nb >= pollinfo->read.buf.min);

	if (done)
	{
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				      SMPD_SUCCESS, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
	}
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	int event_result=SMPD_FAIL;
	
        smpd_dbg_printf("Connection closed while trying to read on socket, sock_set->id = %d, sock_id = %d\n",
                            pollinfo->sock_set->id, pollinfo->sock_id);
	if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_result, result, fn_exit);
	}
	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_result, result, fn_exit);
	
	SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;

    }
    /* --END ERROR HANDLING-- */
    else if (errno == EAGAIN && errno == EWOULDBLOCK)
    {
	/* do nothing... */
	goto fn_exit;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	int disconnected;
	int event_result;
	
	event_result = SMPDU_Socki_os_to_result(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (event_result != SMPD_SUCCESS)
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data 
	     * structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    result = event_result;
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  event_result, result, fn_exit);
		SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
	}
	
	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_result, result, fn_exit);
	SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_handle_read() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_handle_write
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    do
    {
	if (pollinfo->write_iov_flag)
	{ 
	    smpd_enter_fn(FCNAME);
	    nb = writev(pollinfo->fd, pollinfo->write.iov.ptr + pollinfo->write.iov.offset,
			pollinfo->write.iov.count - pollinfo->write.iov.offset);
	    smpd_exit_fn(FCNAME);
	}
	else
	{
	    smpd_enter_fn(FCNAME);
	    nb = write(pollinfo->fd, pollinfo->write.buf.ptr + pollinfo->write_nb,
		       pollinfo->write.buf.max - pollinfo->write_nb);
	    smpd_exit_fn(FCNAME);
	}
    }
    while (nb < 0 && errno == EINTR);

    if (nb >= 0)
    {
	int done;
	    
	pollinfo->write_nb += nb;

	done = pollinfo->write_iov_flag ?
	    SMPDU_Socki_adjust_iov(nb, pollinfo->write.iov.ptr, pollinfo->write.iov.count, &pollinfo->write.iov.offset) :
	    (pollinfo->write_nb >= pollinfo->write.buf.min);

	if (done)
	{
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      SMPD_SUCCESS, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	/* do nothing... */
	goto fn_exit;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	int disconnected;
	int event_result;
	
	event_result = SMPDU_Socki_os_to_result(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (event_result != SMPD_SUCCESS)
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    result = event_result;
	    goto fn_exit;
	}

	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				  event_result, result, fn_exit);
	SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	if (disconnected)
	{
	    /*
	     * The connection is dead but data may still be in the socket buffer; thus, we change the state and let
	     * SMPDU_Sock_wait() clean things up.
	     */
	    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_handle_write() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_handle_connect
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    addr_len = sizeof(struct sockaddr_in);
    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == 0)
    {
	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SMPD_SUCCESS, result, fn_exit);
	pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RW;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	int event_result;
	
	SMPDU_SOCKI_GET_SOCKET_ERROR(pollinfo, pollinfo->os_errno, result, fn_exit);
        result = SMPD_FAIL;
        smpd_err_printf("Connect() failed, sock_set->id = %d, sock_id =%d, errno = %d(%s)\n",
	                    pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->os_errno, strerror(pollinfo->os_errno));
	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, event_result, result, fn_exit);
	pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
    }
    /* --END ERROR HANDLING-- */

    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
    
  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_handle_connect() */
