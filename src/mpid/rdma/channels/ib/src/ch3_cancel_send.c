/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Cancel_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Cancel_send(MPIDI_VC * vc, MPID_Request * sreq, int * cancelled)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CANCEL_SEND);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CANCEL_SEND);

    *cancelled = FALSE;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CANCEL_SEND);
    return MPI_SUCCESS;
}
