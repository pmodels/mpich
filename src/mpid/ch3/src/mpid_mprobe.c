/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Mprobe(int source, int tag, MPIR_Comm *comm, int attr,
                MPIR_Request **message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    int found = FALSE;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int context_id = comm->recvcontext_id + context_offset;

    *message = NULL;

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm->revoked) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }

#ifdef ENABLE_COMM_OVERRIDES
    if (MPIDI_Anysource_improbe_fn) {
        if (source == MPI_ANY_SOURCE) {
            /* if it's anysource, loop while checking the shm recv
               queue and improving the netmod, then do a progress
               test to make some progress. */
            do {
                *message = MPIDI_CH3U_Recvq_FDU_matchonly(source, tag, context_id, comm,&found);
                if (found) goto fn_exit;

                mpi_errno = MPIDI_Anysource_improbe_fn(tag, comm, context_offset, &found, message, status);
                MPIR_ERR_CHECK(mpi_errno);
                if (found) goto fn_exit;

                MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

                /* FIXME could this be replaced with a progress_wait? */
                mpi_errno = MPIDI_CH3_Progress_test();
                MPIR_ERR_CHECK(mpi_errno);
            } while (1);
        }
        else {
            /* it's not anysource, see if this is for the netmod */
            MPIDI_VC_t * vc;
            MPIDI_Comm_get_vc_set_active(comm, source, &vc);

            if (vc->comm_ops && vc->comm_ops->improbe) {
                /* netmod has overridden improbe */
                do {
                    mpi_errno = vc->comm_ops->improbe(vc, source, tag, comm, context_offset, &found,
                                                      message, status);
                    MPIR_ERR_CHECK(mpi_errno);
                    if (found) goto fn_exit;

                    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

                    /* FIXME could this be replaced with a progress_wait? */
                    mpi_errno = MPIDI_CH3_Progress_test();
                    MPIR_ERR_CHECK(mpi_errno);
                } while (1);
            }
            /* fall-through to shm case */
        }
    }
#endif
    /* Inefficient implementation: we poll the unexpected queue looking for a
     * matching request, interleaved with calls to progress.  If there are many
     * non-matching unexpected messages in the queue then we will end up
     * needlessly scanning the UQ.
     *
     * A smarter implementation would enqueue a partial request (one lacking the
     * recv buffer triple) onto the PQ.  Unfortunately, this is a lot harder to
     * do than it seems at first because of the spread-out nature of callers to
     * various CH3U_Recvq routines and especially because of the enqueue/dequeue
     * hooks for native MX tag matching support. */
    MPIDI_CH3_Progress_start(&progress_state);
    do
    {
        *message = MPIDI_CH3U_Recvq_FDU_matchonly(source, tag, context_id, comm, &found);
        if (found)
            break;

        mpi_errno = MPIDI_CH3_Progress_wait(&progress_state);
    }
    while(mpi_errno == MPI_SUCCESS);
    MPIDI_CH3_Progress_end(&progress_state);
    MPIR_ERR_CHECK(mpi_errno);

    if (*message) {
        (*message)->kind = MPIR_REQUEST_KIND__MPROBE;
        MPIR_Request_extract_status((*message), status);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

