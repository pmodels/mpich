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
 * \file src/mpid_recvq.c
 * \brief Functions to manage the Receive Queues
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpidimpl.h>
#include "mpid_recvq.h"

/**
 * \defgroup MPID_RECVQ MPID Receive Queue management
 *
 * Functions to manage the Receive Queues
 */


/** \brief Structure to group the common recvq pointers */
struct MPIDI_Recvq_t MPIDI_Recvq;

#ifdef HAVE_DEBUGGER_SUPPORT
struct MPID_Request ** const MPID_Recvq_posted_head_ptr =
  &MPIDI_Recvq.posted_head;
struct MPID_Request ** const MPID_Recvq_unexpected_head_ptr =
  &MPIDI_Recvq.unexpected_head;
#endif /* HAVE_DEBUGGER_SUPPORT */


/**
 * \brief Set up the request queues
 */
void
MPIDI_Recvq_init()
{
  MPIDI_Recvq.posted_head = NULL;
  MPIDI_Recvq.posted_tail = NULL;
  MPIDI_Recvq.unexpected_head = NULL;
  MPIDI_Recvq.unexpected_tail = NULL;
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
    MPIDI_Recvq_init_queues();
#endif
}


/**
 * \brief Tear down the request queues
 */
void
MPIDI_Recvq_finalize()
{
  if(MPIDI_Process.statistics) MPIDI_Recvq_DumpQueues(MPIDI_Process.verbose);
}


/**
 * \brief Find a request in the unexpected queue
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \return     1/0 if the matching UE request was found or not
 */
