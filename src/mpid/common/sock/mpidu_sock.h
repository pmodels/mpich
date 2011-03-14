/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIDU_SOCK_H_INCLUDED)
#define MPIDU_SOCK_H_INCLUDED

#if defined(__cplusplus)
#if !defined(CPLUSPLUS_BEGIN)
#define CPLUSPLUS_BEGIN extern "C" {
#define CPLUSPLUS_END }
#endif
#else
#define CPLUSPLUS_BEGIN
#define CPLUSPLUS_END
#endif

CPLUSPLUS_BEGIN

/* config header file */
#include "mpidu_sock_conf.h"
/* Load just the utility definitions that we need */
#include "mpichconf.h"
#include "mpibase.h"
#include "mpiutil.h"
#include "mpitypedefs.h"
#include "mpich_param_vals.h"
/* implementation specific header file */    
#include "mpidu_socki.h"


/*D
MPIDU_SOCK_ERR - Extended error classes specific to the Sock module

Notes:
The actual meaning of these error classes is defined by each function.  

Module:
Utility-Sock
D*/
/* FIXME: This is not the right way to add error values to an MPICH module.
   Note that (a) the last class values are not respected by the error handling
   code, (b) the entire point of codes and classes is to provide a 
   natural grouping of codes to a class, (c) this approach can only be used 
   by one module and hence breaks any component design, and (d) this is 
   what the MPI dynamic error codes and classes was designed for. */
#define MPIDU_SOCK_SUCCESS		MPI_SUCCESS
#define MPIDU_SOCK_ERR_FAIL		MPICH_ERR_LAST_CLASS + 1
#define MPIDU_SOCK_ERR_INIT		MPICH_ERR_LAST_CLASS + 2
#define MPIDU_SOCK_ERR_NOMEM		MPICH_ERR_LAST_CLASS + 3
#define MPIDU_SOCK_ERR_BAD_SET		MPICH_ERR_LAST_CLASS + 4
#define MPIDU_SOCK_ERR_BAD_SOCK		MPICH_ERR_LAST_CLASS + 5
#define MPIDU_SOCK_ERR_BAD_HOST		MPICH_ERR_LAST_CLASS + 6
#define MPIDU_SOCK_ERR_BAD_HOSTNAME     MPICH_ERR_LAST_CLASS + 7
#define MPIDU_SOCK_ERR_BAD_PORT		MPICH_ERR_LAST_CLASS + 8
#define MPIDU_SOCK_ERR_BAD_BUF		MPICH_ERR_LAST_CLASS + 9
#define MPIDU_SOCK_ERR_BAD_LEN		MPICH_ERR_LAST_CLASS + 10
#define MPIDU_SOCK_ERR_SOCK_CLOSED	MPICH_ERR_LAST_CLASS + 11
#define MPIDU_SOCK_ERR_CONN_CLOSED	MPICH_ERR_LAST_CLASS + 12
#define MPIDU_SOCK_ERR_CONN_FAILED	MPICH_ERR_LAST_CLASS + 13
#define MPIDU_SOCK_ERR_INPROGRESS	MPICH_ERR_LAST_CLASS + 14
#define MPIDU_SOCK_ERR_TIMEOUT		MPICH_ERR_LAST_CLASS + 15
#define MPIDU_SOCK_ERR_INTR		MPICH_ERR_LAST_CLASS + 16
#define MPIDU_SOCK_ERR_NO_NEW_SOCK	MPICH_ERR_LAST_CLASS + 17


/*E
MPIDU_Sock_op_t - enumeration of posted operations that can be completed by the Sock module

Notes:
MPIDU_SOCK_OP_ACCEPT is different that the other operations.  When returned by MPIDU_Sock_wait(), operations other than
MPIDU_SOCK_OP_ACCEPT mark the completion of a previously posted operation.  MPIDU_SOCK_OP_ACCEPT indicates that a new connection is
being formed and that MPIDU_Sock_accept() should be called.

Module:
Utility-Sock
E*/
typedef enum MPIDU_Sock_op
{
    MPIDU_SOCK_OP_READ,
    MPIDU_SOCK_OP_WRITE,
    MPIDU_SOCK_OP_ACCEPT,
    MPIDU_SOCK_OP_CONNECT,
    MPIDU_SOCK_OP_CLOSE,
    MPIDU_SOCK_OP_WAKEUP
} MPIDU_Sock_op_t;


