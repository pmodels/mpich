/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifdef MPICH_IS_THREADED
static int MPIDU_Socki_wakeup(struct MPIDU_Sock_set * sock_set);
int MPIDI_Sock_update_sock_set( struct MPIDU_Sock_set *, int );
#endif

static int MPIDU_Socki_os_to_mpi_errno(struct pollinfo * pollinfo, 
		     int os_errno, const char * fcname, int line, int * conn_failed);

static int MPIDU_Socki_adjust_iov(ssize_t nb, MPL_IOV * const iov, 
				  const int count, int * const offsetp);

static int MPIDU_Socki_sock_alloc(struct MPIDU_Sock_set * sock_set, 
				  struct MPIDU_Sock ** sockp);
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock);

static int MPIDU_Socki_event_enqueue(struct pollinfo * pollinfo, 
				     enum MPIDU_Sock_op op, 
				     MPIU_Size_t num_bytes,
				     void * user_ptr, int error);
static inline int MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set, 
					    int * set_elem, 
					    struct MPIDU_Sock_event * eventp);

static void MPIDU_Socki_free_eventq_mem(void);

struct MPIDU_Socki_eventq_table
{
    struct MPIDU_Socki_eventq_elem elems[MPIDU_SOCK_EVENTQ_POOL_SIZE];
    struct MPIDU_Socki_eventq_table * next;
};

static struct MPIDU_Socki_eventq_table *MPIDU_Socki_eventq_table_head=NULL;



#define MPIDU_Socki_sock_get_pollfd(sock_)          (&(sock_)->sock_set->pollfds[(sock_)->elem])
#define MPIDU_Socki_sock_get_pollinfo(sock_)        (&(sock_)->sock_set->pollinfos[(sock_)->elem])
#define MPIDU_Socki_pollinfo_get_pollfd(pollinfo_) (&(pollinfo_)->sock_set->pollfds[(pollinfo_)->elem])


/* Enqueue a new event.  If the enqueue fails, generate an error and jump to 
   the fail_label_ */
#define MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo_, op_, nb_, user_ptr_, event_mpi_errno_, mpi_errno_, fail_label_)	\
{									\
    mpi_errno_ = MPIDU_Socki_event_enqueue((pollinfo_), (op_), (nb_), (user_ptr_), (event_mpi_errno_));		\
    if (mpi_errno_ != MPI_SUCCESS)					\
    {									\
	mpi_errno_ = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,	\
					  "**sock|poll|eqfail", "**sock|poll|eqfail %d %d %d",			\
					  pollinfo->sock_set->id, pollinfo->sock_id, (op_));			\
	goto fail_label_;						\
    }									\
}

/* FIXME: These need to separate the operations from the thread-related
   synchronization to ensure that the code that is independent of 
   threads is always the same.  Also, the thread-level check needs 
   to be identical to all others, and there should be an option,
   possibly embedded within special thread macros, to allow
   runtime control of the thread level */

#ifndef MPICH_IS_THREADED
#   define MPIDU_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)	\
    {								\
        (pollfd_)->events |= (op_);				\
        (pollfd_)->fd = (pollinfo_)->fd;			\
    }
#   define MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)	\
    {								\
        (pollfd_)->events &= ~(op_);				\
        (pollfd_)->revents &= ~(op_);				\
        if (((pollfd_)->events & (POLLIN | POLLOUT)) == 0)	\
        {							\
            (pollfd_)->fd = -1;					\
        }							\
    }
#else /* MPICH_IS_THREADED */
/* FIXME: Does this need a runtime check on whether threads are in use? */
#   define MPIDU_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)		\
    {									\
	(pollinfo_)->pollfd_events |= (op_);				\
	if ((pollinfo_)->sock_set->pollfds_active == NULL)		\
	{								\
	    (pollfd_)->events |= (op_);					\
	    (pollfd_)->fd = (pollinfo_)->fd;				\
	}								\
	else								\
	{								\
	    (pollinfo_)->sock_set->pollfds_updated = TRUE;		\
	    MPIDU_Socki_wakeup((pollinfo_)->sock_set);			\
	}								\
    }
