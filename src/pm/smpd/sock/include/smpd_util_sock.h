/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(SMPD_UTIL_SOCK_H_INCLUDED)
#define SMPD_UTIL_SOCK_H_INCLUDED

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

#include "smpd_iov.h"
#include "smpd_util_socki.h"

/* The structures and funcs in this file are identical to ones in mpid/common/sock.
 * Refer to mpid/common/sock for more doc
 */

/* #define SMPDU_SOCK_ERR_CONN_CLOSED  SMPD_LAST_RETVAL+1 */

/*E
SMPDU_Sock_op_t - enumeration of posted operations that can be completed by the Sock module
E*/

typedef enum SMPDU_Sock_op{
    SMPDU_SOCK_OP_READ,
    SMPDU_SOCK_OP_WRITE,
    SMPDU_SOCK_OP_ACCEPT,
    SMPDU_SOCK_OP_CONNECT,
    SMPDU_SOCK_OP_CLOSE,
    SMPDU_SOCK_OP_WAKEUP
} SMPDU_Sock_op_t;


/*S
SMPDU_Sock_event_t - event structure returned by SMPDU_Sock_wait() describing the operation that completed
S*/
typedef struct SMPDU_Sock_event{
    SMPDU_Sock_op_t op_type;
    SMPDU_Sock_size_t num_bytes;
    void * user_ptr;
    int error;
} SMPDU_Sock_event_t;


/*@
SMPDU_Sock_init - initialize the Sock communication library
@*/
int SMPDU_Sock_init(void);


/*@
SMPDU_Sock_finalize - shutdown the Sock communication library
@*/
int SMPDU_Sock_finalize(void);


/*@
SMPDU_Sock_get_host_description - obtain a description of the host's 
communication capabilities
@*/
int SMPDU_Sock_get_host_description(int myRank, char * host_description, int len);


/*@
SMPDU_Sock_hostname_to_host_description - convert a host name to a description of the host's communication capabilities
@*/
int SMPDU_Sock_hostname_to_host_description(char *hostname, char * host_description, int len);

/*@
SMPDU_Sock_create_set - create a new sock set object
@*/
int SMPDU_Sock_create_set(SMPDU_Sock_set_t * set);


/*@
SMPDU_Sock_destroy_set - destroy an existing sock set, releasing an internal resource associated with that set
@*/
int SMPDU_Sock_destroy_set(SMPDU_Sock_set_t set);


/*@
SMPDU_Sock_native_to_sock - convert a native file descriptor/handle to a sock object
@*/
int SMPDU_Sock_native_to_sock(SMPDU_Sock_set_t set, SMPDU_SOCK_NATIVE_FD fd, void * user_ptr, SMPDU_Sock_t * sock);


/*@
SMPDU_Sock_listen - establish a listener sock
@*/
int SMPDU_Sock_listen(SMPDU_Sock_set_t set, void * user_ptr, int * port, SMPDU_Sock_t * sock);


/*@
SMPDU_Sock_accept - obtain the sock object associated with a new connection
@*/
int SMPDU_Sock_accept(SMPDU_Sock_t listener_sock, SMPDU_Sock_set_t set, void * user_ptr, SMPDU_Sock_t * sock);


/*@
SMPDU_Sock_post_connect - request that a new connection be formed

@*/
int SMPDU_Sock_post_connect(SMPDU_Sock_set_t set, void * user_ptr, char * host_description, int port, SMPDU_Sock_t * sock);

/*S
  SMPDU_Sock_ifaddr_t - Structure to hold an Internet address.
S*/
typedef struct SMPDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} SMPDU_Sock_ifaddr_t;

/*@ SMPDU_Sock_post_connect_ifaddr - Post a connection given an interface
  address (bytes, not string).
  @*/
int SMPDU_Sock_post_connect_ifaddr( SMPDU_Sock_set_t sock_set, 
				    void * user_ptr, 
				    SMPDU_Sock_ifaddr_t *ifaddr, int port,
				    SMPDU_Sock_t * sockp);


/*@
SMPDU_Sock_set_user_ptr - change the user pointer associated with a sock object
@*/
int SMPDU_Sock_set_user_ptr(SMPDU_Sock_t sock, void * user_ptr);


/*@
SMPDU_Sock_post_close - request that an existing connection be closed
@*/
int SMPDU_Sock_post_close(SMPDU_Sock_t sock);


/*E
SMPDU_Sock_progress_update_func_t - progress update callback functions
E*/
typedef int (* SMPDU_Sock_progress_update_func_t)(SMPDU_Sock_size_t num_bytes, void * user_ptr);


/*@
SMPDU_Sock_post_read - request that data be read from a sock
@*/
int SMPDU_Sock_post_read(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t minbr, SMPDU_Sock_size_t maxbr,
                         SMPDU_Sock_progress_update_func_t fn);


/*@
SMPDU_Sock_post_readv - request that a vector of data be read from a sock
@*/
int SMPDU_Sock_post_readv(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn);


/*@
SMPDU_Sock_post_write - request that data be written to a sock
@*/
int SMPDU_Sock_post_write(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t min, SMPDU_Sock_size_t max,
			  SMPDU_Sock_progress_update_func_t fn);


/*@
SMPDU_Sock_post_writev - request that a vector of data be written to a sock
@*/
int SMPDU_Sock_post_writev(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn);


/*@
SMPDU_Sock_wait - wait for an event
@*/
int SMPDU_Sock_wait(SMPDU_Sock_set_t set, int timeout, SMPDU_Sock_event_t * event);


/*@
SMPDU_Sock_wakeup - wakeup a SMPDU_Sock_wait blocking in another thread
@*/
int SMPDU_Sock_wakeup(SMPDU_Sock_set_t set);

/*@
SMPDU_Sock_read - perform an immediate read
@*/
int SMPDU_Sock_read(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, SMPDU_Sock_size_t * num_read);


/*@
SMPDU_Sock_readv - perform an immediate vector read
@*/
int SMPDU_Sock_readv(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_size_t * num_read);


/*@
SMPDU_Sock_write - perform an immediate write
@*/
int SMPDU_Sock_write(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, SMPDU_Sock_size_t * num_written);


/*@
SMPDU_Sock_writev - perform an immediate vector write
@*/
int SMPDU_Sock_writev(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_size_t * num_written);


/*@
SMPDU_Sock_get_sock_id - get an integer identifier for a sock object
@*/
int SMPDU_Sock_get_sock_id(SMPDU_Sock_t sock);


/*@
SMPDU_Sock_get_sock_set_id - get an integer identifier for a sock set object
@*/
int SMPDU_Sock_get_sock_set_id(SMPDU_Sock_set_t set);


/*@
SMPDU_Sock_get_error_class_string - get a generic error string from an error code

@*/
int SMPDU_Sock_get_error_class_string(int error, char *error_string, int length);


CPLUSPLUS_END

#endif /* !defined(SMPDU_UTIL_SOCK_H_INCLUDED) */
