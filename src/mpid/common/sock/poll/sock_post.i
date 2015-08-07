/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME: Provide an overview for the functions in this file */

#undef FUNCNAME 
#define FUNCNAME MPIDU_Sock_post_connect_ifaddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* 
 This routine connects to a particular address (in byte form; for ipv4, 
 the address is four bytes, typically the value of h_addr_list[0] in 
 struct hostent.  By avoiding a character name for an interface (we *never*
 connect to a host; we are *always* connecting to a particular interface 
 on a host), we avoid problems with DNS services, including lack of properly
 configured services and scalability problems.  As this routine uses 
 a four-byte field, it is currently restricted to ipv4.  This routine should
 evolve to support ipv4 and ipv6 addresses.

 This routine was constructed from MPIDU_Sock_post_connect by removing the 
 poorly placed use of gethostname within the middle of that routine and
 simply using the ifaddr field that is passed to this routine.  
 MPIDU_Sock_post_connect simply uses the hostname field to get the canonical
 IP address.  The original routine and its API was retained to allow backwards
 compatibility until it is determined that we can always use explicit addrs 
 needed in setting up the socket instead of character strings.
 */
int MPIDU_Sock_post_connect_ifaddr( struct MPIDU_Sock_set * sock_set, 
				    void * user_ptr, 
				    MPIDU_Sock_ifaddr_t *ifaddr, int port,
				    struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CONNECT_IFADDR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CONNECT_IFADDR);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * Create a non-blocking socket with Nagle's algorithm disabled
     */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
	/* FIXME: It would be better to include a special formatting
	   clue for system error messages (e.g., %dSE; in the recommended
	   revision for error reporting (that is, value (errno) is an int, 
	   but should be interpreted as an System Error string) */
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDU_SOCK_ERR_FAIL,
			     "**sock|poll|socket", 
		    "**sock|poll|socket %d %s", errno, MPIU_Strerror(errno));
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDU_SOCK_ERR_FAIL,
			     "**sock|poll|nonblock", 
                    "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1) {
	MPIR_ERR_SETANDJUMP2( mpi_errno, MPIDU_SOCK_ERR_FAIL,
			      "**sock|poll|nonblock", 
			      "**sock|poll|nonblock %d %s",
			      errno, MPIU_Strerror(errno));
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0) {
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDU_SOCK_ERR_FAIL,
			     "**sock|poll|nodelay", 
			     "**sock|poll|nodelay %d %s", 
			     errno, MPIU_Strerror(errno));
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
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPIDU_SOCK_ERR_NOMEM,
			    "**sock|sockalloc");
    }

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, ifaddr->ifaddr, 
	   sizeof(addr.sin_addr.s_addr));
    addr.sin_port = htons( (unsigned short)port);

    /*
     * Set and verify the socket buffer size
     */
    mpi_errno = MPIDU_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
    
    /*
     * Attempt to establish the connection
     */
    MPIU_DBG_STMT(CH3_CONNECT,TYPICAL,{
	char addrString[64];
	MPIDU_Sock_AddrToStr( ifaddr, addrString, sizeof(addrString) );
	MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST,
			      "Connecting to %s:%d", addrString, port )); 
	})

    do
    {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    while (rc == -1 && errno == EINTR);
    
    if (rc == 0)
    {
	/* connection succeeded */
	MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_CONNECTED_RW for sock %p",sock);
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPI_SUCCESS, mpi_errno, fn_fail);
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_CONNECTING for sock %p",sock);
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTING;
	MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);
    }
    else
    {
	MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_DISCONNECTED (failure in connect) for sock %p",sock);
	pollinfo->os_errno = errno;
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;

	if (errno == ECONNREFUSED)
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|connrefused", "**sock|poll|connrefused %d %d %s",
		pollinfo->sock_set->id, pollinfo->sock_id, ""), mpi_errno, fn_fail);
	}
	else
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|oserror", "**sock|poll|oserror %d %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, errno,
		MPIU_Strerror(errno)), mpi_errno, fn_fail);
	}
    }
    /* --END ERROR HANDLING-- */

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT_IFADDR);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (fd != -1)
    { 
	close(fd);
    }

    if (sock != NULL)
    {
	MPIDU_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* FIXME: What does this routine do?  Why does it take a host description
   instead of an interface name or address? */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_connect(struct MPIDU_Sock_set * sock_set, void * user_ptr, 
			    char * host_description, int port,
			    struct MPIDU_Sock ** sockp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDU_Sock_ifaddr_t ifaddr;
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
    mpi_errno = MPIDU_Sock_post_connect_ifaddr( sock_set, user_ptr, 
						&ifaddr, port, sockp );
 fn_exit:
    return mpi_errno;
}
/* end MPIDU_Sock_post_connect() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_listen
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
#ifndef USHRT_MAX
#define USHRT_MAX 65535   /* 2^16-1 */
#endif
int MPIDU_Sock_listen(struct MPIDU_Sock_set * sock_set, void * user_ptr, 
		      int * port, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    int optval;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (*port < 0 || *port > USHRT_MAX)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_PORT,
					 "**sock|badport", "**sock|badport %d", *port);
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d %s", errno, MPIU_Strerror(errno));
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
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|reuseaddr", "**sock|poll|reuseaddr %d %s", errno, MPIU_Strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }

    /* make the socket non-blocking so that accept() will return immediately if no new connection is available */
    flags = fcntl(fd, F_GETFL, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Bind the socket to all interfaces and the specified port.  The port specified by the calling routine may be 0, indicating
     * that the operating system can select an available port in the ephemeral port range.
     */
    if (*port == 0) {
	int portnum;
	/* see if we actually want to find values within a range */

        MPIR_ERR_CHKANDJUMP(MPIR_CVAR_CH3_PORT_RANGE.low < 0 || MPIR_CVAR_CH3_PORT_RANGE.low > MPIR_CVAR_CH3_PORT_RANGE.high, mpi_errno, MPI_ERR_OTHER, "**badportrange");

        /* default MPICH_PORT_RANGE is {0,0} so bind will use any available port */
        for (portnum = MPIR_CVAR_CH3_PORT_RANGE.low; portnum <= MPIR_CVAR_CH3_PORT_RANGE.high; ++portnum) {
	    memset( (void *)&addr, 0, sizeof(addr) );
	    addr.sin_family      = AF_INET;
	    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	    addr.sin_port	 = htons( (unsigned short)portnum );
	    
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
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons((unsigned short) *port);
	rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|bind", "**sock|poll|bind %d %d %s", *port, errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Set and verify the socket buffer size
     */
    mpi_errno = MPIDU_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }
    
    /*
     * Start listening for incoming connections...
     */
    rc = listen(fd, SOMAXCONN);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|listen", "**sock|poll|listen %d %s", errno, MPIU_Strerror(errno));
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|getport", "**sock|poll|getport %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    *port = (unsigned int) ntohs(addr.sin_port);

    /*
     * Allocate and initialize sock and poll structures.  If another thread is
     * blocking in poll(), that thread must be woke up
     * long enough to pick up the addition of the listener socket.
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
    pollinfo->type = MPIDU_SOCKI_TYPE_LISTENER;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
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
/* end MPIDU_Sock_listen() */


/* FIXME: What does this function do? */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_read(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			 MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
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
    
    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
    return mpi_errno;
}
/* end MPIDU_Sock_post_read() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_readv(struct MPIDU_Sock * sock, MPL_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > MPL_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
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

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
    return mpi_errno;
}
/* end MPIDU_Sock_post_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_write(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			  MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
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

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    return mpi_errno;
}
/* end MPIDU_Sock_post_write() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_writev(struct MPIDU_Sock * sock, MPL_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > MPL_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
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

    MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    return mpi_errno;
}
/* end MPIDU_Sock_post_writev() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_close
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_close(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->state == MPIDU_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|closing_already",
	    "**sock|closing_already %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (pollinfo->type == MPIDU_SOCKI_TYPE_COMMUNICATION)
    {
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN | POLLOUT))
	{
	    /* --BEGIN ERROR HANDLING-- */
	    int event_mpi_errno;
	    
	    event_mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_SOCK_CLOSED, "**sock|close_cancel",
		"**sock|close_cancel %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	    
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN))
	    {
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }

	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }
	
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	    /* --END ERROR HANDLING-- */
	}
    }
    else /* if (pollinfo->type == MPIDU_SOCKI_TYPE_LISTENER) */
    {
	/*
	 * The event queue may contain an accept event which means that 
	 * MPIDU_Sock_accept() may be legally called after
	 * MPIDU_Sock_post_close().  However, MPIDU_Sock_accept() must be 
	 * called before the close event is return by
	 * MPIDU_Sock_wait().
	 */
	MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }
    
    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CLOSE, 0, pollinfo->user_ptr, MPI_SUCCESS, mpi_errno, fn_exit);
    pollinfo->state = MPIDU_SOCKI_STATE_CLOSING;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
    return mpi_errno;
}


