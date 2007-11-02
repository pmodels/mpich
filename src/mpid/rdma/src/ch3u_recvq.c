/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#define MPIDI_Recvq_lock() MPID_Thread_lock(&MPIDI_Process.recvq_mutex)
#define MPIDI_Recvq_unlock() MPID_Thread_unlock(&MPIDI_Process.recvq_mutex)

/*
 * MPIDI_CH3U_Recvq_FU()
 *
 * Find a request in the unexpected queue; or return NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FU(int source, int tag, int context_id)
{
    MPID_Request * rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
	MPIDI_Recvq_lock();
	{
	    rreq = MPIDI_Process.recvq_unexpected_head;
	    while(rreq != NULL)
	    {
		if (rreq->dev.match.context_id == context_id && rreq->dev.match.rank == source && rreq->dev.match.tag == tag)
		{
		    MPID_Request_add_ref(rreq);
		    break;
		}
		
		rreq = rreq->dev.next;
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
	    rreq = MPIDI_Process.recvq_unexpected_head;
	    while (rreq != NULL)
	    {
		if (rreq->dev.match.context_id == match.context_id && (rreq->dev.match.rank & mask.rank) == match.rank &&
		    (rreq->dev.match.tag & mask.tag) == match.tag)
		{
		    MPID_Request_add_ref(rreq);
		    break;
		}
	    
		rreq = rreq->dev.next;
	    }
	}
	MPIDI_Recvq_unlock();
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FU);
    return rreq;
}

/*
 * MPIDI_CH3U_Recvq_FDU()
 *
 * Find a request in the unexpected queue and dequeue it; otherwise return NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request sreq_id, MPIDI_Message_match * match)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPID_Request * cur_rreq;
    MPID_Request * matching_prev_rreq;
    MPID_Request * matching_cur_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);

    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;
    
    MPIDI_Recvq_lock();
    {
	cur_rreq = MPIDI_Process.recvq_unexpected_head;
	while(cur_rreq != NULL)
	{
	    if (cur_rreq->dev.sender_req_id == sreq_id && cur_rreq->dev.match.context_id == match->context_id &&
		cur_rreq->dev.match.rank == match->rank && cur_rreq->dev.match.tag == match->tag)
	    {
		matching_prev_rreq = prev_rreq;
		matching_cur_rreq = cur_rreq;
	    }
	    
	    prev_rreq = cur_rreq;
	    cur_rreq = cur_rreq->dev.next;
	}

	if (matching_cur_rreq != NULL)
	{
	    if (matching_prev_rreq != NULL)
	    {
		matching_prev_rreq->dev.next = matching_cur_rreq->dev.next;
	    }
	    else
	    {
		MPIDI_Process.recvq_unexpected_head = matching_cur_rreq->dev.next;
	    }
	
	    if (matching_cur_rreq->dev.next == NULL)
	    {
		MPIDI_Process.recvq_unexpected_tail = matching_prev_rreq;
	    }

	    rreq = matching_cur_rreq;
	}
	else
	{
	    rreq = NULL;
	}
    }
    MPIDI_Recvq_unlock();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);
    return rreq;
}


/*
 * MPIDI_CH3U_Recvq_FDU_or_AEP()
 *
 * Atomically find a request in the unexpected queue and dequeue it, or allocate a new request and enqueue it in the posted queue
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDU_or_AEP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int source, int tag, int context_id, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    MPIDI_Recvq_lock();
    {
	if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
	{
	    prev_rreq = NULL;
	    rreq = MPIDI_Process.recvq_unexpected_head;
	    while(rreq != NULL)
	    {
		if (rreq->dev.match.context_id == context_id && rreq->dev.match.rank == source && rreq->dev.match.tag == tag)
		{
		    if (prev_rreq != NULL)
		    {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else
		    {
			MPIDI_Process.recvq_unexpected_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL)
		    {
			MPIDI_Process.recvq_unexpected_tail = prev_rreq;
		    }
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
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
	    rreq = MPIDI_Process.recvq_unexpected_head;
	    while (rreq != NULL)
	    {
		if (rreq->dev.match.context_id == match.context_id && (rreq->dev.match.rank & mask.rank) == match.rank &&
		    (rreq->dev.match.tag & mask.tag) == match.tag)
		{
		    if (prev_rreq != NULL)
		    {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else
		    {
			MPIDI_Process.recvq_unexpected_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL)
		    {
			MPIDI_Process.recvq_unexpected_tail = prev_rreq;
		    }
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}

	/* A matching request was not found in the unexpected queue, so we need to allocate a new request and add it to the
	   posted queue */
	rreq = MPID_Request_create();
	if (rreq != NULL)
	{
	    MPIU_Object_set_ref(rreq, 2);
	    rreq->kind = MPID_REQUEST_RECV;
	    rreq->dev.match.tag = tag;
	    rreq->dev.match.rank = source;
	    rreq->dev.match.context_id = context_id;
	    rreq->dev.next = NULL;
	
	    if (MPIDI_Process.recvq_posted_tail != NULL)
	    {
		MPIDI_Process.recvq_posted_tail->dev.next = rreq;
	    }
	    else
	    {
		MPIDI_Process.recvq_posted_head = rreq;
	    }
	    MPIDI_Process.recvq_posted_tail = rreq;
	}
    
	found = FALSE;
    }
  lock_exit:
    MPIDI_Recvq_unlock();

    if (!found)
    { 
	MPID_Request_initialized_clear(rreq);
    }
    else
    {
	MPID_Request_initialized_wait(rreq);
    }

    *foundp = found;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);
    return rreq;
}