#   define MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)		\
    {									\
	(pollinfo_)->pollfd_events &= ~(op_);				\
	if ((pollinfo_)->sock_set->pollfds_active == NULL)		\
	{								\
	    (pollfd_)->events &= ~(op_);				\
	    (pollfd_)->revents &= ~(op_);				\
	    if (((pollfd_)->events & (POLLIN | POLLOUT)) == 0)		\
	    {								\
		(pollfd_)->fd = -1;					\
	    }								\
	}								\
	else								\
	{								\
	    (pollinfo_)->sock_set->pollfds_updated = TRUE;		\
	    MPIDU_Socki_wakeup((pollinfo_)->sock_set);			\
	}								\
    }
#endif

#define MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd_, pollinfo_, op_) ((pollfd_)->events & (op_))

/* FIXME: Low usage operations like this should be a function for
   better readability, modularity, and code size */
#define MPIDU_SOCKI_GET_SOCKET_ERROR(pollinfo_, os_errno_, mpi_errno_, fail_label_)				\
{								\
    int rc__;							\
    socklen_t sz__;						\
								\
    sz__ = sizeof(os_errno_);					\
    rc__ = getsockopt((pollinfo_)->fd, SOL_SOCKET, SO_ERROR, &(os_errno_), &sz__);				\
    if (rc__ != 0)						\
    {								\
	if (errno == ENOMEM || errno == ENOBUFS)		\
	{							\
	    mpi_errno_ = MPIR_Err_create_code(			\
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**sock|osnomem",	\
		"**sock|osnomem %s %d %d", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id);		\
	}							\
	else							\
	{							\
	    mpi_errno = MPIR_Err_create_code(			\
		MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|oserror",		\
		"**sock|poll|oserror %s %d %d %d %s", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id,	\
		 (os_errno_), MPIU_Strerror(os_errno_));	\
	}							\
								\
        goto fail_label_;					\
    }								\
}


/*
 * Validation tests
 */
/* FIXME: Are these really optional?  Based on their definitions, it looks
   like they should only be used when debugging the code.  */
#ifdef USE_SOCK_VERIFY
#define MPIDU_SOCKI_VERIFY_INIT(mpi_errno_, fail_label_)		\
{								        \
    if (MPIDU_Socki_initialized <= 0)					\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT,	\
					 "**sock|uninit", NULL);	\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VALIDATE_SOCK_SET(sock_set_, mpi_errno_, fail_label_)


#define MPIDU_SOCKI_VALIDATE_SOCK(sock_, mpi_errno_, fail_label_)	\
{									\
    struct pollinfo * pollinfo__;					\
									\
    if ((sock_) == NULL || (sock_)->sock_set == NULL || (sock_)->elem < 0 ||							\
	(sock_)->elem >= (sock_)->sock_set->poll_array_elems)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);	\
	goto fail_label_;						\
    }									\
									\
    pollinfo__ = MPIDU_Socki_sock_get_pollinfo(sock_);			\
									\
    if (pollinfo__->type <= MPIDU_SOCKI_TYPE_FIRST || pollinfo__->type >= MPIDU_SOCKI_TYPE_INTERRUPTER ||			\
	pollinfo__->state <= MPIDU_SOCKI_STATE_FIRST || pollinfo__->state >= MPIDU_SOCKI_STATE_LAST)				\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);	\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_COMMUNICATION)		\
    {									\
	if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTING)		\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code(			\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|notconnected",		\
		"**sock|notconnected %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_DISCONNECTED)	\
	{								\
	    if ((pollinfo_)->os_errno == 0)				\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	\
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				\
	    }								\
	    else							\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	\
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			\
		    (pollinfo_)->os_errno, MPIU_Strerror((pollinfo_)->os_errno));						\
	    }								\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CLOSING)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code(			\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS, "**sock|closing",		\
		"**sock|closing %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
									\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RW && (pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RO)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
						"**sock|badsock", NULL);							\
	    goto fail_label_;						\
	}								\
    }									\
    else if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_LISTENER)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code(				\
	    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|listener_read",		\
	    "**sock|listener_read %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
									\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_, mpi_errno_, fail_label_)						 \
{									\
    if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_COMMUNICATION)		\
    {									\
	if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTING)		\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
						"**sock|notconnected", "**sock|notconnected %d %d",				 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_DISCONNECTED || (pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTED_RO)	 \
	{								\
	    if ((pollinfo_)->os_errno == 0)				\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	 \
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    }								\
	    else							\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(										 \
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	 \
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			 \
		    (pollinfo_)->os_errno, MPIU_Strerror((pollinfo_)->os_errno));						 \
	    }								\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CLOSING)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS, \
						"**sock|closing", "**sock|closing %d %d",					 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
									\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RW)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
						"**sock|badsock", NULL);							 \
	    goto fail_label_;						\
	}								\
    }									\
    else if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_LISTENER)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
					    "**sock|listener_write", "**sock|listener_write %d %d",				 \
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					 \
									\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VALIDATE_FD(pollinfo_, mpi_errno_, fail_label_)	\
{									\
    if ((pollinfo_)->fd < 0)						\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badhandle", "**sock|poll|badhandle %d %d %d",				\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->fd);			\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if (MPIDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLIN))	\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|reads", "**sock|reads %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;						\
    }									\
}


#define MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if (MPIDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLOUT))	\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|writes", "**sock|writes %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;						\
    }									\
}
#else
/* Use minimal to no checking */
#define MPIDU_SOCKI_VERIFY_INIT(mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VALIDATE_SOCK_SET(sock_set_,mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VALIDATE_SOCK(sock_,mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_,mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_,mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VALIDATE_FD(pollinfo_,mpi_errno_,fail_label_)
#define MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd_,pollinfo_,mpi_errno,fail_label_)
#define MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_,pollinfo_,mpi_errno,fail_label_)

