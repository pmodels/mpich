/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME: Provide an overview for the functions in this file */

#undef FUNCNAME 
#define FUNCNAME SMPDU_Sock_post_connect_ifaddr
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
/* 
 This routine connects to a particular address (in byte form; for ipv4, 
 the address is four bytes, typically the value of h_addr_list[0] in 
 struct hostent.  By avoiding a character name for an interface (we *never*
 connect to a host; we are *always* connecting to a particular interface 
 on a host), we avoid problems with DNS services, including lack of properly
 configured services and scalability problems.  As this routine uses 
 a four-byte field, it is currently restricted to ipv4.  This routine should
 evolve to support ipv4 and ipv6 addresses.

 This routine was constructed from SMPDU_Sock_post_connect by removing the 
 poorly placed use of gethostname within the middle of that routine and
 simply using the ifaddr field that is passed to this routine.  
 SMPDU_Sock_post_connect simply uses the hostname field to get the canonical
 IP address.  The original routine and its API was retained to allow backwards
 compatibility until it is determined that we can always use explicit addrs 
 needed in setting up the socket instead of character strings.
 */
int SMPDU_Sock_post_connect_ifaddr( struct SMPDU_Sock_set * sock_set, 
				    void * user_ptr, 
				    SMPDU_Sock_ifaddr_t *ifaddr, int port,
				    struct SMPDU_Sock ** sockp)
{
    struct SMPDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int rc;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);

    /*
     * Create a non-blocking socket with Nagle's algorithm disabled
     */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
	/* FIXME: It would be better to include a special formatting
	   clue for system error messages (e.g., %dSE; in the recommended
	   revision for error reporting (that is, value (errno) is an int, 
	   but should be interpreted as an System Error string) */
        result = SMPD_FAIL;
        smpd_err_printf("Creating socket failed, errno = %d (%s)\n", errno, strerror(errno));
        goto fn_fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        result = SMPD_FAIL;
        smpd_err_printf("Getting status flags for socket failed, errno = %d (%s)\n", errno, strerror(errno));
        goto fn_fail;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1) {
        result = SMPD_FAIL;
        smpd_err_printf("Setting socket to nonblocking failed, errno = %d (%s)\n", errno, strerror(errno));
        goto fn_fail;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0) {
        result = SMPD_FAIL;
        smpd_err_printf("Setting nodelay option on socket failed, errno = %d (%s)\n", errno, strerror(errno));
        goto fn_fail;
    }

    /*
     * Allocate and initialize sock and poll structures
     *
     * NOTE: pollfd->fd is initialized to -1.  It is only set to the true fd 
     * value when an operation is posted on the sock.  This
     * (hopefully) eliminates a little overhead in the OS and avoids 
     * repetitive POLLHUP events when the connection is closed by
     * the remote process.
     */
    result = SMPDU_Socki_sock_alloc(sock_set, &sock);
    if (result != SMPD_SUCCESS) {
        smpd_err_printf("Unable to alloc sock struct for socket\n");
        goto fn_fail;
    }

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = SMPDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, ifaddr->ifaddr, 
	   sizeof(addr.sin_addr.s_addr));
    addr.sin_port = htons(port);

    /*
     * Set and verify the socket buffer size
     */
    result = SMPDU_Sock_SetSockBufferSize( fd, 1 );
    if (result) { MPIU_ERR_POP(result); }
    
    /*
     * Attempt to establish the connection
     */
    {
	char addrString[64];
	SMPDU_Sock_AddrToStr( ifaddr, addrString, sizeof(addrString) );
        smpd_dbg_printf("Connecting to %s:%d\n", addrString, port ); 
    }

    do
    {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    while (rc == -1 && errno == EINTR);
    
    if (rc == 0)
    {
	/* connection succeeded */
	smpd_dbg_printf("Setting state to SOCKI_STATE_CONNECTED_RW for sock %p",sock);
	pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RW;
	SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CONNECT, 0, user_ptr, SMPD_SUCCESS, result, fn_fail);
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	smpd_dbg_printf("Setting state to SOCKI_STATE_CONNECTING for sock %p",sock);
	pollinfo->state = SMPDU_SOCKI_STATE_CONNECTING;
	SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);
    }
    else
    {
	smpd_dbg_printf("Setting state to SOCKI_STATE_DISCONNECTED (failure in connect) for sock %p",sock);
	pollinfo->os_errno = errno;
	pollinfo->state = SMPDU_SOCKI_STATE_DISCONNECTED;

	if (errno == ECONNREFUSED)
	{
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CONNECT, 0, user_ptr, SMPD_FAIL, 
		result, fn_fail);
	}
	else
	{
	    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CONNECT, 0, user_ptr, SMPD_FAIL,
		result, fn_fail);
	}
    }
    /* --END ERROR HANDLING-- */

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

    if (sock != NULL)
    {
	SMPDU_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* FIXME: What does this routine do?  Why does it take a host description
   instead of an interface name or address? */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_connect
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_connect(struct SMPDU_Sock_set * sock_set, void * user_ptr, 
			    char * host_description, int port,
			    struct SMPDU_Sock ** sockp)
{
    int result = SMPD_SUCCESS;
    SMPDU_Sock_ifaddr_t ifaddr;
    struct hostent * hostent;

    /*
     * Convert hostname to IP address
     *
     * FIXME: this should handle failures caused by a backed up listener queue
     * at the remote process.  It should also use a
     * specific interface if one is specified by the user.
     */
    /* FIXME: strtok may change the contents of host_description.  Shouldn't
       the host description be a const char [] and not modified by this 
       routine? */
    strtok(host_description, " ");
    /* FIXME: For ipv6, we should use getaddrinfo */
    hostent = gethostbyname(host_description);
    /* --BEGIN ERROR HANDLING-- */
    if (hostent == NULL || hostent->h_addrtype != AF_INET) {
	/* FIXME: Set error */
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    /* These are correct for IPv4 */
    memcpy( ifaddr.ifaddr, (unsigned char *)hostent->h_addr_list[0], 4 );
    ifaddr.len  = 4;
    ifaddr.type = AF_INET;
    result = SMPDU_Sock_post_connect_ifaddr( sock_set, user_ptr, 
						&ifaddr, port, sockp );
 fn_exit:
    return result;
}
/* end SMPDU_Sock_post_connect() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_listen
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
#ifndef USHRT_MAX
#define USHRT_MAX 65535   /* 2^16-1 */
#endif
int SMPDU_Sock_listen(struct SMPDU_Sock_set * sock_set, void * user_ptr, 
		      int * port, struct SMPDU_Sock ** sockp)
{
    struct SMPDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    int optval;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (*port < 0 || *port > USHRT_MAX)
    {
        smpd_err_printf("Invalid port (%d)\n", *port);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Create a non-blocking socket for the listener
     */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (fd == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Creating socket failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* set SO_REUSEADDR to a prevent a fixed service port from being bound to during subsequent invocations */
    if (*port != 0)
    {
	optval = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
            result = SMPD_FAIL;
            smpd_err_printf("setsockopt failed, errno = %d (%s)\n", errno, strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }

    /* make the socket non-blocking so that accept() will return immediately if no new connection is available */
    flags = fcntl(fd, F_GETFL, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Getting file status flags for socket failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Unable to set socket to nonblocking mode, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Bind the socket to all interfaces and the specified port.  The port specified by the calling routine may be 0, indicating
     * that the operating system can select an available port in the ephemeral port range.
     */
    if (*port == 0) {
	int portnum, low_port, high_port;
	/* see if we actually want to find values within a range */
	
	low_port = high_port = 0;
	/* Get range here.  These leave low_port, high_port unchanged
	   if the env variable is not set */
	/* FIXME: Use the parameter interface and document this */
	MPL_env2range( "MPICH_PORT_RANGE", &low_port, &high_port );

	for (portnum=low_port; portnum<=high_port; portnum++) {
	    memset( (void *)&addr, 0, sizeof(addr) );
	    addr.sin_family = AF_INET;
	    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	    addr.sin_port	   = htons( portnum );
	    
	    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	    if (rc < 0) {
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
		    close(fd);
		    break;
		}
	    }
	    else 
		break;
	}
    }
    else {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short) *port);
	rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("bind failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Set and verify the socket buffer size
     */
    result = SMPDU_Sock_SetSockBufferSize( fd, 1 );
    if (result) { MPIU_ERR_POP( result ); }
    
    /*
     * Start listening for incoming connections...
     */
    rc = listen(fd, SOMAXCONN);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("listen() failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Get listener port.  Techincally we don't need to do this if a port was 
     * specified by the calling routine; but it adds an extra error check.
     */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("getsockname() failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    *port = (unsigned int) ntohs(addr.sin_port);

    /*
     * Allocate and initialize sock and poll structures.  If another thread is
     * blocking in poll(), that thread must be woke up
     * long enough to pick up the addition of the listener socket.
     */
    result = SMPDU_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (result != SMPD_SUCCESS)
    {
        smpd_err_printf("Allocating sock struct for socket failed\n");
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = SMPDU_SOCKI_TYPE_LISTENER;
    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RO;

    SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

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
/* end SMPDU_Sock_listen() */


/* FIXME: What does this function do? */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_read
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_read(struct SMPDU_Sock * sock, void * buf, SMPDU_Sock_size_t minlen, SMPDU_Sock_size_t maxlen,
			 SMPDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, result, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Invalid length posted for read, sock_set->id = %d, sock_id = %d, minlen = %d, maxlen = %d\n",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    pollinfo->read.buf.ptr = buf;
    pollinfo->read.buf.min = minlen;
    pollinfo->read.buf.max = maxlen;
    pollinfo->read_iov_flag = FALSE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    
    SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_post_read() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_readv
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_readv(struct SMPDU_Sock * sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, result, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > SMPD_IOV_LIMIT)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Invalid number of iovs, sock_set->id =%d, sock_id = %d, iov_n = %d\n",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    pollinfo->read.iov.ptr = iov;
    pollinfo->read.iov.count = iov_n;
    pollinfo->read.iov.offset = 0;
    pollinfo->read_iov_flag = TRUE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;

    SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_post_readv() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_write
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_write(struct SMPDU_Sock * sock, void * buf, SMPDU_Sock_size_t minlen, SMPDU_Sock_size_t maxlen,
			  SMPDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, result, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Invalid length posted for read, sock_set->id = %d, sock_id = %d, minlen = %d, maxlen = %d\n",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    pollinfo->write.buf.ptr = buf;
    pollinfo->write.buf.min = minlen;
    pollinfo->write.buf.max = maxlen;
    pollinfo->write_iov_flag = FALSE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;

    SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_post_write() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_writev
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_writev(struct SMPDU_Sock * sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, result, fn_exit);
    SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, result, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > SMPD_IOV_LIMIT)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Invalid number of iovs, sock_set->id =%d, sock_id = %d, iov_n = %d\n",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    pollinfo->write.iov.ptr = iov;
    pollinfo->write.iov.count = iov_n;
    pollinfo->write.iov.offset = 0;
    pollinfo->write_iov_flag = TRUE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;

    SMPDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Sock_post_writev() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_close
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_close(struct SMPDU_Sock * sock)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    SMPDU_SOCKI_VALIDATE_SOCK(sock, result, fn_exit);

    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);

    SMPDU_SOCKI_VALIDATE_FD(pollinfo, result, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->state == SMPDU_SOCKI_STATE_CLOSING)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Received close on a closing socket, sock_set->id = %d, sock_id = %d\n",
	                    pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (pollinfo->type == SMPDU_SOCKI_TYPE_COMMUNICATION)
    {
	if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN | POLLOUT))
	{
	    /* --BEGIN ERROR HANDLING-- */
	    if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN))
	    {
		SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					  SMPD_SUCCESS, result, fn_exit);
	    }

	    if (SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  SMPD_SUCCESS, result, fn_exit);
	    }
	
	    SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	    /* --END ERROR HANDLING-- */
	}
    }
    else /* if (pollinfo->type == SMPDU_SOCKI_TYPE_LISTENER) */
    {
	/*
	 * The event queue may contain an accept event which means that 
	 * SMPDU_Sock_accept() may be legally called after
	 * SMPDU_Sock_post_close().  However, SMPDU_Sock_accept() must be 
	 * called before the close event is return by
	 * SMPDU_Sock_wait().
	 */
	SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }
    
    SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo, SMPDU_SOCK_OP_CLOSE, 0, pollinfo->user_ptr, SMPD_SUCCESS, result, fn_exit);
    pollinfo->state = SMPDU_SOCKI_STATE_CLOSING;

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
}


