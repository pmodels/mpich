/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Imrecv(void *buf, int count, MPI_Datatype datatype,
                MPID_Request *message, MPID_Request **rreqp)
{
  int mpi_errno = MPI_SUCCESS;

  MPID_Request * rreq;

  /* ---------------------------------------- */
  /* NULL rank means empty request            */
  /* ---------------------------------------- */
  if (unlikely(message == NULL))
    {
      rreq = MPIDI_Request_create2();
      MPIR_Status_set_procnull(&rreq->status);
      rreq->kind = MPID_REQUEST_RECV;
      MPIDI_Request_complete(rreq);
      *rreqp = rreq;
      return MPI_SUCCESS;

    }

  MPIU_Assert(message != NULL);
  MPIU_Assert(message->kind == MPID_REQUEST_MPROBE);

  /* promote the request object to be a "real" recv request */
  message->kind = MPID_REQUEST_RECV;

  *rreqp = rreq = message;

#ifdef OUT_OF_ORDER_HANDLING
  if ((MPIDI_In_cntr[rreq->mpid.peer_pami].n_OutOfOrderMsgs>0))
     MPIDI_Recvq_process_out_of_order_msgs(rreq->mpid.peer_pami, MPIDI_Context[0]);
#endif

#if (MPIDI_STATISTICS)
  MPID_NSTAT(mpid_statp->recvs);
#endif

#ifdef MPIDI_TRACE
{
  size_t ll;
  ll = count * MPID_Datatype_get_basic_size(datatype);
  /*MPIDI_SET_PR_REC(rreq,buf,count,ll,datatype,pami_source,rank,tag,comm,is_blocking); */
}
#endif

  /* ----------------------------------------------------------------- */
  /* populate request with our data                                    */
  /* We can do this because this is not a multithreaded implementation */
  /* ----------------------------------------------------------------- */

  rreq->mpid.userbuf      = buf;
  rreq->mpid.userbufcount = count;
  rreq->mpid.datatype     = datatype;
  MPIDI_RecvMsg_Unexp(rreq, buf, count, datatype);
  mpi_errno = rreq->status.MPI_ERROR;
  if (TOKEN_FLOW_CONTROL_ON) {
     #if TOKEN_FLOW_CONTROL
     if ((rreq->mpid.uebuflen) && (!(rreq->mpid.envelope.msginfo.isRzv))) {
       MPIDI_Token_cntr[(rreq->mpid.peer_pami)].unmatched--;
       MPIDI_Update_rettoks(rreq->mpid.peer_pami);
     }
     MPIDI_Must_return_tokens(MPIDI_Context[0],(rreq->mpid.peer_pami));
     #else
     MPID_assert_always(0);
     #endif
  }

#ifdef OUT_OF_ORDER_HANDLING
  if ((MPIDI_In_cntr[rreq->mpid.peer_pami].n_OutOfOrderMsgs>0))
     MPIDI_Recvq_process_out_of_order_msgs(rreq->mpid.peer_pami, MPIDI_Context[0]);
#endif

#ifdef MPIDI_STATISTICS
  if (!(MPID_cc_is_complete(&rreq->cc)))
    {
        MPID_NSTAT(mpid_statp->recvWaitsComplete);
    }
#endif

  return mpi_errno;
}

