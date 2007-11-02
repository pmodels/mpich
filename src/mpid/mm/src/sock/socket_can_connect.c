/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

#ifdef WITH_METHOD_SOCKET

int socket_can_connect(char *business_card)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_CAN_CONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_CAN_CONNECT);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_CAN_CONNECT);
    return TRUE;
}

#endif
