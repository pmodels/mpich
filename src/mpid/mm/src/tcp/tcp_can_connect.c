/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

int tcp_can_connect(char *business_card)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_CAN_CONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_CAN_CONNECT);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_CAN_CONNECT);
    return TRUE;
}

#endif
