/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpidu_sock.h"
#ifdef HAVE_STRING_H
/* Include for memcpy and memset */
#include <string.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#if defined(HAVE_POLL_H)
#include <sys/poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#include <netdb.h>
#include <errno.h>
#include <stdio.h>


/* FIXME: What do these mean?  Why is 32 a good size (e.g., is it because
   32*32 = 1024 if these are bits in a 4-byte int?  In that case, should
   these be related to a maximum processor count or an OS-defined fd limit? */
#if !defined(MPIDI_CH3I_SOCK_SET_DEFAULT_SIZE)
#define MPIDI_CH3I_SOCK_SET_DEFAULT_SIZE 32
#endif

#if !defined(MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE)
#define MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE 32
#endif


enum MPIDI_CH3I_Socki_state
{
    MPIDI_CH3I_SOCKI_STATE_FIRST = 0,
    MPIDI_CH3I_SOCKI_STATE_CONNECTING,
    MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW,
    MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO,
    MPIDI_CH3I_SOCKI_STATE_DISCONNECTED,
    MPIDI_CH3I_SOCKI_STATE_CLOSING,
    MPIDI_CH3I_SOCKI_STATE_LAST
};

enum MPIDI_CH3I_Socki_type
{
    MPIDI_CH3I_SOCKI_TYPE_FIRST = 0,
    MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION,
    MPIDI_CH3I_SOCKI_TYPE_LISTENER,
    MPIDI_CH3I_SOCKI_TYPE_INTERRUPTER,
    MPIDI_CH3I_SOCKI_TYPE_LAST
};

/*
 * struct pollinfo
 * 
 * sock_id - an integer id comprised of the sock_set id and the element number
 *           in the pollfd/info arrays
 * 
 * sock_set - a pointer to the sock set to which this connection belongs
 * 
 * elem - the element number of this connection in the pollfd/info arrays
 * 
 * sock - at present this is only used to free the sock structure when the 
 *        close is completed by MPIDIU_Sock_wait()
 * 
 * fd - this file descriptor is used whenever the file descriptor is needed. 
 *      this descriptor remains open until the sock, and
 *      thus the socket, are actually closed.  the fd in the pollfd structure
 *      should only be used for telling poll() if it should
 *      check for events on that descriptor.
 * 
 * user_ptr - a user supplied pointer that is included with event associated 
 *            with this connection
 * 
 * state - state of the connection
 *
 */
struct pollinfo
{
    int sock_id;
    struct MPIDI_CH3I_Sock_set * sock_set;
    int elem;
    struct MPIDI_CH3I_Sock * sock;
    int fd;
    void * user_ptr;
    enum MPIDI_CH3I_Socki_type type;
    enum MPIDI_CH3I_Socki_state state;
    int os_errno;
# ifdef MPICH_IS_THREADED
    int pollfd_events;
# endif
    union
    {
	struct
	{
	    MPL_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    size_t min;
	    size_t max;
	} buf;
    } read;
    int read_iov_flag;
    size_t read_nb;
    MPIDI_CH3I_Sock_progress_update_func_t read_progress_update_fn;
    union
    {
	struct
	{
	    MPL_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    size_t min;
	    size_t max;
	} buf;
    } write;
    int write_iov_flag;
    size_t write_nb;
    MPIDI_CH3I_Sock_progress_update_func_t write_progress_update_fn;
};

struct MPIDI_CH3I_Socki_eventq_elem
{
    struct MPIDI_CH3I_Sock_event event;
    int set_elem;
    struct MPIDI_CH3I_Socki_eventq_elem * next;
};

struct MPIDI_CH3I_Sock_set
{
    int id;

    /* when the pollfds array is scanned for activity, start with this element.
       this is used to prevent favoring a particular
       element, such as the first. */
    int starting_elem;

    /* pointers to the pollfd and pollinfo that make up the logical poll array,
       along with the current size of the array and last
       allocated element */
    int poll_array_sz;
    int poll_array_elems;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;

    /* head and tail pointers for the event queue */
    struct MPIDI_CH3I_Socki_eventq_elem * eventq_head;
    struct MPIDI_CH3I_Socki_eventq_elem * eventq_tail;
    
# ifdef MPICH_IS_THREADED
    /* pointer to the pollfds array being actively used by a blocking poll();
       NULL if not blocking in poll() */
    struct pollfd * pollfds_active;
    
    /* flag indicating if updates were made to any pollfd entries while a 
       thread was blocking in poll() */
    int pollfds_updated;
    
    /* flag indicating that a wakeup has already been posted on the 
       interrupter socket */
    int wakeup_posted;
    
    /* sock and fds for the interrpter pipe */
    struct MPIDI_CH3I_Sock * intr_sock;
    int intr_fds[2];
# endif
};

struct MPIDI_CH3I_Sock
{
    struct MPIDI_CH3I_Sock_set * sock_set;
    int elem;
};

/* FIXME: Why aren't these static */
int MPIDI_CH3I_Socki_initialized = 0;

static struct MPIDI_CH3I_Socki_eventq_elem * MPIDI_CH3I_Socki_eventq_pool = NULL;

/* MT: needs to be atomically incremented */
static int MPIDI_CH3I_Socki_set_next_id = 0;

/* Prototypes for functions used only within the socket code. */

/* Set the buffer size on the socket fd from the environment variable
   or other option; if "firm" is true, fail if the buffer size is not
   successfully set */
int MPIDI_CH3I_Sock_SetSockBufferSize( int fd, int firm );



/*********** socki_util.i ***********/

#ifdef MPICH_IS_THREADED
static int MPIDI_CH3I_Socki_wakeup(struct MPIDI_CH3I_Sock_set * sock_set);
int MPIDI_Sock_update_sock_set( struct MPIDI_CH3I_Sock_set *, int );
#endif

static int MPIDI_CH3I_Socki_os_to_mpi_errno(struct pollinfo * pollinfo,
		     int os_errno, const char * fcname, int line, int * conn_failed);

static int MPIDI_CH3I_Socki_adjust_iov(ssize_t nb, MPL_IOV * const iov,
				  const int count, int * const offsetp);

static int MPIDI_CH3I_Socki_sock_alloc(struct MPIDI_CH3I_Sock_set * sock_set,
				  struct MPIDI_CH3I_Sock ** sockp);
static void MPIDI_CH3I_Socki_sock_free(struct MPIDI_CH3I_Sock * sock);

static int MPIDI_CH3I_Socki_event_enqueue(struct pollinfo * pollinfo,
				     enum MPIDI_CH3I_Sock_op op,
				     size_t num_bytes,
				     void * user_ptr, int error);
static inline int MPIDI_CH3I_Socki_event_dequeue(struct MPIDI_CH3I_Sock_set * sock_set,
					    int * set_elem,
					    struct MPIDI_CH3I_Sock_event * eventp);

static void MPIDI_CH3I_Socki_free_eventq_mem(void);

struct MPIDI_CH3I_Socki_eventq_table
{
    struct MPIDI_CH3I_Socki_eventq_elem elems[MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE];
    struct MPIDI_CH3I_Socki_eventq_table * next;
};

static struct MPIDI_CH3I_Socki_eventq_table *MPIDI_CH3I_Socki_eventq_table_head=NULL;



#define MPIDI_CH3I_Socki_sock_get_pollfd(sock_)          (&(sock_)->sock_set->pollfds[(sock_)->elem])
#define MPIDI_CH3I_Socki_sock_get_pollinfo(sock_)        (&(sock_)->sock_set->pollinfos[(sock_)->elem])
#define MPIDI_CH3I_Socki_pollinfo_get_pollfd(pollinfo_) (&(pollinfo_)->sock_set->pollfds[(pollinfo_)->elem])


/* Enqueue a new event.  If the enqueue fails, generate an error and jump to
   the fail_label_ */
#define MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo_, op_, nb_, user_ptr_, event_mpi_errno_, mpi_errno_, fail_label_)	\
{									\
    mpi_errno_ = MPIDI_CH3I_Socki_event_enqueue((pollinfo_), (op_), (nb_), (user_ptr_), (event_mpi_errno_));		\
    if (mpi_errno_ != MPI_SUCCESS)					\
    {									\
	mpi_errno_ = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,	\
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
#   define MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)	\
    {								\
        (pollfd_)->events |= (op_);				\
        (pollfd_)->fd = (pollinfo_)->fd;			\
    }
#   define MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)	\
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
#   define MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)		\
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
	    MPIDI_CH3I_Socki_wakeup((pollinfo_)->sock_set);			\
	}								\
    }
#   define MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)		\
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
	    MPIDI_CH3I_Socki_wakeup((pollinfo_)->sock_set);			\
	}								\
    }
#endif

#define MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd_, pollinfo_, op_) ((pollfd_)->events & (op_))

/* FIXME: Low usage operations like this should be a function for
   better readability, modularity, and code size */
