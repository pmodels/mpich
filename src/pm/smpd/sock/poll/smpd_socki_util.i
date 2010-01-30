/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


static int SMPDU_Socki_os_to_result(struct pollinfo * pollinfo, 
		     int os_errno, char * fcname, int line, int * conn_failed);

static int SMPDU_Socki_adjust_iov(ssize_t nb, SMPD_IOV * const iov, 
				  const int count, int * const offsetp);

static int SMPDU_Socki_sock_alloc(struct SMPDU_Sock_set * sock_set, 
				  struct SMPDU_Sock ** sockp);
static void SMPDU_Socki_sock_free(struct SMPDU_Sock * sock);

static int SMPDU_Socki_event_enqueue(struct pollinfo * pollinfo, 
				     enum SMPDU_Sock_op op, 
				     SMPDU_Sock_size_t num_bytes,
				     void * user_ptr, int error);
static int inline SMPDU_Socki_event_dequeue(struct SMPDU_Sock_set * sock_set, 
					    int * set_elem, 
					    struct SMPDU_Sock_event * eventp);

static void SMPDU_Socki_free_eventq_mem(void);

struct SMPDU_Socki_eventq_table
{
    struct SMPDU_Socki_eventq_elem elems[SMPDU_SOCK_EVENTQ_POOL_SIZE];
    struct SMPDU_Socki_eventq_table * next;
};

static struct SMPDU_Socki_eventq_table *SMPDU_Socki_eventq_table_head=NULL;


#define SMPDI_QUOTE(A) SMPDI_QUOTE2(A)
#define SMPDI_QUOTE2(A) #A

#define SMPDU_Socki_sock_get_pollfd(sock_)          (&(sock_)->sock_set->pollfds[(sock_)->elem])
#define SMPDU_Socki_sock_get_pollinfo(sock_)        (&(sock_)->sock_set->pollinfos[(sock_)->elem])
#define SMPDU_Socki_pollinfo_get_pollfd(pollinfo_) (&(pollinfo_)->sock_set->pollfds[(pollinfo_)->elem])


/* Enqueue a new event.  If the enqueue fails, generate an error and jump to 
   the fail_label_ */
#define SMPDU_SOCKI_EVENT_ENQUEUE(pollinfo_, op_, nb_, user_ptr_, event_result_, result_, fail_label_)  \
{									\
    result_ = SMPDU_Socki_event_enqueue((pollinfo_), (op_), (nb_), (user_ptr_), (event_result_));		\
    if (result_ != SMPD_SUCCESS)					\
    {									\
        smpd_err_printf("Queing an event failed on sock, sock_set->id = %d, sock_id = %d, op = %d\n",   \
                            pollinfo->sock_set->id, pollinfo->sock_id, (op_));                          \
        (result_) = SMPD_FAIL;                                          \
        goto fail_label_;                                               \
    }                                                                   \
}

/* FIXME: These need to separate the operations from the thread-related
   synchronization to ensure that the code that is independent of 
   threads is always the same.  Also, the thread-level check needs 
   to be identical to all others, and there should be an option,
   possibly embedded within special thread macros, to allow
   runtime control of the thread level */

#   define SMPDU_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)	\
    {								\
        (pollfd_)->events |= (op_);				\
        (pollfd_)->fd = (pollinfo_)->fd;			\
    }
#   define SMPDU_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)	\
    {								\
        (pollfd_)->events &= ~(op_);				\
        (pollfd_)->revents &= ~(op_);				\
        if (((pollfd_)->events & (POLLIN | POLLOUT)) == 0)	\
        {							\
            (pollfd_)->fd = -1;					\
        }							\
    }

#define SMPDU_SOCKI_POLLFD_OP_ISSET(pollfd_, pollinfo_, op_) ((pollfd_)->events & (op_))

/* FIXME: Low usage operations like this should be a function for
   better readability, modularity, and code size */
#define SMPDU_SOCKI_GET_SOCKET_ERROR(pollinfo_, os_errno_, result_, fail_label_)				\
{								\
    int rc__;							\
    socklen_t sz__;						\
								\
    sz__ = sizeof(os_errno_);					\
    rc__ = getsockopt((pollinfo_)->fd, SOL_SOCKET, SO_ERROR, &(os_errno_), &sz__);				\
    if (rc__ != 0)						\
    {								\
        (result_) = SMPD_FAIL;                                  \
        smpd_err_printf("Error getting socket error, sock_set->id = %d, sock_id = %d, errno = %d(%s)\n",    \
                            pollinfo->sock_set->id, pollinfo->sock_id, errno, strerror(errno));             \
        goto fail_label_;					\
    }								\
    result_ = SMPD_SUCCESS;                                     \
}


