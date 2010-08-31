/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This routine is called in mpid/ch3/util/sock/ch3u_connect_sock.c */
/* FIXME: This routine is misnamed; it is really get_interface_name (in the 
   case where there are several networks available to the calling process,
   this picks one but even in the current code can pick a different
   interface if a particular environment variable is set) .  

   This routine is used in smpd so we should not change its name yet. */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_host_description(int myRank, 
				    char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (len < 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
				     "**sock|badhdmax", NULL);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* FIXME: Is this documented?  How does it work if the process manager
       cannot give each process a different value for an environment
       name?  What if a different interface is needed? */
    /* Use hostname supplied in environment variable, if it exists */
    env_hostname = getenv("MPICH_INTERFACE_HOSTNAME");

    if (!env_hostname) {
	/* See if there is a per-process name for the interfaces (e.g.,
	   the process manager only delievers the same values for the 
	   environment to each process */
	char namebuf[1024];
	MPIU_Snprintf( namebuf, sizeof(namebuf), 
		       "MPICH_INTERFACE_HOSTNAME_R_%d", myRank );
	env_hostname = getenv( namebuf );
    }

    if (env_hostname != NULL)
    {
	rc = MPIU_Strncpy(host_description, env_hostname, (size_t) len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
					     "**sock|badhdlen", NULL);
	}
	/* --END ERROR HANDLING-- */
    }
    else {
	rc = gethostname(host_description, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
	    if (errno == EINVAL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
						 "**sock|badhdlen", NULL);
	    }
	    else if (errno == EFAULT)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
						 "**sock|badhdbuf", NULL);
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						 "**sock|oserror", "**sock|poll|oserror %d %s", errno, MPIU_Strerror(errno));
	    }
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_native_to_sock(struct MPIDU_Sock_set * sock_set, MPIDU_SOCK_NATIVE_FD fd, void *user_ptr,
			      struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int rc;
    long flags;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /* allocate sock and poll structures */
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
    
    /* set file descriptor to non-blocking */
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

    /* initialize sock and poll structures */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (sock != NULL)
    {
	MPIDU_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_set_user_ptr(struct MPIDU_Sock * sock, void * user_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    if (sock != MPIDU_SOCK_INVALID_SOCK &&
	sock->sock_set != MPIDU_SOCK_INVALID_SET)
    {
	MPIDU_Socki_sock_get_pollinfo(sock)->user_ptr = user_ptr;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
    }
    /* --END ERROR HANDLING-- */

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    return mpi_errno;
}


/* FIXME: What is this for?  It appears to be used in debug printing and
   in smpd */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_id(struct MPIDU_Sock * sock)
{
    int id;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);

    if (sock != MPIDU_SOCK_INVALID_SOCK)
    {
	if (sock->sock_set != MPIDU_SOCK_INVALID_SET)
	{
	    id = MPIDU_Socki_sock_get_pollinfo(sock)->sock_id;
	}
	else
	{
	    id = -1;
	}
    }
    else
    {
	id = -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    return id;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_set_id(struct MPIDU_Sock_set * sock_set)
{
    int id;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);

    if (sock_set != MPIDU_SOCK_INVALID_SET)
    {    
	id = sock_set->id;
    }
    else
    {
	id = -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    return id;
}

/* FIXME: This function violates the internationalization design by
   using English language strings rather than the error translation mechanism.
   This unnecessarily breaks the goal of allowing internationalization.  
   Read the design documentation and if there is a problem, raise it rather 
   than ignoring it.  
*/
/* FIXME: This appears to only be used in smpd */
/* FIXME: It appears that this function was used instead of making use of the
   existing MPI-2 features to extend MPI error classes and code, of to export
   messages to non-MPI application (smpd?) */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
int MPIDU_Sock_get_error_class_string(int error, char *error_string, size_t length)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    switch (MPIR_ERR_GET_CLASS(error))
    {
    case MPIDU_SOCK_ERR_FAIL:
	MPIU_Strncpy(error_string, "generic socket failure", length);
	break;
    case MPIDU_SOCK_ERR_INIT:
	MPIU_Strncpy(error_string, "socket module not initialized", length);
	break;
    case MPIDU_SOCK_ERR_NOMEM:
	MPIU_Strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SET:
	MPIU_Strncpy(error_string, "invalid socket set", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SOCK:
	MPIU_Strncpy(error_string, "invalid socket", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOST:
	MPIU_Strncpy(error_string, "host description buffer not large enough", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOSTNAME:
	MPIU_Strncpy(error_string, "invalid host name", length);
	break;
    case MPIDU_SOCK_ERR_BAD_PORT:
	MPIU_Strncpy(error_string, "invalid port", length);
	break;
    case MPIDU_SOCK_ERR_BAD_BUF:
	MPIU_Strncpy(error_string, "invalid buffer", length);
	break;
    case MPIDU_SOCK_ERR_BAD_LEN:
	MPIU_Strncpy(error_string, "invalid length", length);
	break;
    case MPIDU_SOCK_ERR_SOCK_CLOSED:
	MPIU_Strncpy(error_string, "socket closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_CLOSED:
	MPIU_Strncpy(error_string, "socket connection closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_FAILED:
	MPIU_Strncpy(error_string, "socket connection failed", length);
	break;
    case MPIDU_SOCK_ERR_INPROGRESS:
	MPIU_Strncpy(error_string, "socket operation in progress", length);
	break;
    case MPIDU_SOCK_ERR_TIMEOUT:
	MPIU_Strncpy(error_string, "socket operation timed out", length);
	break;
    case MPIDU_SOCK_ERR_INTR:
	MPIU_Strncpy(error_string, "socket operation interrupted", length);
	break;
    case MPIDU_SOCK_ERR_NO_NEW_SOCK:
	MPIU_Strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPIU_Snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    return MPI_SUCCESS;
}
/* --END ERROR HANDLING-- */