int
MPIDI_Recvq_FU(int source, int tag, int context_id, MPI_Status * status)
{
  MPID_Request * rreq;
  int found = FALSE;
  void * it;
#ifdef USE_STATISTICS
  unsigned search_length = 0;
#endif
#ifdef OUT_OF_ORDER_HANDLING
  MPIDI_In_cntr_t *in_cntr;
  uint nMsgs=0;
  pami_task_t pami_source;
#endif

  if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      if(MPIDI_Process.queue_binary_search_support_on)
      {
#ifndef OUT_OF_ORDER_HANDLING
        MPIDI_Recvq_find_in_uexp(source, tag, context_id, &rreq, &it);
#else
        MPIDI_Recvq_find_in_uexp(source, tag, context_id, -1, &rreq, &it);
#endif
        if (rreq != NULL) {
  #ifdef OUT_OF_ORDER_HANDLING
          pami_source= MPIDI_Request_getPeerRank_pami(rreq);
          in_cntr=&MPIDI_In_cntr[pami_source];
          nMsgs = in_cntr->nMsgs + 1;
          if( ((int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0 )
          {
            if (rreq->mpid.nextR != NULL) {  /* recv is in the out of order list */
               if (MPIDI_Request_getMatchSeq(rreq) == nMsgs) {
                   in_cntr->nMsgs=nMsgs;
                   MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
               } 
            } 
  #endif
            found = TRUE;
            if(status != MPI_STATUS_IGNORE)
              *status = (rreq->status);
#ifdef OUT_OF_ORDER_HANDLING
          }
          else
          {
            MPIDI_Recvq_find_in_uexp(source, tag, context_id, nMsgs, &rreq, &it);
            if (rreq != NULL) {
              if (rreq->mpid.nextR != NULL) {  /* recv is in the out of order list */
                in_cntr->nMsgs=nMsgs;
                MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
              }
              found = TRUE;
              if(status != MPI_STATUS_IGNORE)
                *status = (rreq->status);
            }
          }
#endif /* OUT_OF_ORDER_HANDLING */
        }
      }
      else
      {
#endif /* QUEUE_BINARY_SEARCH_SUPPORT */
        rreq = MPIDI_Recvq.unexpected_head;
        while (rreq != NULL) {
#ifdef USE_STATISTICS
          ++search_length;
#endif
          if ( (MPIDI_Request_getMatchCtxt(rreq) == context_id) &&
               (MPIDI_Request_getMatchRank(rreq) == source    ) &&
               (MPIDI_Request_getMatchTag(rreq)  == tag       )
               )
            {
  #ifdef OUT_OF_ORDER_HANDLING
              pami_source= MPIDI_Request_getPeerRank_pami(rreq);
              in_cntr=&MPIDI_In_cntr[pami_source];
              nMsgs = in_cntr->nMsgs + 1;
              if( ((int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0 )
              {
                if (rreq->mpid.nextR != NULL) {  /* recv is in the out of order list */
                   if (MPIDI_Request_getMatchSeq(rreq) == nMsgs) {
                       in_cntr->nMsgs=nMsgs;
                       MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
                   } 
                } 
  #endif
              found = TRUE;
              if(status != MPI_STATUS_IGNORE)
                *status = (rreq->status);
              break;
#ifdef OUT_OF_ORDER_HANDLING
               }
#endif
          }
          rreq = rreq->mpid.next;
        }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      }
#endif /* QUEUE_BINARY_SEARCH_SUPPORT */
    }
  else
    {
      MPIDI_Message_match match;
      MPIDI_Message_match mask;

      match.context_id = context_id;
      mask.context_id = ~0;
      if (tag == MPI_ANY_TAG)
        {
          match.tag = 0;
          mask.tag = 0;
        }
      else
        {
          match.tag = tag;
          mask.tag = ~0;
        }
      if (source == MPI_ANY_SOURCE)
        {
          match.rank = 0;
          mask.rank = 0;
        }
      else
        {
          match.rank = source;
          mask.rank = ~0;
        }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      if(MPIDI_Process.queue_binary_search_support_on)
      {
#ifndef OUT_OF_ORDER_HANDLING
        MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, &rreq, &it);
#else
        MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, -1, &rreq, &it);
#endif
        if (rreq != NULL) {
#ifdef OUT_OF_ORDER_HANDLING
          pami_source= MPIDI_Request_getPeerRank_pami(rreq);
          in_cntr=&MPIDI_In_cntr[pami_source];
          nMsgs = in_cntr->nMsgs + 1;
          if(( ( (int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0) || (source == MPI_ANY_SOURCE)) {
            if(source == MPI_ANY_SOURCE) {
              if((int) (nMsgs-MPIDI_Request_getMatchSeq(rreq)) < 0 )
                goto NEXT_MSG2;
            }
            if (rreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
              if (MPIDI_Request_getMatchSeq(rreq) == nMsgs)
                in_cntr->nMsgs=nMsgs;
              MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
            }
#endif
            found = TRUE;
            if(status != MPI_STATUS_IGNORE) 
              *status = (rreq->status);
#ifdef OUT_OF_ORDER_HANDLING
          }
          else
          {
     NEXT_MSG2: MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, nMsgs, &rreq, &it);
             if (rreq != NULL) {
               if (rreq->mpid.nextR != NULL) {  /* recv is in the out of order list */
                 in_cntr->nMsgs=nMsgs;
                 MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
               }
               found = TRUE;
               if(status != MPI_STATUS_IGNORE)
                 *status = (rreq->status);
             }
          }
#endif /* OUT_OF_ORDER_HANDLING */
        }
      }
      else
      {
#endif /* QUEUE_BINARY_SEARCH_SUPPORT */
        rreq = MPIDI_Recvq.unexpected_head;
        while (rreq != NULL) {
#ifdef USE_STATISTICS
          ++search_length;
#endif
          if ( (  MPIDI_Request_getMatchCtxt(rreq)              == match.context_id) &&
               ( (MPIDI_Request_getMatchRank(rreq) & mask.rank) == match.rank      ) &&
               ( (MPIDI_Request_getMatchTag(rreq)  & mask.tag ) == match.tag       )
               )
            {
#ifdef OUT_OF_ORDER_HANDLING
              pami_source= MPIDI_Request_getPeerRank_pami(rreq);
              in_cntr=&MPIDI_In_cntr[pami_source];
              nMsgs = in_cntr->nMsgs + 1;
              if(( ( (int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0) || (source == MPI_ANY_SOURCE)) {
                 if(source == MPI_ANY_SOURCE) {
                   if((int) (nMsgs-MPIDI_Request_getMatchSeq(rreq)) < 0 )
                      goto NEXT_MSG;
                 }
              if (rreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
                if (MPIDI_Request_getMatchSeq(rreq) == nMsgs)
                  in_cntr->nMsgs=nMsgs;
                MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
              }
#endif
              found = TRUE;
              if(status != MPI_STATUS_IGNORE) 
                *status = (rreq->status);
              break;
#ifdef OUT_OF_ORDER_HANDLING
             }
#endif
          }
     NEXT_MSG:
          rreq = rreq->mpid.next;
        }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      }
#endif
    }

#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

  return found;
}


/**
 * \brief Find a request in the unexpected queue and dequeue it
 * \param[in]  req        Find by address of request object on sender
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \return     The matching UE request or NULL
 */
MPID_Request *
MPIDI_Recvq_FDUR(MPI_Request req, int source, int tag, int context_id)
{
  MPID_Request * prev_rreq          = NULL; /* previous request in queue */
  MPID_Request * cur_rreq           = NULL; /* current request in queue */
  MPID_Request * matching_cur_rreq  = NULL; /* matching request in queue */
  MPID_Request * matching_prev_rreq = NULL; /* previous in queue to match */
  void         * it                 = NULL;
#ifdef USE_STATISTICS
  unsigned search_length = 0;
#endif

  /* ----------------------- */
  /* first we do the finding */
  /* ----------------------- */
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
  {
#ifndef OUT_OF_ORDER_HANDLING
    MPIDI_Recvq_find_in_uexp(source, tag, context_id, &cur_rreq, &it);
#else
    /* Passing -1 for seqno because I don't care about it here */
    MPIDI_Recvq_find_in_uexp(source, tag, context_id, -1, &cur_rreq, &it);
#endif
    if (cur_rreq != NULL) {
      if (MPIDI_Request_getPeerRequestH(cur_rreq) == req) 
      {
        matching_prev_rreq = prev_rreq;
        matching_cur_rreq  = cur_rreq;
      }
    }
  }
  else
  {
#endif
    cur_rreq = MPIDI_Recvq.unexpected_head;
    while (cur_rreq != NULL) {
#ifdef USE_STATISTICS
      ++search_length;
#endif
      if (MPIDI_Request_getPeerRequestH(cur_rreq) == req        &&
          MPIDI_Request_getMatchCtxt(cur_rreq)    == context_id &&
          MPIDI_Request_getMatchRank(cur_rreq)    == source     &&
          MPIDI_Request_getMatchTag(cur_rreq)     == tag)
        {
          matching_prev_rreq = prev_rreq;
          matching_cur_rreq  = cur_rreq;
          break;
        }
      prev_rreq = cur_rreq;
      cur_rreq  = cur_rreq->mpid.next;
    }
#ifdef  QUEUE_BINARY_SEARCH_SUPPORT
  }
#endif

  /* ----------------------- */
  /* found nothing; return   */
  /* ----------------------- */
  if (matching_cur_rreq == NULL)
    goto fn_exit;
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
  {
#ifndef OUT_OF_ORDER_HANDLING
    MPIDI_Recvq_remove_uexp(source, tag, context_id, it);
#else
    MPIDI_Recvq_remove_uexp(source, tag, context_id, MPIDI_Request_getMatchSeq(cur_rreq),it);
#endif
  }
  else
#endif
    MPIDI_Recvq_remove(MPIDI_Recvq.unexpected, matching_cur_rreq, matching_prev_rreq);

 fn_exit:
#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

  return matching_cur_rreq;
}


/**
 * \brief Out of band part of find a request in the unexpected queue and dequeue it, or allocate a new request and enqueue it in the posted queue
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \param[out] foundp    TRUE iff the request was found
 * \return     The matching UE request or the new posted request
 */
#ifndef OUT_OF_ORDER_HANDLING
MPID_Request *
MPIDI_Recvq_FDU(int source, int tag, int context_id, int * foundp)
#else
MPID_Request *
MPIDI_Recvq_FDU(int source, pami_task_t pami_source, int tag, int context_id, int * foundp)
#endif
{
  int found = FALSE;
  MPID_Request * rreq = NULL;
  MPID_Request * prev_rreq;
  void         * it;
#ifdef USE_STATISTICS
  unsigned search_length = 0;
#endif
#ifdef OUT_OF_ORDER_HANDLING
  MPIDI_In_cntr_t *in_cntr;
  uint nMsgs=0;

  if(pami_source != MPI_ANY_SOURCE) {
    in_cntr=&MPIDI_In_cntr[pami_source];
    nMsgs = in_cntr->nMsgs + 1;
  }
#endif

  //This function is typically called when there are unexp recvs
  if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      if(MPIDI_Process.queue_binary_search_support_on)
      {
#ifdef OUT_OF_ORDER_HANDLING
        MPIDI_Recvq_find_in_uexp(source, tag, context_id, nMsgs, &rreq, &it);
#else
        MPIDI_Recvq_find_in_uexp(source, tag, context_id, &rreq, &it);
#endif
        if (rreq != NULL) {
#ifdef OUT_OF_ORDER_HANDLING
          if(rreq->mpid.nextR != NULL) {       /* recv is in the out of order list */
            if (MPIDI_Request_getMatchSeq(rreq) == nMsgs) {
              in_cntr->nMsgs=nMsgs;
            }
            MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
          }
          MPIDI_Recvq_remove_uexp(source, tag, context_id, MPIDI_Request_getMatchSeq(rreq),it);
#else
          MPIDI_Recvq_remove_uexp(source, tag, context_id, it);
#endif
          found = TRUE;
#ifdef MPIDI_TRACE
          MPIDI_Trace_buf[(rreq->mpid.partner_id)].R[(rreq->mpid.idx)].matchedInUQ2=1;
#endif
        }
      }
      else
      {
#endif
        prev_rreq = NULL;
        rreq = MPIDI_Recvq.unexpected_head;
        while (rreq != NULL) {
#ifdef USE_STATISTICS
          ++search_length;
#endif
#ifdef OUT_OF_ORDER_HANDLING
          if( ((int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0 ) {
#endif
            if ( (MPIDI_Request_getMatchCtxt(rreq) == context_id) &&
                 (MPIDI_Request_getMatchRank(rreq) == source    ) &&
                 (MPIDI_Request_getMatchTag(rreq)  == tag       )
               )
            {
#ifdef OUT_OF_ORDER_HANDLING
              if(rreq->mpid.nextR != NULL) {       /* recv is in the out of order list */
                if (MPIDI_Request_getMatchSeq(rreq) == nMsgs) {
                  in_cntr->nMsgs=nMsgs;
                }
                MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
              }
#endif
            MPIDI_Recvq_remove(MPIDI_Recvq.unexpected, rreq, prev_rreq);
            found = TRUE;
            TRACE_SET_R_BIT((rreq->mpid.partner_id),(rreq->mpid.idx),fl.f.matchedInUQ2);
            goto fn_exit;
          }
#ifdef OUT_OF_ORDER_HANDLING
          }
#endif

          prev_rreq = rreq;
          rreq = rreq->mpid.next;
        }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      }
#endif
    }
  else
    {
      MPIDI_Message_match match;
      MPIDI_Message_match mask;

      match.context_id = context_id;
      mask.context_id = ~0;
      if (tag == MPI_ANY_TAG)
        {
          match.tag = 0;
          mask.tag = 0;
        }
      else
        {
          match.tag = tag;
          mask.tag = ~0;
        }
      if (source == MPI_ANY_SOURCE)
        {
          match.rank = 0;
          mask.rank = 0;
        }
      else
        {
          match.rank = source;
          mask.rank = ~0;
        }

#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      if(MPIDI_Process.queue_binary_search_support_on)
      {
#ifndef OUT_OF_ORDER_HANDLING
        MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, &rreq, &it);
#else
        MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, -1, &rreq, &it);
#endif
        if (rreq != NULL) {
#ifdef OUT_OF_ORDER_HANDLING
          if(( ( (int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0) || (source == MPI_ANY_SOURCE)) {
            if(source == MPI_ANY_SOURCE) {
              in_cntr = &MPIDI_In_cntr[MPIDI_Request_getPeerRank_pami(rreq)];
              nMsgs = in_cntr->nMsgs+1;
              if((int) (nMsgs-MPIDI_Request_getMatchSeq(rreq)) < 0 )
                 goto NEXT_MSG2;
            }
            if (rreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
              if (MPIDI_Request_getMatchSeq(rreq) == nMsgs)
                in_cntr->nMsgs=nMsgs;
              MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
            }
            MPIDI_Recvq_remove_uexp(MPIDI_Request_getMatchRank(rreq), MPIDI_Request_getMatchTag(rreq), match.context_id, MPIDI_Request_getMatchSeq(rreq),it);
#else /* OUT_OF_ORDER_HANDLING */
            MPIDI_Recvq_remove_uexp(MPIDI_Request_getMatchRank(rreq), MPIDI_Request_getMatchTag(rreq), match.context_id, it);
#endif/* OUT_OF_ORDER_HANDLING */
            found = TRUE;
            goto fn_exit;
#ifdef OUT_OF_ORDER_HANDLING
          }
     NEXT_MSG2:
          MPIDI_Recvq_find_in_uexp(source, tag, match.context_id, nMsgs, &rreq, &it);
          if (rreq != NULL) {
            if (rreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
              if (MPIDI_Request_getMatchSeq(rreq) == nMsgs)
                in_cntr->nMsgs=nMsgs;
              MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
            }
            MPIDI_Recvq_remove_uexp(MPIDI_Request_getMatchRank(rreq), MPIDI_Request_getMatchTag(rreq), match.context_id, MPIDI_Request_getMatchSeq(rreq),it);
          }
#endif /* OUT_OF_ORDER_HANDLING */

        }
      }
      else
      {
#endif
        prev_rreq = NULL;
        rreq = MPIDI_Recvq.unexpected_head;
        while (rreq != NULL) {
#ifdef USE_STATISTICS
          ++search_length;
#endif
#ifdef OUT_OF_ORDER_HANDLING
          if(( ( (int)(nMsgs-MPIDI_Request_getMatchSeq(rreq))) >= 0) || (source == MPI_ANY_SOURCE)) {
#endif
            if ( (  MPIDI_Request_getMatchCtxt(rreq)              == match.context_id) &&
                 ( (MPIDI_Request_getMatchRank(rreq) & mask.rank) == match.rank      ) &&
                 ( (MPIDI_Request_getMatchTag(rreq)  & mask.tag ) == match.tag       )
                 )
            {
#ifdef OUT_OF_ORDER_HANDLING
              if(source == MPI_ANY_SOURCE) {
                in_cntr = &MPIDI_In_cntr[MPIDI_Request_getPeerRank_pami(rreq)];
                nMsgs = in_cntr->nMsgs+1;
                if((int) (nMsgs-MPIDI_Request_getMatchSeq(rreq)) < 0 )
                   goto NEXT_MSG;
              }
              if (rreq->mpid.nextR != NULL)  { /* recv is in the out of order list */
                if (MPIDI_Request_getMatchSeq(rreq) == nMsgs)
                  in_cntr->nMsgs=nMsgs;
                MPIDI_Recvq_remove_req_from_ool(rreq,in_cntr);
              }
#endif
              MPIDI_Recvq_remove(MPIDI_Recvq.unexpected, rreq, prev_rreq);
              found = TRUE;
              goto fn_exit;
            }
#ifdef OUT_OF_ORDER_HANDLING
          }
       NEXT_MSG:
#endif
          prev_rreq = rreq;
          rreq = rreq->mpid.next;
        }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
      }
#endif
    }

 fn_exit:
#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

  *foundp = found;
  return rreq;
}


/**
 * \brief Find a request in the posted queue and dequeue it
 * \param[in]  req        Find by address of request object on sender
 * \return     The matching posted request or NULL
 */
int
MPIDI_Recvq_FDPR(MPID_Request * req)
{
  MPID_Request * cur_rreq  = NULL;
  MPID_Request * prev_rreq = NULL;
  int found = FALSE;
#ifdef USE_STATISTICS
  unsigned search_length = 0;
#endif

#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  void * it;
  if(MPIDI_Process.queue_binary_search_support_on)
  {
    MPIDI_Recvq_find_in_post(MPIDI_Request_getMatchRank(req), MPIDI_Request_getMatchTag(req), MPIDI_Request_getMatchCtxt(req), &cur_rreq, &it);
    if (cur_rreq != NULL) {
      if (cur_rreq == req)
      {
        MPIDI_Recvq_remove_post(MPIDI_Request_getMatchRank(req), MPIDI_Request_getMatchTag(req), MPIDI_Request_getMatchCtxt(req),it);
        found = TRUE;
      }
    }
  }
  else
  {
#endif /* QUEUE_BINARY_SEARCH_SUPPORT */
    cur_rreq = MPIDI_Recvq.posted_head;

    while (cur_rreq != NULL) {
#ifdef USE_STATISTICS
      ++search_length;
#endif
      if (cur_rreq == req)
      {
        MPIDI_Recvq_remove(MPIDI_Recvq.posted, cur_rreq, prev_rreq);
        found = TRUE;
        break;
      }

      prev_rreq = cur_rreq;
      cur_rreq = cur_rreq->mpid.next;
    }
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  }
#endif
#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.posted_search, search_length);
#endif

  return found;
}


/**
 * \brief Find a request in the posted queue and dequeue it, or allocate a new request and enqueue it in the unexpected queue
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \param[out] foundp    TRUE iff the request was found
 * \return     The matching posted request or the new UE request
 */
#ifndef OUT_OF_ORDER_HANDLING
MPID_Request *
MPIDI_Recvq_FDP_or_AEU(MPID_Request *newreq, int source, int tag, int context_id, int * foundp)
#else
MPID_Request *
MPIDI_Recvq_FDP_or_AEU(MPID_Request *newreq, int source, pami_task_t pami_source, int tag, int context_id, int msg_seqno, int * foundp)
#endif
{
  MPID_Request * rreq;
  int found = FALSE;

#ifndef OUT_OF_ORDER_HANDLING
  rreq = MPIDI_Recvq_FDP(source, tag, context_id);
#else
  rreq = MPIDI_Recvq_FDP(source, pami_source, tag, context_id, msg_seqno);
#endif

  if (rreq != NULL) {
      found = TRUE;
  } else {
#ifndef OUT_OF_ORDER_HANDLING
      rreq = MPIDI_Recvq_AEU(newreq, source, tag, context_id);
#else
      rreq = MPIDI_Recvq_AEU(newreq, source, pami_source, tag, context_id, msg_seqno);
#endif
  }
  *foundp = found;
  return rreq;
}


/**
 * \brief Allocate a new request and enqueue it in the unexpected queue
 * \param[in]  source     Find by Sender
 * \param[in]  tag        Find by Tag
 * \param[in]  context_id Find by Context ID (communicator)
 * \return     The matching posted request or the new UE request
 */
#ifndef OUT_OF_ORDER_HANDLING
MPID_Request *
MPIDI_Recvq_AEU(MPID_Request *newreq, int source, int tag, int context_id)
#else
MPID_Request *
MPIDI_Recvq_AEU(MPID_Request *newreq, int source, pami_task_t pami_source, int tag, int context_id, int msg_seqno)
#endif
{
  /* A matching request was not found in the posted queue, so we
     need to allocate a new request and add it to the unexpected
     queue */
  MPID_Request *rreq;
  rreq = newreq;
  rreq->kind = MPID_REQUEST_RECV;
  TRACE_MEMSET_R(pami_source,msg_seqno,recv_status);
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.msginfo.MPIseqno,-1);
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.length,-1);
  TRACE_SET_REQ_VAL(rreq->mpid.envelope.data,(void *) 0);
#ifndef OUT_OF_ORDER_HANDLING
  MPIDI_Request_setMatch(rreq, tag, source, context_id);
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
    MPIDI_Recvq_insert_uexp((void*)rreq, source, tag, context_id);
  else
#endif
    MPIDI_Recvq_append(MPIDI_Recvq.unexpected, rreq);
#else /* OUT_OF_ORDER_HANDLING */
  MPID_Request *q;
  MPIDI_In_cntr_t *in_cntr;
  int insert, i;

  in_cntr = &MPIDI_In_cntr[pami_source];
  MPIDI_Request_setMatch(rreq, tag, source, context_id); /* mpi rank needed */
  MPIDI_Request_setPeerRank_pami(rreq, pami_source);
  MPIDI_Request_setPeerRank_comm(rreq, source);
  MPIDI_Request_setMatchSeq(rreq, msg_seqno);
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  if(MPIDI_Process.queue_binary_search_support_on)
  { 
    MPIDI_Recvq_insert_uexp((void*)rreq, source, tag, context_id, msg_seqno);
  }
  else
  {
#endif
    if (!in_cntr->n_OutOfOrderMsgs) {
      MPIDI_Recvq_append(MPIDI_Recvq.unexpected, rreq);
    } else {
      q=in_cntr->OutOfOrderList;
      insert=0;
      for (i=1; i<=in_cntr->n_OutOfOrderMsgs; i++) {
        if ( context_id == MPIDI_Request_getMatchCtxt(q)) {
          if (((int)(msg_seqno - MPIDI_Request_getMatchSeq(q))) < 0) {
             MPIDI_Recvq_insert(MPIDI_Recvq.unexpected, q, rreq);
             insert=1;
             break;
          }
        }
        q=q->mpid.nextR;
      }
      if (!insert) {
        MPIDI_Recvq_append(MPIDI_Recvq.unexpected, rreq);
      }
    }
   TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),req,rreq);
   TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),msgid,msg_seqno);
   TRACE_SET_R_BIT(pami_source,(msg_seqno & SEQMASK),fl.f.ool);
   TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rtag,tag);
   TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rctx,context_id);
   TRACE_SET_REQ_VAL(rreq->mpid.idx,(msg_seqno & SEQMASK));
   TRACE_SET_R_VAL(pami_source,(msg_seqno & SEQMASK),rsource,pami_source);
   TRACE_SET_REQ_VAL(rreq->mpid.partner_id,pami_source);
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  }
#endif

  if (((int)(in_cntr->nMsgs - msg_seqno)) < 0) { /* seqno > nMsgs, out of order */
    MPIDI_Recvq_enqueue_ool(pami_source,rreq);
  }