/*S
MPIDU_Sock_event_t - event structure returned by MPIDU_Sock_wait() describing the operation that completed

Fields:
+ op_type - type of operation that completed
. num_bytes - number of bytes transferred (if appropriate)
. user_ptr - user pointer associated with the sock on which this operation completed
- error - a MPI error code with a Sock extended error class

Notes:
The num_bytes field is only used when a posted read or write operation completes.

Module:
Utility-Sock
S*/
typedef struct MPIDU_Sock_event
{
    MPIDU_Sock_op_t op_type;
    MPIU_Size_t num_bytes;
    void * user_ptr;
    int error;
} MPIDU_Sock_event_t;


/*@
MPIDU_Sock_init - initialize the Sock communication library

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - initialization completed successfully
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure; initialization failed

Notes:
The Sock module may be initialized multiple times.  The implementation should perform reference counting if necessary.

Module:
Utility-Sock
@*/
int MPIDU_Sock_init(void);


/*@
MPIDU_Sock_finalize - shutdown the Sock communication library

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - shutdown completed successfully
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure; shutdown failed

Notes:
<BRT> What are the semantics of finalize?  Is it responsible for releasing any resources (socks and sock sets) that the calling
code(s) leaked?  Should it block until all OS resources are released?

Module:
Utility-Sock
@*/
int MPIDU_Sock_finalize(void);


/*@
MPIDU_Sock_get_host_description - obtain a description of the host's 
communication capabilities

Input Parameters:
+ myRank - Rank of this process in its MPI_COMM_WORLD.  This can be used
  to find environment variables that are specific for this process.
. host_description - character array in which the function can store a string 
  describing the communication capabilities of the host
- len - length of the character array

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - description successfully obtained and placed in host_description
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_LEN - len parameter is less than zero
. MPIDU_SOCK_ERR_BAD_HOST - host_description parameter not big enough to 
  store required information
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - unable to obtain network interface information from OS

Notes:
The host description string returned by the function is defined by the 
implementation and should not be interpreted by the
application.  This string is to be supplied to MPIDU_Sock_post_connect() when 
one wishes to form a connection with this host.

Module:
Utility-Sock
@*/
int MPIDU_Sock_get_host_description(int myRank, char * host_description, int len);


/*@
MPIDU_Sock_hostname_to_host_description - convert a host name to a description of the host's communication capabilities

Input Parameters:
+ hostname - host name string
. host_description - character array in which the function can store a string describing the communication capabilities of the host
- len - length of host_description

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - description successfully obtained and placed in host_description
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_LEN - len parameter is less than zero
. MPIDU_SOCK_ERR_BAD_HOSTNAME - hostname parameter not valid
. MPIDU_SOCK_ERR_BAD_HOST - host_description parameter not big enough to store required information
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - unable to obtain network interface information from OS

Notes:
The host description string returned by the function is defined by the implementation and should not be interpreted by the
application.  This string is to be supplied to MPIDU_Sock_post_connect() when one wishes to form a connection with the host
specified by hostname.

Module:
Utility-Sock
@*/
int MPIDU_Sock_hostname_to_host_description(char *hostname, char * host_description, int len);

/*@
MPIDU_Sock_create_set - create a new sock set object

Output Parameter:
. set - pointer to the new sock set object

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - new sock set successfully create
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - pointer to the sock set object is bad
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure

Notes:
A sock set contains zero or more sock objects.  Each sock object belongs to a single sock set and is bound to that set for the life
of that object.

Module:
Utility-Sock
@*/
int MPIDU_Sock_create_set(MPIDU_Sock_set_t * set);


/*@
MPIDU_Sock_destroy_set - destroy an existing sock set, releasing an internal resource associated with that set

Input Parameter:
. set - set to be destroyed

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - sock set successfully destroyed
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - unable to destroy the sock set (<BRT> because it still contained active sock objects?)

Notes:
<BRT> What are the semantics for destroying a sock set that still contains active sock objects?  sock objects by definition
cannot exist outside of a set.

It is consider erroneous to destroy a set that still contains sock objects or is being operated upon with an of the Sock routines.

Module:
Utility-Sock
@*/
int MPIDU_Sock_destroy_set(MPIDU_Sock_set_t set);