#endif


#ifdef MPICH_IS_THREADED

/*
 * MPIDU_Socki_wakeup()
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDU_Socki_wakeup(struct MPIDU_Sock_set * sock_set)
{
    MPIU_THREAD_CHECK_BEGIN;
    if (sock_set->wakeup_posted == FALSE)
    {
	for(;;)
	{
	    int nb;
	    char c = 0;
	    
	    nb = write(sock_set->intr_fds[1], &c, 1);
	    if (nb == 1)
	    {
		break;
	    }

	    MPIU_Assertp(nb == 0 || errno == EINTR);
	}
	
	sock_set->wakeup_posted = TRUE;
    }
    MPIU_THREAD_CHECK_END;
    return MPIDU_SOCK_SUCCESS;
}
/* end MPIDU_Socki_wakeup() */

#undef FUNCNAME
#define FUNCNAME MPIDI_Sock_update_sock_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Sock_update_sock_set( struct MPIDU_Sock_set *sock_set, 
				int pollfds_active_elems )
{
    int mpi_errno = MPI_SUCCESS;
    int elem;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);
    for (elem = 0; elem < sock_set->poll_array_elems; elem++) {
	sock_set->pollfds[elem].events = sock_set->pollinfos[elem].pollfd_events;
	if ((sock_set->pollfds[elem].events & (POLLIN | POLLOUT)) != 0) {
	    sock_set->pollfds[elem].fd = sock_set->pollinfos[elem].fd;
	}
	else {
	    sock_set->pollfds[elem].fd = -1;
	}

	if (elem < pollfds_active_elems) {
	    if (sock_set->pollfds_active == sock_set->pollfds) {
		sock_set->pollfds[elem].revents &= ~(POLLIN | POLLOUT) | sock_set->pollfds[elem].events;
	    }
	    else {
		sock_set->pollfds[elem].revents = sock_set->pollfds_active[elem].revents &
		    (~(POLLIN | POLLOUT) | sock_set->pollfds[elem].events);				
	    }
	}
	else {
	    sock_set->pollfds[elem].revents = 0;
	}
    }

    if (sock_set->pollfds_active != sock_set->pollfds) {
	MPIU_Free(sock_set->pollfds_active);
    }

    sock_set->pollfds_updated = FALSE;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);
    return mpi_errno;

}

#endif /* (MPICH_IS_THREADED) */
    

