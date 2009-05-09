/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/impl/mpid_recvq.c
 * \brief Functions to manage the Receive Queues
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/**
 * \defgroup MPID_RECVQ MPID Receive Queue management
 *
 * Functions to manage the Receive Queues
 */

/** \brief Unused lock macro to protect the receive queues */
#define MPIDI_Recvq_lock()   // MPID_Thread_lock(&MPIDI_Process.recvq_mutex)
/** \brief Unused unlock macro to protect the receive queues */
#define MPIDI_Recvq_unlock() // MPID_Thread_unlock(&MPIDI_Process.recvq_mutex)


/** \brief Structure to group the common recvq pointers */
static struct MPIDID_Recvq_t
{
  struct MPID_Request * posted_head;      /**< \brief The Head of the Posted queue */
  struct MPID_Request * posted_tail;      /**< \brief The Tail of the Posted queue */
  struct MPID_Request * unexpected_head;  /**< \brief The Head of the Unexpected queue */
  struct MPID_Request * unexpected_tail;  /**< \brief The Tail of the Unexpected queue */
} recvq;

#ifdef HAVE_DEBUGGER_SUPPORT
struct MPID_Request ** const MPID_Recvq_posted_head_ptr =
					&recvq.posted_head;
struct MPID_Request ** const MPID_Recvq_unexpected_head_ptr =
					&recvq.unexpected_head;
#endif /* HAVE_DEBUGGER_SUPPORT */


/**
 * \brief Set up the request queues
 */
void MPIDI_Recvq_init()
{
  recvq.posted_head = NULL;
  recvq.posted_tail = NULL;
  recvq.unexpected_head = NULL;
  recvq.unexpected_tail = NULL;
}


/**
 * \brief Tear down the request queues
 */
void MPIDI_Recvq_finalize()
{
  MPIDI_Recvq_DumpQueues(MPIDI_Process.verbose);
}


/**
 * \brief Find a request in the unexpected queue
 * \param [in]  source     Find by Sender
 * \param [in]  tag        Find by Tag
 * \param [in]  context_id Find by Context ID (communicator)
 * \return      The matching UE request or NULL
 */
int MPIDI_Recvq_FU(int source, int tag, int context_id, MPI_Status * status)
{
    MPID_Request * rreq;
    int found = 0;
#ifdef USE_STATISTICS
    unsigned search_length = 0;
#endif

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
        MPIDI_Recvq_lock();
        {
            rreq = recvq.unexpected_head;
            while(rreq != NULL)
            {
#ifdef USE_STATISTICS
                ++search_length;
#endif
                if ( (MPID_Request_getMatchCtxt(rreq) == context_id) &&
                     (MPID_Request_getMatchRank(rreq) == source    ) &&
                     (MPID_Request_getMatchTag(rreq)  == tag       )
                   )
                  {
                    found = 1;
                    *status = (rreq->status);
                    break;
                  }

                rreq = rreq->dcmf.next;
            }
        }
        MPIDI_Recvq_unlock();
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

        MPIDI_Recvq_lock();
        {
            rreq = recvq.unexpected_head;
            while (rreq != NULL)
            {
#ifdef USE_STATISTICS
                ++search_length;
#endif
                if ( (  MPID_Request_getMatchCtxt(rreq)              == match.context_id) &&
                     ( (MPID_Request_getMatchRank(rreq) & mask.rank) == match.rank      ) &&
                     ( (MPID_Request_getMatchTag(rreq)  & mask.tag ) == match.tag       )
                   )
                  {
                    found = 1;
                    *status = (rreq->status);
                    break;
                  }

                rreq = rreq->dcmf.next;
            }
        }
        MPIDI_Recvq_unlock();
    }

#ifdef USE_STATISTICS
    MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

    return found;
}


/**
 * \brief Find a request in the unexpected queue and dequeue it
 * \param [in]  req        Find by address of request object on sender
 * \param [in]  source     Find by Sender
 * \param [in]  tag        Find by Tag
 * \param [in]  context_id Find by Context ID (communicator)
 * \return      The matching UE request or NULL
 */
