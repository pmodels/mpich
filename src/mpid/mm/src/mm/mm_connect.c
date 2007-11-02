/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_connect(MPID_Info *info_ptr, char *port_name)
{
    char host[100];
    int port;
    char *token;
    int bfd;
    int error;
    MPIDI_STATE_DECL(MPID_STATE_MM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CONNECT);

    strncpy(host, port_name, 100);
    token = strtok(host, ":");
    token = strtok(NULL, "\n");
    if (token == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CONNECT);
	return BFD_INVALID_SOCKET;
    }
    port = atoi(token);

    if (beasy_create(&bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	err_printf("beasy_create failed, error %d\n", error);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CONNECT);
	return BFD_INVALID_SOCKET;
    }
    if (beasy_connect(bfd, host, port) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	err_printf("beasy_connect failed, error %d\n", error);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_CONNECT);
	return BFD_INVALID_SOCKET;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CONNECT);
    return bfd;
}

