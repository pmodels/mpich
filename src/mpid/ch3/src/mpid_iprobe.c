/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int (*MPIDI_Anysource_iprobe_fn)(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                 MPI_Status * status) = NULL;

#undef FUNCNAME
#define FUNCNAME MPID_Iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm->revoked &&
            MPIR_AGREE_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_Process.tagged_coll_mask) &&
            MPIR_SHRINK_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_Process.tagged_coll_mask)) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }

#ifdef ENABLE_COMM_OVERRIDES
    if (MPIDI_Anysource_iprobe_fn) {
        if (source == MPI_ANY_SOURCE) {
            /* if it's anysource, check shm, then check the network.
               If still not found, call progress, and check again. */

            /* check shm*/
            MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
            found = MPIDI_CH3U_Recvq_FU(source, tag, context, status);
            MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
            if (!found) {
                /* not found, check network */
                mpi_errno = MPIDI_Anysource_iprobe_fn(tag, comm, context_offset, &found, status);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                if (!found) {
                    /* still not found, make some progress*/
                    mpi_errno = MPIDI_CH3_Progress_poke();
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    /* check shm again */
                    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
                    found = MPIDI_CH3U_Recvq_FU(source, tag, context, status);
                    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
                    if (!found) {
                        /* check network again */
                        mpi_errno = MPIDI_Anysource_iprobe_fn(tag, comm, context_offset, &found, status);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
            }
            *flag = found;
            goto fn_exit;
        } else {
            /* it's not anysource, check if the netmod has overridden it */
            MPIDI_VC_t * vc;
            MPIDI_Comm_get_vc_set_active(comm, source, &vc);
            if (vc->comm_ops && vc->comm_ops->iprobe) {
                mpi_errno = vc->comm_ops->iprobe(vc, source, tag, comm, context_offset, &found, status);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                *flag = found;
                goto fn_exit;
            }
            /* fall-through to shm case */
        }
    }
#endif
    
    /* FIXME: The routine CH3U_Recvq_FU is used only by the probe functions;
       it should atomically return the flag and status rather than create 
       a request.  Note that in some cases it will be possible to 
       atomically query the unexpected receive list (which is what the
       probe routines are for). */
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    found = MPIDI_CH3U_Recvq_FU( source, tag, context, status );
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    if (!found) {
	/* Always try to advance progress before returning failure
	   from the iprobe test.  */
	/* FIXME: It would be helpful to know if the Progress_poke
	   operation causes any change in state; we could then avoid
	   a second test of the receive queue if we knew that nothing
	   had changed */
	mpi_errno = MPID_Progress_poke();
	MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
	found = MPIDI_CH3U_Recvq_FU( source, tag, context, status );
	MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    }
	
    *flag = found;

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
