/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int node;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Free resources allocated in CH3_Init() */
    MPID_VCRT_Release (MPIR_Process.comm_self->vcrt);
    MPID_VCRT_Release (MPIR_Process.comm_world->vcrt);
    MPIU_Free (MPIDI_CH3I_Process.pg->vc_table);
    MPIU_Free (MPIDI_CH3I_Process.pg->kvs_name);
    MPIU_Free (MPIDI_CH3I_Process.pg);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));

    /* This is a hack to try to get around the fact that gasnet
     * doesn't have a finalize call, and so can't call gm_finalize().
     * This can lead to lost/corrupted messages if the process exits
     * before the NIC can send the message. */
    gasnet_barrier_notify (0, 0);
    if (gasnet_barrier_wait (0, 0) != GASNET_OK)
    {
	MPID_Abort(NULL, MPI_SUCCESS, -1);
    }
    
    PMI_Finalize ();
    printf_d ("MPIDI_CH3_Finalize\n");
    
    return mpi_errno;
}