/*
 * MPIDI_CH3U_Recvq_DP()
 *
 * Given an existing request, dequeue that request from the posted queue, or return NULL if the request was not in the posted
 * queued
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_DP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Recvq_DP(MPID_Request * rreq)
{
    int found;
    MPID_Request * cur_rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_DP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_DP);
    
    found = FALSE;
    prev_rreq = NULL;
    
    MPIDI_Recvq_lock();
    {
	cur_rreq = MPIDI_Process.recvq_posted_head;
	while (cur_rreq != NULL)
	{
	    if (cur_rreq == rreq)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = cur_rreq->dev.next;
		}
		else
		{
		    MPIDI_Process.recvq_posted_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    MPIDI_Process.recvq_posted_tail = prev_rreq;
		}
	    
		found = TRUE;
		break;
	    }
	    
	    prev_rreq = cur_rreq;
	    cur_rreq = cur_rreq->dev.next;
	}
    }
    MPIDI_Recvq_unlock();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_DP);
    return found;
}

/*
 * MPIDI_CH3U_Recvq_FDP
 *
 * Locate a request in the posted queue and dequeue it, or return NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDP(MPIDI_Message_match * match)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDP);
    
    prev_rreq = NULL;

    MPIDI_Recvq_lock();
    {
	rreq = MPIDI_Process.recvq_posted_head;
	while (rreq != NULL)
	{
	    if ((rreq->dev.match.context_id == match->context_id) &&
		(rreq->dev.match.rank == match->rank || rreq->dev.match.rank == MPI_ANY_SOURCE) &&
		(rreq->dev.match.tag == match->tag || rreq->dev.match.tag == MPI_ANY_TAG))
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = rreq->dev.next;
		}
		else
		{
		    MPIDI_Process.recvq_posted_head = rreq->dev.next;
		}
		if (rreq->dev.next == NULL)
		{
		    MPIDI_Process.recvq_posted_tail = prev_rreq;
		}

		break;
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}
    }
    MPIDI_Recvq_unlock();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDP);
    return rreq;
}


/*
 * MPIDI_CH3U_Recvq_FDP_or_AEU()
 *
 * Locate a request in the posted queue and dequeue it, or allocate a new request and enqueue it in the unexpected queue
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDP_or_AEU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);
    
    prev_rreq = NULL;

    MPIDI_Recvq_lock();
    {
	rreq = MPIDI_Process.recvq_posted_head;
	while (rreq != NULL)
	{
	    if ((rreq->dev.match.context_id == match->context_id) &&
		(rreq->dev.match.rank == match->rank || rreq->dev.match.rank == MPI_ANY_SOURCE) &&
		(rreq->dev.match.tag == match->tag || rreq->dev.match.tag == MPI_ANY_TAG))
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = rreq->dev.next;
		}
		else
		{
		    MPIDI_Process.recvq_posted_head = rreq->dev.next;
		}
		if (rreq->dev.next == NULL)
		{
		    MPIDI_Process.recvq_posted_tail = prev_rreq;
		}
		found = TRUE;
		goto lock_exit;
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}

	/* A matching request was not found in the posted queue, so we need to allocate a new request and add it to the
	   unexpected queue */
	rreq = MPID_Request_create();
	if (rreq != NULL)
	{
	    MPIU_Object_set_ref(rreq, 2);
	    rreq->kind = MPID_REQUEST_RECV;
	    rreq->dev.match = *match;
	    rreq->dev.next = NULL;
	
	    if (MPIDI_Process.recvq_unexpected_tail != NULL)
	    {
		MPIDI_Process.recvq_unexpected_tail->dev.next = rreq;
	    }
	    else
	    {
		MPIDI_Process.recvq_unexpected_head = rreq;
	    }
	    MPIDI_Process.recvq_unexpected_tail = rreq;
	}
    
	found = FALSE;
    }
  lock_exit:
    MPIDI_Recvq_unlock();

    if (!found)
    { 
	MPID_Request_initialized_clear(rreq);
    }
    else
    {
	MPID_Request_initialized_wait(rreq);
    }

    *foundp = found;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);
    return rreq;
}