/*
 * MPIDU_Socki_os_to_mpi_errno()
 *
 * This routine assumes that no thread can change the state between state check before the nonblocking OS operation and the call
 * to this routine.
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_os_to_mpi_errno
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
static int MPIDU_Socki_os_to_mpi_errno(struct pollinfo * pollinfo, int os_errno, const char * fcname, int line, int * disconnected)
{
    int mpi_errno;

    if (os_errno == ENOMEM || os_errno == ENOBUFS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|osnomem", "**sock|poll|osnomem %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EFAULT || os_errno == EINVAL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_BUF,
					 "**sock|badbuf", "**sock|poll|badbuf %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EPIPE)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_CLOSED,
					 "**sock|connclosed", "**sock|poll|connclosed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = TRUE;
    }
    else if (os_errno == ECONNRESET || os_errno == ENOTCONN || os_errno == ETIMEDOUT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|connfailed", "**sock|poll|connfailed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }
    else if (os_errno == EBADF)
    {
	/*
	 * If we have a bad file descriptor, then either the sock was bad to 
	 * start with and we didn't catch it in the preliminary
	 * checks, or a sock closure was finalized after the preliminary 
	 * checks were performed.  The latter should not happen if
	 * the thread safety code is correctly implemented.  In any case, 
	 * the data structures associated with the sock are no
	 * longer valid and should not be modified.  We indicate this by 
	 * returning a fatal error.
	 */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
	*disconnected = FALSE;
    }
    else
    {
	/*
	 * Unexpected OS error.
	 *
	 * FIXME: technically we should never reach this section of code.  
	 * What's the right way to handle this situation?  Should
	 * we print an immediate message asking the user to report the errno 
	 * so that we can plug the hole?
	 */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|oserror", "**sock|poll|oserror %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }

    return mpi_errno;
}
/* --END ERROR HANDLING-- */
/* end MPIDU_Socki_os_to_mpi_errno() */


