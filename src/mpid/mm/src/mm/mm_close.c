/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_close(int conn)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CLOSE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CLOSE);
    beasy_closesocket(conn);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CLOSE);
    return 0;
}

