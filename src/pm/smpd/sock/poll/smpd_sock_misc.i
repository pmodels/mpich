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
#define FUNCNAME SMPDU_Sock_get_host_description
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_host_description(int myRank, 
				    char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (len < 0)
    {
	result = MPIR_Err_create_code(SMPD_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, SMPD_FAIL,
				     "**sock|badhdmax", NULL);
        result = SMPD_FAIL;
        smpd_err_printf("Length of host description str is invalid - < 0\n");
	goto fn_fail;
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
	rc = MPIU_Strncpy(host_description, env_hostname, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
            result = SMPD_FAIL;
            smpd_err_printf("Unable to copy hostname to host desc str\n");
            goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }
    else {
	rc = gethostname(host_description, len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
            result = SMPD_FAIL;
            smpd_err_printf("gethostname failed for %s, errno = %d (%s)\n", host_description, errno, strerror(errno));
            goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_native_to_sock(struct SMPDU_Sock_set * sock_set, SMPDU_SOCK_NATIVE_FD fd, void *user_ptr,
			      struct SMPDU_Sock ** sockp)
{
    struct SMPDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int rc;
    long flags;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);

    /* allocate sock and poll structures */
    result = SMPDU_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (result != SMPD_SUCCESS)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Unable to alloc sock structure for sock\n");
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);
    
    /* set file descriptor to non-blocking */
    flags = fcntl(fd, F_GETFL, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Getting status flags failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Setting fd to non blocking failed, errno = %d (%s)\n", errno, strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* initialize sock and poll structures */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = SMPDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = SMPDU_SOCKI_STATE_CONNECTED_RW;

    *sockp = sock;

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (sock != NULL)
    {
	SMPDU_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_set_user_ptr(struct SMPDU_Sock * sock, void * user_ptr)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    SMPDU_SOCKI_VERIFY_INIT(result, fn_exit);

    if (sock != SMPDU_SOCK_INVALID_SOCK &&
	sock->sock_set != SMPDU_SOCK_INVALID_SET)
    {
	SMPDU_Socki_sock_get_pollinfo(sock)->user_ptr = user_ptr;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
        result = SMPD_FAIL;
        smpd_err_printf("Invalid sock structure, unable to set user ptr\n");
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;
  fn_fail:
    goto fn_exit;
}


/* FIXME: What is this for?  It appears to be used in debug printing and
   in smpd */
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_sock_id(struct SMPDU_Sock * sock)
{
    int id;

    smpd_enter_fn(FCNAME);

    if (sock != SMPDU_SOCK_INVALID_SOCK)
    {
	if (sock->sock_set != SMPDU_SOCK_INVALID_SET)
	{
	    id = SMPDU_Socki_sock_get_pollinfo(sock)->sock_id;
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

    smpd_exit_fn(FCNAME);
    return id;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_sock_set_id(struct SMPDU_Sock_set * sock_set)
{
    int id;

    smpd_enter_fn(FCNAME);

    if (sock_set != SMPDU_SOCK_INVALID_SET)
    {    
	id = sock_set->id;
    }
    else
    {
	id = -1;
    }

    smpd_exit_fn(FCNAME);
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
#define FUNCNAME SMPDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
int SMPDU_Sock_get_error_class_string(int error, char *error_string, int length)
{

    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
/* --END ERROR HANDLING-- */
