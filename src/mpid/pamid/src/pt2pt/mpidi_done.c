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
 * \file src/pt2pt/mpidi_done.c
 * \brief "Done" call-backs provided to the message layer for signaling completion
 */
#include <mpidimpl.h>


/**
 * \brief Message layer callback which is invoked on the origin node
 * when the send of the message is done
 *
 * \param[in,out] sreq MPI receive request object
 */
void
MPIDI_SendDoneCB(pami_context_t   context,
                 void           * clientdata,
                 pami_result_t    result)
{
  TRACE_SET_S_BIT((((MPID_Request *) clientdata)->mpid.partner_id),(((MPID_Request *) clientdata)->mpid.idx),fl.f.sendComp);
  MPIDI_SendDoneCB_inline(context,
                          clientdata,
                          result);
}


static inline void
MPIDI_RecvDoneCB_copy(MPID_Request * rreq)
{
  int smpi_errno;
  MPID_assert(rreq->mpid.uebuf != NULL);
  MPIDI_msg_sz_t _count=0;
  MPIDI_Buffer_copy(rreq->mpid.uebuf,        /* source buffer */
                    rreq->mpid.uebuflen,
                    MPI_CHAR,
                    &smpi_errno,
                    rreq->mpid.userbuf,      /* dest buffer */
                    rreq->mpid.userbufcount, /* dest count */
                    rreq->mpid.datatype,     /* dest type */
                    &_count,
                    &rreq->status.MPI_ERROR);
  MPIR_STATUS_SET_COUNT(rreq->status, _count);
}


/**
 * \brief Message layer callback which is invoked on the target node
 * when the incoming message is complete.
 *
 * The MSGQUEUE lock may or may not be held.
 *
 * \param[in,out] rreq MPI receive request object
 */
void
MPIDI_RecvDoneCB(pami_context_t   context,
                 void           * clientdata,
                 pami_result_t    result)
{
  MPID_Request * rreq = (MPID_Request*)clientdata;
  MPID_assert(rreq != NULL);
  switch (MPIDI_Request_getCA(rreq))
    {
    case MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE:
      {
        MPIDI_RecvDoneCB_copy(rreq);
        /* free the unexpected data buffer later */
        break;
      }
    case MPIDI_CA_COMPLETE:
      {
        break;
      }
    default:
      {
        MPID_Abort(NULL, MPI_ERR_OTHER, -1, "Internal: unknown CA");
        break;
      }
    }
#ifdef OUT_OF_ORDER_HANDLING
  MPID_Request * oo_peer = rreq->mpid.oo_peer;
  if (oo_peer) {
     MPIR_STATUS_SET_COUNT(oo_peer->status, MPIR_STATUS_GET_COUNT(rreq->status));
     MPIDI_Request_complete(oo_peer);
  }
#endif
  MPIDI_Request_complete_norelease(rreq);
  /* caller must release rreq, after unlocking MSGQUEUE (if held) */
#ifdef OUT_OF_ORDER_HANDLING
  pami_task_t source;
  source = MPIDI_Request_getPeerRank_pami(rreq);
  if (MPIDI_In_cntr[source].n_OutOfOrderMsgs > 0) {
     MPIDI_Recvq_process_out_of_order_msgs(source, context);
  }
#endif
}


/**
 * \brief Thread-safe message layer callback which is invoked on the
 * target node when the incoming message is complete.
 *
 * \param[in,out] rreq MPI receive request object
 */
void
MPIDI_RecvDoneCB_mutexed(pami_context_t   context,
                         void           * clientdata,
                         pami_result_t    result)
{
  MPID_Request * rreq = (MPID_Request*)clientdata;
  MPIU_THREAD_CS_ENTER(MSGQUEUE, 0);

  MPIDI_RecvDoneCB(context, clientdata, result);

  MPIU_THREAD_CS_EXIT(MSGQUEUE, 0);
  MPID_Request_release(rreq);
}


#ifdef OUT_OF_ORDER_HANDLING
/**
 * \brief Checks if any of the messages in the out of order list is ready
 * to be processed, if so, process it
 */
