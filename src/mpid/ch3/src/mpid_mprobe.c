/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME I think we still need to handle the anysource probe case for
 * channel/netmod override.  See MPID_Iprobe for more info. */

#undef FUNCNAME
#define FUNCNAME MPID_Mprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Mprobe(int source, int tag, MPID_Comm *comm, int context_offset,
                MPID_Request **message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    int found = FALSE;
    int context_id = comm->recvcontext_id + context_offset;

    *message = NULL;

    if (source == MPI_PROC_NULL)
    {
        MPIR_Status_set_procnull(status);
        found = TRUE;
        goto fn_exit;
    }

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
        MPIU_THREAD_CS_ENTER(MSGQUEUE,);
        *message = MPIDI_CH3U_Recvq_FDU_matchonly(source, tag, context_id, comm, &found);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,);
        if (found)
            break;

        mpi_errno = MPIDI_CH3_Progress_wait(&progress_state);
    }
    while(mpi_errno == MPI_SUCCESS);
    MPIDI_CH3_Progress_end(&progress_state);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    if (*message) {
        (*message)->kind = MPID_REQUEST_MPROBE;
        MPIR_Request_extract_status((*message), status);
    }

    return mpi_errno;
fn_fail:
    goto fn_exit;
}

