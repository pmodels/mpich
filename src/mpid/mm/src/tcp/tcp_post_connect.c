/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

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
	err_printf("tcp_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	free(temp);
	err_printf("tcp_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    strcpy(host, token); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	free(temp);
	err_printf("tcp_post_connect:GetHostAndPort: invalid business card\n");
	return -1;
    }
    *port = atoi(token);
    free(temp);

    return MPI_SUCCESS;
}

int tcp_post_connect(MPIDI_VC *vc_ptr, char *business_card)
{
    char host[100];
    int port;
    MPIDI_STATE_DECL(MPID_STATE_TCP_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_TCP_POST_CONNECT);
    dbg_printf("tcp_post_connect\n");

    if (vc_ptr->data.tcp.connected)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return MPI_SUCCESS;
    }
    MPID_Thread_lock(vc_ptr->lock);
    if (vc_ptr->data.tcp.connected)
    {
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return MPI_SUCCESS;
    }
    if ((business_card == NULL) || (strlen(business_card) > 100))
    {
	err_printf("tcp_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }

    if (GetHostAndPort(host, &port, business_card) != MPI_SUCCESS)
    {
	err_printf("tcp_post_connect: unable to parse the host and port from the business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }
    /*msg_printf("GetHostAndPort returned %s:%d\n", host, port);*/

    if (beasy_create(&vc_ptr->data.tcp.bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_create failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }

    if (beasy_connect(vc_ptr->data.tcp.bfd, host, port) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_connect(%s:%d) failed, error %d: %s\n", host, port, TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }

    /* what do i send the other side? what context? what rank */
    if (beasy_send(vc_ptr->data.tcp.bfd, (void*)&MPIR_Process.comm_world->rank, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_send(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }
    if (beasy_send(vc_ptr->data.tcp.bfd, (void*)&MPIR_Process.comm_world->context_id, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_send(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
	return -1;
    }

    /* add the vc to the active read list */
    TCP_Process.max_bfd = BFD_MAX(vc_ptr->data.tcp.bfd, TCP_Process.max_bfd);
    if (!BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.readset))
    {
	BFD_SET(vc_ptr->data.tcp.bfd, &TCP_Process.readset);
	TCP_Process.num_readers++;
    }
    vc_ptr->read_next_ptr = TCP_Process.read_list;
    TCP_Process.read_list = vc_ptr;

    vc_ptr->data.tcp.connecting = TRUE;
    vc_ptr->data.tcp.reject_received = FALSE;
    MPID_Thread_unlock(vc_ptr->lock);

    dbg_printf("tcp_post_connect returning MPI_SUCCESS\n");

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_CONNECT);
    return MPI_SUCCESS;
}

#endif