/*@
MPIDU_Sock_native_to_sock - convert a native file descriptor/handle to a sock object

Input Parameters:
+ set - sock set to which the new sock should be added
. fd - native file descriptor
- user_ptr - user pointer to be associated with the new sock

Output Parameter:
. sock - new sock object

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - sock successfully created
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set
. MPIDU_SOCK_ERR_BAD_NATIVE_FD - invalid native file descriptor
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure; listener sock could not be created

Notes:
The constraints on which file descriptors/handles may be converted to a sock object are defined by the implementation.  The
implementation may return MPIDU_SOCK_ERR_BAD_NATIVE_FD if the descriptor/handle cannot be used with the implementation.  It is
possible, however, that the conversion of an inappropriate descriptor/handle may complete successfully but the sock object may not
function properly.

Thread safety:
The addition of a new sock object to the sock set may occur while other threads are performing operations on the same sock set.
Thread safety of simultaneously operations on the same sock set must be guaranteed by the Sock implementation.
  
@*/
int MPIDU_Sock_native_to_sock(MPIDU_Sock_set_t set, MPIDU_SOCK_NATIVE_FD fd, void * user_ptr, MPIDU_Sock_t * sock);


/*@
MPIDU_Sock_listen - establish a listener sock

Input Parameters:
+ set - sock set to which the listening sock should be added
. user_ptr - user pointer associated with the new sock object
- port - desired port (or zero if a specific port is not desired)

Output Parameters:
+ port - port assigned to the listener
- sock - new listener sock

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - listener sock successfully established
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set
. MPIDU_SOCK_ERR_BAD_PORT - port number is out of range or pointer to port parameter is invalid
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure; listener sock could not be created

Events generated:
. MPIDU_SOCK_OP_ACCEPT - each time a new connection is being formed and needs to be accepted (with MPIDU_Sock_accept())

Event errors:
+ MPI_SUCCESS - new sock waiting to be accepted
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure?

Notes:
While not a post routine, this routine can generate events.  In fact,
unlike the post routine, many MPIDU_SOCK_OP_ACCEPT events can 
be generated from a listener (typically one per incoming connection attempt).

The implementation may generate an event as soon it is notified that a
new connection is forming.  In such an implementation, 
MPIDU_Sock_accept() may be responsible for finalizing the connection.
It is also possible that the connection may fail to 
complete, causing MPIDU_Sock_accept() to be unable to obtain a sock
despite the event notification. 

The environment variable MPICH_PORT_RANGE=min:max may be used to
restrict the ports mpich processes listen on. 

Thread safety:
The addition of the listener sock object to the sock set may occur
while other threads are performing operations on the same sock 
set.  Thread safety of simultaneously operations on the same sock set
must be guaranteed by the Sock implementation. 

Module:
Utility-Sock
@*/
int MPIDU_Sock_listen(MPIDU_Sock_set_t set, void * user_ptr, int * port, MPIDU_Sock_t * sock);


/*@
MPIDU_Sock_accept - obtain the sock object associated with a new connection

Input Parameters:
+ listener_sock - listener sock object from which to obtain the new connection
. set - sock set to which the new sock object should be added
- user_ptr - user pointer associated with the new sock object

Output Parameter:
. sock - sock object for the new connection

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - new connection successfully established and associated with new sock objecta
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_NO_NEW_SOCK - no new connection was available
. MPIDU_SOCK_ERR_BAD_SOCK - invalid listener sock or bad pointer to new sock object
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - failed to acquire a new connection

Notes:
In the event of a connection failure, MPIDU_Sock_accept() may fail to acquire and return a new sock despite any
MPIDU_SOCK_OP_ACCEPT event notification.  On the other hand, MPIDU_Sock_accept() may return a sock for which the underlying
connection has already failed.  (The Sock implementation may be unaware of the failure until read/write operations are performed.)

Thread safety:
The addition of the new sock object to the sock set may occur while other threads are performing operations on the same sock set.
Thread safety of simultaneously operations on the same sock set must be guaranteed by the Sock implementation.

MPIDU_Sock_accept() may fail to return a new sock if multiple threads call MPIDU_Sock_accept() and queue of new connections is
depleted.  In this case, MPIDU_SOCK_ERR_NO_SOCK is returned.

Module:
Utility-Sock
@*/
int MPIDU_Sock_accept(MPIDU_Sock_t listener_sock, MPIDU_Sock_set_t set, void * user_ptr, MPIDU_Sock_t * sock);


