/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_send(int conn, char *buffer, int length)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_MM_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_SEND);

    if (beasy_send(conn, buffer, length) != SOCKET_ERROR)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_SEND);
	return length;
    }
    error = beasy_getlasterror();
    err_printf("beasy_send failed, error %d\n", error);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_SEND);
    return SOCKET_ERROR;
}