#define MPIDI_CH3I_SOCKI_GET_SOCKET_ERROR(pollinfo_, os_errno_, mpi_errno_, fail_label_)				\
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
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM, "**sock|osnomem",	\
		"**sock|osnomem %s %d %d", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id);		\
	}							\
	else							\
	{							\
	    mpi_errno = MPIR_Err_create_code(			\
		MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|oserror",		\
		"**sock|poll|oserror %s %d %d %d %s", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id,	\
		 (os_errno_), MPIR_Strerror(os_errno_));	\
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
#define MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno_, fail_label_)		\
{								        \
    if (MPIDI_CH3I_Socki_initialized <= 0)					\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_INIT,	\
					 "**sock|uninit", NULL);	\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VALIDATE_SOCK_SET(sock_set_, mpi_errno_, fail_label_)


#define MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock_, mpi_errno_, fail_label_)	\
{									\
    struct pollinfo * pollinfo__;					\
									\
    if ((sock_) == NULL || (sock_)->sock_set == NULL || (sock_)->elem < 0 ||							\
	(sock_)->elem >= (sock_)->sock_set->poll_array_elems)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);	\
	goto fail_label_;						\
    }									\
									\
    pollinfo__ = MPIDI_CH3I_Socki_sock_get_pollinfo(sock_);			\
									\
    if (pollinfo__->type <= MPIDI_CH3I_SOCKI_TYPE_FIRST || pollinfo__->type >= MPIDI_CH3I_SOCKI_TYPE_INTERRUPTER ||			\
	pollinfo__->state <= MPIDI_CH3I_SOCKI_STATE_FIRST || pollinfo__->state >= MPIDI_CH3I_SOCKI_STATE_LAST)				\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);	\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if ((pollinfo_)->type == MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)		\
    {									\
	if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING)		\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code(			\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK, "**sock|notconnected",		\
		"**sock|notconnected %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_DISCONNECTED)	\
	{								\
	    if ((pollinfo_)->os_errno == 0)				\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	\
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				\
	    }								\
	    else							\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	\
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			\
		    (pollinfo_)->os_errno, MPIR_Strerror((pollinfo_)->os_errno));						\
	    }								\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_CLOSING)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code(			\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_INPROGRESS, "**sock|closing",		\
		"**sock|closing %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
									\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state != MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW && (pollinfo_)->state != MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	\
						"**sock|badsock", NULL);							\
	    goto fail_label_;						\
	}								\
    }									\
    else if ((pollinfo_)->type == MPIDI_CH3I_SOCKI_TYPE_LISTENER)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code(				\
	    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK, "**sock|listener_read",		\
	    "**sock|listener_read %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
									\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_, mpi_errno_, fail_label_)						 \
{									\
    if ((pollinfo_)->type == MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)		\
    {									\
	if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING)		\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	 \
						"**sock|notconnected", "**sock|notconnected %d %d",				 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_DISCONNECTED || (pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO)	 \
	{								\
	    if ((pollinfo_)->os_errno == 0)				\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(			\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	 \
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    }								\
	    else							\
	    {								\
		(mpi_errno_) = MPIR_Err_create_code(										 \
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	 \
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			 \
		    (pollinfo_)->os_errno, MPIR_Strerror((pollinfo_)->os_errno));						 \
	    }								\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state == MPIDI_CH3I_SOCKI_STATE_CLOSING)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_INPROGRESS, \
						"**sock|closing", "**sock|closing %d %d",					 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
									\
	    goto fail_label_;						\
	}								\
	else if ((pollinfo_)->state != MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW)	\
	{								\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	 \
						"**sock|badsock", NULL);							 \
	    goto fail_label_;						\
	}								\
    }									\
    else if ((pollinfo_)->type == MPIDI_CH3I_SOCKI_TYPE_LISTENER)		\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	 \
					    "**sock|listener_write", "**sock|listener_write %d %d",				 \
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					 \
									\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo_, mpi_errno_, fail_label_)	\
{									\
    if ((pollinfo_)->fd < 0)						\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badhandle", "**sock|poll|badhandle %d %d %d",				\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->fd);			\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLIN))	\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_INPROGRESS,	\
					    "**sock|reads", "**sock|reads %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;						\
    }									\
}


#define MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{									\
    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLOUT))	\
    {									\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_INPROGRESS,	\
					    "**sock|writes", "**sock|writes %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;						\
    }									\
}
#else
/* Use minimal to no checking */
#define MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VALIDATE_SOCK_SET(sock_set_,mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock_,mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_,mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_,mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo_,mpi_errno_,fail_label_)
#define MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd_,pollinfo_,mpi_errno,fail_label_)
#define MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_,pollinfo_,mpi_errno,fail_label_)

#endif


#ifdef MPICH_IS_THREADED

/*
 * MPIDI_CH3I_Socki_wakeup()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_wakeup(struct MPIDI_CH3I_Sock_set * sock_set)
{
    MPIR_THREAD_CHECK_BEGIN;
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

	    MPIR_Assertp(nb == 0 || errno == EINTR);
	}

	sock_set->wakeup_posted = TRUE;
    }
    MPIR_THREAD_CHECK_END;
    return MPIDI_CH3I_SOCK_SUCCESS;
}
/* end MPIDI_CH3I_Socki_wakeup() */

#undef FUNCNAME
#define FUNCNAME MPIDI_Sock_update_sock_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Sock_update_sock_set( struct MPIDI_CH3I_Sock_set *sock_set,
				int pollfds_active_elems )
{
    int mpi_errno = MPI_SUCCESS;
    int elem;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);
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
	MPL_free(sock_set->pollfds_active);
    }

    sock_set->pollfds_updated = FALSE;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SOCK_UPDATE_SOCK_SET);
    return mpi_errno;

}

#endif /* (MPICH_IS_THREADED) */


/*
 * MPIDI_CH3I_Socki_os_to_mpi_errno()
 *
 * This routine assumes that no thread can change the state between state check before the nonblocking OS operation and the call
 * to this routine.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_os_to_mpi_errno
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
static int MPIDI_CH3I_Socki_os_to_mpi_errno(struct pollinfo * pollinfo, int os_errno, const char * fcname, int line, int * disconnected)
{
    int mpi_errno;

    if (os_errno == ENOMEM || os_errno == ENOBUFS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDI_CH3I_SOCK_ERR_NOMEM,
					 "**sock|osnomem", "**sock|poll|osnomem %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIR_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EFAULT || os_errno == EINVAL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDI_CH3I_SOCK_ERR_BAD_BUF,
					 "**sock|badbuf", "**sock|poll|badbuf %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIR_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EPIPE)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDI_CH3I_SOCK_ERR_CONN_CLOSED,
					 "**sock|connclosed", "**sock|poll|connclosed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIR_Strerror(os_errno));
	*disconnected = TRUE;
    }
    else if (os_errno == ECONNRESET || os_errno == ENOTCONN || os_errno == ETIMEDOUT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDI_CH3I_SOCK_ERR_CONN_FAILED,
					 "**sock|connfailed", "**sock|poll|connfailed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIR_Strerror(os_errno));
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDI_CH3I_SOCK_ERR_CONN_FAILED,
					 "**sock|oserror", "**sock|poll|oserror %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIR_Strerror(os_errno));
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }

    return mpi_errno;
}
/* --END ERROR HANDLING-- */
/* end MPIDI_CH3I_Socki_os_to_mpi_errno() */


/*
 * MPIDI_CH3I_Socki_adjust_iov()
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
#define FUNCNAME MPIDI_CH3I_Socki_adjust_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_adjust_iov(ssize_t nb, MPL_IOV * const iov, const int count, int * const offsetp)
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
/* end MPIDI_CH3I_Socki_adjust_iov() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_sock_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_sock_alloc(struct MPIDI_CH3I_Sock_set * sock_set, struct MPIDI_CH3I_Sock ** sockp)
{
    struct MPIDI_CH3I_Sock * sock = NULL;
    int avail_elem;
    struct pollfd * pollfds = NULL;
    struct pollinfo * pollinfos = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_ALLOC);

    /* FIXME: Should this use the CHKPMEM macros (perm malloc)? */
    sock = MPL_malloc(sizeof(struct MPIDI_CH3I_Sock), MPL_MEM_ADDRESS);
    /* --BEGIN ERROR HANDLING-- */
    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM, "**nomem", 0);
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

	pollfds = MPL_malloc((sock_set->poll_array_sz + MPIDI_CH3I_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd), MPL_MEM_ADDRESS);
	/* --BEGIN ERROR HANDLING-- */
	if (pollfds == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					     "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	pollinfos = MPL_malloc((sock_set->poll_array_sz + MPIDI_CH3I_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo), MPL_MEM_ADDRESS);
	/* --BEGIN ERROR HANDLING-- */
	if (pollinfos == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
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
		MPL_free(sock_set->pollfds);
	    }
#	    else
	    {
		if (sock_set->pollfds_active == NULL)
		{
		    memcpy(pollfds, sock_set->pollfds, sock_set->poll_array_sz * sizeof(struct pollfd));
		}
		if  (sock_set->pollfds_active != sock_set->pollfds)
		{
		    MPL_free(sock_set->pollfds);
		}
	    }
#           endif

	    memcpy(pollinfos, sock_set->pollinfos, sock_set->poll_array_sz * sizeof(struct pollinfo));
	    MPL_free(sock_set->pollinfos);
	}

	sock_set->poll_array_elems = avail_elem + 1;
	sock_set->poll_array_sz += MPIDI_CH3I_SOCK_SET_DEFAULT_SIZE;
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
	    pollinfos[elem].type  = MPIDI_CH3I_SOCKI_TYPE_FIRST;
	    pollinfos[elem].state = MPIDI_CH3I_SOCKI_STATE_FIRST;
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
    MPIR_Assert(sock_set->pollinfos[avail_elem].sock_set == sock_set);
    MPIR_Assert(sock_set->pollinfos[avail_elem].elem == avail_elem);
    MPIR_Assert(sock_set->pollinfos[avail_elem].fd == -1);
    MPIR_Assert(sock_set->pollinfos[avail_elem].sock == NULL);
    MPIR_Assert(sock_set->pollinfos[avail_elem].sock_id == -1);
    MPIR_Assert(sock_set->pollinfos[avail_elem].type == MPIDI_CH3I_SOCKI_TYPE_FIRST);
    MPIR_Assert(sock_set->pollinfos[avail_elem].state == MPIDI_CH3I_SOCKI_STATE_FIRST);