#endif/* OUT_OF_ORDER_HANDLING */

  return rreq;
}


/**
 * \brief Dump the queues
 */
void
MPIDI_Recvq_DumpQueues(int verbose)
{
  if(verbose < MPIDI_VERBOSE_SUMMARY_ALL)
    return;

  MPID_Request * rreq = MPIDI_Recvq.posted_head;
  unsigned i=0, numposted=0, numue=0;
  unsigned postedbytes=0, uebytes=0;

  if(verbose >= MPIDI_VERBOSE_DETAILS_ALL)
    fprintf(stderr,"Posted Queue:\n-------------\n");
  while (rreq != NULL) {
    if(verbose >= MPIDI_VERBOSE_DETAILS_ALL)
      fprintf(stderr, "P %d: MPItag=%d MPIrank=%d ctxt=%d count=%d\n",
              i++,
              MPIDI_Request_getMatchTag(rreq),
              MPIDI_Request_getMatchRank(rreq),
              MPIDI_Request_getMatchCtxt(rreq),
              rreq->mpid.userbufcount
              );
    numposted++;
    postedbytes+=rreq->mpid.userbufcount;
    rreq = rreq->mpid.next;
  }
  fprintf(stderr, "Posted Requests %d, Total Mem: %d bytes\n",
          numposted, postedbytes);


  i=0;
  rreq = MPIDI_Recvq.unexpected_head;
  if(verbose >= MPIDI_VERBOSE_DETAILS_ALL)
    fprintf(stderr, "Unexpected Queue:\n-----------------\n");
  while (rreq != NULL) {
    if(verbose >= MPIDI_VERBOSE_DETAILS_ALL)
#ifndef OUT_OF_ORDER_HANDLING
      fprintf(stderr, "UE %d: MPItag=%d MPIrank=%d ctxt=%d uebuf=%p uebuflen=%u\n",
              i++,
              MPIDI_Request_getMatchTag(rreq),
              MPIDI_Request_getMatchRank(rreq),
              MPIDI_Request_getMatchCtxt(rreq),
              rreq->mpid.uebuf,
              rreq->mpid.uebuflen);
#else
      fprintf(stderr, "UE %d: MPItag=%d MPIrank=%d pami_task_id=%d MPIseq=%d ctxt=%d uebuf=%p uebuflen=%u\n",
              i++,
              MPIDI_Request_getMatchTag(rreq),
              MPIDI_Request_getMatchRank(rreq),
              MPIDI_Request_getPeerRank_pami(rreq),
              MPIDI_Request_getMatchSeq(rreq),
              MPIDI_Request_getMatchCtxt(rreq),
              rreq->mpid.uebuf,
              rreq->mpid.uebuflen);
#endif
    numue++;
    uebytes+=rreq->mpid.uebuflen;
    rreq = rreq->mpid.next;
  }
  fprintf(stderr, "Unexpected Requests %d, Total Mem: %d bytes\n",
          numue, uebytes);
}


