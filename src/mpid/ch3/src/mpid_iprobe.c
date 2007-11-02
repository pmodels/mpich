/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Iprobe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Iprobe(int source, int tag, MPID_Comm *comm, int context_offset, 
		int *flag, MPI_Status *status)
{
    const int context = comm->recvcontext_id + context_offset;
    int found = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_IPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IPROBE);

    if (source == MPI_PROC_NULL)
    {
	MPIR_Status_set_procnull(status);
	/* We set the flag to true because an MPI_Recv with this rank will
	   return immediately */
	*flag = TRUE;
	goto fn_exit;
    }

    /* FIXME: The routine CH3U_Recvq_FU is used only by the probe functions;
       it should atomically return the flag and status rather than create 
       a request.  Note that in some cases it will be possible to 
       atomically query the unexpected receive list (which is what the
       probe routines are for). */
    found = MPIDI_CH3U_Recvq_FU( source, tag, context, status );
    if (!found) {
	/* Always try to advance progress before returning failure
	   from the iprobe test.  */
	/* FIXME: It would be helpful to know if the Progress_poke
	   operation causes any change in state; we could then avoid
	   a second test of the receive queue if we knew that nothing
	   had changed */
	mpi_errno = MPID_Progress_poke();
	found = MPIDI_CH3U_Recvq_FU( source, tag, context, status );
    }
	
    *flag = found;

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
}
