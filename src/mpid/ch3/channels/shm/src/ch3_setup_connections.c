/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Setup_connections
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Setup_connections()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    /* shared memory is connected when it is created */

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);
    return 0;
}
