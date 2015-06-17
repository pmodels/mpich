/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/pt2pt/mpidi_recvmsg.c
 * \brief ADI level implemenation of common recv code.
 */
#include <mpidimpl.h>
#include <mpidi_macros.h>


void
MPIDI_RecvMsg_Unexp(MPID_Request  * rreq,
                    void          * buf,
                    MPI_Aint        count,
                    MPI_Datatype    datatype)
{
  /* ------------------------------------------------------------ */
  /* message was found in unexpected queue                        */
  /* ------------------------------------------------------------ */
  /* We must acknowledge synchronous send requests                */
  /* The recvnew callback will acknowledge the posted messages    */
  /* Recv functions will ack the messages that are unexpected     */
  /* ------------------------------------------------------------ */
  TRACE_SET_R_BIT((rreq->mpid.partner_id),(rreq->mpid.idx),fl.f.matchedInUQ);

  if (MPIDI_Request_isRzv(rreq))
    {
      const unsigned is_sync = MPIDI_Request_isSync(rreq);
      const unsigned is_zero = (rreq->mpid.envelope.length==0);

      /* -------------------------------------------------------- */
      /* Received an expected flow-control rendezvous RTS.        */
      /*     This is very similar to the found/incomplete case    */
      /* -------------------------------------------------------- */
      if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
        {
          MPID_Datatype_get_ptr(datatype, rreq->mpid.datatype_ptr);
          MPID_Datatype_add_ref(rreq->mpid.datatype_ptr);
        }

      if (likely((is_sync+is_zero) == 0))
        MPIDI_Context_post(MPIDI_Context_local(rreq), &rreq->mpid.post_request, MPIDI_RendezvousTransfer, rreq);
      else if (is_sync != 0)
        MPIDI_Context_post(MPIDI_Context_local(rreq), &rreq->mpid.post_request, MPIDI_RendezvousTransfer_SyncAck, rreq);
      else
        MPIDI_Context_post(MPIDI_Context_local(rreq), &rreq->mpid.post_request, MPIDI_RendezvousTransfer_zerobyte, rreq);
    }
  else 
    {
     if (MPID_cc_is_complete(&rreq->cc))
     {
      if (unlikely(MPIDI_Request_isSync(rreq)))
      {
        /* Post this to the context for asynchronous progresss. We cannot do
         * the send-immediate inline here because we may not have the
         * context locked (its is being asynchrously advanced).
         * Must "uncomplete" the message (increment the ref and completion counts) so we
         * hold onto this request object until this send has completed.  When MPIDI_SyncAck_handoff
         * finishes sending the ack, it will complete the request, decrementing the ref and
         * completion counts.
         */
        MPIDI_Request_uncomplete(rreq);
        MPIDI_Send_post(MPIDI_SyncAck_handoff, rreq);
      }
      /* -------------------------------- */
      /* request is complete              */
      /* -------------------------------- */
      if (rreq->mpid.uebuf != NULL)
        {
          if (likely(MPIR_STATUS_GET_CANCEL_BIT(rreq->status) == FALSE))
            {
              MPIDI_msg_sz_t _count=0;
              MPIDI_Buffer_copy(rreq->mpid.uebuf,
                                rreq->mpid.uebuflen,
                                MPI_CHAR,
                                &rreq->status.MPI_ERROR,
                                buf,
                                count,
                                datatype,
                                &_count,
                                &rreq->status.MPI_ERROR);
              MPIR_STATUS_SET_COUNT(rreq->status, _count);
            }
        }
      else
        {
          MPID_assert(rreq->mpid.uebuflen == 0);
          MPIR_STATUS_SET_COUNT(rreq->status, 0);
        }
     }
     else
     {
      /* -------------------------------- */
      /* request is incomplete            */
      /* -------------------------------- */
      if (unlikely(MPIDI_Request_isSync(rreq)))
        {
          /* Post this to the context for asynchronous progresss. We cannot do
           * the send-immediate inline here because we may not have the
           * context locked (its is being asynchrously advanced).
           * Must "uncomplete" the message (increment the ref and completion counts) so we
           * hold onto this request object until this send has completed.  When MPIDI_SyncAck_handoff
           * finishes sending the ack, it will complete the request, decrementing the ref and
           * completion counts.
           */
          MPIDI_Request_uncomplete(rreq);
          MPIDI_Send_post(MPIDI_SyncAck_handoff, rreq);
        }
      if (MPIR_STATUS_GET_CANCEL_BIT(rreq->status) == FALSE)
        {
          MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
        }
      if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
        {
          MPID_Datatype_get_ptr(datatype, rreq->mpid.datatype_ptr);
          MPID_Datatype_add_ref(rreq->mpid.datatype_ptr);
        }
     }
    }
}



void
MPIDI_RecvMsg_procnull(MPID_Comm     * comm,
                       unsigned        is_blocking,
                       MPI_Status    * status,
                       MPID_Request ** request)
{
  if (is_blocking)
    {
      MPIR_Status_set_procnull(status);
      *request = NULL;
    }
  else
    {
      MPID_Request * rreq;
      rreq = MPIDI_Request_create2();
      MPIR_Status_set_procnull(&rreq->status);
      rreq->kind = MPID_REQUEST_RECV;
      rreq->comm = comm;
      MPIR_Comm_add_ref(comm);
      MPIDI_Request_complete(rreq);
      *request = rreq;
    }
}
