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
 * \file src/pt2pt/mpidi_callback_rzv.c
 * \brief The callback for a new RZV RTS
 */
#include <mpidimpl.h>


/**
 * \brief The callback for a new RZV RTS
 * \note  Because this is a short message, the data is already received
 * \param[in]  context      The context on which the message is being received.
 * \param[in]  sender       The origin endpoint
 * \param[in]  _msginfo     The extended header information
 * \param[in]  msginfo_size The size of the extended header information
 * \param[in]  is_zero_byte The rendezvous message is zero bytes in length.
 */
void
MPIDI_RecvRzvCB_impl(pami_context_t    context,
                     pami_endpoint_t   sender,
                     const void      * _msginfo,
                     size_t            msginfo_size,
                     const unsigned    is_zero_byte)
{
  MPID_assert(_msginfo != NULL);
  MPID_assert(msginfo_size == sizeof(MPIDI_MsgEnvelope));
  const MPIDI_MsgEnvelope * envelope = (const MPIDI_MsgEnvelope *)_msginfo;
  const MPIDI_MsgInfo * msginfo = (const MPIDI_MsgInfo *)&envelope->msginfo;

  MPID_Request * rreq = NULL;
  int found;
  pami_task_t source;
#if TOKEN_FLOW_CONTROL
  int  rettoks=0;
#endif

  /* -------------------- */
  /*  Match the request.  */
  /* -------------------- */
  unsigned rank       = msginfo->MPIrank;
  unsigned tag        = msginfo->MPItag;
  unsigned context_id = msginfo->MPIctxt;

  MPID_Request *newreq = MPIDI_Request_create2();
  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  source = PAMIX_Endpoint_query(sender);
  MPIDI_Receive_tokens(msginfo,source);
#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_FDP_or_AEU(newreq, rank, tag, context_id, &found);
#else
  rreq = MPIDI_Recvq_FDP_or_AEU(newreq, rank, source, tag, context_id, msginfo->MPIseqno, &found);
#endif
  TRACE_ERR("RZV CB for req=%p remote-mr=0x%llx bytes=%zu (%sfound)\n",
            rreq,
            *(unsigned long long*)&envelope->envelope.memregion,
            envelope->envelope.length,
            found?"":"not ");

  /* ---------------------- */
  /*  Copy in information.  */
  /* ---------------------- */
  rreq->status.MPI_SOURCE = rank;
  rreq->status.MPI_TAG    = tag;
  MPIR_STATUS_SET_COUNT(rreq->status, envelope->length);
  MPIDI_Request_setPeerRank_comm(rreq, rank);
  MPIDI_Request_setPeerRank_pami(rreq, source);
  MPIDI_Request_cpyPeerRequestH (rreq, msginfo);
  MPIDI_Request_setSync         (rreq, msginfo->isSync);
  MPIDI_Request_setRzv          (rreq, 1);

  /* ----------------------------------------------------- */
  /* Save the rendezvous information for when the target   */
  /* node calls a receive function and the data is         */
  /* retreived from the origin node.                       */
  /* ----------------------------------------------------- */
  if (is_zero_byte)
    {
      rreq->mpid.envelope.length = 0;
      rreq->mpid.envelope.data   = NULL;
    }
  else
    {
#ifdef USE_PAMI_RDMA
      memcpy(&rreq->mpid.envelope.memregion,
             &envelope->memregion,
             sizeof(pami_memregion_t));
#else
      rreq->mpid.envelope.memregion_used = envelope->memregion_used;
      if(envelope->memregion_used)
        {
          memcpy(&rreq->mpid.envelope.memregion,
                 &envelope->memregion,
                 sizeof(pami_memregion_t));
        }
      rreq->mpid.envelope.data   = envelope->data;
#endif
      rreq->mpid.envelope.length = envelope->length;
     TRACE_SET_R_VAL(source,(rreq->mpid.idx),req,rreq);
     TRACE_SET_R_VAL(source,(rreq->mpid.idx),rlen,envelope->length);
     TRACE_SET_R_VAL(source,(rreq->mpid.idx),fl.f.sync,msginfo->isSync);
     TRACE_SET_R_BIT(source,(rreq->mpid.idx),fl.f.rzv);
     if (TOKEN_FLOW_CONTROL_ON)
       {
         #if TOKEN_FLOW_CONTROL
         MPIDI_Must_return_tokens(context,source);
         #else
         MPID_assert_always(0);
         #endif
       }
    }
  /* ----------------------------------------- */
  /* figure out target buffer for request data */
  /* ----------------------------------------- */
  if (found)
    {
#if (MPIDI_STATISTICS)
       MPID_NSTAT(mpid_statp->earlyArrivalsMatched);
#endif
      /* --------------------------- */
      /* if synchronized, post ack.  */
      /* --------------------------- */
      if (unlikely(MPIDI_Request_isSync(rreq)))
        MPIDI_SyncAck_post(context, rreq, MPIDI_Request_getPeerRank_pami(rreq));

      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);

      if (is_zero_byte)
        MPIDI_RecvRzvDoneCB_zerobyte(context, rreq, PAMI_SUCCESS);
      else
        {
          MPIDI_RendezvousTransfer(context, rreq);
          TRACE_SET_R_BIT(source,(rreq->mpid.idx),fl.f.sync_com_in_HH);
          TRACE_SET_R_BIT(source,(rreq->mpid.idx),fl.f.matchedInHH);
          TRACE_SET_R_VAL(source,(rreq->mpid.idx),bufadd,rreq->mpid.userbuf);
        }
      MPID_Request_discard(newreq);
    }

  /* ------------------------------------------------------------- */
  /* Request was not posted. */
  /* ------------------------------------------------------------- */
  else
    {
#if (MPIDI_STATISTICS)
       MPID_NSTAT(mpid_statp->earlyArrivals);
#endif
      /*
       * This is to test that the fields don't need to be
       * initialized.  Remove after this doesn't fail for a while.
       */
      MPID_assert(rreq->mpid.uebuf    == NULL);
      MPID_assert(rreq->mpid.uebuflen == 0);
      /* rreq->mpid.uebuf = NULL; */
      /* rreq->mpid.uebuflen = 0; */
#ifdef OUT_OF_ORDER_HANDLING
  if (MPIDI_In_cntr[source].n_OutOfOrderMsgs > 0) {
     MPIDI_Recvq_process_out_of_order_msgs(source, context);
  }
#endif
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }
  /* ---------------------------------------- */
  /*  Signal that the recv has been started.  */
  /* ---------------------------------------- */
  MPIDI_Progress_signal();
}

