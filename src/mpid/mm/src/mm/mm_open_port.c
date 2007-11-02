/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"
#ifdef HAVE_STDIO_H
#include <stdio.h> /* snprintf */
#endif

int mm_open_port(MPID_Info *info_ptr, char *port_name)
{
    int bfd;
    int error;
    char host[40];
    int port;
    OpenPortNode *p;
    MPIDI_STATE_DECL(MPID_STATE_MM_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_OPEN_PORT);

    if (beasy_create(&bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	err_printf("beasy_create failed, error %d\n", error);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_OPEN_PORT);
	return error;
    }
    if (blisten(bfd, 5) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	err_printf("blisten failed, error %d\n", error);
	MPIDI_FUNC_EXIT(MPID_STATE_MM_OPEN_PORT);
	return error;
    }
    beasy_get_sock_info(bfd, host, &port);
    beasy_get_ip_string(host);

    snprintf(port_name, MPI_MAX_PORT_NAME, "%s:%d", host, port);

    p = (OpenPortNode*)MPIU_Malloc(sizeof(OpenPortNode));
    p->bfd = bfd;
    strncpy(p->port_name, port_name, MPI_MAX_PORT_NAME);
    p->next = MPID_Process.port_list;
    MPID_Process.port_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_OPEN_PORT);
    return 0;
}
