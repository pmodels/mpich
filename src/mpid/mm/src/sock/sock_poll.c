/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "socketimpl.h"
#include "sock.h"

#if (WITH_SOCK_TYPE == SOCK_POLL)

int progress_update(int num_bytes, void *user_ptr)
{
    return 0;
}

/* function prototypes */
int sock_init()
{
    printf("sock interface not implemented for poll yet.\nExiting.\n");
    exit(-1);
    return 0;
}
int sock_finalize()
{
    return 0;
}

int sock_create_set(sock_set_t *set)
{
    return 0;
}
int sock_destroy_set(sock_set_t set)
{
    return 0;
}

int sock_set_user_ptr(sock_t sock, void *user_ptr)
{
    return 0;
}

int sock_listen(sock_set_t set, void *user_ptr, int *port, sock_t *listener)
{
    return 0;
}
int sock_post_connect(sock_set_t set, void *user_ptr, char *host, int port, sock_t *connected)
{
    return 0;
}
int sock_post_close(sock_t sock)
{
    return 0;
}
int sock_post_read(sock_t sock, void *buf, int len, int (*read_progress_update)(int, void*))
{
    return 0;
}
int sock_post_readv(sock_t sock, SOCK_IOV *iov, int n, int (*read_progress_update)(int, void*))
{
    return 0;
}
int sock_post_write(sock_t sock, void *buf, int len, int (*write_progress_update)(int, void*))
{
    return 0;
}
int sock_post_writev(sock_t sock, SOCK_IOV *iov, int n, int (*write_progress_update)(int, void*))
{
    return 0;
}

int sock_wait(sock_set_t set, int millisecond_timeout, sock_wait_t *out)
{
    return 0;
}

int sock_accept(sock_set_t set, void *user_ptr, sock_t listener, sock_t *accepted)
{
    return 0;
}
int sock_read(sock_t sock, void *buf, int len, int *num_read)
{
    return 0;
}
int sock_readv(sock_t sock, SOCK_IOV *iov, int n, int *num_read)
{
    return 0;
}
int sock_write(sock_t sock, void *buf, int len, int *num_written)
{
    return 0;
}
int sock_writev(sock_t sock, SOCK_IOV *iov, int n, int *num_written)
{
    return 0;
}

/* extended functions */
int sock_easy_receive(sock_t sock, void *buf, int len, int *num_read)
{
    return 0;
}
int sock_easy_send(sock_t sock, void *buf, int len, int *num_written)
{
    return 0;
}
int sock_getid(sock_t sock)
{
    return 0;
}

#endif /* WITH_SOCK_TYPE == SOCK_POLL */