void MPIDI_Recvq_process_out_of_order_msgs(pami_task_t src, pami_context_t context)
{
   /*******************************************************/
   /* If the next message to be processed in the          */
   /* a recv is posted, then process the message.         */
   /*******************************************************/
   MPIDI_In_cntr_t *in_cntr;
   MPID_Request *ooreq, *rreq, *prev_rreq;
   pami_get_simple_t xferP;
   MPIDI_msg_sz_t _count=0;
   int matched;
   void * it;

   in_cntr  = &MPIDI_In_cntr[src];
   prev_rreq = NULL;
   ooreq = in_cntr->OutOfOrderList;
   while((in_cntr->n_OutOfOrderMsgs !=0) && ((MPIDI_Request_getMatchSeq(ooreq) == (in_cntr->nMsgs+1)) || (MPIDI_Request_getMatchSeq(ooreq) == in_cntr->nMsgs)))
   {
      matched=0;
      matched=MPIDI_Search_recv_posting_queue(MPIDI_Request_getMatchRank(ooreq),MPIDI_Request_getMatchTag(ooreq),MPIDI_Request_getMatchCtxt(ooreq),&rreq, &it);

      if (matched)  {
        /* process a completed message i.e. data is in EA   */
        if (TOKEN_FLOW_CONTROL_ON) {
           #if TOKEN_FLOW_CONTROL
           if ((ooreq->mpid.uebuflen) && (!(ooreq->mpid.envelope.msginfo.isRzv))) {
               MPIDI_Token_cntr[src].unmatched--;
               MPIDI_Update_rettoks(src);
           }
           MPIDI_Must_return_tokens(context,src);
           #else
           MPID_assert_always(0);
           #endif
         }
        if (MPIDI_Request_getMatchSeq(ooreq) == (in_cntr->nMsgs+ 1))
          in_cntr->nMsgs++;

        if (ooreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
          MPIDI_Recvq_remove_req_from_ool(ooreq,in_cntr);
        }

        /* ----------------------------------------- */
        /*  Calculate message length for reception.  */
        /* ----------------------------------------- */
        unsigned dt_contig, dt_size;
        MPID_Datatype *dt_ptr;
        MPI_Aint dt_true_lb;
        MPIDI_Datatype_get_info(rreq->mpid.userbufcount,
                                rreq->mpid.datatype,
                                dt_contig,
                                dt_size,
                                dt_ptr,
                                dt_true_lb);
        if (unlikely(ooreq->mpid.uebuflen > dt_size))
          {
            MPIR_STATUS_SET_COUNT(rreq->status, dt_size);
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
          }

        TRACE_SET_REQ_VAL(rreq->mpid.idx,ooreq->mpid.idx);
        TRACE_SET_R_BIT(src,(rreq->mpid.idx),fl.f.matchedInOOL);
        TRACE_SET_R_VAL(src,(rreq->mpid.idx),rlen,dt_size);
        ooreq->comm = rreq->comm;
        MPIR_Comm_add_ref(ooreq->comm);
        ooreq->mpid.userbuf = rreq->mpid.userbuf;
        ooreq->mpid.userbufcount = rreq->mpid.userbufcount;
        ooreq->mpid.datatype = rreq->mpid.datatype;
	if (HANDLE_GET_KIND(ooreq->mpid.datatype) != HANDLE_KIND_BUILTIN)
          {
            MPID_Datatype_get_ptr(ooreq->mpid.datatype, ooreq->mpid.datatype_ptr);
            MPID_Datatype_add_ref(ooreq->mpid.datatype_ptr);
          }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
        if(MPIDI_Process.queue_binary_search_support_on)
          MPIDI_Recvq_remove_uexp_noit(MPIDI_Request_getMatchRank(ooreq),MPIDI_Request_getMatchTag(ooreq),MPIDI_Request_getMatchCtxt(ooreq), MPIDI_Request_getMatchSeq(ooreq));
        else
#endif
          MPIDI_Recvq_remove(MPIDI_Recvq.unexpected, ooreq, ooreq->mpid.prev);
	if (!MPID_cc_is_complete(&ooreq->cc)) {
	  ooreq->mpid.oo_peer = rreq;
          MPIDI_RecvMsg_Unexp(ooreq, rreq->mpid.userbuf, rreq->mpid.userbufcount, rreq->mpid.datatype);
	} else {
          MPIDI_RecvMsg_Unexp(ooreq, rreq->mpid.userbuf, rreq->mpid.userbufcount, rreq->mpid.datatype);
          MPIR_STATUS_SET_COUNT(rreq->status, MPIR_STATUS_GET_COUNT(ooreq->status));
          rreq->status.MPI_SOURCE = ooreq->status.MPI_SOURCE;
          rreq->status.MPI_TAG = ooreq->status.MPI_TAG;
          rreq->mpid.envelope.msginfo.MPIseqno = ooreq->mpid.envelope.msginfo.MPIseqno;
	  MPIDI_Request_complete(rreq);
        }
        MPID_Request_release(ooreq);

      } else {
        if (MPIDI_Request_getMatchSeq(ooreq) == (in_cntr->nMsgs+ 1))
          in_cntr->nMsgs++;
        if (ooreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
            MPIDI_Recvq_remove_req_from_ool(ooreq,in_cntr);
        }
      }
      if (in_cntr->n_OutOfOrderMsgs > 0)
        ooreq = in_cntr->OutOfOrderList;
   }  /* while */
}