#   ifdef MPICH_IS_THREADED
    {
	MPIR_Assert(sock_set->pollinfos[avail_elem].pollfd_events == 0);
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
        MPIR_THREAD_CHECK_BEGIN;
	if (sock_set->pollfds_active != NULL)
	{
	    sock_set->pollfds_updated = TRUE;
	}
        MPIR_THREAD_CHECK_END;
    }
#   endif

    *sockp = sock;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_ALLOC);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (pollinfos != NULL)
    {
	MPL_free(pollinfos);
    }

    if (pollfds != NULL)
    {
	MPL_free(pollfds);
    }

    if (sock != NULL)
    {
	MPL_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end MPIDI_CH3I_Socki_sock_alloc() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_sock_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void MPIDI_CH3I_Socki_sock_free(struct MPIDI_CH3I_Sock * sock)
{
    struct pollfd * pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    struct pollinfo * pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);
    struct MPIDI_CH3I_Sock_set * sock_set = sock->sock_set;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_FREE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_FREE);

    /* FIXME: We need an abstraction for the thread sync operations */
#   ifdef MPICH_IS_THREADED
    {
	/*
	 * Freeing a sock while Sock_wait() is blocked in poll() is not supported
	 */
	MPIR_Assert(sock_set->pollfds_active == NULL);
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
    pollinfo->type    = MPIDI_CH3I_SOCKI_TYPE_FIRST;
    pollinfo->state   = MPIDI_CH3I_SOCKI_STATE_FIRST;
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

    MPL_free(sock);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_FREE);
}
/* end MPIDI_CH3I_Socki_sock_free() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_event_enqueue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_event_enqueue(struct pollinfo * pollinfo, MPIDI_CH3I_Sock_op_t op, size_t num_bytes,
				     void * user_ptr, int error)
{
    struct MPIDI_CH3I_Sock_set * sock_set = pollinfo->sock_set;
    struct MPIDI_CH3I_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_ENQUEUE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_ENQUEUE);

    if (MPIDI_CH3I_Socki_eventq_pool != NULL)
    {
	eventq_elem = MPIDI_CH3I_Socki_eventq_pool;
	MPIDI_CH3I_Socki_eventq_pool = MPIDI_CH3I_Socki_eventq_pool->next;
    }
    else
    {
	int i;
	struct MPIDI_CH3I_Socki_eventq_table *eventq_table;

	eventq_table = MPL_malloc(sizeof(struct MPIDI_CH3I_Socki_eventq_table), MPL_MEM_OTHER);
	/* --BEGIN ERROR HANDLING-- */
	if (eventq_table == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**sock|poll|eqmalloc", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

        eventq_elem = eventq_table->elems;

        eventq_table->next = MPIDI_CH3I_Socki_eventq_table_head;
        MPIDI_CH3I_Socki_eventq_table_head = eventq_table;

	if (MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE > 1)
	{
	    MPIDI_CH3I_Socki_eventq_pool = &eventq_elem[1];
	    for (i = 0; i < MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE - 2; i++)
	    {
		MPIDI_CH3I_Socki_eventq_pool[i].next = &MPIDI_CH3I_Socki_eventq_pool[i+1];
	    }
	    MPIDI_CH3I_Socki_eventq_pool[MPIDI_CH3I_SOCK_EVENTQ_POOL_SIZE - 2].next = NULL;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_ENQUEUE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_event_dequeue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Socki_event_dequeue(struct MPIDI_CH3I_Sock_set * sock_set, int * set_elem, struct MPIDI_CH3I_Sock_event * eventp)
{
    struct MPIDI_CH3I_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_DEQUEUE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_DEQUEUE);

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

	eventq_elem->next = MPIDI_CH3I_Socki_eventq_pool;
	MPIDI_CH3I_Socki_eventq_pool = eventq_elem;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* FIXME: Shouldn't this be an mpi error code? */
	mpi_errno = MPIDI_CH3I_SOCK_ERR_FAIL;
    }
    /* --END ERROR HANDLING-- */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_DEQUEUE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_event_dequeue() */


/* FIXME: Who allocates eventq tables?  Should there be a check that these
   tables are empty first? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_free_eventq_mem
#undef FCNAME
#define FCNAME "MPIDI_CH3I_Socki_free_eventq_mem"
static void MPIDI_CH3I_Socki_free_eventq_mem(void)
{
    struct MPIDI_CH3I_Socki_eventq_table *eventq_table, *eventq_table_next;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);

    eventq_table = MPIDI_CH3I_Socki_eventq_table_head;
    while (eventq_table) {
        eventq_table_next = eventq_table->next;
        MPL_free(eventq_table);
        eventq_table = eventq_table_next;
    }
    MPIDI_CH3I_Socki_eventq_table_head = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SOCKI_FREE_EVENTQ_MEM);
}

/* Provide a standard mechanism for setting the socket buffer size.
   The value is -1 if the default size hasn't been set, 0 if no size
   should be set, and > 0 if that size should be used */
static int sockBufSize = -1;

/* Set the socket buffer sizes on fd to the standard values (this is controlled
   by the parameter MPICH_SOCK_BUFSIZE).  If "firm" is true, require that the
   sockets actually accept that buffer size.  */
int MPIDI_CH3I_Sock_SetSockBufferSize( int fd, int firm )
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
	MPL_DBG_MSG_D(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,"Sock buf size = %d",sockBufSize);
    }

    if (sockBufSize > 0) {
	int bufsz;
	socklen_t bufsz_len;

	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsz, bufsz_len);
	if (rc == -1) {
	    MPIR_ERR_SETANDJUMP3(mpi_errno,MPIDI_CH3I_SOCK_ERR_FAIL,
				 "**sock|poll|setsndbufsz",
				 "**sock|poll|setsndbufsz %d %d %s",
				 bufsz, errno, MPIR_Strerror(errno));
	}
	bufsz     = sockBufSize;
	bufsz_len = sizeof(bufsz);
	rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsz, bufsz_len);
	if (rc == -1) {
	    MPIR_ERR_SETANDJUMP3(mpi_errno,MPIDI_CH3I_SOCK_ERR_FAIL,
				 "**sock|poll|setrcvbufsz",
				 "**sock|poll|setrcvbufsz %d %d %s",
				 bufsz, errno, MPIR_Strerror(errno));
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

/*********** end of socki_util.i *********/

/*********** sock_init.i *****************/

#if defined (MPL_USE_DBG_LOGGING)
MPL_dbg_class MPIDI_CH3I_DBG_SOCK_CONNECT;
#endif /* MPL_USE_DBG_LOGGING */

/* FIXME: The usual missing documentation (what are these routines for?
   preconditions?  who calls? post conditions? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_init(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_INIT);

#if defined (MPL_USE_DBG_LOGGING)
    MPIDI_CH3I_DBG_SOCK_CONNECT = MPL_dbg_class_alloc("SOCK_CONNECT", "sock_connect");
#endif

    MPIDI_CH3I_Socki_initialized++;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_INIT);
    return MPI_SUCCESS;
}

/* FIXME: Who calls?  When?  Should this be a finalize handler instead? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_FINALIZE);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_FINALIZE);

    MPIDI_CH3I_Socki_initialized--;

    if (MPIDI_CH3I_Socki_initialized == 0)
    {
	MPIDI_CH3I_Socki_free_eventq_mem();
    }

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_FINALIZE);
    return mpi_errno;
}

/*********** end of sock_init.i *****************/

/*********** sock_set.i *****************/

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_create_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_create_set(struct MPIDI_CH3I_Sock_set ** sock_setp)
{
    struct MPIDI_CH3I_Sock_set * sock_set = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_CREATE_SET);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_CREATE_SET);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * Allocate and initialized a new sock set structure
     */
    sock_set = MPL_malloc(sizeof(struct MPIDI_CH3I_Sock_set), MPL_MEM_ADDRESS);
    /* --BEGIN ERROR HANDLING-- */
    if (sock_set == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					 "**sock|setalloc", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    sock_set->id = MPIDI_CH3I_Socki_set_next_id++;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;
    /* FIXME: Move the thread-specific operations into thread-specific
       routines (to allow for alternative thread sync models and
       for runtime control of thread level) */
#   ifdef MPICH_IS_THREADED
    {
	sock_set->pollfds_active = NULL;
	sock_set->pollfds_updated = FALSE;
	sock_set->wakeup_posted = FALSE;
	sock_set->intr_fds[0] = -1;
	sock_set->intr_fds[1] = -1;
	sock_set->intr_sock = NULL;
    }
#   endif

#   ifdef MPICH_IS_THREADED
    MPIR_THREAD_CHECK_BEGIN;
    {
	struct MPIDI_CH3I_Sock * sock = NULL;
	struct pollfd * pollfd;
	struct pollinfo * pollinfo;
	long flags;
	int rc;

	/*
	 * Acquire a pipe (the interrupter) to wake up a blocking poll should
	 * it become necessary.
	 *
	 * Make the read descriptor nonblocking.  The write descriptor is left
	 * as a blocking descriptor.  The write has to
	 * succeed or the system will lock up.  Should the blocking descriptor
	 * prove to be a problem, then (1) copy the above
	 * code, applying it to the write descriptor, and (2) update
	 * MPIDI_CH3I_Socki_wakeup() so that it loops while write returns 0,
	 * performing a thread yield between iterations.
	 */
	rc = pipe(sock_set->intr_fds);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					     "**sock|poll|pipe", "**sock|poll|pipe %d %s", errno, MPIR_Strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	flags = fcntl(sock_set->intr_fds[0], F_GETFL, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (flags == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					     "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d %s",
					     errno, MPIR_Strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	rc = fcntl(sock_set->intr_fds[0], F_SETFL, flags | O_NONBLOCK);
	/* --BEGIN ERROR HANDLING-- */
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					     "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d %s",
					     errno, MPIR_Strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	/*
	 * Allocate and initialize a sock structure for the interrupter pipe
	 */
	mpi_errno = MPIDI_CH3I_Socki_sock_alloc(sock_set, &sock);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					     "**sock|sockalloc", NULL);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	sock_set->intr_sock = sock;

	pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
	pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

	pollfd->fd = sock_set->intr_fds[0];
	pollinfo->fd = sock_set->intr_fds[0];
	pollinfo->user_ptr = NULL;
	pollinfo->type = MPIDI_CH3I_SOCKI_TYPE_INTERRUPTER;
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	pollinfo->os_errno = 0;

	MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
    }
    MPIR_THREAD_CHECK_END;
#   endif

    *sock_setp = sock_set;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_CREATE_SET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (sock_set != NULL)
    {
#       ifdef MPICH_IS_THREADED
	MPIR_THREAD_CHECK_BEGIN;
	{
	    if (sock_set->intr_fds[0] != -1)
	    {
		close(sock_set->intr_fds[0]);
	    }

	    if (sock_set->intr_fds[1] != -1)
	    {
		close(sock_set->intr_fds[1]);
	    }
	}
	MPIR_THREAD_CHECK_END;
#	endif

	MPL_free(sock_set);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_close_open_sockets
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_close_open_sockets(struct MPIDI_CH3I_Sock_set * sock_set, void** user_ptr ){

    int i;
    int mpi_errno = MPI_SUCCESS;
    struct pollinfo * pollinfos = NULL;
    pollinfos = sock_set->pollinfos;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_CLOSE_OPEN_SOCKETS);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_CLOSE_OPEN_SOCKETS);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    /* wakeup waiting socket if mullti-threades */
    *user_ptr = NULL;
    for (i = 0; i < sock_set->poll_array_elems; i++) {
        if(pollinfos[i].sock != NULL &&  pollinfos[i].type != MPIDI_CH3I_SOCKI_TYPE_INTERRUPTER){
             close(pollinfos[i].fd);
             MPIDI_CH3I_Socki_sock_free(pollinfos[i].sock);
            *user_ptr = pollinfos[i].user_ptr;
             break;
        }
    }
#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_CLOSE_OPEN_SOCKETS);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_destroy_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_destroy_set(struct MPIDI_CH3I_Sock_set * sock_set)
{
    int elem;
    struct MPIDI_CH3I_Sock_event event;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_DESTROY_SET);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_DESTROY_SET);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * FIXME: check for open socks and return an error if any are found
     */

    /*
     * FIXME: verify no other thread is blocked in poll().  wake it up and
     * get it to exit.
     */

    /*
     * Close pipe for interrupting a blocking poll()
     */
#   ifdef MPICH_IS_THREADED
    MPIR_THREAD_CHECK_BEGIN;
    {
	close(sock_set->intr_fds[1]);
	close(sock_set->intr_fds[0]);
	MPIDI_CH3I_Socki_sock_free(sock_set->intr_sock);

	sock_set->pollfds_updated = FALSE;
	sock_set->pollfds_active = NULL;
	sock_set->wakeup_posted = FALSE;
	sock_set->intr_fds[0] = -1;
	sock_set->intr_fds[1] = -1;
	sock_set->intr_sock = NULL;
    }
    MPIR_THREAD_CHECK_END;
#   endif

    /*
     * Clear the event queue to eliminate memory leaks
     */
    while (MPIDI_CH3I_Socki_event_dequeue(sock_set, &elem, &event) == MPI_SUCCESS);

    /*
     * Free structures used by the sock set
     */
    MPL_free(sock_set->pollinfos);
    MPL_free(sock_set->pollfds);

    /*
     * Reset the sock set fields
     */
    sock_set->id = ~0;
    sock_set->poll_array_sz = 0;
    sock_set->poll_array_elems = 0;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;

    /*
     * Free the structure
     */
    MPL_free(sock_set);

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_DESTROY_SET);
    return mpi_errno;
}

