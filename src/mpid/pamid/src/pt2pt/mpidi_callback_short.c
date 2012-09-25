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
 * \file src/pt2pt/mpidi_callback_short.c
 * \brief The callback for a new short message
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
static inline void
MPIDI_RecvShortCB(pami_context_t    context,
                  const void      * _msginfo,
                  const void      * sndbuf,
                  size_t            sndlen,
                  pami_endpoint_t   sender,
                  unsigned          isSync)
  __attribute__((__always_inline__));
static inline void
MPIDI_RecvShortCB(pami_context_t    context,
                  const void      * _msginfo,
                  const void      * sndbuf,
                  size_t            sndlen,
                  pami_endpoint_t   sender,
                  unsigned          isSync)
{
  MPID_assert(_msginfo != NULL);

  const MPIDI_MsgInfo *msginfo = (const MPIDI_MsgInfo *)_msginfo;
  MPID_Request * rreq = NULL;

  /* -------------------- */
  /*  Match the request.  */
  /* -------------------- */
  unsigned rank       = msginfo->MPIrank;
  unsigned tag        = msginfo->MPItag;
  unsigned context_id = msginfo->MPIctxt;

  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_FDP(rank, tag, context_id);
#else
  pami_task_t source = PAMIX_Endpoint_query(sender);
  rreq = MPIDI_Recvq_FDP(rank, source, tag, context_id, msginfo->MPIseqno);
#endif

  /* Match not found */
  if (unlikely(rreq == NULL))
    {
#if (MPIDI_STATISTICS)
        MPID_NSTAT(mpid_statp->earlyArrivals);
#endif
      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
      MPID_Request *newreq = MPIDI_Request_create2();
      MPID_assert(newreq != NULL);
      if (sndlen)
      {
        newreq->mpid.uebuflen = sndlen;
        newreq->mpid.uebuf = MPIU_Malloc(sndlen);
        MPID_assert(newreq->mpid.uebuf != NULL);
        newreq->mpid.uebuf_malloc = 1;
      }
      MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
#ifndef OUT_OF_ORDER_HANDLING
      rreq = MPIDI_Recvq_FDP(rank, tag, context_id);
#else
      rreq = MPIDI_Recvq_FDP(rank, PAMIX_Endpoint_query(sender), tag, context_id, msginfo->MPIseqno);
#endif
      
      if (unlikely(rreq == NULL))
      {
        MPIDI_Callback_process_unexp(newreq, context, msginfo, sndlen, sender, sndbuf, NULL, isSync);
        /* request is always complete now */
        MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
        MPID_Request_release(newreq);
        goto fn_exit_short;
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

      MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
    }

  /* the receive queue processing has been completed and we found match*/

  /* ---------------------- */
  /*  Copy in information.  */
  /* ---------------------- */
  rreq->status.MPI_SOURCE = rank;
  rreq->status.MPI_TAG    = tag;
  rreq->status.count      = sndlen;
  MPIDI_Request_setCA          (rreq, MPIDI_CA_COMPLETE);
  MPIDI_Request_cpyPeerRequestH(rreq, msginfo);
  MPIDI_Request_setSync        (rreq, isSync);
  MPIDI_Request_setRzv         (rreq, 0);

  /* ----------------------------- */
  /*  Request was already posted.  */
  /* ----------------------------- */
  if (unlikely(isSync))
    MPIDI_SyncAck_post(context, rreq, PAMIX_Endpoint_query(sender));

  if (unlikely(HANDLE_GET_KIND(rreq->mpid.datatype) != HANDLE_KIND_BUILTIN))
    {
      MPIDI_Callback_process_userdefined_dt(context, sndbuf, sndlen, rreq);
      goto fn_exit_short;
    }

  size_t dt_size = rreq->mpid.userbufcount * MPID_Datatype_get_basic_size(rreq->mpid.datatype);

  /* ----------------------------- */
  /*  Test for truncated message.  */
  /* ----------------------------- */
  if (unlikely(sndlen > dt_size))
    {
#if ASSERT_LEVEL > 0
      MPIDI_Callback_process_trunc(context, rreq, NULL, sndbuf);
      goto fn_exit_short;
#else
      sndlen = dt_size;
#endif
    }

  MPID_assert(rreq->mpid.uebuf    == NULL);
  MPID_assert(rreq->mpid.uebuflen == 0);
  void* rcvbuf = rreq->mpid.userbuf;

  if (sndlen > 0)
    memcpy(rcvbuf, sndbuf, sndlen);
#ifdef MPIDI_TRACE
   MPIDI_In_cntr[(PAMIX_Endpoint_query(sender))].R[(rreq->mpid.idx)].rlen=sndlen;
   MPIDI_In_cntr[(PAMIX_Endpoint_query(sender))].R[(rreq->mpid.idx)].comp_in_HH=1;
   MPIDI_In_cntr[(PAMIX_Endpoint_query(sender))].R[(rreq->mpid.idx)].bufadd=rreq->mpid.userbuf;
#endif
  MPIDI_Request_complete(rreq);

#ifdef OUT_OF_ORDER_HANDLING
  MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
  if ((rank != MPI_ANY_SOURCE) && (MPIDI_In_cntr[source].n_OutOfOrderMsgs>0))  {
    MPIDI_Recvq_process_out_of_order_msgs(source, context);
  }
  MPIU_THREAD_CS_EXIT(MSGQUEUE,0);
#endif

 fn_exit_short:
  /* ---------------------------------------- */
  /*  Signal that the recv has been started.  */
  /* ---------------------------------------- */
  MPIDI_Progress_signal();
}


void
MPIDI_RecvShortAsyncCB(pami_context_t    context,
                       void            * cookie,
                       const void      * _msginfo,
                       size_t            msginfo_size,
                       const void      * sndbuf,
                       size_t            sndlen,
                       pami_endpoint_t   sender,
                       pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(msginfo_size == sizeof(MPIDI_MsgInfo));
  MPIDI_RecvShortCB(context,
                    _msginfo,
                    sndbuf,
                    sndlen,
                    sender,
                    0);
}


void
MPIDI_RecvShortSyncCB(pami_context_t    context,
                      void            * cookie,
                      const void      * _msginfo,
                      size_t            msginfo_size,
                      const void      * sndbuf,
                      size_t            sndlen,
                      pami_endpoint_t   sender,
                      pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(msginfo_size == sizeof(MPIDI_MsgInfo));
  MPIDI_RecvShortCB(context,
                    _msginfo,
                    sndbuf,
                    sndlen,
                    sender,
                    1);
}