/*@
MPIDU_Sock_post_connect - request that a new connection be formed

Input Parameters:
+ set - sock set to which the new sock object should be added
. user_ptr - user pointer associated with the new sock object
. host_description - string containing the communication capabilities of the listening host
+ port - port number of listener sock on the listening host

Output Parameter:
. sock - new sock object associated with the connection request

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to form new connection successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set
. MPIDU_SOCK_ERR_BAD_HOST - host description string is not valid
. MPIDU_SOCK_ERR_BAD_PORT - port number is out of range
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure attempting to post connection request

Events generated:
. MPIDU_SOCK_OP_CONNECT

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_CONN_FAILED - failed to connect to the remote host
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error?

<BRT> Any other event errors?  Does the sock channel require finer granularity?

Notes:
The host description of the listening host is supplied MPIDU_Sock_get_host_description().  The intention is that the description
contain an enumeration of interface information so that the MPIDU_Sock_connect() can try each of the interfaces until it succeeds
in forming a connection.  Having a complete set of interface information also allows a particular interface be used selected by the
user at runtime using the MPICH_NETMASK.  <BRT> The name of the environment variable seems wrong.  Perhaps MPICH_INTERFACE?  We
should ask the Systems group.

Thread safety:
The addition of the new sock object to the sock set may occur while other threads are performing operations on the same sock set.
Thread safety of simultaneously operations on the same sock set must be guaranteed by the Sock implementation.

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_connect(MPIDU_Sock_set_t set, void * user_ptr, char * host_description, int port, MPIDU_Sock_t * sock);

/*S
  MPIDU_Sock_ifaddr_t - Structure to hold an Internet address.

+ len - Length of the address.  4 for IPv4, 16 for IPv6.
- ifaddr - Address bytes (as bytes, not characters)

S*/
typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;

/*@ MPIDU_Sock_post_connect_ifaddr - Post a connection given an interface
  address (bytes, not string).

  This is the basic routine.  MPIDU_Sock_post_connect converts the
  host description into the ifaddr and calls this routine.
  @*/
int MPIDU_Sock_post_connect_ifaddr( MPIDU_Sock_set_t sock_set, 
				    void * user_ptr, 
				    MPIDU_Sock_ifaddr_t *ifaddr, int port,
				    MPIDU_Sock_t * sockp);


/*@
MPIDU_Sock_set_user_ptr - change the user pointer associated with a sock object

Input Parameters:
+ sock - sock object
- user_ptr - new user pointer

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - user pointer successfully updated
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
- MPIDU_SOCK_ERR_FAIL - other failure?

Module:
Utility-Sock
@*/
int MPIDU_Sock_set_user_ptr(MPIDU_Sock_t sock, void * user_ptr);


/*@
MPIDU_Sock_post_close - request that an existing connection be closed

Input Parameter:
. sock - sock object to be closed

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to close the connection was successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object or close already posted
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other failure?

Events generated:
+ MPIDU_SOCK_OP_CLOSE
. MPIDU_SOCK_OP_READ
- MPIDU_SOCK_OP_WRITE

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - an error occurred closing the sock; sock object is still destroyed

Notes:
If any other operations are posted on the specified sock, they will be terminated.  An appropriate event will be generated for each
terminated operation.  All such events will be delivered by MPIDU_Sock_wait() prior to the delivery of the MPIDU_SOCK_OP_CLOSE
event.

The sock object is destroyed just prior to the MPIDU_SOCK_OP_CLOSE event being returned by MPIDU_Sock_wait().  Any oustanding
references to the sock object held by the application should be considered invalid and not used again.

Thread safety:
MPIDU_Sock_post_close() may be called while another thread is calling or blocking in MPIDU_Sock_wait() specifying the same sock set
to which this sock belongs.  If another thread is blocking MPIDU_Sock_wait() and the close operation causes the sock set to become
empty, then MPIDU_Sock_wait() will return with an error.

Calling any of the immediate or post routines during or after the call to MPIDU_Sock_post_close() is consider an application error.
The result of doing so is undefined.  The application should coordinate the closing of a sock with the activities of other threads
to ensure that simultaneous calls do not occur.

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_close(MPIDU_Sock_t sock);


/*E
MPIDU_Sock_progress_update_func_t - progress update callback functions

If a pointer to a function of this type is passed to one of the post read or write functions, the implementation must honor the
following rules:

1) The sock progress engine will call this function when partial data has been read or written for the posted operation.

2) All progress_update calls must complete before completion notification is signalled.  In other words, MPIDU_Sock_wait() will not
return until all progress_update calls have completed.

Notes:
<BRT> We need to define ordering and delivery from multiple threads.  Do the handlers have to be thread safe?  If so, then the
internal progress engine could block on an application routine.

Module:
Utility-Sock
E*/
typedef int (* MPIDU_Sock_progress_update_func_t)(MPIU_Size_t num_bytes, void * user_ptr);