/*********** end of sock_set.i *****************/

/*********** sock_post.i *****************/

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_connect_ifaddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*
 This routine connects to a particular address (in byte form).
  By avoiding a character name for an interface (we *never*
 connect to a host; we are *always* connecting to a particular interface
 on a host), we avoid problems with DNS services, including lack of properly
 configured services and scalability problems.

 This routine was constructed from MPIDI_CH3I_Sock_post_connect by removing the
 poorly placed use of gethostname within the middle of that routine and
 simply using the ifaddr field that is passed to this routine.
 MPIDI_CH3I_Sock_post_connect simply uses the hostname field to get the canonical
 IP address.  The original routine and its API was retained to allow backwards
 compatibility until it is determined that we can always use explicit addrs
 needed in setting up the socket instead of character strings.
 */
int MPIDI_CH3I_Sock_post_connect_ifaddr( struct MPIDI_CH3I_Sock_set * sock_set,
				    void * user_ptr,
				    MPL_sockaddr_t *p_addr, int port,
				    struct MPIDI_CH3I_Sock ** sockp)
{
    struct MPIDI_CH3I_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_CONNECT_IFADDR);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_CONNECT_IFADDR);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /*
     * Create a non-blocking socket with Nagle's algorithm disabled
     */
    fd = MPL_socket();
    if (fd == -1) {
	/* FIXME: It would be better to include a special formatting
	   clue for system error messages (e.g., %dSE; in the recommended
	   revision for error reporting (that is, value (errno) is an int,
	   but should be interpreted as an System Error string) */
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDI_CH3I_SOCK_ERR_FAIL,
			     "**sock|poll|socket",
		    "**sock|poll|socket %d %s", errno, MPIR_Strerror(errno));
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDI_CH3I_SOCK_ERR_FAIL,
			     "**sock|poll|nonblock",
                    "**sock|poll|nonblock %d %s", errno, MPIR_Strerror(errno));
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1) {
	MPIR_ERR_SETANDJUMP2( mpi_errno, MPIDI_CH3I_SOCK_ERR_FAIL,
			      "**sock|poll|nonblock",
			      "**sock|poll|nonblock %d %s",
			      errno, MPIR_Strerror(errno));
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0) {
	MPIR_ERR_SETANDJUMP2(mpi_errno,MPIDI_CH3I_SOCK_ERR_FAIL,
			     "**sock|poll|nodelay",
			     "**sock|poll|nodelay %d %s",
			     errno, MPIR_Strerror(errno));
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
    mpi_errno = MPIDI_CH3I_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPIDI_CH3I_SOCK_ERR_NOMEM,
			    "**sock|sockalloc");
    }

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;

    /*
     * Set and verify the socket buffer size
     */
    mpi_errno = MPIDI_CH3I_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

    /*
     * Attempt to establish the connection
     */
    MPL_DBG_STMT(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,{
	char addrString[64];
        MPL_sockaddr_to_str(p_addr, addrString, 64);
	MPL_DBG_MSG_FMT(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,(MPL_DBG_FDEST,
			      "Connecting to %s:%d", addrString, port ));
	})

    do
    {
        rc = MPL_connect(fd, p_addr, port);
    }
    while (rc == -1 && errno == EINTR);

    if (rc == 0)
    {
	/* connection succeeded */
	MPL_DBG_MSG_P(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_CONNECTED_RW for sock %p",sock);
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW;
	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CONNECT, 0, user_ptr, MPI_SUCCESS, mpi_errno, fn_fail);
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	MPL_DBG_MSG_P(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_CONNECTING for sock %p",sock);
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTING;
	MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);
    }
    else
    {
	MPL_DBG_MSG_P(MPIDI_CH3I_DBG_SOCK_CONNECT,TYPICAL,"Setting state to SOCKI_STATE_DISCONNECTED (failure in connect) for sock %p",sock);
	pollinfo->os_errno = errno;
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;

	if (errno == ECONNREFUSED)
	{
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_FAILED,
		"**sock|connrefused", "**sock|poll|connrefused %d %d %s",
		pollinfo->sock_set->id, pollinfo->sock_id, ""), mpi_errno, fn_fail);
	}
	else
	{
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_FAILED,
		"**sock|oserror", "**sock|poll|oserror %d %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, errno,
		MPIR_Strerror(errno)), mpi_errno, fn_fail);
	}
    }
    /* --END ERROR HANDLING-- */

    *sockp = sock;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_CONNECT_IFADDR);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (fd != -1)
    {
	close(fd);
    }

    if (sock != NULL)
    {
	MPIDI_CH3I_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* FIXME: What does this routine do?  Why does it take a host description
   instead of an interface name or address? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_connect(struct MPIDI_CH3I_Sock_set * sock_set, void * user_ptr,
			    char * host_description, int port,
			    struct MPIDI_CH3I_Sock ** sockp)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPL_sockaddr_t addr;

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
    ret = MPL_get_sockaddr(host_description, &addr);
    /* --BEGIN ERROR HANDLING-- */
    if (ret) {
	/* FIXME: Set error */
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPIDI_CH3I_Sock_post_connect_ifaddr( sock_set, user_ptr,
						&addr, port, sockp );
 fn_exit:
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_post_connect() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_listen
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
#ifndef USHRT_MAX
#define USHRT_MAX 65535   /* 2^16-1 */
#endif
int MPIDI_CH3I_Sock_listen(struct MPIDI_CH3I_Sock_set * sock_set, void * user_ptr,
		      int * port, struct MPIDI_CH3I_Sock ** sockp)
{
    struct MPIDI_CH3I_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    int optval;
    MPL_sockaddr_t addr;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_LISTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_LISTEN);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (*port < 0 || *port > USHRT_MAX)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_PORT,
					 "**sock|badport", "**sock|badport %d", *port);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Create a non-blocking socket for the listener
     */
    fd = MPL_socket();
    /* --BEGIN ERROR HANDLING-- */
    if (fd == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d %s", errno, MPIR_Strerror(errno));
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
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					     "**sock|poll|reuseaddr", "**sock|poll|reuseaddr %d %s", errno, MPIR_Strerror(errno));
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }

    /* make the socket non-blocking so that accept() will return immediately if no new connection is available */
    flags = fcntl(fd, F_GETFL, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Set and verify the socket buffer size
     * (originally this is set after bind and before listen).
     */
    mpi_errno = MPIDI_CH3I_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }

    /*
     * Bind the socket to all interfaces and the specified port.  The port specified by the calling routine may be 0, indicating
     * that the operating system can select an available port in the ephemeral port range.
     */
    if (*port == 0) {
        unsigned short portnum;

	/* see if we actually want to find values within a range */
        MPIR_ERR_CHKANDJUMP(MPIR_CVAR_CH3_PORT_RANGE.low < 0 || MPIR_CVAR_CH3_PORT_RANGE.low > MPIR_CVAR_CH3_PORT_RANGE.high, mpi_errno, MPI_ERR_OTHER, "**badportrange");

        /* default MPICH_PORT_RANGE is {0,0} so bind will use any available port */
        if (MPIR_CVAR_CH3_PORT_RANGE.low == 0){
            rc = MPL_listen_anyport(fd, &portnum);
        }
        else {
            rc = MPL_listen_portrange(fd, &portnum, MPIR_CVAR_CH3_PORT_RANGE.low, MPIR_CVAR_CH3_PORT_RANGE.high);
        }
        *port = portnum;
    }
    else {
        rc = MPL_listen(fd, *port);
    }
    /*
     * listening for incoming connections...
     */
    /* --BEGIN ERROR HANDLING-- */
    if (rc)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|listen", "**sock|poll|listen %d %s", errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Allocate and initialize sock and poll structures.  If another thread is
     * blocking in poll(), that thread must be woke up
     * long enough to pick up the addition of the listener socket.
     */
    mpi_errno = MPIDI_CH3I_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDI_CH3I_SOCKI_TYPE_LISTENER;
    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;

    MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

    *sockp = sock;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_LISTEN);
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
/* end MPIDI_CH3I_Sock_listen() */


