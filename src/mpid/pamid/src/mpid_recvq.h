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
 * \file src/mpid_recvq.h
 * \brief Accessors and actors for MPID Requests
 */
#ifndef  __src_pt2pt_mpidi_recvq_h__
#define  __src_pt2pt_mpidi_recvq_h__


struct MPIDI_Recvq_t
{
  struct MPID_Request * posted_head;      /**< \brief The Head of the Posted queue */
  struct MPID_Request * posted_tail;      /**< \brief The Tail of the Posted queue */
  struct MPID_Request * unexpected_head;  /**< \brief The Head of the Unexpected queue */
  struct MPID_Request * unexpected_tail;  /**< \brief The Tail of the Unexpected queue */
};
extern struct MPIDI_Recvq_t MPIDI_Recvq;


#ifndef OUT_OF_ORDER_HANDLING
#define MPIDI_Recvq_append(__Q, __req)                  \
({                                                      \
  /* ---------------------------------------------- */  \
  /*  The tail request should point to the new one  */  \
  /* ---------------------------------------------- */  \
  if (__Q ## _tail != NULL)                             \
      __Q ## _tail->mpid.next = __req;                  \
  else                                                  \
      __Q ## _head = __req;                             \
  /* ------------------------------------------ */      \
  /*  The tail should point to the new request  */      \
  /* ------------------------------------------ */      \
  __Q ## _tail = __req;                                 \
})
#else
#define MPIDI_Recvq_append(__Q, __req)                  \
({                                                      \
  /* ---------------------------------------------- */  \
  /*  The tail request should point to the new one  */  \
  /* ---------------------------------------------- */  \
  if (__Q ## _tail != NULL)  {                          \
      __Q ## _tail->mpid.next = __req;                  \
      __req->mpid.prev = __Q ## _tail;                  \
  }                                                     \
  else {                                                \
      __Q ## _head = __req;                             \
      __req->mpid.prev = NULL;                          \
  }                                                     \
  /* ------------------------------------------ */      \
  /*  The tail should point to the new request  */      \
  /* ------------------------------------------ */      \
  __Q ## _tail = __req;                                 \
})
#endif


#ifndef OUT_OF_ORDER_HANDLING
#define MPIDI_Recvq_remove(__Q, __req, __prev)          \
({                                                      \
  /* --------------------------------------------- */   \
  /*  Patch the next pointers to skip the request  */   \
  /* --------------------------------------------- */   \
  if (__prev != NULL)                                   \
    __prev->mpid.next = __req->mpid.next;               \
  else                                                  \
    __Q ## _head = __req->mpid.next;                    \
  /* ------------------------------------------- */     \
  /*  Set tail pointer if removing the last one  */     \
  /* ------------------------------------------- */     \
  if (__req->mpid.next == NULL)                         \
    __Q ## _tail = __prev;                              \
})
#else
#define MPIDI_Recvq_remove(__Q, __req, __prev)          \
({                                                      \
  /* --------------------------------------------- */   \
  /*  Patch the next pointers to skip the request  */   \
  /* --------------------------------------------- */   \
  if (__prev != NULL) {                                 \
    __prev->mpid.next = __req->mpid.next;               \
  }                                                     \
  else                                                  \
    __Q ## _head = __req->mpid.next;                    \
  /* ------------------------------------------- */     \
  /*  Set tail pointer if removing the last one  */     \
  /* ------------------------------------------- */     \
  if (__req->mpid.next == NULL)                         \
    __Q ## _tail = __prev;                              \
  else                                                  \
    (__req->mpid.next)->mpid.prev = __prev;             \
})
#endif


/**
 * \brief A thread-safe version of MPIDI_Recvq_FU
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \return     1/0 if the matching UE request was found or not
 */
static inline int
MPIDI_Recvq_FU_r(int source, int tag, int context, MPI_Status * status)
{
  int rc = FALSE;
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
  {
    if (likely(!MPIDI_Recvq_empty_uexp()))
    {
      MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
      rc = MPIDI_Recvq_FU(source, tag, context, status);
      MPIU_THREAD_CS_EXIT(MSGQUEUE, 0);
    }
  }
  else
  {
#endif
    if (likely(MPIDI_Recvq.unexpected_head != NULL)) 
    {
      MPIU_THREAD_CS_ENTER(MSGQUEUE,0);
      rc = MPIDI_Recvq_FU(source, tag, context, status);
      MPIU_THREAD_CS_EXIT(MSGQUEUE, 0);
    }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  }
#endif
  return rc;
}


/**
 * \brief Find a request in the unexpected queue and dequeue it, or allocate a new request and enqueue it in the posted queue
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \param[out] foundp     TRUE iff the request was found
 * \return     The matching UE request or the new posted request
 */
#ifndef OUT_OF_ORDER_HANDLING
static inline MPID_Request *
MPIDI_Recvq_FDU_or_AEP(MPID_Request *newreq, int source, int tag, int context_id, int * foundp)
#else
static inline MPID_Request *
MPIDI_Recvq_FDU_or_AEP(MPID_Request *newreq, int source, pami_task_t pami_source, int tag, int context_id, int * foundp)
#endif
{
  MPID_Request * rreq = NULL;
  /* We have unexpected messages, so search unexpected queue */
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
  {
    if (unlikely(!MPIDI_Recvq_empty_uexp()))
    {
#ifndef OUT_OF_ORDER_HANDLING
      rreq = MPIDI_Recvq_FDU(source, tag, context_id, foundp);
#else
      rreq = MPIDI_Recvq_FDU(source, pami_source, tag, context_id, foundp);
#endif
      if (*foundp == TRUE)
        return rreq;
#if (MPIDI_STATISTICS)
      else {
       MPID_NSTAT(mpid_statp->lateArrivals);
      }
#endif
    }
  }
  else
  {
#endif
    if (unlikely(MPIDI_Recvq.unexpected_head != NULL)) {
#ifndef OUT_OF_ORDER_HANDLING
      rreq = MPIDI_Recvq_FDU(source, tag, context_id, foundp);
#else
      rreq = MPIDI_Recvq_FDU(source, pami_source, tag, context_id, foundp);
#endif
      if (*foundp == TRUE)
        return rreq;
#if (MPIDI_STATISTICS)
      else {
       MPID_NSTAT(mpid_statp->lateArrivals);
      }
#endif
    }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  }
#endif
  /* A matching request was not found in the unexpected queue,
     so we need to allocate a new request and add it to the
     posted queue */
  rreq = newreq;
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.msginfo.MPIseqno,-1);
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.length,-1);
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.data,(void *) 0);
  rreq->kind = MPID_REQUEST_RECV;
  MPIDI_Request_setMatch(rreq, tag, source, context_id);
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
    MPIDI_Recvq_insert_post(rreq, source, tag, context_id);
  else
