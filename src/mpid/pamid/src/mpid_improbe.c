/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Improbe(int source, int tag, MPID_Comm *comm, int context_offset,
                 int *flag, MPID_Request **message, MPI_Status *status)
{
    const int context = comm->recvcontext_id + context_offset;
    int foundp = FALSE;
    MPID_Request * rreq = NULL;

    if (source == MPI_PROC_NULL)
      {
        MPIR_Status_set_procnull(status);
        /* We set the flag to true because an MPI_Recv with this rank will
         * return immediately */
        *flag = TRUE;
        *message = NULL;
        return MPI_SUCCESS;
      }

#ifndef OUT_OF_ORDER_HANDLING
    rreq = MPIDI_Recvq_FDU(source, tag, context, &foundp);
#else
    int pami_source;
    if(source != MPI_ANY_SOURCE) {
      pami_source = MPID_VCR_GET_LPID(comm->vcr, source);
    } else {
      pami_source = MPI_ANY_SOURCE;
    }
    rreq = MPIDI_Recvq_FDU(source, pami_source, tag, context, &foundp);
#endif
    if (rreq==NULL) {
      MPID_Progress_poke();
    }
    else {
      rreq->kind = MPID_REQUEST_MPROBE;
      MPIR_Request_extract_status(rreq, status);
    }

    *message = rreq;
    *flag = foundp;
    return MPI_SUCCESS;
}