/*@
MPIDU_Sock_post_read - request that data be read from a sock

Input Parameters:
+ sock - sock object from which data is to be read
. buf - buffer into which the data should be placed
. minlen - minimum number of bytes to read
. maxlen - maximum number of bytes to read
+ upate_fn - application progress update function (may be NULL)

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to read was successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_LEN - length parameters must be greater than zero and maxlen must be greater than minlen
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
- MPIDU_SOCK_ERR_FAIL - other error attempting to post read

Events generated:
. MPIDU_SOCK_OP_READ

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by buf and maxlen resulted in a memory fault
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error completing the read

Notes:
Only one read operation may be posted at a time.  Furthermore, an immediate read may not be performed while a posted write is
outstanding.  This is considered to be an application error, and the results of doing so are undefined.  The implementation may
choose to catch the error and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

If MPIDU_Sock_post_close() is called before the posted read operation completes, the read operation will be terminated and a
MPIDU_SOCK_OP_READ event containing a MPIDU_SOCK_ERR_SOCK_CLOSED error will be generated.  This event will be returned by
MPIDU_Sock_wait() prior to the MPIDU_SOCK_OP_CLOSE event.

Thread safety:
MPIDU_Sock_post_read() may be called while another thread is attempting to perform an immediate write or post a write operation on
the same sock.  MPIDU_Sock_post_read() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the
same sock set to which the specified sock belongs.

MPIDU_Sock_post_write() may not be called while another thread is performing an immediate read on the same sock.  This is
considered to be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error
and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_post_read() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result of
doing so is undefined.  The application should coordinate the closing of a sock with the activities of other threads to ensure that
one thread is not attempting to post a new operation while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t minbr, MPIU_Size_t maxbr,
                         MPIDU_Sock_progress_update_func_t fn);


/*@
MPIDU_Sock_post_readv - request that a vector of data be read from a sock

Input Parameters:
+ sock - sock object from which the data is to read
. iov - I/O vector describing buffers into which the data is placed
. iov_n - number of elements in I/O vector (must be less than MPID_IOV_LIMIT)
+ upate_fn - application progress update function (may be NULL)

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to read was successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_LEN - iov_n is out of range
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
- MPIDU_SOCK_ERR_FAIL - other error attempting to post read

Events generated:
. MPIDU_SOCK_OP_READ

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by iov and iov_n resulted in a memory fault
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error completing the read

Notes:
Only one read operation may be posted at a time.  Furthermore, an immediate read may not be performed while a posted write is
outstanding.  This is considered to be an application error, and the results of doing so are undefined.  The implementation may
choose to catch the error and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

If MPIDU_Sock_post_close() is called before the posted read operation completes, the read operation will be terminated and a
MPIDU_SOCK_OP_READ event containing a MPIDU_SOCK_ERR_SOCK_CLOSED error will be generated.  This event will be returned by
MPIDU_Sock_wait() prior to the MPIDU_SOCK_OP_CLOSE event.

Thread safety:
MPIDU_Sock_post_readv() may be called while another thread is attempting to perform an immediate write or post a write operation on
the same sock.  MPIDU_Sock_post_readv() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the
same sock set to which the specified sock belongs.

MPIDU_Sock_post_readv() may not be called while another thread is performing an immediate read on the same sock.  This is
considered to be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error
and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_post_readv() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result
of doing so is undefined.  The application should coordinate the closing of a sock with the activities of other threads to ensure
that one thread is not attempting to post a new operation while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn);


/*@
MPIDU_Sock_post_write - request that data be written to a sock

Input Parameters:
+ sock - sock object which the data is to be written
. buf - buffer containing the data
. minlen - minimum number of bytes to write
. maxlen - maximum number of bytes to write
+ upate_fn - application progress aupdate function (may be NULL)

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to write was successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_LEN - length parameters must be greater than zero and maxlen must be greater than minlen
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
- MPIDU_SOCK_ERR_FAIL - other error attempting to post write

Events generated:
. MPIDU_SOCK_OP_WRITE

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by buf and maxlen resulted in a memory fault
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error completing the write

Notes:
Only one write operation may be posted at a time.  Furthermore, an immediate write may not be performed while a posted write is
outstanding.  This is considered to be an application error, and the results of doing so are undefined.  The implementation may
choose to catch the error and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

If MPIDU_Sock_post_close() is called before the posted write operation completes, the write operation will be terminated and a
MPIDU_SOCK_OP_WRITE event containing a MPIDU_SOCK_ERR_SOCK_CLOSED error will be generated.  This event will be returned by
MPIDU_Sock_wait() prior to the MPIDU_SOCK_OP_CLOSE event.

Thread safety:
MPIDU_Sock_post_write() may be called while another thread is attempting to perform an immediate read or post a read operation on
the same sock.  MPIDU_Sock_post_write() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the
same sock set to which the specified sock belongs.

MPIDU_Sock_post_write() may not be called while another thread is performing an immediate write on the same sock.  This is
considered to be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error
and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_post_write() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result
of doing so is undefined.  The application should coordinate the closing of a sock with the activities of other threads to ensure
that one thread is not attempting to post a new operation while another thread is attempting to close the sock.  <BRT> Do we really
need this flexibility?

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t min, MPIU_Size_t max,
			  MPIDU_Sock_progress_update_func_t fn);


/*@
MPIDU_Sock_post_writev - request that a vector of data be written to a sock

Input Parameters:
+ sock - sock object which the data is to be written
. iov - I/O vector describing buffers of data to be written
. iov_n - number of elements in I/O vector (must be less than MPID_IOV_LIMIT)
+ upate_fn - application progress update function (may be NULL)

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - request to write was successfully posted
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_LEN - iov_n is out of range
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
- MPIDU_SOCK_ERR_FAIL - other error attempting to post write

Events generated:
. MPIDU_SOCK_OP_WRITE

Event errors: a MPI error code with a Sock extended error class
+ MPI_SUCCESS -  successfully established
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by iov and iov_n resulted in a memory fault
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error completing the write

Notes:
Only one write operation may be posted at a time.  Furthermore, an immediate write may not be performed while a posted write is
outstanding.  This is considered to be an application error, and the results of doing so are undefined.  The implementation may
choose to catch the error and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

If MPIDU_Sock_post_close() is called before the posted write operation completes, the write operation will be terminated and a
MPIDU_SOCK_OP_WRITE event containing a MPIDU_SOCK_ERR_SOCK_CLOSED error will be generated.  This event will be returned by
MPIDU_Sock_wait() prior to the MPIDU_SOCK_OP_CLOSE event.

Thread safety:
MPIDU_Sock_post_writev() may be called while another thread is attempting to perform an immediate read or post a read operation on
the same sock.  MPIDU_Sock_post_writev() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the
same sock set to which the specified sock belongs.

MPIDU_Sock_post_writev() may not be called while another thread is performing an immediate write on the same sock.  This is
considered to be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error
and return MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_post_writev() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result
of doing so is undefined.  The application should coordinate the closing of a sock with the activities of other threads to ensure
that one thread is not attempting to post a new operation while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_post_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn);


/*@
MPIDU_Sock_wait - wait for an event

Input Parameters:
+ set - sock set upon which to wait for an event
- timeout - timeout in milliseconds (<0 for infinity)

Output Parameter:
. event - pointer to the event structure to be populated

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - a new event was returned
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set object
. MPIDU_SOCK_ERR_TIMEOUT - a timeout occurred
. MPIDU_SOCK_ERR_INTR - the routine was interrupted by a call to MPIDU_Sock_wakeup()
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error (are there any?)

Notes:
MPIDU_Sock_wakeup() can be called from another thread to force MPIDU_Sock_wait() to return with a MPIDU_SOCK_ERR_INTR error.
MPIDU_Sock_wakeup() may not be called from within a progress update function or any function directly or indirectly called by a
progress update function.

Thread safety:
New operations may be posted to sock contained in the specified sock set while another thread is calling or blocking in
MPIDU_Sock_wait().  These operations should complete as though they were posted before MPIDU_Sock_wait() was called.

Module:
Utility-Sock
@*/
int MPIDU_Sock_wait(MPIDU_Sock_set_t set, int timeout, MPIDU_Sock_event_t * event);


