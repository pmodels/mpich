/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Mprobe(int source, int tag, MPID_Comm *comm, int context_offset,
                MPID_Request **message, MPI_Status *status)
{
    const int context = comm->recvcontext_id + context_offset;
    int foundp;
    MPID_Request * rreq = NULL;

    if (source == MPI_PROC_NULL)
      {
          MPIR_Status_set_procnull(status);
          return MPI_SUCCESS;
      }

#ifndef OUT_OF_ORDER_HANDLING
    MPID_PROGRESS_WAIT_WHILE((rreq=MPIDI_Recvq_FDU(source, tag, context, &foundp) ) == NULL );
#else
    int pami_source;
    if(source != MPI_ANY_SOURCE) {
      pami_source = MPID_VCR_GET_LPID(comm->vcr, source);
    } else {
      pami_source = MPI_ANY_SOURCE;
    }
    MPID_PROGRESS_WAIT_WHILE((rreq=MPIDI_Recvq_FDU(source, pami_source, tag, context, &foundp) ) == NULL );
#endif

    if (rreq) {
       rreq->kind = MPID_REQUEST_MPROBE;
       MPIR_Request_extract_status(rreq, status);
    }
     *message = rreq;
    return MPI_SUCCESS;
}