/**
 * \brief  search the posted recv queue and if found eliminate the
 * element from the queue and return in *request; else return NULL
 */
int MPIDI_Search_recv_posting_queue(int src, int tag, int context_id,
                                   MPID_Request **request, void** it )
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq = NULL;

    *request = NULL;
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
    if(MPIDI_Process.queue_binary_search_support_on)
    {
      MPIDI_Recvq_find_in_post(src, tag, context_id, &rreq, it);
      if (rreq != NULL)
      {
        /* The communicator test is not yet correct */
        MPIDI_Recvq_remove_post(src, tag, context_id, *it);
        *request = rreq;
#if (MPIDI_STATISTICS)
        MPID_NSTAT(mpid_statp->earlyArrivalsMatched);
#endif
        return 1;
      }
      else
      {
        MPIDI_Recvq_find_in_post(src, MPI_ANY_TAG, context_id, &rreq, it);
        if (rreq == NULL)
        {
          MPIDI_Recvq_find_in_post(MPI_ANY_SOURCE, tag, context_id, &rreq, it);
          if (rreq == NULL)
            MPIDI_Recvq_find_in_post(MPI_ANY_SOURCE, MPI_ANY_TAG, context_id, &rreq, it);
        }
        if (rreq != NULL)
        {
          /* The communicator test is not yet correct */
          MPIDI_Recvq_remove_post(MPIDI_Request_getMatchRank(rreq), MPIDI_Request_getMatchTag(rreq), context_id, *it);
          *request = rreq;
#if (MPIDI_STATISTICS)
          MPID_NSTAT(mpid_statp->earlyArrivalsMatched);
#endif
          return 1;
        }
      }
    }
    else
    {
#endif
      rreq = MPIDI_Recvq.posted_head;
      while (rreq != NULL)
      {
        /* The communicator test is not yet correct */
        if ((MPIDI_Request_getMatchRank(rreq)==src || MPIDI_Request_getMatchRank(rreq)==MPI_ANY_SOURCE) &&
        (MPIDI_Request_getMatchCtxt(rreq)==context_id) &&
        (MPIDI_Request_getMatchTag(rreq)  == tag  || MPIDI_Request_getMatchTag(rreq)  == MPI_ANY_TAG)
        ) {
            MPIDI_Recvq_remove(MPIDI_Recvq.posted, rreq, prev_rreq);
            *request = rreq;
#if (MPIDI_STATISTICS)
            MPID_NSTAT(mpid_statp->earlyArrivalsMatched);
#endif
            return 1;
        }
        prev_rreq = rreq;
        rreq = rreq->mpid.next;
      }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
    }
#endif
    return 0;
}
#endif