MPID_Request * MPIDI_Recvq_FDUR (MPID_Request * req, int source, int tag, int context_id)
{
  MPID_Request * prev_rreq          = NULL; /* previous request in queue */
  MPID_Request * cur_rreq           = NULL; /* current request in queue */
  MPID_Request * matching_cur_rreq  = NULL; /* matching request in queue */
  MPID_Request * matching_prev_rreq = NULL; /* previous in queue to match */
#ifdef USE_STATISTICS
    unsigned search_length = 0;
#endif

  /* ----------------------- */
  /* first we do the finding */
  /* ----------------------- */
  MPIDI_Recvq_lock();
  {
    cur_rreq = recvq.unexpected_head;
    while(cur_rreq != NULL)
      {
#ifdef USE_STATISTICS
        ++search_length;
#endif
        if (MPID_Request_getPeerRequest(cur_rreq) == req        &&
            MPID_Request_getMatchCtxt(cur_rreq)   == context_id &&
            MPID_Request_getMatchRank(cur_rreq)   == source     &&
            MPID_Request_getMatchTag(cur_rreq)    == tag)
          {
            matching_prev_rreq = prev_rreq;
            matching_cur_rreq  = cur_rreq;
            break;
          }
        prev_rreq = cur_rreq;
        cur_rreq  = cur_rreq->dcmf.next;
      }

    /* ----------------------- */
    /* found nothing; return   */
    /* ----------------------- */
    if (matching_cur_rreq == NULL)
      goto fn_exit;

    /* --------------------------------------------------------------------- */
    /* adjust the "next" pointer of the request previous to the matching one */
    /* --------------------------------------------------------------------- */
    if (matching_prev_rreq != NULL)
      matching_prev_rreq->dcmf.next = matching_cur_rreq->dcmf.next;
    else
      recvq.unexpected_head = matching_cur_rreq->dcmf.next;

    /* --------------------------------------- */
    /* adjust the request queue's tail pointer */
    /* --------------------------------------- */
    if (matching_cur_rreq->dcmf.next == NULL)
      recvq.unexpected_tail = matching_prev_rreq;
  }
 fn_exit:
  MPIDI_Recvq_unlock();

#ifdef USE_STATISTICS
  MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

  return (matching_cur_rreq);
}


/**
 * \brief Find a request in the unexpected queue and dequeue it, or allocate a new request and enqueue it in the posted queue
 * \param [in]  source     Find by Sender
 * \param [in]  tag        Find by Tag
 * \param [in]  context_id Find by Context ID (communicator)
 * \param [out] foundp    TRUE iff the request was found
 * \return      The matching UE request or the new posted request
 */
