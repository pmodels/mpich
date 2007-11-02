/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

#ifdef WITH_METHOD_SHM

int shm_can_connect(char *business_card)
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_CAN_CONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_CAN_CONNECT);

    /*
    if (strncmp(business_card, SHM_Process.host, 100) == 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SHM_CAN_CONNECT);
	return TRUE;
    }
    */

    MPIDI_FUNC_EXIT(MPID_STATE_SHM_CAN_CONNECT);
    return FALSE;
}

#endif
