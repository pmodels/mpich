/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

int tcp_accept_connection(void)
{
    int bfd;
    int remote_rank;
    int context;
    MPIDI_VC *vc_ptr;
    char ack;
    BOOL inwriteset;
    MPIDI_STATE_DECL(MPID_STATE_TCP_ACCEPT_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_ACCEPT_CONNECTION);

    /* accept new connection */
    bfd = beasy_accept(TCP_Process.listener);
    if (bfd == BFD_INVALID_SOCKET)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_accept_connection: beasy_accpet failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
	return -1;
    }
    /* receive the remote rank */
    if (beasy_receive(bfd, (void*)&remote_rank, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_accept_connection: beasy_receive(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
	return -1;
    }
    /* receive the remote context */
    if (beasy_receive(bfd, (void*)&context, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_accept_connection: beasy_receive(context) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
	return -1;
    }

    /* resolve the context to a virtual connection */
    vc_ptr = mm_vc_from_context(context, remote_rank);
    
    MPID_Thread_lock(vc_ptr->lock);

    if (vc_ptr->method == MM_UNBOUND_METHOD)
    {
	/* This block really is impossible to get to because tcp_accept_connection 
	 * can only be called if you know that the vc is bound to the tcp method.
	 */
	vc_ptr->method = MM_TCP_METHOD;
	vc_ptr->data.tcp.bfd = bfd;
	vc_ptr->fn.post_read = tcp_post_read;
	vc_ptr->fn.merge_with_unexpected = tcp_merge_with_unexpected;
	vc_ptr->fn.post_write = tcp_post_write;
	vc_ptr->fn.post_read_pkt = tcp_post_read_pkt;

	/* send a keep acknowledgement */
	ack = TCP_ACCEPT_CONNECTION;
	beasy_send(bfd, &ack, 1);

	bmake_nonblocking(bfd);
	/* add the new connection to the read set */
	TCP_Process.max_bfd = BFD_MAX(bfd, TCP_Process.max_bfd);
	if (!BFD_ISSET(bfd, &TCP_Process.readset))
	{
	    BFD_SET(bfd, &TCP_Process.readset);
	    TCP_Process.num_readers++;
	}
	vc_ptr->read_next_ptr = TCP_Process.read_list;
	TCP_Process.read_list = vc_ptr;

	/* change the state of the vc to connected */
	vc_ptr->data.tcp.connected = TRUE;
	vc_ptr->data.tcp.connecting = FALSE;
	
	MPID_Thread_unlock(vc_ptr->lock);

	/* post the first packet read on the newly connected vc */
	tcp_post_read_pkt(vc_ptr);
    }
    else
    {
#ifdef MPICH_DEV_BUILD
	if (vc_ptr->method != MM_TCP_METHOD)
	{
	    err_printf("Error:tcp_accept_connection: vc is already connected with method %d\n", vc_ptr->method);
	    MPID_Thread_unlock(vc_ptr->lock);
	    MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
	    return -1;
	}
	if (!vc_ptr->data.tcp.connecting || vc_ptr->data.tcp.connected)
	{
	    err_printf("Error:tcp_accept_connection: vc is already connected.\n");
	    MPID_Thread_unlock(vc_ptr->lock);
	    MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
	    return -1;
	}
#endif
	if (remote_rank > MPIR_Process.comm_world->rank)
	{
	    if (vc_ptr->data.tcp.bfd == TCP_Process.max_bfd)
	    {
		/* somehow figure out a new max bfd */
		/*err_printf("Error: tcp_accept_connection: max_bfd is invalid.\n");*/
	    }
	    /* close the old socket and keep the new */
	    if (BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.readset))
	    {
		BFD_CLR(vc_ptr->data.tcp.bfd, &TCP_Process.readset);
		TCP_Process.num_readers--;
	    }
	    inwriteset = BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
	    if (inwriteset)
	    {
		BFD_CLR(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
		TCP_Process.num_writers--;
	    }

	    /* if tcp_read hasn't read the reject ack already, do so here and then close the socket. */
	    if (!vc_ptr->data.tcp.reject_received)
	    {
		beasy_receive(vc_ptr->data.tcp.bfd, &ack, 1);
		if (ack != TCP_REJECT_CONNECTION)
		{
		    err_printf("Error:tcp_accept_connection: expecting reject ack, received #%d\n", (int)ack);
		}
	    }
	    /* close the socket */
	    beasy_closesocket(vc_ptr->data.tcp.bfd);
	    /* save the new connection */
	    vc_ptr->data.tcp.bfd = bfd;
	    /* add the new connection to the read set and possibly the write set */
	    TCP_Process.max_bfd = BFD_MAX(bfd, TCP_Process.max_bfd);
	    if (!BFD_ISSET(bfd, &TCP_Process.readset))
	    {
		BFD_SET(bfd, &TCP_Process.readset);
		TCP_Process.num_readers++;
	    }
	    if (inwriteset)
	    {
		BFD_SET(bfd, &TCP_Process.writeset);
		TCP_Process.num_writers++;
	    }
	}
	else
	{
	    /* send a reject acknowledgement */
	    ack = TCP_REJECT_CONNECTION;
	    beasy_send(bfd, &ack, 1);
	    /* close the new socket and keep the old */
	    beasy_closesocket(bfd);
	}
	/* change the state of the vc to connected */
	vc_ptr->data.tcp.connected = TRUE;
	vc_ptr->data.tcp.connecting = FALSE;

	bmake_nonblocking(vc_ptr->data.tcp.bfd);
	
	vc_ptr->data.tcp.bytes_of_header_read = 0;
	vc_ptr->data.tcp.read = tcp_read_header;

	MPID_Thread_unlock(vc_ptr->lock);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_ACCEPT_CONNECTION);
    return MPI_SUCCESS;
}

/*@
   tcp_make_progress - make progress

   Notes:
@*/
int tcp_make_progress(void)
{
    int nready = 0;
    struct timeval tv;
    MPIDI_VC *vc_iter;
    bfd_set readset;
    bfd_set writeset;
    MPIDI_STATE_DECL(MPID_STATE_TCP_MAKE_PROGRESS);
    /*MPIDI_STATE_DECL(MPID_STATE_BSELECT);*/

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_MAKE_PROGRESS);

    if ((TCP_Process.num_readers == 0) &&
	(TCP_Process.num_writers == 0))
    {
	/* shortcut out because there are no sockets in either the read set or the write set */
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }


    /* This function needs a way to know if it should block or not */
    tv.tv_sec = 0;
    tv.tv_usec = 1;

    bcopyset(&readset, &TCP_Process.readset);
    if (TCP_Process.num_writers)
	bcopyset(&writeset, &TCP_Process.writeset);

    /* select */
    /*MPIDI_FUNC_ENTER(MPID_STATE_BSELECT);*/
    nready = bselect(TCP_Process.max_bfd, 
	TCP_Process.num_readers ? &readset : NULL,
	TCP_Process.num_writers ? &writeset : NULL,
	NULL, &tv);
    /*MPIDI_FUNC_EXIT(MPID_STATE_BSELECT);*/

    if (nready == 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }

    /* handle readers */
    if ((vc_iter = TCP_Process.read_list) != NULL)
    {
	do
	{
	    if (BFD_ISSET(vc_iter->data.tcp.bfd, &readset))
	    {
		/* read */
		vc_iter->data.tcp.read(vc_iter);
		nready--;
	    }
	    if (nready == 0)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
		return MPI_SUCCESS;
	    }
	    vc_iter = vc_iter->read_next_ptr;
	}while (vc_iter);
    }

    /* handle writers */
    if ((vc_iter = TCP_Process.write_list) != NULL)
    {
	do
	{
	    if (BFD_ISSET(vc_iter->data.tcp.bfd, &writeset))
	    {
		/* write */
		/*tcp_write(vc_iter);*/
		tcp_write_aggressive(vc_iter);
		nready--;
	    }
	    if (nready == 0)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
		return MPI_SUCCESS;
	    }
	    vc_iter = vc_iter->write_next_ptr;
	}while (vc_iter);
    }

    if (nready == 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }

    /* handle new connections */
    if (BFD_ISSET(TCP_Process.listener, &readset))
    {
	nready--;
	if (tcp_accept_connection() != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
	    return -1;
	}
    }

#ifdef MPICH_DEV_BUILD
    if (nready)
    {
	err_printf("Error: %d sockets still signalled after traversing read_list, write_list and listener.");
	/* return some error */
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
	return -1;
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#endif
