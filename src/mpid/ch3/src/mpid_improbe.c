/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME I think we still need to handle the anysource probe case for
 * channel/netmod override.  See MPID_Iprobe for more info. */

#undef FUNCNAME
#define FUNCNAME MPID_Improbe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Improbe(int source, int tag, MPID_Comm *comm, int context_offset,
                 int *flag, MPID_Request **message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id = comm->recvcontext_id + context_offset;

    *message = NULL;

    if (source == MPI_PROC_NULL)
    {
        MPIR_Status_set_procnull(status);
        *flag = TRUE;
        goto fn_exit;
    }

    MPIU_THREAD_CS_ENTER(MSGQUEUE,);
    *message = MPIDI_CH3U_Recvq_FDU_matchonly(source, tag, context_id, comm, flag);
    MPIU_THREAD_CS_EXIT(MSGQUEUE,);

    if (!*flag) {
        /* Always try to advance progress before returning failure
           from the improbe test. */
        /* FIXME: It would be helpful to know if the Progress_poke
           operation causes any change in state; we could then avoid
           a second test of the receive queue if we knew that nothing
           had changed */
        mpi_errno = MPID_Progress_poke();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_THREAD_CS_ENTER(MSGQUEUE,);
        *message = MPIDI_CH3U_Recvq_FDU_matchonly(source, tag, context_id, comm, flag);
        MPIU_THREAD_CS_EXIT(MSGQUEUE,);
    }

    if (*flag && *message) {
        (*message)->kind = MPID_REQUEST_MPROBE;
        MPIR_Request_extract_status((*message), status);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