/*@
MPIDU_Sock_wakeup - wakeup a MPIDU_Sock_wait blocking in another thread

Input Parameter:
. set - sock set upon which to wait for an event

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - wakeup request successfully processed
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SET - invalid sock set object
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory (is this possible?)
- MPIDU_SOCK_ERR_FAIL - other error (are there any?)

Notes:
This routine forces a MPIDU_Sock_wait() blocking in another thread to wakeup and return a MPIDU_SOCK_ERR_INTR error.
MPIDU_Sock_wakeup() may not be called from within a progress update function or any function directly or indirectly called by a
progress update function.

The implementation should strive to only wakeup a MPIDU_Sock_wait() that is already blocking; however, it is acceptable (although
undesireable) for it wakeup a MPIDU_Sock_wait() that is called in the future.

Module:
Utility-Sock
@*/
int MPIDU_Sock_wakeup(MPIDU_Sock_set_t set);

/*@
MPIDU_Sock_read - perform an immediate read

Input Parameters:
+ sock - sock object from which data is to be read
. buf - buffer into which the data should be placed
- len - maximum number of bytes to read

Output Parameter:
. num_read - number of bytes actually read

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - no error encountered during the read operation
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by buf and len resulted in a memory fault
. MPIDU_SOCK_ERR_BAD_LEN - length parameter must be greater than zero
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error attempting to post read

Notes:

An immediate read may not be performed while a posted read is outstanding on the same sock.  This is considered to be an
application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Thread safety:
MPIDU_Sock_read() may be called while another thread is attempting to perform an immediate write or post a write operation on the
same sock.  MPIDU_Sock_read() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the same sock
set to which the specified sock belongs.

A immediate read may not be performed if another thread is performing an immediate read on the same sock.  This is considered to be
an application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_read() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result of
doing so is undefined, although the implementation may choose to return MPIDU_SOCK_ERR_SOCK_CLOSED if it is able to catch the
error.  The application should coordinate the closing of a sock with the activities of other threads to ensure that one thread is
not attempting to perform an immediate read while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_read);


/*@
MPIDU_Sock_readv - perform an immediate vector read

Input Parameters:
+ sock - sock object from which data is to be read
. iov - I/O vector describing buffers into which the data is placed
- iov_n - number of elements in I/O vector (must be less than MPID_IOV_LIMIT)

Output Parameter:
. num_read - number of bytes actually read

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - no error encountered during the read operation
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by iov and iov_n resulted in a memory fault
. MPIDU_SOCK_ERR_BAD_LEN - iov_n parameter must be greater than zero and not greater than MPID_IOV_LIMIT
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error attempting to post read

Notes:

An immediate read may not be performed while a posted read is outstanding on the same sock.  This is considered to be an
application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Thread safety:
MPIDU_Sock_read() may be called while another thread is attempting to perform an immediate write or post a write operation on the
same sock.  MPIDU_Sock_read() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the same sock
set to which the specified sock belongs.

A immediate read may not be performed if another thread is performing an immediate read on the same sock.  This is considered to be
an application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_read() during or after the call to MPIDU_Sock_post_close() is consider an application error.  The result of
doing so is undefined, although the implementation may choose to return MPIDU_SOCK_ERR_SOCK_CLOSED if it is able to catch the
error.  The application should coordinate the closing of a sock with the activities of other threads to ensure that one thread is
not attempting to perform an immediate read while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_read);


/*@
MPIDU_Sock_write - perform an immediate write

Input Parameters:
+ sock - sock object to which data is to be written
. buf - buffer containing the data to be written
- len - maximum number of bytes to written

Output Parameter:
. num_written - actual number of bytes written

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - no error encountered during the write operation
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by buf and len resulted in a memory fault
. MPIDU_SOCK_ERR_BAD_LEN - length parameter must be greater than zero
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error attempting to perform the write

Notes:
An immediate write may not be performed while a posted write is outstanding on the same sock.  This is considered to be an
application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Thread safety:
MPIDU_Sock_write() may be called while another thread is attempting to perform an immediate read or post a read operation on the
same sock.  MPIDU_Sock_write() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the same sock
set to which the specified sock belongs.

A immediate write may not be performed if another thread is performing an immediate write on the same sock.  This is considered to
be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_write() during or after the call to MPIDU_Sock_post_close() is consider to be an application error.  The result
of doing so is undefined, although the implementation may choose to return MPIDU_SOCK_ERR_SOCK_CLOSED if it is able to catch the
error.  The application should coordinate the closing of a sock with the activities of other threads to ensure that one thread is
not attempting to perform an immediate write while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_written);


/*@
MPIDU_Sock_writev - perform an immediate vector write

Input Parameters:
+ sock - sock object to which data is to be written
. iov - I/O vector describing buffers of data to be written
- iov_n - number of elements in I/O vector (must be less than MPID_IOV_LIMIT)

Output Parameter:
. num_written - actual number of bytes written

Return value: a MPI error code with a Sock extended error class
+ MPI_SUCCESS - no error encountered during the write operation
. MPIDU_SOCK_ERR_INIT - Sock module not initialized
. MPIDU_SOCK_ERR_BAD_SOCK - invalid sock object
. MPIDU_SOCK_ERR_BAD_BUF - using the buffer described by iov and iov_n resulted in a memory fault
. MPIDU_SOCK_ERR_BAD_LEN - iov_n parameter must be greater than zero and not greater than MPID_IOV_LIMIT
. MPIDU_SOCK_ERR_SOCK_CLOSED - the sock object was closed locally
. MPIDU_SOCK_ERR_CONN_CLOSED - the connection was closed by the peer
. MPIDU_SOCK_ERR_CONN_FAILED - the connection failed
. MPIDU_SOCK_ERR_INPROGRESS - this operation overlapped with another like operation already in progress
. MPIDU_SOCK_ERR_NOMEM - unable to allocate required memory
- MPIDU_SOCK_ERR_FAIL - other error attempting to perform the write

Notes:
An immediate write may not be performed while a posted write is outstanding on the same sock.  This is considered to be an
application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Thread safety:
MPIDU_Sock_write() may be called while another thread is attempting to perform an immediate read or post a read operation on the
same sock.  MPIDU_Sock_write() may also be called while another thread is calling or blocking in MPIDU_Sock_wait() on the same sock
set to which the specified sock belongs.

A immediate write may not be performed if another thread is performing an immediate write on the same sock.  This is considered to
be an application error, and the results of doing so are undefined.  The implementation may choose to catch the error and return
MPIDU_SOCK_ERR_INPROGRESS, but it is not required to do so.

Calling MPIDU_Sock_write() during or after the call to MPIDU_Sock_post_close() is consider to be an application error.  The result
of doing so is undefined, although the implementation may choose to return MPIDU_SOCK_ERR_SOCK_CLOSED if it is able to catch the
error.  The application should coordinate the closing of a sock with the activities of other threads to ensure that one thread is
not attempting to perform an immediate write while another thread is attempting to close the sock.

Module:
Utility-Sock
@*/
int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_written);


