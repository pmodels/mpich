/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

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
#define FUNCNAME MPIDU_Sock_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_accept(struct MPIDU_Sock * listener, 
		      struct MPIDU_Sock_set * sock_set, void * user_ptr,
		      struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    struct sockaddr_in addr;
    socklen_t addr_len;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(listener, mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK_SET(sock_set, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(listener);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(listener);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->type != MPIDU_SOCKI_TYPE_LISTENER)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
		 FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
		 "**sock|listener_bad_sock", "**sock|listener_bad_sock %d %d",
		 pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }
    
    if (pollinfo->state != MPIDU_SOCKI_STATE_CONNECTED_RO && 
	pollinfo->state != MPIDU_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
		FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
	     "**sock|listener_bad_state", "**sock|listener_bad_state %d %d %d",
		pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->state);
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

    if (pollinfo->state != MPIDU_SOCKI_STATE_CLOSING)
    {
	/*
	 * Unless the listener sock is being closed, add it back into the 
	 * poll list so that new connections will be detected.
	 */
	MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (fd == -1)
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
			     FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK,
			     "**sock|nosock", NULL);
	}
	else if (errno == ENOBUFS || errno == ENOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
				"**sock|osnomem", NULL);
	}
	else if (errno == EBADF || errno == ENOTSOCK || errno == EOPNOTSUPP)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
                           FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
			  "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
			  pollinfo->sock_set->id, pollinfo->sock_id, 
			  pollinfo->fd);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                           FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK,
			   "**sock|poll|accept", "**sock|poll|accept %d %s", 
			   errno, MPIU_Strerror(errno));
	}
	
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    flags = fcntl(fd, F_GETFL, 0);
    /* FIXME: There should be a simpler macro for reporting errno messages */
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", 
			 errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s",
			 errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    /* --BEGIN ERROR HANDLING-- */
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
			 FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
			 "**sock|poll|nodelay", "**sock|poll|nodelay %d %s", 
                         errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Verify that the socket buffer size is correct
     */
    /* FIXME: Who sets the socket buffer size?  Why isn't the test
       made at that time? */
#if 1
    mpi_errno = MPIDU_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
#else
    if (MPIDU_Socki_socket_bufsz > 0)
    {
	int bufsz;
	socklen_t bufsz_len;

	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
	/* FIXME: There's normally no need to check that the socket buffer
	   size was set to the requested size.  This should only be part of
	   some more verbose diagnostic output, not a general action */
	/* --BEGIN ERROR HANDLING-- */
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || 
		bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPL_msg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}
	/* --END ERROR HANDLING-- */

    	bufsz_len = sizeof(bufsz);
	rc = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
	/* FIXME: There's normally no need to check that the socket buffer
	   size was set to the requested size.  This should only be part of
	   some more verbose diagnostic output, not a general action */
	/* FIXME: Cut and paste code is a disaster waiting to happen. 
	   Particularly in any non-performance critical section,
	   create a separate routine instead of using cut and paste. */
	/* --BEGIN ERROR HANDLING-- */
	if (rc == 0)
	{
	    if (bufsz < MPIDU_Socki_socket_bufsz * 0.9 || 
		bufsz < MPIDU_Socki_socket_bufsz * 1.0)
	    {
		MPL_msg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDU_Socki_socket_bufsz, bufsz);
	    }
	}
	/* --END ERROR HANDLING-- */
    }
#endif    
    /*
     * Allocate and initialize sock and poll structures.
     *
     * NOTE: pollfd->fd is initialized to -1.  It is only set to the true fd 
     * value when an operation is posted on the sock.  This
     * (hopefully) eliminates a little overhead in the OS and avoids 
     * repetitive POLLHUP events when the connection is closed by
     * the remote process.
     */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;
    
    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (fd != -1)
    {
	close(fd);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end MPIDU_Sock_accept() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, 
		    MPIU_Size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    size_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READ);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READ);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);
    
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
	MPIDI_FUNC_ENTER(MPID_STATE_READ);
	nb = read(pollinfo->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_READ);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (MPIU_Size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;
	
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPIDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", 
	    pollinfo->sock_set->id, pollinfo->sock_id);
	
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    /* A write is posted on this connection.  Enqueue an event for
	       the write indicating the connection is closed. */
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE,
				      pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, 
					    FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
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
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event 
		   for the write indicating the connection is closed. */
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, 
					pollinfo->write_nb, pollinfo->user_ptr,
					mpi_errno, mpi_errno, fn_exit);
		MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READ);
    return mpi_errno;
}
/* end MPIDU_Sock_read() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, 
		     MPIU_Size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len 
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent 
     * limitation in the Sock interface?
     */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = MPL_large_readv(pollinfo->fd, iov, iov_n);
	MPIDI_FUNC_EXIT(MPID_STATE_READV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (MPIU_Size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;
	
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPIDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", 
	    pollinfo->sock_set->id, pollinfo->sock_id);
	
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    
	    /* A write is posted on this connection.  Enqueue an event 
	       for the write indicating the connection is closed. */
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, 
				      pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME,
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
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
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event 
		   for the write indicating the connection is closed. */
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, 
					pollinfo->write_nb, pollinfo->user_ptr,
					  mpi_errno, mpi_errno, fn_exit);
		MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READV);
    return mpi_errno;
}
/* end MPIDU_Sock_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, 
		     MPIU_Size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITE);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	nb = write(pollinfo->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITE);
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
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, 
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
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
	     * MPIDU_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
    return mpi_errno;
}
/* end MPIDU_Sock_write() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, MPIU_Size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITEV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);
    
    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len 
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent 
     * limitation in the Sock interface?
     */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	nb = MPL_large_writev(pollinfo->fd, iov, iov_n);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = (MPIU_Size_t) nb;
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
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, 
						__LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
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
	     * MPIDU_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
    return mpi_errno;
}
/* end MPIDU_Sock_writev() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_wakeup(struct MPIDU_Sock_set * sock_set)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WAKEUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WAKEUP);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK_SET(sock_set, mpi_errno, fn_exit);

    /* FIXME: We need (1) a standardized test for including multithreaded
       code and (2) include support for user requests for a lower-level
       of thread safety.  Finally, things like this should probably 
       be implemented as an abstraction (e.g., wakeup_progress_threads?)
       rather than this specific code.  */
#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
	struct pollinfo * pollinfo;
	
	pollinfo = MPIDU_Socki_sock_get_pollinfo(sock_set->intr_sock);
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WAKEUP, 0, NULL, 
				  mpi_errno, mpi_errno, fn_exit);
	MPIDU_Socki_wakeup(sock_set);
    }
    MPIU_THREAD_CHECK_END;
#   endif

#ifdef MPICH_IS_THREADED
    fn_exit:
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAKEUP);
    return mpi_errno;
}
/* end MPIDU_Sock_wakeup() */


