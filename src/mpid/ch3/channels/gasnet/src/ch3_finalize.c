/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int node;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    
    /* This is a hack to try to get around the fact that gasnet
     * doesn't have a finalize call, and so can't call gm_finalize().
     * This can lead to lost/corrupted messages if the process exits
     * before the NIC can send the message. */
    gasnet_barrier_notify (0, 0);
    if (gasnet_barrier_wait (0, 0) != GASNET_OK)
    {
	MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet barrier failed");
    }
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;
}
