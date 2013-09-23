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
 * \file src/pt2pt/mpidi_callback_eager.c
 * \brief The callback for a new eager message
 */
#include <mpidimpl.h>


/**
 * \brief The standard callback for a new message
 *
 * \param[in]  context      The context on which the message is being received.
 * \param[in]  cookie       Unused
 * \param[in]  _msginfo     The header information
 * \param[in]  msginfo_size The size of the header information
 * \param[in]  sndbuf       If the message is short, this is the data
 * \param[in]  sndlen       The size of the incoming data
 * \param[out] recv         If the message is long, this tells the message layer how to handle the data.
 */
void
MPIDI_RecvCB(pami_context_t    context,
             void            * cookie,
             const void      * _msginfo,
             size_t            msginfo_size,
             const void      * sndbuf,
             size_t            sndlen,
             pami_endpoint_t   sender,
             pami_recv_t     * recv)
{
  const MPIDI_MsgInfo *msginfo = (const MPIDI_MsgInfo *)_msginfo;
#if TOKEN_FLOW_CONTROL
  int          rettoks=0;
  void         *uebuf;
#endif
  pami_task_t  source;
  if (recv == NULL)
    {
      if (msginfo->isSync)
        MPIDI_RecvShortSyncCB(context,
                              cookie,
                              _msginfo,
                              msginfo_size,
                              sndbuf,
                              sndlen,
                              sender,
                              recv);
      else
        MPIDI_RecvShortAsyncCB(context,
                               cookie,
                               _msginfo,
                               msginfo_size,
                               sndbuf,
                               sndlen,
                               sender,
                               recv);
      return;
    }

  MPID_assert(sndbuf == NULL);
  MPID_assert(_msginfo != NULL);
  MPID_assert(msginfo_size == sizeof(MPIDI_MsgInfo));

  MPID_Request * rreq = NULL;
  source=PAMIX_Endpoint_query(sender);

  /* -------------------- */
  /*  Match the request.  */
  /* -------------------- */
  unsigned rank       = msginfo->MPIrank;
  unsigned tag        = msginfo->MPItag;
  unsigned context_id = msginfo->MPIctxt;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  if (TOKEN_FLOW_CONTROL_ON)
    {
      #if TOKEN_FLOW_CONTROL
      MPIDI_Receive_tokens(msginfo,source);
      #else
      MPID_assert_always(0);
      #endif
    }
#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_FDP(rank, tag, context_id);
#else
  rreq = MPIDI_Recvq_FDP(rank, source, tag, context_id, msginfo->MPIseqno);
#endif

  /* Match not found */
  if (unlikely(rreq == NULL))
    {
      /* No request was found and hence no sync needed */
#if (MPIDI_STATISTICS)
      MPID_NSTAT(mpid_statp->earlyArrivals);
#endif
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      MPID_Request *newreq = MPIDI_Request_create2();
      MPID_assert(newreq != NULL);
      if (TOKEN_FLOW_CONTROL_ON)
        {
          #if TOKEN_FLOW_CONTROL
          MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
          #else
          MPID_assert_always(0);
          #endif
        }

      if (sndlen)
      {
        newreq->mpid.uebuflen = sndlen;
        if (!(TOKEN_FLOW_CONTROL_ON))
          {
            newreq->mpid.uebuf = MPIU_Malloc(sndlen);
            newreq->mpid.uebuf_malloc = mpiuMalloc ;
          }
        else
          {
            #if TOKEN_FLOW_CONTROL
            newreq->mpid.uebuf = MPIDI_mm_alloc(sndlen);
            newreq->mpid.uebuf_malloc = mpidiBufMM;
            #else
            MPID_assert_always(0);
            #endif
          }
        MPID_assert(newreq->mpid.uebuf != NULL);
      }
      if (!TOKEN_FLOW_CONTROL_ON)
        {
          MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
        }
#ifndef OUT_OF_ORDER_HANDLING
      rreq = MPIDI_Recvq_FDP(rank, tag, context_id);
#else
      rreq = MPIDI_Recvq_FDP(rank, source, tag, context_id, msginfo->MPIseqno);
#endif

      if (unlikely(rreq == NULL))
      {
        MPIDI_Callback_process_unexp(newreq, context, msginfo, sndlen, sender, sndbuf, recv, msginfo->isSync);
        int completed = MPID_Request_is_complete(newreq);
        if (TOKEN_FLOW_CONTROL_ON)
          {
            #if TOKEN_FLOW_CONTROL
            MPIDI_Token_cntr[sender].unmatched++;
            #else
            MPID_assert_always(0);
            #endif
          }
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        if (completed) MPID_Request_release(newreq);
        goto fn_exit_eager;
      }
      else
      {
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        MPID_Request_discard(newreq);
      }
    }
  else
    {
#if (MPIDI_STATISTICS)
      MPID_NSTAT(mpid_statp->earlyArrivalsMatched);
#endif
      if (TOKEN_FLOW_CONTROL_ON)
        {
          #if TOKEN_FLOW_CONTROL
          MPIDI_Update_rettoks(source);
          MPIDI_Must_return_tokens(context,source);
          #else
          MPID_assert_always(0);
          #endif
        }
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }

  /* -------------------------------------------- */
  /*  Figure out target buffer for request data.  */
  /* -------------------------------------------- */

  /* ---------------------- */
  /*  Copy in information.  */
  /* ---------------------- */
  rreq->status.MPI_SOURCE = rank;
  rreq->status.MPI_TAG    = tag;
  MPIR_STATUS_SET_COUNT(rreq->status, sndlen);
  MPIDI_Request_setCA          (rreq, MPIDI_CA_COMPLETE);
  MPIDI_Request_cpyPeerRequestH(rreq, msginfo);
  MPIDI_Request_setSync        (rreq, msginfo->isSync);
  MPIDI_Request_setRzv         (rreq, 0);
  TRACE_SET_R_VAL(source,(rreq->mpid.PR_idx),len,sndlen);
  /* --------------------------------------- */
  /*  We have to fill in the callback info.  */
  /* --------------------------------------- */
  recv->local_fn = MPIDI_RecvDoneCB_mutexed;
  recv->cookie   = rreq;
#if ASSERT_LEVEL > 0
  /* This ensures that the value is set, even to something impossible */
  recv->addr = NULL;
#endif

  /* ----------------------------- */
  /*  Request was already posted.  */
  /* ----------------------------- */

  if (unlikely(msginfo->isSync))
    MPIDI_SyncAck_post(context, rreq, source);

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

  /* ----------------------------- */
  /*  Test for truncated message.  */
  /* ----------------------------- */
  if (unlikely(sndlen > dt_size))
    {
      MPIDI_Callback_process_trunc(context, rreq, recv, sndbuf);
      goto fn_exit_eager;
    }

  /* --------------------------------------- */
  /*  If buffer is contiguous, we are done.  */
  /* --------------------------------------- */
  if (likely(dt_contig))
    {
      /*
       * This is to test that the fields don't need to be
       * initialized.  Remove after this doesn't fail for a while.
       */
      MPID_assert(rreq->mpid.uebuf    == NULL);
      MPID_assert(rreq->mpid.uebuflen == 0);
      /* rreq->mpid.uebuf    = NULL; */
      /* rreq->mpid.uebuflen = 0; */
      void* rcvbuf = rreq->mpid.userbuf + dt_true_lb;

      recv->addr = rcvbuf;
    }

  /* ----------------------------------------------- */
  /*  Buffer is non-contiguous. we need to allocate  */
  /*  a temporary buffer, and unpack later.          */
  /* ----------------------------------------------- */
  else
    {
      /* ----------------------------------------------- */
      /*  Buffer is non-contiguous. the data is already  */
      /*  available, so we can just unpack it now.       */
      /* ----------------------------------------------- */
      MPIDI_Request_setCA(rreq, MPIDI_CA_UNPACK_UEBUF_AND_COMPLETE);
      rreq->mpid.uebuflen = sndlen;
      if (sndlen)
        {
          if (!TOKEN_FLOW_CONTROL_ON)
            {
              rreq->mpid.uebuf    = MPIU_Malloc(sndlen);
              rreq->mpid.uebuf_malloc = mpiuMalloc;
            }
          else
            {
              #if TOKEN_FLOW_CONTROL
              MPIDI_Alloc_lock(&rreq->mpid.uebuf,sndlen);
              rreq->mpid.uebuf_malloc = mpidiBufMM;
              #else
              MPID_assert_always(0);
              #endif
            }
          MPID_assert(rreq->mpid.uebuf != NULL);
        }
      /* -------------------------------------------------- */
      /*  Let PAMI know where to put the rest of the data.  */
      /* -------------------------------------------------- */
      recv->addr = rreq->mpid.uebuf;
    }
   TRACE_SET_R_VAL(source,(rreq->mpid.idx),fl.f.comp_in_HH,2);
   TRACE_SET_R_VAL(source,(rreq->mpid.idx),bufadd,rreq->mpid.userbuf);

 fn_exit_eager:
#ifdef OUT_OF_ORDER_HANDLING
  if (MPIDI_In_cntr[source].n_OutOfOrderMsgs > 0) {
    MPIDI_Recvq_process_out_of_order_msgs(source, context);
  }
#endif

  /* ---------------------------------------- */
  /*  Signal that the recv has been started.  */
  /* ---------------------------------------- */
  MPIDI_Progress_signal();
}