MPID_Request * MPIDI_Recvq_FDU_or_AEP(int source, int tag, int context_id, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
#ifdef USE_STATISTICS
    unsigned search_length = 0;
#endif

    MPIDI_Recvq_lock();
    {
        if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
        {
            prev_rreq = NULL;
            rreq = recvq.unexpected_head;
            while(rreq != NULL)
            {
#ifdef USE_STATISTICS
                ++search_length;
#endif
                if ( (MPID_Request_getMatchCtxt(rreq) == context_id) &&
                     (MPID_Request_getMatchRank(rreq) == source    ) &&
                     (MPID_Request_getMatchTag(rreq)  == tag       )
                   )
                {
                    if (prev_rreq != NULL)
                    {
                        prev_rreq->dcmf.next = rreq->dcmf.next;
                    }
                    else
                    {
                        recvq.unexpected_head = rreq->dcmf.next;
                    }
                    if (rreq->dcmf.next == NULL)
                    {
                        recvq.unexpected_tail = prev_rreq;
                    }
                    found = TRUE;
                    goto lock_exit;
                }

                prev_rreq = rreq;
                rreq = rreq->dcmf.next;
            }
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

            prev_rreq = NULL;
            rreq = recvq.unexpected_head;
            while (rreq != NULL)
            {
#ifdef USE_STATISTICS
                ++search_length;
#endif
                if ( (  MPID_Request_getMatchCtxt(rreq)              == match.context_id) &&
                     ( (MPID_Request_getMatchRank(rreq) & mask.rank) == match.rank      ) &&
                     ( (MPID_Request_getMatchTag(rreq)  & mask.tag ) == match.tag       )
                   )
                {
                    if (prev_rreq != NULL)
                    {
                        prev_rreq->dcmf.next = rreq->dcmf.next;
                    }
                    else
                    {
                        recvq.unexpected_head = rreq->dcmf.next;
                    }
                    if (rreq->dcmf.next == NULL)
                    {
                        recvq.unexpected_tail = prev_rreq;
                    }
                    found = TRUE;
                    goto lock_exit;
                }

                prev_rreq = rreq;
                rreq = rreq->dcmf.next;
            }
        }

        /* A matching request was not found in the unexpected queue,
           so we need to allocate a new request and add it to the
           posted queue */
        rreq = MPID_Request_create();
        if (rreq != NULL)
        {
            rreq->kind = MPID_REQUEST_RECV;
            MPID_Request_setMatch(rreq, tag, source, context_id);
            rreq->dcmf.next = NULL;

            if (recvq.posted_tail != NULL)
            {
                recvq.posted_tail->dcmf.next = rreq;
            }
            else
            {
                recvq.posted_head = rreq;
            }
            recvq.posted_tail = rreq;
        }

        found = FALSE;
    }
  lock_exit:
    MPIDI_Recvq_unlock();

#ifdef USE_STATISTICS
    MPIDI_Statistics_time(MPIDI_Statistics.recvq.unexpected_search, search_length);
#endif

    *foundp = found;
    return rreq;
}


/**
 * \brief Find a request in the posted queue and dequeue it
 * \param [in]  req        Find by address of request object on sender
 * \return      The matching posted request or NULL
 */
int MPIDI_Recvq_FDPR(MPID_Request * req)
{
    MPID_Request * cur_rreq  = NULL;
    MPID_Request * prev_rreq = NULL;
    int found = FALSE;
#ifdef USE_STATISTICS
    unsigned search_length = 0;
#endif

    MPIDI_Recvq_lock();
    {
        cur_rreq = recvq.posted_head;
        while (cur_rreq != NULL)
        {
#ifdef USE_STATISTICS
            ++search_length;
#endif
            if (cur_rreq == req)
            {
                if (prev_rreq != NULL)
                {
                    prev_rreq->dcmf.next = cur_rreq->dcmf.next;
                }
                else
                {
                    recvq.posted_head = cur_rreq->dcmf.next;
                }
                if (cur_rreq->dcmf.next == NULL)
                {
                    recvq.posted_tail = prev_rreq;
                }

                found = TRUE;
                break;
            }

            prev_rreq = cur_rreq;
            cur_rreq = cur_rreq->dcmf.next;
        }
    }
    MPIDI_Recvq_unlock();

#ifdef USE_STATISTICS
    MPIDI_Statistics_time(MPIDI_Statistics.recvq.posted_search, search_length);
#endif

    return found;
}


/**
 * \brief Find a request in the posted queue and dequeue it, or allocate a new request and enqueue it in the unexpected queue
 * \param [in]  source     Find by Sender
 * \param [in]  tag        Find by Tag
 * \param [in]  context_id Find by Context ID (communicator)
 * \param [out] foundp    TRUE iff the request was found
 * \return      The matching posted request or the new UE request
 */
MPID_Request * MPIDI_Recvq_FDP_or_AEU(int source, int tag, int context_id, int * foundp)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq = NULL;
    int found = FALSE;
#ifdef USE_STATISTICS
    unsigned search_length = 0;