/* FIXME: What does this function do? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_read(struct MPIDI_CH3I_Sock * sock, void * buf, size_t minlen, size_t maxlen,
			 MPIDI_CH3I_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_READ);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_READ);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_LEN,
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

    MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_READ);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_post_read() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_readv(struct MPIDI_CH3I_Sock * sock, MPL_IOV * iov, int iov_n, MPIDI_CH3I_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_READV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_READV);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > MPL_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_LEN,
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

    MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_READV);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_post_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_write(struct MPIDI_CH3I_Sock * sock, void * buf, size_t minlen, size_t maxlen,
			  MPIDI_CH3I_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITE);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_LEN,
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

    MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_post_write() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_writev(struct MPIDI_CH3I_Sock * sock, MPL_IOV * iov, int iov_n, MPIDI_CH3I_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITEV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITEV);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (iov_n < 1 || iov_n > MPL_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_LEN,
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

    MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_WRITEV);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_post_writev() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_post_close
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_post_close(struct MPIDI_CH3I_Sock * sock)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;

    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_POST_CLOSE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_POST_CLOSE);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK, "**sock|closing_already",
	    "**sock|closing_already %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)
    {
	if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN | POLLOUT))
	{
	    /* --BEGIN ERROR HANDLING-- */
	    int event_mpi_errno;

	    event_mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_SOCK_CLOSED, "**sock|close_cancel",
		"**sock|close_cancel %d %d", pollinfo->sock_set->id, pollinfo->sock_id);

	    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLIN))
	    {
		MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }

	    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  MPI_SUCCESS, mpi_errno, fn_exit);
	    }

	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	    /* --END ERROR HANDLING-- */
	}
    }
    else /* if (pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_LISTENER) */
    {
	/*
	 * The event queue may contain an accept event which means that
	 * MPIDI_CH3I_Sock_accept() may be legally called after
	 * MPIDI_CH3I_Sock_post_close().  However, MPIDI_CH3I_Sock_accept() must be
	 * called before the close event is return by
	 * MPIDI_CH3I_Sock_wait().
	 */
	MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }

    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CLOSE, 0, pollinfo->user_ptr, MPI_SUCCESS, mpi_errno, fn_exit);
    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CLOSING;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_POST_CLOSE);
    return mpi_errno;
}

/*********** end of sock_post.i *****************/

/*********** sock_immed.i *****************/

/* FIXME: Why is this the _immed file (what does immed stand for?) */

/* FIXME: What do any of these routines do?  What are the arguments?
   Special conditions (see the FIXME on len = SSIZE_MAX)?  preconditions?
   postconditions? */

/* FIXME: What does this function do?  What are its arguments?
   It appears to execute a nonblocking accept call */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_accept(struct MPIDI_CH3I_Sock * listener,
		      struct MPIDI_CH3I_Sock_set * sock_set, void * user_ptr,
		      struct MPIDI_CH3I_Sock ** sockp)
{
    struct MPIDI_CH3I_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    MPL_sockaddr_t addr;
    socklen_t addr_len;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_ACCEPT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_ACCEPT);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(listener, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK_SET(sock_set, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(listener);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(listener);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->type != MPIDI_CH3I_SOCKI_TYPE_LISTENER)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
		 FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,
		 "**sock|listener_bad_sock", "**sock|listener_bad_sock %d %d",
		 pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }

    if (pollinfo->state != MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO &&
	pollinfo->state != MPIDI_CH3I_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
		FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,
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
    addr_len = sizeof(addr);
    /* FIXME: Either use the syscall macro or correctly wrap this in a
       test for EINTR */
    fd = accept(pollinfo->fd, (struct sockaddr *) &addr, &addr_len);

    if (pollinfo->state != MPIDI_CH3I_SOCKI_STATE_CLOSING)
    {
	/*
	 * Unless the listener sock is being closed, add it back into the
	 * poll list so that new connections will be detected.
	 */
	MPIDI_CH3I_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (fd == -1)
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
			     FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NO_NEW_SOCK,
			     "**sock|nosock", NULL);
	}
	else if (errno == ENOBUFS || errno == ENOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
				FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
				"**sock|osnomem", NULL);
	}
	else if (errno == EBADF || errno == ENOTSOCK || errno == EOPNOTSUPP)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                           FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,
			  "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
			  pollinfo->sock_set->id, pollinfo->sock_id,
			  pollinfo->fd);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                           FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NO_NEW_SOCK,
			   "**sock|poll|accept", "**sock|poll|accept %d %s",
			   errno, MPIR_Strerror(errno));
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
			 FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s",
			 errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
			 FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
			 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s",
			 errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    /* --BEGIN ERROR HANDLING-- */
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
			 FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
			 "**sock|poll|nodelay", "**sock|poll|nodelay %d %s",
                         errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /*
     * Verify that the socket buffer size is correct
     */
    /* FIXME: Who sets the socket buffer size?  Why isn't the test
       made at that time? */
