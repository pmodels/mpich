/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

#ifdef HAVE_STRING_H
#include <string.h> /* strdup */
#endif

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);
    */
    return nMask;
}

static int GetHostAndPort(char *host, int *port, char *business_card)
{
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet, nNicMask;
    char *temp, *pszHost, *pszIP, *pszPort;
    unsigned int ip;

    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	strcpy(pszNetMask, pEnv);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);

		/* parse each line of the business card and match the ip address with the network mask */
		temp = strdup(business_card);
		token = strtok(temp, ":\r\n");
		while (token)
		{
		    pszHost = token;
		    pszIP = strtok(NULL, ":\r\n");
		    pszPort = strtok(NULL, ":\r\n");
		    ip = GetIP(pszIP);
		    /*msg_printf("masking '%s'\n", pszIP);*/
		    if ((ip & nNicMask) == nNicNet)
		    {
			/* the current ip address matches the requested network so return these values */
			strcpy(host, pszIP); /*pszHost);*/
			*port = atoi(pszPort);
			free(temp);
			return MPI_SUCCESS;
		    }
		    token = strtok(NULL, ":\r\n");
		}
		free(temp);
	    }
	}
    }

    temp = strdup(business_card);
    /* move to the host part */
    token = strtok(temp, ":");
    if (token == NULL)
    {
	free(temp);
	err_printf("socket_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	free(temp);
	err_printf("socket_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    strcpy(host, token); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	free(temp);
	err_printf("socket_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    *port = atoi(token);
    free(temp);

    return MPI_SUCCESS;
}

int socket_post_connect(MPIDI_VC *vc_ptr, char *business_card)
{
    char host[100];
    int port;
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_CONNECT);

    /*MPIU_DBG_PRINTF(("socket_post_connect\n"));*/

    MPID_Thread_lock(vc_ptr->lock);

    if (vc_ptr->data.socket.state & (SOCKET_WRITING_ACK | SOCKET_CONNECTED))
    {
	MPIU_DBG_PRINTF(("socket_post_connect: socket connection in progress, no need to post a connect.\n"));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return MPI_SUCCESS;
    }

    if ((business_card == NULL) || (strlen(business_card) > 100))
    {
	err_printf("socket_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }

    if (GetHostAndPort(host, &port, business_card) != MPI_SUCCESS)
    {
	err_printf("socket_post_connect: unable to parse the host and port from the business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }
    /*msg_printf("GetHostAndPort returned %s:%d\n", host, port);*/

    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_POSTING_CONNECT);

    if ((error = sock_post_connect(SOCKET_Process.set, vc_ptr, host, port, &vc_ptr->data.socket.sock)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_post_connect: sock_post_connect failed.");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }

    MPIU_DBG_PRINTF(("socket_post_connect(%d)\n", sock_getid(vc_ptr->data.socket.sock)));

    MPID_Thread_unlock(vc_ptr->lock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
    return MPI_SUCCESS;
}

int socket_handle_connect(MPIDI_VC *vc_ptr)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_CONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_CONNECT);

    MPID_Thread_lock(vc_ptr->lock);

    MPIU_DBG_PRINTF(("socket_handle_connect(%d)\n", sock_getid(vc_ptr->data.socket.sock)));

    /* Change the state */
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_POSTING_CONNECT);
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_WRITING_CONTEXT_PKT);

    /* post a write of the connection stuff - context, rank */
    MPIU_DBG_PRINTF(("sock_post_write(%d:context,%d)\n", sock_getid(vc_ptr->data.socket.sock), MPIR_Process.comm_world->rank));
    vc_ptr->pkt_car.msg_header.pkt.u.context.rank = MPIR_Process.comm_world->rank;
    vc_ptr->pkt_car.msg_header.pkt.u.context.context = MPIR_Process.comm_world->context_id;
    if ((error = sock_post_write(vc_ptr->data.socket.sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.context, sizeof(MPID_Context_pkt), NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_post_write(MPID_Context_pkt) failed.");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
	return -1;
    }

    MPID_Thread_unlock(vc_ptr->lock);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
    return MPI_SUCCESS;
}

int socket_handle_written_ack(MPIDI_VC *temp_vc_ptr, int num_written)
{
    MPIDI_VC *vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);

    MPIU_DBG_PRINTF(("socket_handle_written_ack(%d) - %d byte\n", sock_getid(temp_vc_ptr->data.socket.sock), num_written));

    /* Change the state */
    SOCKET_CLR_BIT(temp_vc_ptr->data.socket.state, SOCKET_WRITING_ACK);

    switch (temp_vc_ptr->pkt_car.msg_header.pkt.u.context.ack_out)
    {
    case SOCKET_ACCEPT_CONNECTION:
	if (temp_vc_ptr->data.socket.connect_vc_ptr)
	{
	    MPIU_DBG_PRINTF(("socket_handle_written_ack(%d,headtohead) = accept\n", sock_getid(temp_vc_ptr->data.socket.sock)));
	    vc_ptr = temp_vc_ptr->data.socket.connect_vc_ptr;
	    if (vc_ptr->data.socket.state & SOCKET_FREEME_BIT)
	    {
		MPIU_DBG_PRINTF(("Connection completed in socket_handle_written_ack(%d), starting the regular message logic\n", sock_getid(vc_ptr->data.socket.sock)));
		/* close the current socket */
		sock_set_user_ptr(vc_ptr->data.socket.sock, NULL);
		sock_post_close(vc_ptr->data.socket.sock);
		/* copy the temp socket to the VC */
		vc_ptr->data.socket.sock = temp_vc_ptr->data.socket.sock;
		vc_ptr->data.socket.connect_vc_ptr = NULL;
		/* point the socket to the VC */
		sock_set_user_ptr(vc_ptr->data.socket.sock, vc_ptr);
		/* set the state to connected */
		SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
		/* post a read of a packet header and write any outstanding data */
		socket_post_read_pkt(vc_ptr);
		socket_write_aggressive(vc_ptr);
		/* free the unused VC */
		mm_vc_free(temp_vc_ptr);
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("Connection not completed in socket_handle_written_ack(%d), setting the freeme flag\n", sock_getid(vc_ptr->data.socket.sock)));
		/* the accept path reached the end first so signal the 
		   connect path to do the freeing and packet posting stuff */
		SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_FREEME_BIT);
	    }
	}
	else
	{
	    vc_ptr = temp_vc_ptr;
	    MPIU_DBG_PRINTF(("socket_handle_written_ack(%d) = accept\n", sock_getid(vc_ptr->data.socket.sock)));
	    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
	    socket_post_read_pkt(vc_ptr);
	    socket_write_aggressive(vc_ptr);
	}
	break;
    case SOCKET_REJECT_CONNECTION:
	MPIU_DBG_PRINTF(("socket_handle_written_ack(%d) = reject\n", sock_getid(temp_vc_ptr->data.socket.sock)));
	sock_set_user_ptr(temp_vc_ptr->data.socket.sock, NULL);
	MPIU_DBG_PRINTF(("sock_post_close(%d)\n", sock_getid(temp_vc_ptr->data.socket.sock)));
	sock_post_close(temp_vc_ptr->data.socket.sock);
	mm_vc_free(temp_vc_ptr);
	break;
    default:
	MPIU_DBG_PRINTF(("socket_handle_written_ack(%d) = unknown ack #%d\n", 
	    sock_getid(temp_vc_ptr->data.socket.sock), (int)temp_vc_ptr->pkt_car.msg_header.pkt.u.context.ack_out));
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);
    return MPI_SUCCESS;
}

int socket_handle_written_context_pkt(MPIDI_VC *vc_ptr, int num_written)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT);

    MPIU_DBG_PRINTF(("socket_handle_written_context_pkt(%d) - %d bytes of %d\n", sock_getid(vc_ptr->data.socket.sock), num_written, sizeof(MPID_Context_pkt)));

    /* Change the state */
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_WRITING_CONTEXT_PKT);
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READING_ACK);

    /* post a read of the ack */
    MPIU_DBG_PRINTF(("sock_post_read(%d:ack)\n", sock_getid(vc_ptr->data.socket.sock)));
    if ((error = sock_post_read(vc_ptr->data.socket.sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.context.ack_in, 1, NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_post_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT);
    return MPI_SUCCESS;
}

#endif