#endif
    MPIDI_Recvq_append(MPIDI_Recvq.posted, rreq);
  *foundp = FALSE;

  return rreq;
}

#if TOKEN_FLOW_CONTROL
typedef struct MPIDI_Token_cntr {
    uint16_t unmatched;          /* no. of unmatched EA messages              */
    uint16_t rettoks;            /* no. of tokens to be returned              */
    int  tokens;                 /* no. of tokens available-pairwise          */
    int  n_tokenStarved;         /* no. of times token starvation occured     */
} MPIDI_Token_cntr_t;

MPIDI_Token_cntr_t  *MPIDI_Token_cntr;
#endif

#ifdef OUT_OF_ORDER_HANDLING

/**
 * data structures that tracks pair-wise in-coming communication.
 */
typedef struct MPIDI_In_cntr {
  uint               n_OutOfOrderMsgs:16; /* the number of out-of-order messages received */
  uint               nMsgs;               /* the number of received messages */
  MPID_Request       *OutOfOrderList;     /* link list of out-of-order messages */
} MPIDI_In_cntr_t;

/**
 * data structures that tracks pair-wise Out-going communication.
 */
typedef struct MPIDI_Out_cntr {
  uint         unmatched:16;             /* the number of un-matched messages */
  uint         nMsgs;                    /* the number of out-going messages */
} MPIDI_Out_cntr_t;

/* global data to keep track of pair-wise communication, storage malloced
during initialization time */
MPIDI_In_cntr_t *MPIDI_In_cntr;
MPIDI_Out_cntr_t *MPIDI_Out_cntr;

#endif


/**
 * \brief Find a request in the posted queue and dequeue it
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \return     The matching posted request or the new UE request
 */
