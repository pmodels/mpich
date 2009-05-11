/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd_util_sock.h"
/*#include "mpiimpl.h"*/
#ifdef HAVE_STRING_H
/* Include for memcpy and memset */
#include <string.h>
#endif

#include "mpishared.h"

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
#include "smpd.h"


/* FIXME: What do these mean?  Why is 32 a good size (e.g., is it because
   32*32 = 1024 if these are bits in a 4-byte int?  In that case, should
   these be related to a maximum processor count or an OS-defined fd limit? */
#if !defined(SMPDU_SOCK_SET_DEFAULT_SIZE)
#define SMPDU_SOCK_SET_DEFAULT_SIZE 32
#endif

#if !defined(SMPDU_SOCK_EVENTQ_POOL_SIZE)
#define SMPDU_SOCK_EVENTQ_POOL_SIZE 32
#endif


enum SMPDU_Socki_state
{
    SMPDU_SOCKI_STATE_FIRST = 0,
    SMPDU_SOCKI_STATE_CONNECTING,
    SMPDU_SOCKI_STATE_CONNECTED_RW,
    SMPDU_SOCKI_STATE_CONNECTED_RO,
    SMPDU_SOCKI_STATE_DISCONNECTED,
    SMPDU_SOCKI_STATE_CLOSING,
    SMPDU_SOCKI_STATE_LAST
};

enum SMPDU_Socki_type
{
    SMPDU_SOCKI_TYPE_FIRST = 0,
    SMPDU_SOCKI_TYPE_COMMUNICATION,
    SMPDU_SOCKI_TYPE_LISTENER,
    SMPDU_SOCKI_TYPE_INTERRUPTER,
    SMPDU_SOCKI_TYPE_LAST
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
 *        close is completed by SMPDIU_Sock_wait()
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
    struct SMPDU_Sock_set * sock_set;
    int elem;
    struct SMPDU_Sock * sock;
    int fd;
    void * user_ptr;
    enum SMPDU_Socki_type type;
    enum SMPDU_Socki_state state;
    int os_errno;
    union
    {
	struct
	{
	    SMPD_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    SMPDU_Sock_size_t min;
	    SMPDU_Sock_size_t max;
	} buf;
    } read;
    int read_iov_flag;
    SMPDU_Sock_size_t read_nb;
    SMPDU_Sock_progress_update_func_t read_progress_update_fn;
    union
    {
	struct
	{
	    SMPD_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    SMPDU_Sock_size_t min;
	    SMPDU_Sock_size_t max;
	} buf;
    } write;
    int write_iov_flag;
    SMPDU_Sock_size_t write_nb;
    SMPDU_Sock_progress_update_func_t write_progress_update_fn;
};

struct SMPDU_Socki_eventq_elem
{
    struct SMPDU_Sock_event event;
    int set_elem;
    struct SMPDU_Socki_eventq_elem * next;
};

struct SMPDU_Sock_set
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
    struct SMPDU_Socki_eventq_elem * eventq_head;
    struct SMPDU_Socki_eventq_elem * eventq_tail;
};

struct SMPDU_Sock
{
    struct SMPDU_Sock_set * sock_set;
    int elem;
};

/* FIXME: Why aren't these static */
int SMPDU_Socki_initialized = 0;

static struct SMPDU_Socki_eventq_elem * SMPDU_Socki_eventq_pool = NULL;

/* MT: needs to be atomically incremented */
static int SMPDU_Socki_set_next_id = 0;

/* Prototypes for functions used only within the socket code. */

/* Set the buffer size on the socket fd from the environment variable
   or other option; if "firm" is true, fail if the buffer size is not
   successfully set */
int SMPDU_Sock_SetSockBufferSize( int fd, int firm );
/* Get a string version of the address in ifaddr*/
int SMPDU_Sock_AddrToStr( SMPDU_Sock_ifaddr_t *ifaddr, char *str, int maxlen );

/* FIXME: Why are these files included in this way?  Why not make them either
   separate files or simply part of (one admittedly large) source file? */
#include "smpd_socki_util.i"

#include "smpd_sock_init.i"
#include "smpd_sock_set.i"
#include "smpd_sock_post.i"
#include "smpd_sock_immed.i"
#include "smpd_sock_misc.i"
#include "smpd_sock_wait.i"