/*
 * MPIDU_Socki_adjust_iov()
 *
 * Use the specified number of bytes (nb) to adjust the iovec and associated
 * values.  If the iovec has been consumed, return
 * true; otherwise return false.
 *
 * The input is an iov (MPL_IOV is just an iov) and the offset into which 
 * to start (start with entry iov[*offsetp]) and remove nb bytes from the iov.
 * The use of the offsetp term allows use to remove values from the iov without
 * making a copy to shift down elements when only part of the iov is
 * consumed.
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_adjust_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDU_Socki_adjust_iov(ssize_t nb, MPL_IOV * const iov, const int count, int * const offsetp)
{
    int offset = *offsetp;
    
    while (offset < count)
    {
	if (iov[offset].MPL_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPL_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPL_IOV_BUF = (char *) iov[offset].MPL_IOV_BUF + nb;
	    iov[offset].MPL_IOV_LEN -= nb;
	    *offsetp = offset;
	    return FALSE;
	}
    }
    
    *offsetp = offset;
    return TRUE;
}
/* end MPIDU_Socki_adjust_iov() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_sock_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDU_Socki_sock_alloc(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    int avail_elem;
    struct pollfd * pollfds = NULL;
    struct pollinfo * pollinfos = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);
    
    /* FIXME: Should this use the CHKPMEM macros (perm malloc)? */
    sock = MPIU_Malloc(sizeof(struct MPIDU_Sock));
    /* --BEGIN ERROR HANDLING-- */
    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Check existing poll structures for a free element.
     */
    for (avail_elem = 0; avail_elem < sock_set->poll_array_sz; avail_elem++)
    {
	if (sock_set->pollinfos[avail_elem].sock_id == -1)
	{
	    if (avail_elem >= sock_set->poll_array_elems)
	    {
		sock_set->poll_array_elems = avail_elem + 1;
	    }
	    
	    break;
	}
    }

    /*
     * No free elements were found.  Larger pollfd and pollinfo arrays need to 
     * be allocated and the existing data transfered over.
     */
    if (avail_elem == sock_set->poll_array_sz)
    {
	int elem;
	
	pollfds = MPIU_Malloc((sock_set->poll_array_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
	/* --BEGIN ERROR HANDLING-- */
	if (pollfds == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	pollinfos = MPIU_Malloc((sock_set->poll_array_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
	/* --BEGIN ERROR HANDLING-- */
	if (pollinfos == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	
	if (sock_set->poll_array_sz > 0)
	{
	    /*
	     * Copy information from the old arrays and then free them.  
	     *
	     * In the multi-threaded case, the pollfd array can only be copied
	     * if another thread is not already blocking in poll()
	     * and thus potentially modifying the array.  Furthermore, the 
	     * pollfd array must not be freed if it is the one
	     * actively being used by pol().
	     */
#	    ifndef MPICH_IS_THREADED
	    {
		memcpy(pollfds, sock_set->pollfds, sock_set->poll_array_sz * sizeof(struct pollfd));
		MPIU_Free(sock_set->pollfds);
	    }
#	    else
	    {
		if (sock_set->pollfds_active == NULL)
		{ 
		    memcpy(pollfds, sock_set->pollfds, sock_set->poll_array_sz * sizeof(struct pollfd));
		}
		if  (sock_set->pollfds_active != sock_set->pollfds)
		{
		    MPIU_Free(sock_set->pollfds);
		}
	    }
#           endif
	    
	    memcpy(pollinfos, sock_set->pollinfos, sock_set->poll_array_sz * sizeof(struct pollinfo));
	    MPIU_Free(sock_set->pollinfos);
	}

	sock_set->poll_array_elems = avail_elem + 1;
	sock_set->poll_array_sz += MPIDU_SOCK_SET_DEFAULT_SIZE;
	sock_set->pollfds = pollfds;
	sock_set->pollinfos = pollinfos;
	
	/*
	 * Initialize new elements
	 */
	for (elem = avail_elem; elem < sock_set->poll_array_sz; elem++)
	{
	    pollfds[elem].fd = -1;
	    pollfds[elem].events = 0;
	    pollfds[elem].revents = 0;
	}
	for (elem = avail_elem; elem < sock_set->poll_array_sz; elem++)
	{
	    pollinfos[elem].fd = -1;
	    pollinfos[elem].sock_set = sock_set;
	    pollinfos[elem].elem = elem;
	    pollinfos[elem].sock = NULL;
	    pollinfos[elem].sock_id = -1;
	    pollinfos[elem].type  = MPIDU_SOCKI_TYPE_FIRST;
	    pollinfos[elem].state = MPIDU_SOCKI_STATE_FIRST;
#	    ifdef MPICH_IS_THREADED
	    {
		pollinfos[elem].pollfd_events = 0;
	    }
#	    endif
	}
    }

    /*
     * Verify that memory hasn't been messed up.
     */
    MPIU_Assert(sock_set->pollinfos[avail_elem].sock_set == sock_set);
    MPIU_Assert(sock_set->pollinfos[avail_elem].elem == avail_elem);
    MPIU_Assert(sock_set->pollinfos[avail_elem].fd == -1);
    MPIU_Assert(sock_set->pollinfos[avail_elem].sock == NULL);
    MPIU_Assert(sock_set->pollinfos[avail_elem].sock_id == -1);
    MPIU_Assert(sock_set->pollinfos[avail_elem].type == MPIDU_SOCKI_TYPE_FIRST);
    MPIU_Assert(sock_set->pollinfos[avail_elem].state == MPIDU_SOCKI_STATE_FIRST);
#   ifdef MPICH_IS_THREADED
    {
	MPIU_Assert(sock_set->pollinfos[avail_elem].pollfd_events == 0);
    }
#   endif

    /*
     * Initialize newly allocated sock structure and associated poll structures
     */
    sock_set->pollinfos[avail_elem].sock_id = (sock_set->id << 24) | avail_elem;
    sock_set->pollinfos[avail_elem].sock = sock;
    sock->sock_set = sock_set;
    sock->elem = avail_elem;

    sock_set->pollfds[avail_elem].fd = -1;
    sock_set->pollfds[avail_elem].events = 0;
    sock_set->pollfds[avail_elem].revents = 0;

#   ifdef MPICH_IS_THREADED
    {
        MPIU_THREAD_CHECK_BEGIN;
	if (sock_set->pollfds_active != NULL)
	{
	    sock_set->pollfds_updated = TRUE;
	}
        MPIU_THREAD_CHECK_END;
    }
#   endif
    
    *sockp = sock;
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (pollinfos != NULL)
    {
	MPIU_Free(pollinfos);
    }

    if (pollfds != NULL)
    {
	MPIU_Free(pollfds);
    }
	
    if (sock != NULL)
    {
	MPIU_Free(sock);
    }
    
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end MPIDU_Socki_sock_alloc() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_sock_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    struct pollinfo * pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    struct MPIDU_Sock_set * sock_set = sock->sock_set;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    /* FIXME: We need an abstraction for the thread sync operations */
#   ifdef MPICH_IS_THREADED
    {
	/*
	 * Freeing a sock while Sock_wait() is blocked in poll() is not supported
	 */
	MPIU_Assert(sock_set->pollfds_active == NULL);
    }
#   endif

    /*
     * Compress poll array
     */
     /* FIXME: move last element into current position and update sock associated with last element.
     */
    if (sock->elem + 1 == sock_set->poll_array_elems)
    { 
	sock_set->poll_array_elems -= 1;
	if (sock_set->starting_elem >= sock_set->poll_array_elems)
	{
	    sock_set->starting_elem = 0;
	}
    }

    /*
     * Remove entry from the poll list and mark the entry as free
     */
    pollinfo->fd      = -1;
    pollinfo->sock    = NULL;
    pollinfo->sock_id = -1;
    pollinfo->type    = MPIDU_SOCKI_TYPE_FIRST;
    pollinfo->state   = MPIDU_SOCKI_STATE_FIRST;
#   ifdef MPICH_IS_THREADED
    {
	pollinfo->pollfd_events = 0;
    }
#   endif
		
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;

    /*
     * Mark the sock as invalid so that any future use might be caught
     */
    sock->sock_set = NULL;
    sock->elem = -1;
    
    MPIU_Free(sock);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);
}
/* end MPIDU_Socki_sock_free() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_enqueue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDU_Socki_event_enqueue(struct pollinfo * pollinfo, MPIDU_Sock_op_t op, MPIU_Size_t num_bytes,
				     void * user_ptr, int error)
{
    struct MPIDU_Sock_set * sock_set = pollinfo->sock_set;
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    if (MPIDU_Socki_eventq_pool != NULL)
    {
	eventq_elem = MPIDU_Socki_eventq_pool;
	MPIDU_Socki_eventq_pool = MPIDU_Socki_eventq_pool->next;
    }
    else
    {
	int i;
	struct MPIDU_Socki_eventq_table *eventq_table;

	eventq_table = MPIU_Malloc(sizeof(struct MPIDU_Socki_eventq_table));
	/* --BEGIN ERROR HANDLING-- */
	if (eventq_table == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**sock|poll|eqmalloc", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

        eventq_elem = eventq_table->elems;

        eventq_table->next = MPIDU_Socki_eventq_table_head;
        MPIDU_Socki_eventq_table_head = eventq_table;

	if (MPIDU_SOCK_EVENTQ_POOL_SIZE > 1)
	{ 
	    MPIDU_Socki_eventq_pool = &eventq_elem[1];
	    for (i = 0; i < MPIDU_SOCK_EVENTQ_POOL_SIZE - 2; i++)
	    {
		MPIDU_Socki_eventq_pool[i].next = &MPIDU_Socki_eventq_pool[i+1];
	    }
	    MPIDU_Socki_eventq_pool[MPIDU_SOCK_EVENTQ_POOL_SIZE - 2].next = NULL;
	}
    }
    
    eventq_elem->event.op_type = op;
    eventq_elem->event.num_bytes = num_bytes;
    eventq_elem->event.user_ptr = user_ptr;
    eventq_elem->event.error = error;
    eventq_elem->set_elem = pollinfo->elem;
    eventq_elem->next = NULL;

    if (sock_set->eventq_head == NULL)
    { 
	sock_set->eventq_head = eventq_elem;
    }
    else
    {
	sock_set->eventq_tail->next = eventq_elem;
    }
    sock_set->eventq_tail = eventq_elem;
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_ENQUEUE);
    return mpi_errno;
}
/* end MPIDU_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_dequeue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set, int * set_elem, struct MPIDU_Sock_event * eventp)
{
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    if (sock_set->eventq_head != NULL)
    {
	eventq_elem = sock_set->eventq_head;
	
	sock_set->eventq_head = eventq_elem->next;
	if (eventq_elem->next == NULL)
	{
	    sock_set->eventq_tail = NULL;
	}
	
	*eventp = eventq_elem->event;
	*set_elem = eventq_elem->set_elem;
	
	eventq_elem->next = MPIDU_Socki_eventq_pool;
	MPIDU_Socki_eventq_pool = eventq_elem;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* FIXME: Shouldn't this be an mpi error code? */
	mpi_errno = MPIDU_SOCK_ERR_FAIL;
    }
    /* --END ERROR HANDLING-- */

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_DEQUEUE);
    return mpi_errno;
}
/* end MPIDU_Socki_event_dequeue() */