/*
 * Validation tests
 */
/* FIXME: Are these really optional?  Based on their definitions, it looks
   like they should only be used when debugging the code.  */
#ifdef USE_SOCK_VERIFY
#define SMPDU_SOCKI_VERIFY_INIT(result_, fail_label_)		\
{								        \
    if (SMPDU_Socki_initialized <= 0)					\
    {									\
        (result_) = SMPD_FAIL;                                          \
        smpd_err_printf("Sock not initialized\n");                      \
	goto fail_label_;						\
    }									\
    (result_) = SMPD_SUCCESS;                                           \
}


#define SMPDU_SOCKI_VALIDATE_SOCK_SET(sock_set_, result_, fail_label_)


#define SMPDU_SOCKI_VALIDATE_SOCK(sock_, result_, fail_label_)	\
{									\
    struct pollinfo * pollinfo__;					\
    (result_) = SMPD_SUCCESS;                                           \
									\
    if ((sock_) == NULL || (sock_)->sock_set == NULL || (sock_)->elem < 0 ||							\
	(sock_)->elem >= (sock_)->sock_set->poll_array_elems)		\
    {									\
        (result_) = SMPD_FAIL;                                          \
        smpd_err_printf("Invalid sock struct\n");                       \
	goto fail_label_;						\
    }									\
									\
    pollinfo__ = SMPDU_Socki_sock_get_pollinfo(sock_);			\
									\
    if (pollinfo__->type <= SMPDU_SOCKI_TYPE_FIRST || pollinfo__->type >= SMPDU_SOCKI_TYPE_INTERRUPTER ||			\
	pollinfo__->state <= SMPDU_SOCKI_STATE_FIRST || pollinfo__->state >= SMPDU_SOCKI_STATE_LAST)				\
    {									\
        (result_) = SMPD_FAIL;                                          \
        smpd_err_printf("Invalid sock struct, type = %d, state = %d\n", \
                            (pollinfo_)->type, (pollinfo_)->state);     \
	goto fail_label_;						\
    }									\
}