#ifndef OUT_OF_ORDER_HANDLING
static inline MPID_Request *
MPIDI_Recvq_FDP(size_t source, size_t tag, size_t context_id)
#else
static inline MPID_Request *
MPIDI_Recvq_FDP(size_t source, pami_task_t pami_source, int tag, int context_id, int msg_seqno)
#endif
{
  MPID_Request * rreq;
  MPID_Request * prev_rreq = NULL;
#ifdef USE_STATISTICS
  unsigned search_length = 0;
#endif
  TRACE_MEMSET_R(pami_source,msg_seqno,recv_status);

#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  void * it;
  if(MPIDI_Process.queue_binary_search_support_on)
  {
    MPIDI_Recvq_find_in_post(source, tag, context_id, &rreq, &it);
    if(rreq == NULL)
    {
      MPIDI_Recvq_find_in_post(source, MPI_ANY_TAG, context_id, &rreq, &it);
      if(rreq == NULL)
      {
        MPIDI_Recvq_find_in_post(MPI_ANY_SOURCE, tag, context_id, &rreq, &it);
        if(rreq == NULL)
          MPIDI_Recvq_find_in_post(MPI_ANY_SOURCE, MPI_ANY_TAG, context_id, &rreq, &it);
      }
    }
  }
  else
#endif
    rreq = MPIDI_Recvq.posted_head;

#ifdef OUT_OF_ORDER_HANDLING
  MPIDI_In_cntr_t *in_cntr = &MPIDI_In_cntr[pami_source];
  int nMsgs=(in_cntr->nMsgs+1);

  if(msg_seqno == nMsgs) {
        in_cntr->nMsgs = msg_seqno;
  }

  if( ((int)(in_cntr->nMsgs - msg_seqno)) >= 0) {
#endif
  while (rreq != NULL) {
#ifdef USE_STATISTICS
    ++search_length;
#endif

    int match_src = MPIDI_Request_getMatchRank(rreq);
    int match_tag = MPIDI_Request_getMatchTag(rreq);
    int match_ctx = MPIDI_Request_getMatchCtxt(rreq);

    int flag0  = (source == match_src);
    flag0     |= (match_src == MPI_ANY_SOURCE);
    int flag1  = (context_id == match_ctx);
    int flag2  = (tag == match_tag);
    flag2     |= (match_tag == MPI_ANY_TAG);
    int flag   = flag0 & flag1 & flag2;

#if 0
  if ((MPIDI_Request_getMatchRank(rreq)==source || MPIDI_Request_getMatchRank(rreq)==MPI_ANY_SOURCE) &&
      (MPIDI_Request_getMatchCtxt(rreq)==context_id) &&
      (MPIDI_Request_getMatchTag(rreq)  == tag  || MPIDI_Request_getMatchTag(rreq)  == MPI_ANY_TAG)
      )
#else
    if (flag)
#endif
      {
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),req,rreq);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),msgid,msg_seqno);
        TRACE_SET_R_BIT(pami_source,(msg_seqno & SEQMASK),fl.f.posted);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),bufadd,rreq->mpid.userbuf);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),len,rreq->mpid.envelope.length);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rtag,tag);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rctx,context_id);
        TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rsource,pami_source);
        TRACE_SET_R_BIT(pami_source,(msg_seqno & SEQMASK),fl.f.matchedInHH);
        TRACE_SET_REQ_VAL(rreq->mpid.idx,(msg_seqno & SEQMASK));
        TRACE_SET_REQ_VAL(rreq->mpid.partner_id,pami_source);
#ifdef OUT_OF_ORDER_HANDLING
        MPIDI_Request_setPeerRank_pami(rreq, pami_source);
#endif
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
        if(MPIDI_Process.queue_binary_search_support_on)
          MPIDI_Recvq_remove_post(match_src, match_tag, match_ctx, it);
        else
#endif
          MPIDI_Recvq_remove(MPIDI_Recvq.posted, rreq, prev_rreq);
#ifdef USE_STATISTICS
        MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif
        return rreq;
      }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
    if(MPIDI_Process.queue_binary_search_support_on)
      break;
    else
    {
#endif
      prev_rreq = rreq;
      rreq = rreq->mpid.next;
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
    }
#endif
  }
#ifdef OUT_OF_ORDER_HANDLING
  }
#endif

#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif
  return NULL;
}


#ifdef OUT_OF_ORDER_HANDLING
/**
 * insert a request _req2 before _req1
 */
#define MPIDI_Recvq_insert(__Q, __req1, __req2)         \
({                                                      \
  (__req2)->mpid.next = __req1;                         \
  if((__req1)->mpid.prev != NULL)                       \
   ((__req1)->mpid.prev)->mpid.next = (__req2);         \
  else                                                  \
   __Q ## _head = __req2;                               \
  (__req2)->mpid.prev = (__req1)->mpid.prev;            \
  (__req1)->mpid.prev = (__req2);                       \
})

/**
 * remove a request from out of order list
 */
#define MPIDI_Recvq_remove_req_from_ool(req,in_cntr)                        \
({                                                                          \
        in_cntr->n_OutOfOrderMsgs--;                                        \
        if (in_cntr->n_OutOfOrderMsgs == 0) {                               \
            in_cntr->OutOfOrderList=NULL;                                   \
            req->mpid.nextR=NULL;                                           \
            req->mpid.prevR=NULL;                                           \
        } else if (in_cntr->n_OutOfOrderMsgs > 0) {                         \
          in_cntr->OutOfOrderList=req->mpid.nextR;                          \
          /* remove req from out of order list */                           \
          ((MPID_Request *)(req)->mpid.prevR)->mpid.nextR = (req)->mpid.nextR; \
          ((MPID_Request *)(req)->mpid.nextR)->mpid.prevR = (req)->mpid.prevR; \
            (req)->mpid.nextR=NULL;                                         \
            (req)->mpid.prevR=NULL;                                         \
        }                                                                   \
})
#endif


#endif