/* FIXME: Who allocates eventq tables?  Should there be a check that these
   tables are empty first? */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_free_eventq_mem
#undef FCNAME
#define FCNAME "MPIDU_Socki_free_eventq_mem"
static void MPIDU_Socki_free_eventq_mem(void)
{
    struct MPIDU_Socki_eventq_table *eventq_table, *eventq_table_next;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);

    eventq_table = MPIDU_Socki_eventq_table_head;
    while (eventq_table) {
        eventq_table_next = eventq_table->next;
        MPIU_Free(eventq_table);
        eventq_table = eventq_table_next;
    }
    MPIDU_Socki_eventq_table_head = NULL;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);
}

/* Provide a standard mechanism for setting the socket buffer size.
   The value is -1 if the default size hasn't been set, 0 if no size 
   should be set, and > 0 if that size should be used */
static int sockBufSize = -1;

/* Set the socket buffer sizes on fd to the standard values (this is controlled
   by the parameter MPICH_SOCK_BUFSIZE).  If "firm" is true, require that the
   sockets actually accept that buffer size.  */
int MPIDU_Sock_SetSockBufferSize( int fd, int firm )
{
    int mpi_errno = MPI_SUCCESS;
    int rc;

    /* Get the socket buffer size if we haven't yet acquired it */
    if (sockBufSize < 0) {
	/* FIXME: Is this the name that we want to use (this was chosen
	   to match the original, undocumented name) */
	rc = MPL_env2int( "MPICH_SOCKET_BUFFER_SIZE", &sockBufSize );
	if (rc <= 0) {
	    sockBufSize = 0;
	}
	MPIU_DBG_MSG_D(CH3_CONNECT,TYPICAL,"Sock buf size = %d",sockBufSize);
    }

    if (sockBufSize > 0) {
	int bufsz;
	socklen_t bufsz_len;

	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
	if (rc == -1) {
	    MPIR_ERR_SETANDJUMP3(mpi_errno,MPIDU_SOCK_ERR_FAIL, 
				 "**sock|poll|setsndbufsz",
				 "**sock|poll|setsndbufsz %d %d %s", 
				 bufsz, errno, MPIU_Strerror(errno));
	}
	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
	if (rc == -1) {
	    MPIR_ERR_SETANDJUMP3(mpi_errno,MPIDU_SOCK_ERR_FAIL, 
				 "**sock|poll|setrcvbufsz",
				 "**sock|poll|setrcvbufsz %d %d %s", 
				 bufsz, errno, MPIU_Strerror(errno));
	}
	bufsz_len = sizeof(bufsz);

	if (firm) {
	    rc = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rc == 0) {
		if (bufsz < sockBufSize * 0.9) {
		MPL_msg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				sockBufSize, bufsz);
		}
	    }
	    /* --END ERROR HANDLING-- */
	    
	    bufsz_len = sizeof(bufsz);
	    rc = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rc == 0) {
		if (bufsz < sockBufSize * 0.9) {
		    MPL_msg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				    sockBufSize, bufsz);
		}
	    }
	    /* --END ERROR HANDLING-- */
	}
    }
 fn_fail:
    return mpi_errno;
}

/* This routine provides a string version of the address. */
int MPIDU_Sock_AddrToStr( MPIDU_Sock_ifaddr_t *ifaddr, char *str, int maxlen )
{ 
    int i;
    unsigned char *p = ifaddr->ifaddr;
    for (i=0; i<ifaddr->len && maxlen > 4; i++) { 
	snprintf( str, maxlen, "%.3d.", *p++ );
	str += 4;
	maxlen -= 4;
    }
    /* Change the last period to a null; but be careful in case len was zero */
    if (i > 0) *--str = 0;
    else       *str = 0;
    return 0;
}