#if 1
    mpi_errno = MPIDI_CH3I_Sock_SetSockBufferSize( fd, 1 );
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
#else
    if (MPIDI_CH3I_Socki_socket_bufsz > 0)
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
	    if (bufsz < MPIDI_CH3I_Socki_socket_bufsz * 0.9 ||
		bufsz < MPIDI_CH3I_Socki_socket_bufsz * 1.0)
	    {
		MPL_msg_printf("WARNING: send socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDI_CH3I_Socki_socket_bufsz, bufsz);
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
	    if (bufsz < MPIDI_CH3I_Socki_socket_bufsz * 0.9 ||
		bufsz < MPIDI_CH3I_Socki_socket_bufsz * 1.0)
	    {
		MPL_msg_printf("WARNING: receive socket buffer size differs from requested size (requested=%d, actual=%d)\n",
				MPIDI_CH3I_Socki_socket_bufsz, bufsz);
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
    mpi_errno = MPIDI_CH3I_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;

    *sockp = sock;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_ACCEPT);
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
/* end MPIDI_CH3I_Sock_accept() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_read(MPIDI_CH3I_Sock_t sock, void * buf, size_t len,
		    size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    size_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_READ);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_READ);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_READ);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

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
	MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_READ);
	nb = read(pollinfo->fd, buf, len);
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_READ);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;

	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
	    MPIDI_CH3I_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d",
	    pollinfo->sock_set->id, pollinfo->sock_id);

	if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{
	    /* A write is posted on this connection.  Enqueue an event for
	       the write indicating the connection is closed. */
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE,
				      pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}

	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;

	*num_read = 0;

	mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno,
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
	    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		/* A write is posted on this connection.  Enqueue an event
		   for the write indicating the connection is closed. */
		MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE,
					pollinfo->write_nb, pollinfo->user_ptr,
					mpi_errno, mpi_errno, fn_exit);
		MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }

	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_READ);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_read() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_readv(MPIDI_CH3I_Sock_t sock, MPL_IOV * iov, int iov_n,
		     size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_READV);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_READV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_READV);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent
     * limitation in the Sock interface?
     */
    do
    {
	MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_READV);
	nb = MPL_large_readv(pollinfo->fd, iov, iov_n);
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_READV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (size_t) nb;
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	*num_read = 0;

	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
	    MPIDI_CH3I_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d",
	    pollinfo->sock_set->id, pollinfo->sock_id);

	if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{

	    /* A write is posted on this connection.  Enqueue an event
	       for the write indicating the connection is closed. */
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE,
				      pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}

	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;

	*num_read = 0;

	mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME,
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
	    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		/* A write is posted on this connection.  Enqueue an event
		   for the write indicating the connection is closed. */
		MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE,
					pollinfo->write_nb, pollinfo->user_ptr,
					  mpi_errno, mpi_errno, fn_exit);
		MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }

	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_READV);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_write(MPIDI_CH3I_Sock_t sock, void * buf, size_t len,
		     size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WRITE);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_WRITE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_WRITE);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }

    do
    {
	MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WRITE);
	nb = write(pollinfo->fd, buf, len);
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WRITE);
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

	mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME,
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
	     * MPIDI_CH3I_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_WRITE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_write() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_writev(MPIDI_CH3I_Sock_t sock, MPL_IOV * iov, int iov_n, size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WRITEV);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_WRITEV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_WRITEV);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    MPIDI_CH3I_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);

    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len
     * fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent
     * limitation in the Sock interface?
     */
    do
    {
	MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WRITEV);
	nb = MPL_large_writev(pollinfo->fd, iov, iov_n);
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WRITEV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = (size_t) nb;
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

	mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME,
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
	     * MPIDI_CH3I_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_WRITEV);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_writev() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_wakeup(struct MPIDI_CH3I_Sock_set * sock_set)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_WAKEUP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_WAKEUP);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDI_CH3I_SOCKI_VALIDATE_SOCK_SET(sock_set, mpi_errno, fn_exit);

    /* FIXME: We need (1) a standardized test for including multithreaded
       code and (2) include support for user requests for a lower-level
       of thread safety.  Finally, things like this should probably
       be implemented as an abstraction (e.g., wakeup_progress_threads?)
       rather than this specific code.  */
#ifdef MPICH_IS_THREADED
    MPIR_THREAD_CHECK_BEGIN;
    {
	struct pollinfo * pollinfo;

	pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock_set->intr_sock);
	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WAKEUP, 0, NULL,
				  mpi_errno, mpi_errno, fn_exit);
	MPIDI_CH3I_Socki_wakeup(sock_set);
    }
    MPIR_THREAD_CHECK_END;
#   endif

#ifdef MPICH_IS_THREADED
    fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_WAKEUP);
    return mpi_errno;
}
/* end MPIDI_CH3I_Sock_wakeup() */

/*********** end of sock_immed.i *****************/

/*********** sock_misc.i *****************/

/* This routine is called in mpid/ch3/util/sock/ch3u_connect_sock.c */
/* FIXME: This routine is misnamed; it is really get_interface_name (in the
   case where there are several networks available to the calling process,
   this picks one but even in the current code can pick a different
   interface if a particular environment variable is set) .  */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_get_host_description
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_get_host_description(int myRank,
				    char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_GET_HOST_DESCRIPTION);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    /* --BEGIN ERROR HANDLING-- */
    if (len < 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_LEN,
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
	MPL_snprintf( namebuf, sizeof(namebuf),
		       "MPICH_INTERFACE_HOSTNAME_R_%d", myRank );
	env_hostname = getenv( namebuf );
    }

    if (env_hostname != NULL)
    {
	rc = MPL_strncpy(host_description, env_hostname, (size_t) len);
	/* --BEGIN ERROR HANDLING-- */
	if (rc != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_HOST,
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
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_HOST,
						 "**sock|badhdlen", NULL);
	    }
	    else if (errno == EFAULT)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_HOST,
						 "**sock|badhdbuf", NULL);
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
						 "**sock|oserror", "**sock|poll|oserror %d %s", errno, MPIR_Strerror(errno));
	    }
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_GET_HOST_DESCRIPTION);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_native_to_sock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_native_to_sock(struct MPIDI_CH3I_Sock_set * sock_set, MPIDI_CH3I_SOCK_NATIVE_FD fd, void *user_ptr,
			      struct MPIDI_CH3I_Sock ** sockp)
{
    struct MPIDI_CH3I_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int rc;
    long flags;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /* allocate sock and poll structures */
    mpi_errno = MPIDI_CH3I_Socki_sock_alloc(sock_set, &sock);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    pollfd = MPIDI_CH3I_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDI_CH3I_Socki_sock_get_pollinfo(sock);

    /* set file descriptor to non-blocking */
    flags = fcntl(fd, F_GETFL, 0);
    /* --BEGIN ERROR HANDLING-- */
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    /* --BEGIN ERROR HANDLING-- */
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIR_Strerror(errno));
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* initialize sock and poll structures */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW;

    *sockp = sock;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (sock != NULL)
    {
	MPIDI_CH3I_Socki_sock_free(sock);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_set_user_ptr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_set_user_ptr(struct MPIDI_CH3I_Sock * sock, void * user_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_SET_USER_PTR);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_SET_USER_PTR);

    MPIDI_CH3I_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    if (sock != MPIDI_CH3I_SOCK_INVALID_SOCK &&
	sock->sock_set != MPIDI_CH3I_SOCK_INVALID_SET)
    {
	MPIDI_CH3I_Socki_sock_get_pollinfo(sock)->user_ptr = user_ptr;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
    }
    /* --END ERROR HANDLING-- */

#ifdef USE_SOCK_VERIFY
  fn_exit:
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_SET_USER_PTR);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_get_sock_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_get_sock_id(struct MPIDI_CH3I_Sock * sock)
{
    int id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_ID);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_ID);

    if (sock != MPIDI_CH3I_SOCK_INVALID_SOCK)
    {
	if (sock->sock_set != MPIDI_CH3I_SOCK_INVALID_SET)
	{
	    id = MPIDI_CH3I_Socki_sock_get_pollinfo(sock)->sock_id;
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_ID);
    return id;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_get_sock_set_id(struct MPIDI_CH3I_Sock_set * sock_set)
{
    int id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_SET_ID);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_SET_ID);

    if (sock_set != MPIDI_CH3I_SOCK_INVALID_SET)
    {
	id = sock_set->id;
    }
    else
    {
	id = -1;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_GET_SOCK_SET_ID);
    return id;
}