#endif


    MPIDI_Recvq_lock();
    {
        rreq = recvq.posted_head;
        while (rreq != NULL)
        {
#ifdef USE_STATISTICS
            ++search_length;
#endif
            if ( (MPID_Request_getMatchCtxt(rreq) == context_id) &&
                 (MPID_Request_getMatchRank(rreq) == source || MPID_Request_getMatchRank(rreq) == MPI_ANY_SOURCE) &&
                 (MPID_Request_getMatchTag(rreq)  == tag    || MPID_Request_getMatchTag(rreq)  == MPI_ANY_TAG)
               )
            {
                if (prev_rreq != NULL)
                {
                    prev_rreq->dcmf.next = rreq->dcmf.next;
                }
                else
                {
                    recvq.posted_head = rreq->dcmf.next;
                }
                if (rreq->dcmf.next == NULL)
                {
                    recvq.posted_tail = prev_rreq;
                }
                found = TRUE;
                goto lock_exit;
            }

            prev_rreq = rreq;
            rreq = rreq->dcmf.next;
        }

        /* A matching request was not found in the posted queue, so we
           need to allocate a new request and add it to the unexpected
           queue */
        rreq = MPID_Request_create();
        if (rreq != NULL)
        {
            rreq->kind = MPID_REQUEST_RECV;
            MPID_Request_setMatch(rreq, tag, source, context_id);
            rreq->dcmf.next = NULL;

            if (recvq.unexpected_tail != NULL)
            {
                recvq.unexpected_tail->dcmf.next = rreq;
            }
            else
            {
                recvq.unexpected_head = rreq;
            }
            recvq.unexpected_tail = rreq;
        }

        found = FALSE;
    }
  lock_exit:
    MPIDI_Recvq_unlock();

#ifdef USE_STATISTICS
    MPIDI_Statistics_time(MPIDI_Statistics.recvq.posted_search, search_length);
#endif

    *foundp = found;
    return rreq;
}


/**
 * \brief Dump the queues
 */
void MPIDI_Recvq_DumpQueues (int verbose)
{
  if(verbose == 0)
    return;

  MPID_Request * rreq = recvq.posted_head;
  MPID_Request * prev_rreq = NULL;
  unsigned i=0, numposted=0, numue=0;
  unsigned postedbytes=0, uebytes=0;

  fprintf(stderr,"Posted Queue:\n");
  fprintf(stderr,"-------------\n");
  while (rreq != NULL)
    {
      if(verbose >= 2)
        fprintf (stderr, "P %d: MPItag=%d MPIrank=%d ctxt=%d cc=%d count=%d\n",
                 i++,
                 MPID_Request_getMatchTag(rreq),
                 MPID_Request_getMatchRank(rreq),
                 MPID_Request_getMatchCtxt(rreq),
                 rreq->cc,
                 rreq->dcmf.userbufcount
                 );
      numposted++;
      postedbytes+=rreq->dcmf.userbufcount;
      prev_rreq = rreq;
      rreq = rreq->dcmf.next;
    }
  fprintf(stderr, "Posted Requests %d, Total Mem: %d bytes\n",
          numposted, postedbytes);


  i=0;
  rreq = recvq.unexpected_head;
  fprintf(stderr, "Unexpected Queue:\n");
  fprintf(stderr, "-----------------\n");
  while (rreq != NULL)
    {
      if(verbose >= 2)
        fprintf (stderr, "UE %d: MPItag=%d MPIrank=%d ctxt=%d cc=%d uebuf=%p uebuflen=%u\n",
                 i++,
                 MPID_Request_getMatchTag(rreq),
                 MPID_Request_getMatchRank(rreq),
                 MPID_Request_getMatchCtxt(rreq),
                 *rreq->cc_ptr,
                 rreq->dcmf.uebuf,
                 rreq->dcmf.uebuflen);
      numue++;
      uebytes+=rreq->dcmf.uebuflen;
      prev_rreq = rreq;
      rreq = rreq->dcmf.next;
    }
  fprintf(stderr, "Unexpected Requests %d, Total Mem: %d bytes\n",
          numue, uebytes);

}
