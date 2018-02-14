/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Probe(int source, int tag, MPIR_Comm * comm, int context_offset,
	       MPI_Status * status)
{
    MPID_Progress_state progress_state;
    const int context = comm->recvcontext_id + context_offset;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROBE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROBE);

    if (source == MPI_PROC_NULL)
    {
	MPIR_Status_set_procnull(status);
	goto fn_exit;
    }

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm->revoked &&
            MPIR_AGREE_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_TAG_COLL_BIT) &&
            MPIR_SHRINK_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_TAG_COLL_BIT)) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }

#ifdef ENABLE_COMM_OVERRIDES
    if (MPIDI_Anysource_iprobe_fn) {
        if (source == MPI_ANY_SOURCE) {
            /* if it's anysource, loop while checking the shm recv
               queue and iprobing the netmod, then do a progress
               test to make some progress. */
            do {
                int found;
                
                MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
                found = MPIDI_CH3U_Recvq_FU(source, tag, context, status);
                MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
                if (found) goto fn_exit;

                mpi_errno = MPIDI_Anysource_iprobe_fn(tag, comm, context_offset, &found, status);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                if (found) goto fn_exit;

                MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                
                mpi_errno = MPIDI_CH3_Progress_test();
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            } while (1);
        } else {
            /* it's not anysource, see if this is for the netmod */
            MPIDI_VC_t * vc;
            MPIDI_Comm_get_vc_set_active(comm, source, &vc);
            
            if (vc->comm_ops && vc->comm_ops->iprobe) {
                /* netmod has overridden iprobe */
                do {
                    int found;
                    
                    mpi_errno = vc->comm_ops->iprobe(vc, source, tag, comm, context_offset, &found,
                                                     status);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    if (found) goto fn_exit;
                    
                    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                    
                    mpi_errno = MPIDI_CH3_Progress_test();
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                } while (1);
            }
            /* fall-through to shm case */
        }
    }
#endif
    MPIDI_CH3_Progress_start(&progress_state);
    do
    {
        int found;

        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
        found = MPIDI_CH3U_Recvq_FU(source, tag, context, status);
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
        if (found) break;

	mpi_errno = MPIDI_CH3_Progress_wait(&progress_state);
    }
    while(mpi_errno == MPI_SUCCESS);
    MPIDI_CH3_Progress_end(&progress_state);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
