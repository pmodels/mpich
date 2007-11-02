/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

int socket_handle_accept(void)
{
    int error;
    sock_t sock;
    MPIDI_VC *vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_ACCEPT);

    /*MPIU_DBG_PRINTF(("socket_handle_accept\n"));*/

    /* Create a temporary VC structure for reading the connect packet */
    /* We can't get the real VC because we don't know the remote rank and context yet */
    vc_ptr = mm_vc_alloc(MM_SOCKET_METHOD);

    /* Change the state */
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READING_CONTEXT_PKT);

    /* accept new connection */
    if ((error = sock_accept(SOCKET_Process.set, vc_ptr, SOCKET_Process.listener, &sock)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_accept: sock_accept failed.");
	mm_vc_free(vc_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
	return -1;
    }

    MPIU_DBG_PRINTF(("socket_handle_accept(%d)\n", sock_getid(sock)));

    /* save the socket */
    vc_ptr->data.socket.sock = sock;

    MPIU_DBG_PRINTF(("sock_post_read(%d:context)\n", sock_getid(sock)));

    if ((error = sock_post_read(sock, &vc_ptr->pkt_car.msg_header.pkt.u.context, sizeof(MPID_Context_pkt), NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_accept: sock_post_read failed.");
	mm_vc_free(vc_ptr);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

/*@
   socket_make_progress - make progress

   Notes:
@*/
int socket_make_progress(void)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_MAKE_PROGRESS);

    error = sock_wait(SOCKET_Process.set, 0, &SOCKET_Process.out);
    if (error == SOCK_SUCCESS)
    {
	switch (SOCKET_Process.out.op_type)
	{
	case SOCK_OP_READ:
	    socket_handle_read(SOCKET_Process.out.user_ptr, SOCKET_Process.out.num_bytes);
	    break;
	case SOCK_OP_WRITE:
	    socket_handle_written(SOCKET_Process.out.user_ptr, SOCKET_Process.out.num_bytes);
	    break;
	case SOCK_OP_CONNECT:
	    socket_handle_connect(SOCKET_Process.out.user_ptr);
	    break;
	case SOCK_OP_ACCEPT:
	    socket_handle_accept();
	    break;
#ifdef MPICH_DEV_BUILD
	case SOCK_OP_CLOSE:
	    if (SOCKET_Process.out.user_ptr != NULL)
	    {
		MPIU_DBG_PRINTF(("socket(%d) closed.\n", sock_getid(((MPIDI_VC*)SOCKET_Process.out.user_ptr)->data.socket.sock)));
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("socket closed.\n"));
	    }
	    break;
#endif
	default:
	    break;
	}
    }
    else
    {
	if (error != SOCK_ERR_TIMEOUT)
	{
	    socket_format_sock_error(SOCKET_Process.out.error);
	    err_printf("socket_make_progress: sock_wait failed, error %d, out.error %d - %s\n", error, 
		SOCKET_Process.out.error, SOCKET_Process.err_msg);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#endif