/**
 * \brief The callback for a new RZV RTS
 * \param[in]  context      The context on which the message is being received.
 * \param[in]  cookie       Unused
 * \param[in]  _msginfo     The extended header information
 * \param[in]  msginfo_size The size of the extended header information
 * \param[in]  sndbuf       Unused
 * \param[in]  sndlen       Unused
 * \param[in]  sender       The origin endpoint
 * \param[out] recv         Unused
 */
void
MPIDI_RecvRzvCB(pami_context_t    context,
                void            * cookie,
                const void      * _msginfo,
                size_t            msginfo_size,
                const void      * sndbuf,
                size_t            sndlen,
                pami_endpoint_t   sender,
                pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(sndlen == 0);
  MPIDI_RecvRzvCB_impl (context, sender, _msginfo, msginfo_size, 0);
}

/**
 * \brief The callback for a new "zero byte" RZV RTS
 * \param[in]  context      The context on which the message is being received.
 * \param[in]  cookie       Unused
 * \param[in]  _msginfo     The extended header information
 * \param[in]  msginfo_size The size of the extended header information
 * \param[in]  sndbuf       Unused
 * \param[in]  sndlen       Unused
 * \param[in]  sender       The origin endpoint
 * \param[out] recv         Unused
 */
void
MPIDI_RecvRzvCB_zerobyte(pami_context_t    context,
                         void            * cookie,
                         const void      * _msginfo,
                         size_t            msginfo_size,
                         const void      * sndbuf,
                         size_t            sndlen,
                         pami_endpoint_t   sender,
                         pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(sndlen == 0);
  MPIDI_RecvRzvCB_impl (context, sender, _msginfo, msginfo_size, 1);
}
