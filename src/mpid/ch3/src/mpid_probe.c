/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Probe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Probe(int source, int tag, MPID_Comm * comm, int context_offset, 
	       MPI_Status * status)
{
    MPID_Progress_state progress_state;
    const int context = comm->recvcontext_id + context_offset;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_PROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_PROBE);

    if (source == MPI_PROC_NULL)
    {
	MPIR_Status_set_procnull(status);
	goto fn_exit;
    }

    MPIDI_CH3_Progress_start(&progress_state);
    do
    {
        int found;

        MPIU_THREAD_CS_ENTER(MSGQUEUE,);
        found = MPIDI_CH3U_Recvq_FU(source, tag, context, status);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,);
        if (found) break;

	mpi_errno = MPIDI_CH3_Progress_wait(&progress_state);
    }
    while(mpi_errno == MPI_SUCCESS);
    MPIDI_CH3_Progress_end(&progress_state);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;
}