#ifdef OUT_OF_ORDER_HANDLING
/**
 * Insert a request in the OutOfOrderList, make sure this list is
 * arranged in the ascending order.
 */
void MPIDI_Recvq_enqueue_ool(pami_task_t src, MPID_Request *req)
{
  MPID_Request  *q;
  void *head;
  int insert,i;
  MPIDI_In_cntr_t *in_cntr;

  in_cntr=&MPIDI_In_cntr[src];
  if (in_cntr->n_OutOfOrderMsgs != 0) {
    head=in_cntr->OutOfOrderList;
    q=in_cntr->OutOfOrderList;
    insert=0;
    MPID_assert(q->mpid.nextR != NULL);
    while(q->mpid.nextR != head) {
      if (((int)(MPIDI_Request_getMatchSeq(q) - MPIDI_Request_getMatchSeq(req))) > 0) {
        insert=1;
        break;
      }
      q=q->mpid.nextR;
    }
    if (insert) {
      MPIDI_Recvq_insert_ool(q,req);
      if (q == head) { /* 1st element in the list */
        in_cntr->OutOfOrderList=req;
      }
    } else {
      if (((int)(MPIDI_Request_getMatchSeq(q) - MPIDI_Request_getMatchSeq(req))) > 0) {
        MPIDI_Recvq_insert_ool(q,req);
        if (q == head) { /* 1st element in the list */
          in_cntr->OutOfOrderList=req;
        }
      } else {
        MPIDI_Recvq_insert_ool((MPID_Request *)q->mpid.nextR,req);
      }
    }
  } else {   /*  empty list    */
    in_cntr->OutOfOrderList=req;
    req->mpid.prevR=req;
    req->mpid.nextR=req;
  }
  in_cntr->n_OutOfOrderMsgs++;
#if (MPIDI_STATISTICS)
  MPID_NSTAT(mpid_statp->unorderedMsgs);
#endif
} /* void MPIDI_Recvq_insert_ool(pami_task_t src, MPID_Request *N) */


/**
 *  MPIDI_Recvq_insert_ool: place e between q and q->prevR
 *
 */
void  MPIDI_Recvq_insert_ool(MPID_Request *q,MPID_Request *e)
{
  (e)->mpid.prevR = (q)->mpid.prevR;
  ((MPID_Request *)((q)->mpid.prevR))->mpid.nextR = (e);
  (e)->mpid.nextR = (q);
  (q)->mpid.prevR = (e);
}
#endif
