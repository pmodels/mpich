/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

TCP_PerProcess TCP_Process;
MPIDI_VC_functions g_tcp_vc_functions = 
{
    tcp_post_read,
    tcp_car_head_enqueue,
    tcp_merge_with_unexpected,
    tcp_merge_with_posted,
    tcp_merge_unexpected_data,
    tcp_post_write,
    tcp_car_head_enqueue,
    tcp_reset_car,
    tcp_setup_packet_car,
    tcp_post_read_pkt
};

/*@
   tcp_init - initialize the tcp method

   Notes:
@*/
int tcp_init(void)
{
    int error;
    char *env;
    MPIDI_STATE_DECL(MPID_STATE_TCP_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_INIT);

    error = bsocket_init();
    if (error)
    {
	err_printf("tcp_init: bsocket_init failed, error %d\n", error);
    }
    if (beasy_create(&TCP_Process.listener, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to create the listener socket\nError: %s\n", TCP_Process.err_msg);
    }
    if (blisten(TCP_Process.listener, 10) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to listen on the listener socket\nError: %s\n", TCP_Process.err_msg);
    }
    if (beasy_get_sock_info(TCP_Process.listener, TCP_Process.host, &TCP_Process.port) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to get the hostname and port of the listener socket\nError: %s\n", TCP_Process.err_msg);
    }

    BFD_ZERO(&TCP_Process.writeset);
    BFD_ZERO(&TCP_Process.readset);
    BFD_SET(TCP_Process.listener, &TCP_Process.readset);
    TCP_Process.max_bfd = TCP_Process.listener;
    TCP_Process.num_readers = 0;
    TCP_Process.num_writers = 0;

    TCP_Process.nTCP_EAGER_LIMIT = TCP_EAGER_LIMIT;
    env = getenv("MPICH_TCP_EAGER_LIMIT");
    if (env)
    {
	TCP_Process.nTCP_EAGER_LIMIT = atoi(env);
	if (TCP_Process.nTCP_EAGER_LIMIT < 1)
	    TCP_Process.nTCP_EAGER_LIMIT = TCP_EAGER_LIMIT;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_INIT);
    return MPI_SUCCESS;
}

/*@
   tcp_finalize - finalize the tcp method

   Notes:
@*/
int tcp_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_FINALIZE);

    beasy_closesocket(TCP_Process.listener);
    TCP_Process.listener = BFD_INVALID_SOCKET;

    bsocket_finalize();

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_FINALIZE);
    return MPI_SUCCESS;
}

#endif
