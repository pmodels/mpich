/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IBU_H
#define IBU_H

#ifdef __cplusplus
extern "C" {
#endif

/* config header file */
#include "mpichconf.h"
#include "iba.h"
#include "psc_iba.h"


/* definitions */

#define IBU_UNIX    1
#define IBU_WINDOWS 2

typedef enum IBU_OP
{
    IBU_OP_TIMEOUT,
    IBU_OP_READ,
    IBU_OP_WRITE,
/*
    IBU_OP_ACCEPT,
    IBU_OP_CONNECT,
*/
    IBU_OP_CLOSE
} IBU_OP;
/*
#define IBU_OP_READ    1
#define IBU_OP_WRITE   2
#define IBU_OP_ACCEPT  3
#define IBU_OP_CONNECT 4
#define IBU_OP_CLOSE   5
#define IBU_OP_TIMEOUT 6
*/
#define IBU_IOV_LIMIT   16

/* insert error codes here */
#define IBU_SUCCESS           0
#define IBU_FAIL              -1
#define IBU_ERR_TIMEOUT       1001
#define IBU_ERR_CONN_REFUSED  1002
#define IBU_ERR_OS_SPECIFIC   1003



#define WITH_IBU_TYPE IBU_WINDOWS

/* definitions/structures specific to Windows */
#if (WITH_IBU_TYPE == IBU_WINDOWS)

#include <winsock2.h>
#define IBU_IOV             WSABUF
#define IBU_IOV_LEN         len
#define IBU_IOV_BUF         buf
#define IBU_IOV_MAXLEN      16
#define IBU_INFINITE        INFINITE
#define IBU_INVALID_QP      0

typedef ib_cq_handle_t ibu_set_t;
typedef struct ibu_state_t * ibu_t;

/* definitions/structures specific to Unix */
#elif (WITH_IBU_TYPE == IBU_UNIX)

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define IBU_IOV             struct iovec
#define IBU_IOV_LEN         iov_len
#define IBU_IOV_BUF         iov_base
#define IBU_IOV_MAXLEN      16
#define IBU_INFINITE        -1
#define IBU_INVALID_QP      0

typedef ib_cq_handle_t ibu_set_t;
typedef int ibu_t;

#else
#error Error: WITH_IBU_TYPE not defined
#endif



/* common structures */

typedef struct ibu_wait_t
{
    IBU_OP op_type;
    int num_bytes;
    void *user_ptr;
    int error;
} ibu_wait_t;


/* user callback function */

/* If a user function is passed to one of the ibu_post_... functions the following applies:
   1) The ibu progress engine will call this function when partial data has been read or
      written for the posted operation.
   2) All progress_update calls must complete before completion notification is signalled.
      In other words, ibu_wait will not return until all progress_update calls have completed.
*/
int progress_update(int num_bytes, void *user_ptr);

/* function prototypes */
int ibu_init(void);
int ibu_finalize(void);

int ibu_get_lid();

int ibu_create_set(ibu_set_t *set);
int ibu_destroy_set(ibu_set_t set);

int ibu_set_user_ptr(ibu_t ibu, void *user_ptr);

/*
int ibu_listen(ibu_set_t set, void *user_ptr, int *port, ibu_t *listener);
int ibu_post_connect(ibu_set_t set, void *user_ptr, char *host, int port, ibu_t *connected);
int ibu_post_close(ibu_t ibu);
*/
ibu_t ibu_create_qp(ibu_set_t set, int dlid);
int ibu_post_read(ibu_t ibu, void *buf, int len, int (*read_progress_update)(int, void*));
int ibu_post_readv(ibu_t ibu, IBU_IOV *iov, int n, int (*read_progress_update)(int, void*));
int ibu_post_write(ibu_t ibu, void *buf, int len, int (*write_progress_update)(int, void*));
int ibu_post_writev(ibu_t ibu, IBU_IOV *iov, int n, int (*write_progress_update)(int, void*));

int ibu_wait(ibu_set_t set, int millisecond_timeout, ibu_wait_t *out);

/*
int ibu_accept(ibu_set_t set, void *user_ptr, ibu_t listener, ibu_t *accepted);
int ibu_read(ibu_t ibu, void *buf, int len, int *num_read);
int ibu_readv(ibu_t ibu, IBU_IOV *iov, int n, int *num_read);
int ibu_write(ibu_t ibu, void *buf, int len, int *num_written);
int ibu_writev(ibu_t ibu, IBU_IOV *iov, int n, int *num_written);
*/

/* extended functions */
/*
int ibu_easy_receive(ibu_t ibu, void *buf, int len, int *num_read);
int ibu_easy_send(ibu_t ibu, void *buf, int len, int *num_written);
int ibu_getid(ibu_t ibu);
*/

#ifdef __cplusplus
}
#endif

#endif