/* FIXME: This function violates the internationalization design by
   using English language strings rather than the error translation mechanism.
   This unnecessarily breaks the goal of allowing internationalization.
   Read the design documentation and if there is a problem, raise it rather
   than ignoring it.
*/
/* FIXME: It appears that this function was used instead of making use of the
   existing MPI-2 features to extend MPI error classes and code, of to export
   messages to non-MPI application */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Sock_get_error_class_string
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* --BEGIN ERROR HANDLING-- */
int MPIDI_CH3I_Sock_get_error_class_string(int error, char *error_string, size_t length)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_GET_ERROR_CLASS_STRING);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_GET_ERROR_CLASS_STRING);
    switch (MPIR_ERR_GET_CLASS(error))
    {
    case MPIDI_CH3I_SOCK_ERR_FAIL:
	MPL_strncpy(error_string, "generic socket failure", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_INIT:
	MPL_strncpy(error_string, "socket module not initialized", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_NOMEM:
	MPL_strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_SET:
	MPL_strncpy(error_string, "invalid socket set", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_SOCK:
	MPL_strncpy(error_string, "invalid socket", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_HOST:
	MPL_strncpy(error_string, "host description buffer not large enough", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_HOSTNAME:
	MPL_strncpy(error_string, "invalid host name", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_PORT:
	MPL_strncpy(error_string, "invalid port", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_BUF:
	MPL_strncpy(error_string, "invalid buffer", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_BAD_LEN:
	MPL_strncpy(error_string, "invalid length", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_SOCK_CLOSED:
	MPL_strncpy(error_string, "socket closed", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_CONN_CLOSED:
	MPL_strncpy(error_string, "socket connection closed", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_CONN_FAILED:
	MPL_strncpy(error_string, "socket connection failed", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_INPROGRESS:
	MPL_strncpy(error_string, "socket operation in progress", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_TIMEOUT:
	MPL_strncpy(error_string, "socket operation timed out", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_INTR:
	MPL_strncpy(error_string, "socket operation interrupted", length);
	break;
    case MPIDI_CH3I_SOCK_ERR_NO_NEW_SOCK:
	MPL_strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPL_snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_GET_ERROR_CLASS_STRING);
    return MPI_SUCCESS;
}
/* --END ERROR HANDLING-- */

/*********** end of sock_misc.i *****************/

/*********** sock_wait.i *****************/

/* Make sure that we can properly ensure atomic access to the poll routine */
#ifdef MPICH_IS_THREADED
#if !(MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
#error selected multi-threaded implementation is not supported
#endif
#endif


static int MPIDI_CH3I_Socki_handle_pollhup(struct pollfd * const pollfd,
				      struct pollinfo * const pollinfo);
static int MPIDI_CH3I_Socki_handle_pollerr(struct pollfd * const pollfd,
				      struct pollinfo * const pollinfo);
static int MPIDI_CH3I_Socki_handle_read(struct pollfd * const pollfd,
				   struct pollinfo * const pollinfo);
static int MPIDI_CH3I_Socki_handle_write(struct pollfd * const pollfd,
				    struct pollinfo * const pollinfo);
static int MPIDI_CH3I_Socki_handle_connect(struct pollfd * const pollfd,
				      struct pollinfo * const pollinfo);

/*
 * MPIDI_CH3I_Sock_wait()
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
#define FUNCNAME MPIDI_CH3I_Sock_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Sock_wait(struct MPIDI_CH3I_Sock_set * sock_set, int millisecond_timeout,
		    struct MPIDI_CH3I_Sock_event * eventp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCK_WAIT);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POLL);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCK_WAIT);

    for (;;)
    {
	int elem=0;   /* Keep compiler happy */
	int n_fds;
	int n_elems;
	int found_active_elem = FALSE;

	mpi_errno = MPIDI_CH3I_Socki_event_dequeue(sock_set, &elem, eventp);
	if (mpi_errno == MPI_SUCCESS) {
	    struct pollinfo * pollinfo;
	    int flags;

	    if (eventp->op_type != MPIDI_CH3I_SOCK_OP_CLOSE)
	    {
		break;
	    }

	    pollinfo = &sock_set->pollinfos[elem];

	    /*
	     * Attempt to set socket back to blocking.  This *should* prevent
	     * any data in the socket send buffer from being
	     * discarded.  Instead close() will block until the buffer is
	     * flushed or the connection timeouts and is considered
	     * lost.  Theoretically, this could cause the MPIDI_CH3I_Sock_wait() to
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

	    MPIDI_CH3I_Socki_sock_free(pollinfo->sock);

	    break;
	}

	for(;;)
	{
#	    ifndef MPICH_IS_THREADED
	    {
		MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POLL);
		n_fds = poll(sock_set->pollfds, sock_set->poll_array_elems,
			     millisecond_timeout);
		MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POLL);
	    }
#	    else /* MPICH_IS_THREADED */
	    {
		/* If we've enabled runtime checking of the thread level,
		 then test for that and if we are *not* multithreaded,
		 just use the same code as above.  Otherwise, use
		 multithreaded code (and we don't then need the
		 MPIR_THREAD_CHECK_BEGIN/END macros) */
		if (!MPIR_ThreadInfo.isThreaded) {
		    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POLL);
		    n_fds = poll(sock_set->pollfds, sock_set->poll_array_elems,
				 millisecond_timeout);
		    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POLL);
		}
		else
		{
		/*
		 * First try a non-blocking poll to see if any immediate
		 * progress can be made.  This avoids the lock manipulation
		 * overhead.
		 */
		MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POLL);
		n_fds = poll(sock_set->pollfds, sock_set->poll_array_elems, 0);
		MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POLL);

		if (n_fds == 0 && millisecond_timeout != 0)
		{
		    int pollfds_active_elems = sock_set->poll_array_elems;
                    int err;

		    /* The abstraction here is a shared (blocking) resource that
		       the threads must coordinate.  That means not holding
		       a lock across the blocking operation but also
		       ensuring that only one thread at a time attempts
		       to use this resource.

		       What isn't yet clear in this where the test is made
		       to ensure that two threads don't call the poll operation,
		       even in a nonblocking sense.
		    */
		    sock_set->pollfds_active = sock_set->pollfds;

		    /* Release the lock so that other threads may make
		       progress while this thread waits for something to
		       do */
		    MPL_DBG_MSG(MPIR_DBG_OTHER,TYPICAL,"Exit global critical section (sock_wait)");
		    /* 		    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
				    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); */
		    MPID_thread_mutex_state_t state;
		    MPID_THREAD_CS_EXIT_ST(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, state);

		    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POLL);
		    n_fds = poll(sock_set->pollfds_active,
				 pollfds_active_elems, millisecond_timeout);
		    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POLL);

		    /* Reaquire the lock before processing any of the
		       information returned from poll */
		    MPL_DBG_MSG(MPIR_DBG_OTHER,TYPICAL,"Enter global critical section (sock_wait)");
		    /* 		    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
				    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); */
		    MPID_THREAD_CS_ENTER_ST(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, state);

		    /*
		     * Update pollfds array if changes were posted while we
		     * were blocked in poll
		     */
		    if (sock_set->pollfds_updated) {
			mpi_errno = MPIDI_Sock_update_sock_set(
				       sock_set, pollfds_active_elems );
		    }

		    sock_set->pollfds_active = NULL;
		    sock_set->wakeup_posted = FALSE;
		}
		} /* else !MPIR_ThreadInfo.isThreaded */
	    }
#	    endif /* MPICH_IS_THREADED */

	    if (n_fds > 0)
	    {
		break;
	    }
	    else if (n_fds == 0)
	    {
		mpi_errno = MPIDI_CH3I_SOCK_ERR_TIMEOUT;
		goto fn_exit;
	    }
	    else if (errno == EINTR)
	    {
		if (millisecond_timeout != MPIDI_CH3I_SOCK_INFINITE_TIME)
		{
		    mpi_errno = MPIDI_CH3I_SOCK_ERR_TIMEOUT;
		    goto fn_exit;
		}

		continue;
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else if (errno == ENOMEM || errno == EAGAIN)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_NOMEM,
						 "**sock|osnomem", NULL);
		goto fn_exit;
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL,
						 "**sock|oserror", "**sock|poll|oserror %d %s", errno, MPIR_Strerror(errno));
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

	    MPIR_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);
	    MPIR_Assert(pollfd->fd >= 0 || pollfd->fd == -1);

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
		mpi_errno = MPIR_Err_create_code(
		    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|badhandle",
		    "**sock|poll|badhandle %d %d %d %d", pollinfo->sock_set->id, pollinfo->sock_id, pollfd->fd, pollinfo->fd);
		goto fn_exit;
	    }

	    /* --BEGIN ERROR HANDLING-- */
	    if (pollfd->revents & POLLHUP)
	    {
		mpi_errno = MPIDI_CH3I_Socki_handle_pollhup(pollfd, pollinfo);
		if (MPIR_Err_is_fatal(mpi_errno))
		{
		    goto fn_exit;
		}
	    }

	    /* According to Stevens, some errors are reported as normal data
	       (POLLIN) and some are reported with POLLERR. */
	    if (pollfd->revents & POLLERR)
	    {
		mpi_errno = MPIDI_CH3I_Socki_handle_pollerr(pollfd, pollinfo);
		if (MPIR_Err_is_fatal(mpi_errno))
		{
		    goto fn_exit;
		}
	    }
	    /* --END ERROR HANDLING-- */

	    if (pollfd->revents & POLLIN)
	    {
		if (pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)
		{
		    if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW ||
			pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO)
		    {
			mpi_errno = MPIDI_CH3I_Socki_handle_read(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    /* --BEGIN ERROR HANDLING-- */
		    else
		    {
			mpi_errno = MPIR_Err_create_code(
			    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
			    "**sock|poll|unhandledstate %d", pollinfo->state);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */

		}
		else if (pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_LISTENER)
		{
		    pollfd->events &= ~POLLIN;
		    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_ACCEPT, 0, pollinfo->user_ptr,
					      MPI_SUCCESS, mpi_errno, fn_exit);
		}
	else if ((MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE) && pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_INTERRUPTER)
		{
		    char c[16];
		    ssize_t nb;

		    do
		    {
			nb = read(pollfd->fd, c, 16);
		    }
		    while (nb > 0 || (nb < 0 && errno == EINTR));
		}
		/* --BEGIN ERROR HANDLING-- */
		else
		{
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
			"**sock|poll|unhandledtype %d", pollinfo->type);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->type == MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)
		{
		    if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW)
		    {
			mpi_errno = MPIDI_CH3I_Socki_handle_write(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING)
		    {
			mpi_errno = MPIDI_CH3I_Socki_handle_connect(pollfd, pollinfo);
			/* --BEGIN ERROR HANDLING-- */
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
			/* --END ERROR HANDLING-- */
		    }
		    /* --BEGIN ERROR HANDLING-- */
		    else
		    {
			mpi_errno = MPIR_Err_create_code(
			    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
			    "**sock|poll|unhandledstate %d", pollinfo->state);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */
		}
		/* --BEGIN ERROR HANDLING-- */
		else
		{
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
			"**sock|poll|unhandledtype %d", pollinfo->type);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCK_WAIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_handle_pollhup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_handle_pollhup(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLHUP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLHUP);

    if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW)
    {
	/*
	 * If a write was posted then cancel it and generate an connection closed event.  If a read is posted, it will be handled
	 * by the POLLIN handler.
	 */
	/* --BEGIN ERROR HANDLING-- */
	if (pollfd->events & POLLOUT)
	{
	    int event_mpi_errno;

	    event_mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_CLOSED,
		"**sock|connclosed", "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	}
	/* --END ERROR HANDLING-- */
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLHUP event.
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIR_Assert(pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_DISCONNECTED)
    {
	/*
	 * We should never reach this state because pollfd->fd should be set to -1 if we are in the disconnected state.
	 */
	MPIR_Assert(pollinfo->state == MPIDI_CH3I_SOCKI_STATE_DISCONNECTED && pollfd->fd == -1);
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIR_Assert(pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
	    "**sock|poll|unhandledstate %d", pollinfo->state);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLHUP);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_handle_pollhup() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_handle_pollerr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_handle_pollerr(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLERR);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLERR);

    /* --BEGIN ERROR HANDLING-- */
    if (pollinfo->type != MPIDI_CH3I_SOCKI_TYPE_COMMUNICATION)
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
	    "**sock|poll|unhandledtype %d", pollinfo->type);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW)
    {
	/*
	 * Stevens suggests that some older version of UNIX did not properly reset so_error, which could allow POLLERR to be
	 * continuously triggered.  We remove the socket from the poll list (pollfd->fd = 1) in order to prevent this issue.
	 * Here, we simple check that things are as we expect them to be.
	 */
	MPIR_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);

	/* If a write was posted then cancel it and generate an write completion event */
	if (pollfd->events & POLLOUT)
	{
	    int disconnected;
	    int os_errno;
	    int event_mpi_errno;

	    MPIDI_CH3I_SOCKI_GET_SOCKET_ERROR(pollinfo, os_errno, mpi_errno, fn_exit);

	    event_mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, os_errno, FCNAME, __LINE__, &disconnected);
	    /* --BEGIN ERROR HANDLING-- */
	    if (MPIR_Err_is_fatal(event_mpi_errno))
	    {
		mpi_errno = event_mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	}
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLERR event.
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIR_Assert(pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIR_Assert(pollinfo->state == MPIDI_CH3I_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    else if (pollinfo->state == MPIDI_CH3I_SOCKI_STATE_DISCONNECTED)
    {
	/* We are already disconnected!  Why are we handling an error? */
	MPIR_Assert(pollfd->fd == -1);
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
	    "**sock|poll|unhandledstate %d", pollinfo->state);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_POLLERR);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_handle_pollerr() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_handle_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_READ);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_READV);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_READ);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_READ);

    do
    {
	if (pollinfo->read_iov_flag)
	{
	    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_READV);
	    nb = MPL_large_readv(pollinfo->fd, pollinfo->read.iov.ptr + pollinfo->read.iov.offset,
		       pollinfo->read.iov.count - pollinfo->read.iov.offset);
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_READV);
	}
	else
	{
	    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_READ);
	    nb = read(pollinfo->fd, pollinfo->read.buf.ptr + pollinfo->read_nb,
		      pollinfo->read.buf.max - pollinfo->read_nb);
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_READ);
	}
    }
    while (nb < 0 && errno == EINTR);

    if (nb > 0)
    {
	int done;

	pollinfo->read_nb += nb;

	done = pollinfo->read_iov_flag ?
	    MPIDI_CH3I_Socki_adjust_iov(nb, pollinfo->read.iov.ptr, pollinfo->read.iov.count, &pollinfo->read.iov.offset) :
	    (pollinfo->read_nb >= pollinfo->read.buf.min);

	if (done)
	{
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
	}
    }
    /* --BEGIN ERROR HANDLING-- */
    else if (nb == 0)
    {
	int event_mpi_errno;

	event_mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",
	    "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	}
	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);

	MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;

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
	int event_mpi_errno;

	event_mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(event_mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data
	     * structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    mpi_errno = event_mpi_errno;
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (MPIDI_CH3I_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    {
		MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  event_mpi_errno, mpi_errno, fn_exit);
		MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }

	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
	}

	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);
	MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_READ);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_handle_read() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_handle_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WRITE);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_WRITEV);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_WRITE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_WRITE);

    do
    {
	if (pollinfo->write_iov_flag)
	{
	    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WRITEV);
	    nb = MPL_large_writev(pollinfo->fd, pollinfo->write.iov.ptr + pollinfo->write.iov.offset,
			pollinfo->write.iov.count - pollinfo->write.iov.offset);
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WRITEV);
	}
	else
	{
	    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_WRITE);
	    nb = write(pollinfo->fd, pollinfo->write.buf.ptr + pollinfo->write_nb,
		       pollinfo->write.buf.max - pollinfo->write_nb);
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_WRITE);
	}
    }
    while (nb < 0 && errno == EINTR);

    if (nb >= 0)
    {
	int done;

	pollinfo->write_nb += nb;

	done = pollinfo->write_iov_flag ?
	    MPIDI_CH3I_Socki_adjust_iov(nb, pollinfo->write.iov.ptr, pollinfo->write.iov.count, &pollinfo->write.iov.offset) :
	    (pollinfo->write_nb >= pollinfo->write.buf.min);

	if (done)
	{
	    MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, mpi_errno, fn_exit);
	    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
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
	int event_mpi_errno;

	event_mpi_errno = MPIDI_CH3I_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(event_mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    mpi_errno = event_mpi_errno;
	    goto fn_exit;
	}

	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);
	MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	if (disconnected)
	{
	    /*
	     * The connection is dead but data may still be in the socket buffer; thus, we change the state and let
	     * MPIDI_CH3I_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RO;
	}
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_WRITE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_handle_write() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Socki_handle_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    MPL_sockaddr_t addr;
    socklen_t addr_len;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_CONNECT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_CONNECT);

    addr_len = sizeof(addr);
    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == 0)
    {
	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, MPI_SUCCESS, mpi_errno, fn_exit);
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_CONNECTED_RW;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	int event_mpi_errno;

	MPIDI_CH3I_SOCKI_GET_SOCKET_ERROR(pollinfo, pollinfo->os_errno, mpi_errno, fn_exit);
	event_mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDI_CH3I_SOCK_ERR_CONN_FAILED, "**sock|connfailed",
	    "**sock|poll|connfailed %d %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->os_errno,
	    MPIR_Strerror(pollinfo->os_errno));
	MPIDI_CH3I_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDI_CH3I_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, event_mpi_errno, mpi_errno, fn_exit);
	pollinfo->state = MPIDI_CH3I_SOCKI_STATE_DISCONNECTED;
    }
    /* --END ERROR HANDLING-- */

    MPIDI_CH3I_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SOCKI_HANDLE_CONNECT);
    return mpi_errno;
}
/* end MPIDI_CH3I_Socki_handle_connect() */

/*********** end of sock_wait.i *****************/
