/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


/* FIXME: Why is this the _immed file (what does immed stand for?) */

/* FIXME: What do any of these routines do?  What are the arguments?
   Special conditions (see the FIXME on len = SSIZE_MAX)?  preconditions?
   postconditions? */

/* FIXME: What does this function do?  What are its arguments?
   It appears to execute a nonblocking accept call */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_accept
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_accept(struct SMPDU_Sock * listener, 
		      struct SMPDU_Sock_set * sock_set, void * user_ptr,
		      struct SMPDU_Sock ** sockp)
{
    struct SMPDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    struct sockaddr_in addr;
    socklen_t addr_len;
    long flags;
    int nodelay;
    int rc;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(listener, result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK_SET(sock_set, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(listener);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(listener);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->type != SMPDU_SOCKI_TYPE_LISTENER)
    {
        /*
	result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, 
		 FCNAME, __LINE__, SMPDU_SOCK_ERR_BAD_SOCK,
		 "**sock|listener_bad_sock", "**sock|listener_bad_sock %d %d",
		 pollinfo->sock_set->id, pollinfo->sock_id);
        */
        smpd_err_printf("Bad listen socket\n");
	goto fn_exit;
    }
    
    if (pollinfo->state != SMPDU_SOCKI_STATE_CONNECTED_RO && 
	pollinfo->state != SMPDU_SOCKI_STATE_CLOSING)
    {
        /*
	result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, 
		FCNAME, __LINE__, SMPDU_SOCK_ERR_BAD_SOCK,
	     "**sock|listener_bad_state", "**sock|listener_bad_state %d %d %d",
		pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->state);
        */
        smpd_err_printf("Bad listen socket state, sock_set->id =  %d, sock_id = %d, state =  %d\n", pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->state);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Get a socket for the new connection from the operating system.  
     * Make the socket nonblocking, and disable Nagle's
     * alogorithm (to minimize latency of small messages).
     */
    addr_len = sizeof(struct sockaddr_in);
    /* FIXME: Either use the syscall macro or correctly wrap this in a
       test for EINTR */
    fd = accept(pollinfo->fd, (struct sockaddr *) &addr, &addr_len);

    if (pollinfo->state != SMPDU_SOCKI_STATE_CLOSING)
    {
	/*
	 * Unless the listener sock is being closed, add it back into the 
	 * poll list so that new connections will be detected.
	 */
	SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (fd == -1)
    {
        /*
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, 
			     FCNAME, __LINE__, SMPDU_SOCK_ERR_NO_NEW_SOCK,
			     "**sock|nosock", NULL);
	}
	else if (errno == ENOBUFS || errno == ENOMEM)
	{
	    result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, 
				FCNAME, __LINE__, SMPDU_SOCK_ERR_NOMEM,
				"**sock|osnomem", NULL);
	}
	else if (errno == EBADF || errno == ENOTSOCK || errno == EOPNOTSUPP)
	{
	    result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, 
                           FCNAME, __LINE__, SMPDU_SOCK_ERR_BAD_SOCK,
			  "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
			  pollinfo->sock_set->id, pollinfo->sock_id, 
			  pollinfo->fd);
	}
	else
	{
	    result = MPIR_Err_create_code(SMPD_SUCCESS, MPIR_ERR_RECOVERABLE,
                           FCNAME, __LINE__, SMPDU_SOCK_ERR_NO_NEW_SOCK,
			   "**sock|poll|accept", "**sock|poll|accept %d %s", 
			   errno, MPIU_Strerror(errno));
	}
        */
        smpd_err_printf("Error accepting new connection, errno = %d (%s)\n", errno, strerror(errno));
	
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    flags = fcntl(fd, F_GETFL, 0);
    /* FIXME: There should be a simpler macro for reporting errno messages */
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
        /*
	result = MPIR_Err_create_code(SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, SMPD_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", 
			 errno, MPIU_Strerror(errno));
        */
        smpd_err_printf("Error getting status flags for new conn fd, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        /*
	result = MPIR_Err_create_code(SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, SMPD_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s",
			 errno, MPIU_Strerror(errno));
        */
        smpd_err_printf("Error setting new conn to non-blocking, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    /* --BEGIN ERROR HANDLING-- */
    if (rc != 0)
    {
        /*
	result = MPIR_Err_create_code(SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, SMPD_FAIL,
			 "**sock|poll|nodelay", "**sock|poll|nodelay %d %s", 
                         errno, MPIU_Strerror(errno));
        */
        smpd_err_printf("Error setting TCP_NODELAY for the new conn, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Verify that the socket buffer size is correct
     */
    /* FIXME: Who sets the socket buffer size?  Why isn't the test
       made at that time? */
    result = SMPDU_Sock_SetSockBufferSize( fd, 1 );
    if (result) {
        smpd_err_printf("Error setting socket buffer size for new conn\n");
        goto fn_fail;    
    }

    /*
     * Allocate and initialize sock and poll structures.
     *
     * NOTE: pollfd->fd is initialized to -1.  It is only set to the true fd 
     * value when an operation is posted on the sock.  This
     * (hopefully) eliminates a little overhead in the OS and avoids 
     * repetitive POLLHUP events when the connection is closed by
     * the remote process.
     */
    result = SMPDU_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (result != SMPD_SUCCESS)
    {
        /*
	result = MPIR_Err_create_code(result, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
        */
        smpd_err_printf("Error allocating sock structure for new conn\n");
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = SMPDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;
    
    *sockp = sock;

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (fd != -1)
    {
	close(fd);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end SMPDU_Sock_accept() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_read
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_read(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, 
		    SMPDU_Sock_size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    size_t nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, result, fn_exit);
    
    /* FIXME: multiple passes should be made if 
       len > SSIZE_MAX and nb == SSIZE_MAX */
    /* FIXME: This is a scary test/assignment.  It needs an explanation 
       (presumably that this routine will be called again if len is 
       shortened.  However, in that case, the description of the routine 
       (which is also missing!!!!) needs to be very clear about this
       requirement.  */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	smpd_enter_fn(FCNAME);
	nb = read(pollinfo->fd, buf, len);
	smpd_exit_fn(FCNAME);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (SMPDU_Sock_size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;
	
        /*
	result = MPIR_Err_create_code(
	    SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    SMPDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", 
	    pollinfo->sock_set->id, pollinfo->sock_id);
        */
        smpd_dbg_printf("Trying to read from a closed socket, errno = %d (%s)\n", errno, strerror(errno));
        result = SMPD_FAIL;
	
	if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    /* A write is posted on this connection.  Enqueue an event for
	       the write indicating the connection is closed. */
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE,
				      pollinfo->write_nb, pollinfo->user_ptr,
				      result, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	result = SMPDU_Socki_os_to_result(pollinfo, errno, 
					    FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(result))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the 
	     * data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event 
		   for the write indicating the connection is closed. */
		SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, 
					pollinfo->write_nb, pollinfo->user_ptr,
					result, result, fn_exit);
		SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_read() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_readv
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_readv(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, 
		     SMPDU_Sock_size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, result, fn_exit);

    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len 
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent 
     * limitation in the Sock interface?
     */
    do
    {
	smpd_enter_fn(FCNAME);
	nb = readv(pollinfo->fd, iov, iov_n);
	smpd_exit_fn(FCNAME);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (SMPDU_Sock_size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;
	
        /*
	result = MPIR_Err_create_code(
	    SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    SMPDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", 
	    pollinfo->sock_set->id, pollinfo->sock_id);
        */
        smpd_err_printf("Error trying to read from a closed socket, errno = %d (%s)\n", errno, strerror(errno));
	
	if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    
	    /* A write is posted on this connection.  Enqueue an event 
	       for the write indicating the connection is closed. */
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, 
				      pollinfo->write_nb, pollinfo->user_ptr,
				      result, result, fn_exit);
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	result = SMPDU_Socki_os_to_result(pollinfo, errno, FCNAME,
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(result))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the 
	     * data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event 
		   for the write indicating the connection is closed. */
		SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, 
					pollinfo->write_nb, pollinfo->user_ptr,
					  result, result, fn_exit);
		SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_readv() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_write
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_write(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, 
		     SMPDU_Sock_size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, result, fn_exit);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	smpd_enter_fn(FCNAME);
	nb = write(pollinfo->fd, buf, len);
	smpd_exit_fn(FCNAME);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_written = 0;
    }
    else
    {
	int disconnected;
	
	*num_written = 0;
	
	result = SMPDU_Socki_os_to_result(pollinfo, errno, FCNAME, 
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(result))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data 
	     * structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    /*
	     * The connection is dead but data may still be in the socket
	     * buffer; thus, we change the state and let
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
/* end SMPDU_Sock_write() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_writev
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_writev(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, result, fn_exit);
    
    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len 
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent 
     * limitation in the Sock interface?
     */
    do
    {
	smpd_enter_fn(FCNAME);
	nb = writev(pollinfo->fd, iov, iov_n);
	smpd_exit_fn(FCNAME);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = (SMPDU_Sock_size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_written = 0;
    }
    else
    {
	int disconnected;
	
	*num_written = 0;
	
	result = SMPDU_Socki_os_to_result(pollinfo, errno, FCNAME, 
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(result))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the 
	     * data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    /*
	     * The connection is dead but data may still be in the socket 
	     * buffer; thus, we change the state and let
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
/* end SMPDU_Sock_writev() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_wakeup
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_wakeup(struct SMPDU_Sock_set * sock_set)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK_SET(sock_set, result, fn_exit);

    /* FIXME: We need (1) a standardized test for including multithreaded
       code and (2) include support for user requests for a lower-level
       of thread safety.  Finally, things like this should probably 
       be implemented as an abstraction (e.g., wakeup_progress_threads?)
       rather than this specific code.  */

    fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_wakeup() */