/*@
MPIDU_Sock_get_sock_id - get an integer identifier for a sock object

Input Parameter:
. sock - sock object

Return value: an integer that uniquely identifies the sock object

Notes:
The integer is unique relative to all other open sock objects in the local process.  The integer may later be reused for a
different sock once the current object is closed and destroyed.

This function does not return an error code.  Passing in an invalid sock object has undefined results (garbage in, garbage out).

Module:
Utility-Sock
@*/
int MPIDU_Sock_get_sock_id(MPIDU_Sock_t sock);


/*@
MPIDU_Sock_get_sock_set_id - get an integer identifier for a sock set object

Input Parameter:
. sock set - sock set object

Return value: an integer that uniquely identifies the sock set object

Notes:

The integer is unique relative to all other sock set objects currently existing in the local process.  The integer may later be
reused for a different sock set once the current object destroyed.

This function does not return an error code.  Passing in an invalid sock set object has undefined results (garbage in, garbage
out).

Module:
Utility-Sock
@*/
int MPIDU_Sock_get_sock_set_id(MPIDU_Sock_set_t set);


/*@
MPIDU_Sock_get_error_class_string - get a generic error string from an error code

Input Parameter:
+ error - sock error
- length - length of error string

Output Parameter:
. error_string - error string

Return value: a string representation of the sock error

Notes:

The returned string is the generic error message for the supplied error code.

Module:
Utility-Sock
@*/
int MPIDU_Sock_get_error_class_string(int error, char *error_string, size_t length);


CPLUSPLUS_END

#endif /* !defined(MPIDU_SOCK_H_INCLUDED) */