#define SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_, result_, fail_label_)  \
{                                                                       \
    if ((pollinfo_)->type == SMPDU_SOCKI_TYPE_COMMUNICATION)            \
    {                                                                   \
        if ((pollinfo_)->state == SMPDU_SOCKI_STATE_CONNECTED_RW || (pollinfo_)->state == SMPDU_SOCKI_STATE_CONNECTED_RO)    \
            (result_) = SMPD_SUCCESS;                               \
        else{                                                       \
            (result_) = SMPD_FAIL;                                  \
            smpd_err_printf("Sock not readable, sock_set->id = %d, sock_id = %d, state = %d\n", \
                                (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);   \
            goto fail_label_;                                           \
        }                                                               \
    }                                                                   \
    else if ((pollinfo_)->type == SMPDU_SOCKI_TYPE_LISTENER)            \
    {                                                                   \
        (result_) = SMPD_FAIL;                                  \
        smpd_err_printf("Listener Sock not connected, sock_set->id = %d, sock_id = %d, state = %d\n",   \
                            (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);       \
        goto fail_label_;                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (result_) = SMPD_SUCCESS;                                       \
    }                                                                   \
}


#define SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_, result_, fail_label_)  \
{                                                                       \
    if ((pollinfo_)->type == SMPDU_SOCKI_TYPE_COMMUNICATION)            \
    {                                                                   \
        if ((pollinfo_)->state == SMPDU_SOCKI_STATE_CONNECTED_RW || (pollinfo_)->state == SMPDU_SOCKI_STATE_CONNECTED_WO)    \
            (result_) = SMPD_SUCCESS;                               \
        else{                                                       \
            (result_) = SMPD_FAIL;                                  \
            smpd_err_printf("Sock not writable, sock_set->id = %d, sock_id = %d, state = %d\n", \
                                (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);   \
            goto fail_label_;                                           \
        }                                                               \
    }                                                                   \
    else if ((pollinfo_)->type == SMPDU_SOCKI_TYPE_LISTENER)            \
    {                                                                   \
        (result_) = SMPD_FAIL;                                  \
        smpd_err_printf("Listener Sock not connected, sock_set->id = %d, sock_id = %d, state = %d\n",   \
                            (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);       \
        goto fail_label_;                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (result_) = SMPD_SUCCESS;                                       \
    }                                                                   \
}


#define SMPDU_SOCKI_VALIDATE_FD(pollinfo_, result_, fail_label_)        \
{                                                                       \
    if((pollinfo_)->fd < 0)                                             \
    {                                                                   \
        (result_) = SMPD_FAIL;                                          \
        smpd_err_printf("Invalid sock fd\n");                           \
        goto fail_label_;                                               \
    }                                                                   \
    (result_) = SMPD_SUCCESS;                                           \
}

#define SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd_, pollinfo_, result_, fail_label_) \
{                                                                                   \
    if (SMPDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLIN))                \
    {                                                                               \
        (result_) = SMPD_FAIL;                                                      \
        smpd_err_printf("Read already posted on sock, sock_set->id = %d, sock_id = %d, state = %d\n",   \
                            (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);       \
        goto fail_label_;                                                           \
    }                                                                               \
    (result_) = SMPD_SUCCESS;                                                       \
}

#define SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_, pollinfo_, result_, fail_label_) \
{                                                                                   \
    if (SMPDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLOUT))               \
    {                                                                               \
        (result_) = SMPD_FAIL;                                                      \
        smpd_err_printf("Write already posted on sock, sock_set->id = %d, sock_id = %d, state = %d\n",  \
                            (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->state);       \
        goto fail_label_;                                                           \
    }                                                                               \
    (result_) = SMPD_SUCCESS;                                                       \
}

#else
/* Use minimal to no checking */
#define SMPDU_SOCKI_VERIFY_INIT(result_,fail_label_)
#define SMPDU_SOCKI_VALIDATE_SOCK_SET(sock_set_,result_,fail_label_)
#define SMPDU_SOCKI_VALIDATE_SOCK(sock_,result_,fail_label_)
#define SMPDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_,result_,fail_label_)
#define SMPDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_,result_,fail_label_)
#define SMPDU_SOCKI_VALIDATE_FD(pollinfo_,result_,fail_label_)
#define SMPDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd_,pollinfo_,result,fail_label_)
#define SMPDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_,pollinfo_,result,fail_label_)

#endif


/*
 * SMPDU_Socki_os_to_result()
 *
 * This routine assumes that no thread can change the state between state check before the nonblocking OS operation and the call
 * to this routine.
 */
#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_os_to_result
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
static int SMPDU_Socki_os_to_result(struct pollinfo * pollinfo, int os_errno, char * fcname, int line, int * disconnected)
{
    int result = SMPD_FAIL;

    smpd_err_printf("os_error = %d (%s)\n", os_errno, strerror(os_errno));
    if (os_errno == ENOMEM || os_errno == ENOBUFS)
    {
	*disconnected = FALSE;
    }
    else if (os_errno == EFAULT || os_errno == EINVAL)
    {
	*disconnected = FALSE;
    }
    else if (os_errno == EPIPE)
    {
	*disconnected = TRUE;
    }
    else if (os_errno == ECONNRESET || os_errno == ENOTCONN || os_errno == ETIMEDOUT)
    {
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
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }

    return result;
}
/* --END ERROR HANDLING-- */
/* end SMPDU_Socki_os_to_result() */


/*
 * SMPDU_Socki_adjust_iov()
 *
 * Use the specified number of bytes (nb) to adjust the iovec and associated
 * values.  If the iovec has been consumed, return
 * true; otherwise return false.
 *
 * The input is an iov (SMPD_IOV is just an iov) and the offset into which 
 * to start (start with entry iov[*offsetp]) and remove nb bytes from the iov.
 * The use of the offsetp term allows use to remove values from the iov without
 * making a copy to shift down elements when only part of the iov is
 * consumed.
 */
#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_adjust_iov
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_adjust_iov(ssize_t nb, SMPD_IOV * const iov, const int count, int * const offsetp)
{
    int offset = *offsetp;
    
    while (offset < count)
    {
	if (iov[offset].SMPD_IOV_LEN <= nb)
	{
	    nb -= iov[offset].SMPD_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].SMPD_IOV_BUF = (char *) iov[offset].SMPD_IOV_BUF + nb;
	    iov[offset].SMPD_IOV_LEN -= nb;
	    *offsetp = offset;
	    return FALSE;
	}
    }
    
    *offsetp = offset;
    return TRUE;
}
/* end SMPDU_Socki_adjust_iov() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_sock_alloc
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_sock_alloc(struct SMPDU_Sock_set * sock_set, struct SMPDU_Sock ** sockp)
{
    struct SMPDU_Sock * sock = NULL;
    int avail_elem;
    struct pollfd * pollfds = NULL;
    struct pollinfo * pollinfos = NULL;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    
    /* FIXME: Should this use the CHKPMEM macros (perm malloc)? */
    sock = MPIU_Malloc(sizeof(struct SMPDU_Sock));
    /* --BEGIN ERROR HANDLING-- */
    if (sock == NULL)
    {
        result = SMPD_FAIL;
        smpd_err_printf("Allocating memory for sock struct failed\n");
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
	
	pollfds = MPIU_Malloc((sock_set->poll_array_sz + SMPDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
	/* --BEGIN ERROR HANDLING-- */
	if (pollfds == NULL)
	{
            result = SMPD_FAIL;
            smpd_err_printf("Allocating memory for pollfds failed\n");
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	pollinfos = MPIU_Malloc((sock_set->poll_array_sz + SMPDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
	/* --BEGIN ERROR HANDLING-- */
	if (pollinfos == NULL)
	{
            result = SMPD_FAIL;
            smpd_err_printf("Allocating memory for pollinfos failed\n");
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
	    {
		memcpy(pollfds, sock_set->pollfds, sock_set->poll_array_sz * sizeof(struct pollfd));
		MPIU_Free(sock_set->pollfds);
	    }
	    
	    memcpy(pollinfos, sock_set->pollinfos, sock_set->poll_array_sz * sizeof(struct pollinfo));
	    MPIU_Free(sock_set->pollinfos);
	}

	sock_set->poll_array_elems = avail_elem + 1;
	sock_set->poll_array_sz += SMPDU_SOCK_SET_DEFAULT_SIZE;
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
	    pollinfos[elem].type  = SMPDU_SOCKI_TYPE_FIRST;
	    pollinfos[elem].state = SMPDU_SOCKI_STATE_FIRST;
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
    MPIU_Assert(sock_set->pollinfos[avail_elem].type == SMPDU_SOCKI_TYPE_FIRST);
    MPIU_Assert(sock_set->pollinfos[avail_elem].state == SMPDU_SOCKI_STATE_FIRST);

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

    *sockp = sock;
    
  fn_exit:
    smpd_exit_fn(FCNAME);
    return result;

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
/* end SMPDU_Socki_sock_alloc() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_sock_free
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static void SMPDU_Socki_sock_free(struct SMPDU_Sock * sock)
{
    struct pollfd * pollfd = SMPDU_Socki_sock_get_pollfd(sock);
    struct pollinfo * pollinfo = SMPDU_Socki_sock_get_pollinfo(sock);
    struct SMPDU_Sock_set * sock_set = sock->sock_set;

    smpd_enter_fn(FCNAME);

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
    pollinfo->type    = SMPDU_SOCKI_TYPE_FIRST;
    pollinfo->state   = SMPDU_SOCKI_STATE_FIRST;
		
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;

    /*
     * Mark the sock as invalid so that any future use might be caught
     */
    sock->sock_set = NULL;
    sock->elem = -1;
    
    MPIU_Free(sock);
    
    smpd_exit_fn(FCNAME);
}
/* end SMPDU_Socki_sock_free() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_event_enqueue
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int SMPDU_Socki_event_enqueue(struct pollinfo * pollinfo, SMPDU_Sock_op_t op, SMPDU_Sock_size_t num_bytes,
				     void * user_ptr, int error)
{
    struct SMPDU_Sock_set * sock_set = pollinfo->sock_set;
    struct SMPDU_Socki_eventq_elem * eventq_elem;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    if (SMPDU_Socki_eventq_pool != NULL)
    {
	eventq_elem = SMPDU_Socki_eventq_pool;
	SMPDU_Socki_eventq_pool = SMPDU_Socki_eventq_pool->next;
    }
    else
    {
	int i;
	struct SMPDU_Socki_eventq_table *eventq_table;

	eventq_table = MPIU_Malloc(sizeof(struct SMPDU_Socki_eventq_table));
	/* --BEGIN ERROR HANDLING-- */
	if (eventq_table == NULL)
	{
            smpd_err_printf("Unable to allocate memory for eventq table\n");
            result = SMPD_FAIL;
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

        eventq_elem = eventq_table->elems;

        eventq_table->next = SMPDU_Socki_eventq_table_head;
        SMPDU_Socki_eventq_table_head = eventq_table;

	if (SMPDU_SOCK_EVENTQ_POOL_SIZE > 1)
	{ 
	    SMPDU_Socki_eventq_pool = &eventq_elem[1];
	    for (i = 0; i < SMPDU_SOCK_EVENTQ_POOL_SIZE - 2; i++)
	    {
		SMPDU_Socki_eventq_pool[i].next = &SMPDU_Socki_eventq_pool[i+1];
	    }
	    SMPDU_Socki_eventq_pool[SMPDU_SOCK_EVENTQ_POOL_SIZE - 2].next = NULL;
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
    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_event_dequeue
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static inline int SMPDU_Socki_event_dequeue(struct SMPDU_Sock_set * sock_set, int * set_elem, struct SMPDU_Sock_event * eventp)
{
    struct SMPDU_Socki_eventq_elem * eventq_elem;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

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
	
	eventq_elem->next = SMPDU_Socki_eventq_pool;
	SMPDU_Socki_eventq_pool = eventq_elem;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* FIXME: Shouldn't this be an mpi error code? */
	result = SMPD_FAIL;
    }
    /* --END ERROR HANDLING-- */

    smpd_exit_fn(FCNAME);
    return result;
}
/* end SMPDU_Socki_event_dequeue() */


/* FIXME: Who allocates eventq tables?  Should there be a check that these
   tables are empty first? */
#undef FUNCNAME
#define FUNCNAME SMPDU_Socki_free_eventq_mem
#undef FCNAME
#define FCNAME "SMPDU_Socki_free_eventq_mem"
static void SMPDU_Socki_free_eventq_mem(void)
{
    struct SMPDU_Socki_eventq_table *eventq_table, *eventq_table_next;

    smpd_enter_fn(FCNAME);

    eventq_table = SMPDU_Socki_eventq_table_head;
    while (eventq_table) {
        eventq_table_next = eventq_table->next;
        MPIU_Free(eventq_table);
        eventq_table = eventq_table_next;
    }
    SMPDU_Socki_eventq_table_head = NULL;

    smpd_exit_fn(FCNAME);
}

/* Provide a standard mechanism for setting the socket buffer size.
   The value is -1 if the default size hasn't been set, 0 if no size 
   should be set, and > 0 if that size should be used */
static int sockBufSize = -1;

/* Set the socket buffer sizes on fd to the standard values (this is controlled
   by the parameter MPICH_SOCK_BUFSIZE).  If "firm" is true, require that the
   sockets actually accept that buffer size.  */
int SMPDU_Sock_SetSockBufferSize( int fd, int firm )
{
    int result = SMPD_SUCCESS;
    char *env = NULL;
    int rc;

    /* Get the socket buffer size if we haven't yet acquired it */
    if (sockBufSize < 0) {
	/* FIXME: Is this the name that we want to use (this was chosen
	   to match the original, undocumented name) */
	rc = MPL_env2int( "MPICH_SOCKET_BUFFER_SIZE", &sockBufSize );
        env = getenv("MPICH_SOCKET_BUFFER_SIZE");
        if(env) { 
            sockBufSize = atoi(env);
	    if (sockBufSize <= 0) {
	        sockBufSize = 0;
	    }
        }
        smpd_dbg_printf("Sock buf size = %d\n", sockBufSize);
    }

    if (sockBufSize > 0) {
	int bufsz;
	socklen_t bufsz_len;

	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
	if (rc == -1) {
            result = SMPD_FAIL;
            smpd_err_printf("Setting send buf size for socket failed, bufsz = %d, errno = %d(%s)\n",
                                bufsz, errno, strerror(errno));
            goto fn_fail;
	}
	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
	if (rc == -1) {
            result = SMPD_FAIL;
            smpd_err_printf("Setting recv buf size for socket failed, bufsz = %d, errno = %d(%s)\n",
                                bufsz, errno, strerror(errno));
            goto fn_fail;
	}
	bufsz_len = sizeof(bufsz);

	if (firm) {
	    rc = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, &bufsz_len);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rc == 0) {
		if (bufsz < sockBufSize * 0.9) {
		    smpd_dbg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				sockBufSize, bufsz);
		}
	    }
	    /* --END ERROR HANDLING-- */
	    
	    bufsz_len = sizeof(bufsz);
	    rc = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, &bufsz_len);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rc == 0) {
		if (bufsz < sockBufSize * 0.9) {
		    smpd_dbg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				    sockBufSize, bufsz);
		}
	    }
	    /* --END ERROR HANDLING-- */
	}
    }
 fn_fail:
    return result;
}

/* This routine provides a string version of the address. */
int SMPDU_Sock_AddrToStr( SMPDU_Sock_ifaddr_t *ifaddr, char *str, int maxlen )
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
