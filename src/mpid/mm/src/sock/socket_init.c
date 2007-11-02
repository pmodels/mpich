/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* gethostname */
#endif

SOCKET_PerProcess SOCKET_Process;
MPIDI_VC_functions g_socket_vc_functions = 
{
    socket_post_read,
    socket_car_head_enqueue_read,
    socket_merge_with_unexpected,
    socket_merge_with_posted,
    socket_merge_unexpected_data,
    socket_post_write,
    socket_car_head_enqueue_write,
    socket_reset_car,
    socket_setup_packet_car,
    socket_post_read_pkt
};

/*@
   socket_init - initialize the socket method

   Notes:
@*/
int socket_init(void)
{
    int error;
    char *env;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_INIT);

    error = sock_init();

    if (error != SOCK_SUCCESS)
    {
	err_printf("socket_init: sock_init failed, error %d\n", error);
    }

    SOCKET_Process.nSOCKET_EAGER_LIMIT = SOCKET_EAGER_LIMIT;
    env = getenv("MPICH_SOCKET_EAGER_LIMIT");
    if (env)
    {
	SOCKET_Process.nSOCKET_EAGER_LIMIT = atoi(env);
	if (SOCKET_Process.nSOCKET_EAGER_LIMIT < 1)
	    SOCKET_Process.nSOCKET_EAGER_LIMIT = SOCKET_EAGER_LIMIT;
    }

    gethostname(SOCKET_Process.host, 100);

    if ((error = sock_create_set(&SOCKET_Process.set)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_init: sock_create_set failed.");
	sock_finalize();
	return 0;
    }
    if ((error = sock_listen(SOCKET_Process.set, SOCKET_LISTENER_POINTER, &SOCKET_Process.port, &SOCKET_Process.listener)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_init: sock_listen failed.");
	sock_finalize();
	return 0;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_INIT);
    return MPI_SUCCESS;
}

/*@
   socket_finalize - finalize the socket method

   Notes:
@*/
int socket_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_FINALIZE);

    sock_post_close(SOCKET_Process.listener);
    sock_destroy_set(SOCKET_Process.set);
    sock_finalize();

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_FINALIZE);
    return MPI_SUCCESS;
}

void socket_format_sock_error(int error)
{
#ifdef HAVE_WINSOCK2_H
    HLOCAL str_local;
    int num_bytes;

    SOCKET_Process.error = error;

    num_bytes = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_ALLOCATE_BUFFER,
	0,
	error,
	MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	(LPTSTR) &str_local,
	0,0);
    if (num_bytes < SOCKET_ERROR_MSG_LENGTH)
	memcpy(SOCKET_Process.err_msg, str_local, num_bytes+1);
    else
    {
	LocalFree(str_local);
	return;
    }
    LocalFree(str_local);
    strtok(SOCKET_Process.err_msg, "\r\n"); /* remove any CR/LF characters from the output */
#else
    SOCKET_Process.error = error;
    strncpy(SOCKET_Process.err_msg, strerror(error), SOCKET_ERROR_MSG_LENGTH);
#endif
}

void socket_print_sock_error(int error, char *msg)
{
    socket_format_sock_error(error);
    err_printf("%s\nError %d: %s\n", msg, SOCKET_Process.error, SOCKET_Process.err_msg);
}

#endif
