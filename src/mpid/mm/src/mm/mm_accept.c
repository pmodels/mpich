/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_accept(MPID_Info *info_ptr, char *port_name)
{
    int error;
    int bfd;
    OpenPortNode *p;
    MPIDI_STATE_DECL(MPID_STATE_MM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_ACCEPT);

    p = MPID_Process.port_list;
    while (p)
    {
	if (strncmp(p->port_name, port_name, MPI_MAX_PORT_NAME) == 0)
	{
	    bfd = beasy_accept(p->bfd);
	    if (bfd == BFD_INVALID_SOCKET)
	    {
		error = beasy_getlasterror();
		err_printf("beasy_accept failed, error %d\n", error);
		MPIDI_FUNC_EXIT(MPID_STATE_MM_ACCEPT);
		return BFD_INVALID_SOCKET;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_ACCEPT);
	    return bfd;
	}
	p = p->next;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_ACCEPT);
    return BFD_INVALID_SOCKET;
}

