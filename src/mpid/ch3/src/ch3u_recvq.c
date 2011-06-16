/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* MPIDI_POSTED_RECV_ENQUEUE_HOOK(req): Notifies channel that req has
   been enqueued on the posted recv queue.  Returns void. */
#ifndef MPIDI_POSTED_RECV_ENQUEUE_HOOK
#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(req) do{}while(0)
#endif
/* MPIDI_POSTED_RECV_DEQUEUE_HOOK(req): Notifies channel that req has
   been dequeued from the posted recv queue.  Returns non-zero if the
   channel has already matched the request; 0 otherwise.  This happens
   when the channel supports shared-memory and network communication
   with a network capable of matching, and the same request is matched
   by the network and, e.g., shared-memory.  When that happens the
   dequeue functions below should, either search for the next matching
   request, or report that no request was found. */
#ifndef MPIDI_POSTED_RECV_DEQUEUE_HOOK
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(req) 0
#endif

/* FIXME: 
 * Recvq_lock/unlock removed because it is not needed for the SINGLE_CS
 * approach and we might want a different, non-lock-based approach in 
 * a finer-grained thread-sync version.  For example, 
 * some routines can be implemented in a lock-free
 * fashion (because the user is required to guarantee non-conflicting 
 * accesses, such as doing a probe and a receive that matches in different 
 * threads).  
 *
 * There are a lot of routines here.  Do we really need them all?
 * 
 * The search criteria can be implemented in a single 64 bit compare on
 * systems with efficient 64-bit operations.  The rank and contextid can also
 * in many cases be combined into a single 32-bit word for the comparison
 * (in which case the message info should be stored in the queue in a 
 * naturally aligned, 64-bit word.
 * 
 */

static MPID_Request * recvq_posted_head = 0;
static MPID_Request * recvq_posted_tail = 0;
static MPID_Request * recvq_unexpected_head = 0;
static MPID_Request * recvq_unexpected_tail = 0;

/* Export the location of the queue heads if debugger support is enabled.
 * This allows the queue code to rely on the local variables for the
 * queue heads while also exporting those variables to the debugger.
 * See src/mpi/debugger/dll_mpich2.c for how this is used to 
 * access the message queues.
 */
#ifdef HAVE_DEBUGGER_SUPPORT
MPID_Request ** const MPID_Recvq_posted_head_ptr     = &recvq_posted_head;
MPID_Request ** const MPID_Recvq_unexpected_head_ptr = &recvq_unexpected_head;
#endif

/* If the MPIDI_Message_match structure fits into a pointer size, we
 * can directly work on it */
/* MATCH_WITH_NO_MASK compares the match values without masking
 * them. This is useful for the case where there are no ANY_TAG or
 * ANY_SOURCE wild cards. */
#define MATCH_WITH_NO_MASK(match1, match2)                              \
    ((sizeof(MPIDI_Message_match) == SIZEOF_VOID_P) ? ((match1).whole == (match2).whole) : \
     (((match1).parts.rank == (match2).parts.rank) &&                   \
      ((match1).parts.tag == (match2).parts.tag) &&                     \
      ((match1).parts.context_id == (match2).parts.context_id)))

/* MATCH_WITH_LEFT_MASK compares the match values after masking only
 * the left field. This is useful for the case where the right match
 * is a part of the unexpected queue and has no ANY_TAG or ANY_SOURCE
 * wild cards, but the left match might have them. */
#define MATCH_WITH_LEFT_MASK(match1, match2, mask)                      \
    ((sizeof(MPIDI_Message_match) == SIZEOF_VOID_P) ?                   \
     (((match1).whole & (mask).whole) == (match2).whole) :              \
     ((((match1).parts.rank & (mask).parts.rank) == (match2).parts.rank) && \
      (((match1).parts.tag & (mask).parts.tag) == (match2).parts.tag) && \
      ((match1).parts.context_id == (match2).parts.context_id)))

/* This is the most general case where both matches have to be
 * masked. Both matches are masked with the same value. There doesn't
 * seem to be a need for two different masks at this time. */
#define MATCH_WITH_LEFT_RIGHT_MASK(match1, match2, mask)                \
    ((sizeof(MPIDI_Message_match) == SIZEOF_VOID_P) ?                   \
     (((match1).whole & (mask).whole) == ((match2).whole & (mask).whole)) : \
     ((((match1).parts.rank & (mask).parts.rank) == ((match2).parts.rank & (mask).parts.rank)) && \
      (((match1).parts.tag & (mask).parts.tag) == ((match2).parts.tag & (mask).parts.tag)) && \
      ((match1).parts.context_id == (match2).parts.context_id)))


/* FIXME: If this routine is only used by probe/iprobe, then we don't need
   to set the cancelled field in status (only set for nonblocking requests) */
/*
 * MPIDI_CH3U_Recvq_FU()
 *
 * Search for a matching request in the unexpected receive queue.  Return 
 * true if one is found, false otherwise.  If the status arguement is
 * not MPI_STATUS_IGNORE, return information about the request in that
 * parameter.  This routine is used by mpid_probe and mpid_iprobe.
 *
 * Multithread - As this is a read-only routine, it need not
 * require an external critical section (careful organization of the
 * queue updates would not even require a critical section within this
 * routine).  However, this routine is used both from within the progress
 * engine and from without it.  To make that work with the current
 * design for MSGQUEUE and the brief-global mode, the critical section 
 * is *outside* of this routine.
 *
 * This routine is used only in mpid_iprobe and mpid_probe
 *
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Recvq_FU(int source, int tag, int context_id, MPI_Status *s)
{
    MPID_Request * rreq;
    int found = 0;
    MPIDI_Message_match match, mask;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    MPIU_THREAD_CS_ASSERT_HELD(MSGQUEUE);

    rreq = recvq_unexpected_head;

    match.parts.context_id = context_id;
    match.parts.tag = tag;
    match.parts.rank = source;

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE) {
	while (rreq != NULL) {
	    if (MATCH_WITH_NO_MASK(rreq->dev.match, match))
		break;
	    rreq = rreq->dev.next;
	}
    }
    else {
	mask.parts.context_id = mask.parts.rank = mask.parts.tag = ~0;
	if (tag == MPI_ANY_TAG)
	    match.parts.tag = mask.parts.tag = 0;
	if (source == MPI_ANY_SOURCE)
	    match.parts.rank = mask.parts.rank = 0;

	while (rreq != NULL) {
	    if (MATCH_WITH_LEFT_MASK(rreq->dev.match, match, mask))
		break;
	    rreq = rreq->dev.next;
	}
    }

    /* Save the information about the request before releasing the 
       queue */
    if (rreq) {
	if (s != MPI_STATUS_IGNORE) {
	    /* Avoid setting "extra" fields like MPI_ERROR */
	    s->MPI_SOURCE = rreq->status.MPI_SOURCE;
	    s->MPI_TAG    = rreq->status.MPI_TAG;
	    s->count      = rreq->status.count;
	    s->cancelled  = rreq->status.cancelled;
	}
	found = 1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FU);
    return found;
}

/*
 * MPIDI_CH3U_Recvq_FDU()
 *
 * Find a request in the unexpected queue and dequeue it; otherwise return NULL.
 *
 * Multithread - This routine must be atomic (since it dequeues a
 * request).  However, once the request is dequeued, no other thread can
 * see it, so this routine provides its own atomicity.
 *
 * This routine is used only in the case of send_cancel.  However, it is used both
 * within mpid_send_cancel and within a packet handler.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request sreq_id, 
				    MPIDI_Message_match * match)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPID_Request * cur_rreq;
    MPID_Request * matching_prev_rreq;
    MPID_Request * matching_cur_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);

    MPIU_THREAD_CS_ASSERT_HELD(MSGQUEUE);

    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;

    /* Note that since this routine is used only in the case of send_cancel,
       there can be only one match if at all. */
    /* FIXME: Why doesn't this exit after it finds the first match? */
    cur_rreq = recvq_unexpected_head;
    while (cur_rreq != NULL) {
	if (cur_rreq->dev.sender_req_id == sreq_id &&
	    (MATCH_WITH_NO_MASK(cur_rreq->dev.match, *match))) {
	    matching_prev_rreq = prev_rreq;
	    matching_cur_rreq = cur_rreq;
	}
	prev_rreq = cur_rreq;
	cur_rreq = cur_rreq->dev.next;
    }

    if (matching_cur_rreq != NULL) {
	if (matching_prev_rreq != NULL) {
	    matching_prev_rreq->dev.next = matching_cur_rreq->dev.next;
	}
	else {
	    recvq_unexpected_head = matching_cur_rreq->dev.next;
	}
	
	if (matching_cur_rreq->dev.next == NULL) {
	    recvq_unexpected_tail = matching_prev_rreq;
	}

	rreq = matching_cur_rreq;
    }
    else {
	rreq = NULL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDU);
    return rreq;
}


/*
 * MPIDI_CH3U_Recvq_FDU_or_AEP()
 *
 * Atomically find a request in the unexpected queue and dequeue it, or 
 * allocate a new request and enqueue it in the posted queue
 *
 * Multithread - This routine must be called from within a MSGQUEUE 
 * critical section.  If a request is allocated, it must not release
 * the MSGQUEUE until the request is completely valid, as another thread
 * may then find it and dequeue it.
 *
 * This routine is used in mpid_irecv and mpid_recv.
 *
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDU_or_AEP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int source, int tag, 
                                           int context_id, MPID_Comm *comm, void *user_buf,
                                           int user_count, MPI_Datatype datatype, int * foundp)
{
    int found;
    MPID_Request *rreq, *prev_rreq;
    MPIDI_Message_match match;
    MPIDI_Message_match mask;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    MPIU_THREAD_CS_ASSERT_HELD(MSGQUEUE);

    /* Optimize this loop for an empty unexpected receive queue */
    rreq = recvq_unexpected_head;
    if (rreq) {
	prev_rreq = NULL;

	match.parts.context_id = context_id;
	match.parts.tag = tag;
	match.parts.rank = source;

	if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE) {
	    do {
		if (MATCH_WITH_NO_MASK(rreq->dev.match, match)) {
		    if (prev_rreq != NULL) {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else {
			recvq_unexpected_head = rreq->dev.next;
		    }

		    if (rreq->dev.next == NULL) {
			recvq_unexpected_tail = prev_rreq;
		    }

		    rreq->comm = comm;
		    MPIR_Comm_add_ref(comm);
		    rreq->dev.user_buf = user_buf;
		    rreq->dev.user_count = user_count;
		    rreq->dev.datatype = datatype;
		    found = TRUE;
		    goto lock_exit;
		}
		prev_rreq = rreq;
		rreq      = rreq->dev.next;
	    } while (rreq);
	}
	else {
	    mask.parts.context_id = mask.parts.rank = mask.parts.tag = ~0;
	    if (tag == MPI_ANY_TAG)
		match.parts.tag = mask.parts.tag = 0;
	    if (source == MPI_ANY_SOURCE)
		match.parts.rank = mask.parts.rank = 0;

	    do {
		if (MATCH_WITH_LEFT_MASK(rreq->dev.match, match, mask)) {
		    if (prev_rreq != NULL) {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else {
			recvq_unexpected_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL) {
			recvq_unexpected_tail = prev_rreq;
		    }
		    rreq->comm                 = comm;
		    MPIR_Comm_add_ref(comm);
		    rreq->dev.user_buf         = user_buf;
		    rreq->dev.user_count       = user_count;
		    rreq->dev.datatype         = datatype;
		    found = TRUE;
		    goto lock_exit;
		}
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    } while (rreq);
	}
    }
    
    /* A matching request was not found in the unexpected queue, so we 
       need to allocate a new request and add it to the posted queue */
    {
	int mpi_errno = MPI_SUCCESS;

        found = FALSE;

	MPIDI_Request_create_rreq( rreq, mpi_errno, goto lock_exit );
	rreq->dev.match.parts.tag	   = tag;
	rreq->dev.match.parts.rank	   = source;
	rreq->dev.match.parts.context_id   = context_id;

	/* Added a mask for faster search on 64-bit capable
	 * platforms */
	rreq->dev.mask.parts.context_id = ~0;
	if (rreq->dev.match.parts.rank == MPI_ANY_SOURCE)
	    rreq->dev.mask.parts.rank = 0;
	else
	    rreq->dev.mask.parts.rank = ~0;
	if (rreq->dev.match.parts.tag == MPI_ANY_TAG)
	    rreq->dev.mask.parts.tag = 0;
	else
	    rreq->dev.mask.parts.tag = ~0;

        rreq->comm                 = comm;
        MPIR_Comm_add_ref(comm);
        rreq->dev.user_buf         = user_buf;
        rreq->dev.user_count       = user_count;
        rreq->dev.datatype         = datatype;

        /* check whether VC has failed, or this is an ANY_SOURCE in a
           failed communicator */
        if (source != MPI_ANY_SOURCE) {
            MPIDI_VC_t *vc;
            MPIDI_Comm_get_vc(comm, source, &vc);
            if (vc->state == MPIDI_VC_STATE_MORIBUND) {
                MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
                rreq->status.MPI_ERROR = mpi_errno;
                MPIDI_CH3U_Request_complete(rreq);
                goto lock_exit;
            }
        } else if (MPID_VCRT_Contains_failed_vc(comm->vcrt)) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**comm_fail");
            rreq->status.MPI_ERROR = mpi_errno;
            MPIDI_CH3U_Request_complete(rreq);
            goto lock_exit;
        }
        
	rreq->dev.next = NULL;
	if (recvq_posted_tail != NULL) {
	    recvq_posted_tail->dev.next = rreq;
	}
	else {
	    recvq_posted_head = rreq;
	}
	recvq_posted_tail = rreq;
	MPIDI_POSTED_RECV_ENQUEUE_HOOK(rreq);
    }
    
  lock_exit:

    *foundp = found;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);
    return rreq;
}


/*
 * MPIDI_CH3U_Recvq_DP()
 *
 * Given an existing request, dequeue that request from the posted queue, or 
 * return NULL if the request was not in the posted queued
 *
 * Multithread - This routine is atomic
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
    int dequeue_failed;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_DP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_DP);

    found = FALSE;
    prev_rreq = NULL;

    /* MT FIXME is this right? or should the caller do this? */
    MPIU_THREAD_CS_ENTER(MSGQUEUE,);
    cur_rreq = recvq_posted_head;
    while (cur_rreq != NULL) {
	if (cur_rreq == rreq) {
	    if (prev_rreq != NULL) {
		prev_rreq->dev.next = cur_rreq->dev.next;
	    }
	    else {
		recvq_posted_head = cur_rreq->dev.next;
	    }
	    if (cur_rreq->dev.next == NULL) {
		recvq_posted_tail = prev_rreq;
	    }
            /* Notify channel that rreq has been dequeued and check if
               it has already matched rreq, fail if so */
	    dequeue_failed = MPIDI_POSTED_RECV_DEQUEUE_HOOK(rreq);
            if (!dequeue_failed)
                found = TRUE;
	    break;
	}
	
	prev_rreq = cur_rreq;
	cur_rreq = cur_rreq->dev.next;
    }

    MPIU_THREAD_CS_EXIT(MSGQUEUE,);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_DP);
    return found;
}

/*
 * MPIDI_CH3U_Recvq_FDP_or_AEU()
 *
 * Locate a request in the posted queue and dequeue it, or allocate a new 
 * request and enqueue it in the unexpected queue
 *
 * Multithread - This routine must be called from within a MSGQUEUE 
 * critical section.  If a request is allocated, it must not release
 * the MSGQUEUE until the request is completely valid, as another thread
 * may then find it and dequeue it.
 *
 * This routine is used in ch3u_eager, ch3u_eagersync, ch3u_handle_recv_pkt,
 * ch3u_rndv, and mpidi_isend_self.  Routines within the progress engine
 * will need to be careful to avoid nested critical sections.  
 *
 * FIXME: Currently, the routines called from within the progress engine
 * do not use the MSGQUEUE CS, because in the brief-global mode, that
 * simply uses the global_mutex .  
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDP_or_AEU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match, 
					   int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int channel_matched;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);

    MPIU_THREAD_CS_ASSERT_HELD(MSGQUEUE);

 top_loop:
    prev_rreq = NULL;

    rreq = recvq_posted_head;

    while (rreq != NULL) {
	if (MATCH_WITH_LEFT_RIGHT_MASK(rreq->dev.match, *match, rreq->dev.mask)) {
	    if (prev_rreq != NULL) {
		prev_rreq->dev.next = rreq->dev.next;
	    }
	    else {
		recvq_posted_head = rreq->dev.next;
	    }
	    if (rreq->dev.next == NULL) {
		recvq_posted_tail = prev_rreq;
	    }

            /* give channel a chance to match the request, try again if so */
	    channel_matched = MPIDI_POSTED_RECV_DEQUEUE_HOOK(rreq);
            if (channel_matched)
                goto top_loop;
            
	    found = TRUE;                
	    goto lock_exit;
	}
	prev_rreq = rreq;
	rreq = rreq->dev.next;
    }

    /* A matching request was not found in the posted queue, so we 
       need to allocate a new request and add it to the unexpected queue */
    {
	int mpi_errno=0;
	MPIDI_Request_create_rreq( rreq, mpi_errno, 
				   found=FALSE;goto lock_exit );
        rreq->dev.recv_pending_count = 1;
	rreq->dev.match	= *match;
	rreq->dev.next	= NULL;
	if (recvq_unexpected_tail != NULL) {
	    recvq_unexpected_tail->dev.next = rreq;
	}
	else {
	    recvq_unexpected_head = rreq;
	}
	recvq_unexpected_tail = rreq;
    }
    
    found = FALSE;

  lock_exit:

    *foundp = found;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);
    return rreq;
}

/* returns TRUE iff the request was sent on the vc */
static inline int req_uses_vc(const MPID_Request* req, const MPIDI_VC_t *vc)
{
    MPIDI_VC_t *vc1;
    
    MPIDI_Comm_get_vc(req->comm, req->dev.match.parts.rank, &vc1);
    return vc == vc1;
}

/* returns TRUE iff the vc is part of the comm*/
static inline int is_vc_in_comm(const MPIDI_VC_t *vc, const MPID_Comm *comm)
{
    int i;

    for (i = 0; i < comm->remote_size; ++i) {
        MPIDI_VC_t *vc1;
        MPIDI_Comm_get_vc(comm, i, &vc1);
        if (vc == vc1)
            return TRUE;
    }
    return FALSE;
}

#undef FUNCNAME
#define FUNCNAME dequeue_and_set_error
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* This dequeues req from the posted recv queue, set req's error code to comm_fail, and updates the req pointer.
   Note that this creates a new error code if one hasn't already been created (i.e., if *error is MPI_SUCCESS). */
static inline void dequeue_and_set_error(MPID_Request **req,  MPID_Request *prev_req, int *error, int rank)
{
    MPID_Request *next = (*req)->dev.next;

    if (*error == MPI_SUCCESS)
        MPIU_ERR_SET1(*error, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", rank);

    /* remove from queue */
    if (recvq_posted_head == *req)
        recvq_posted_head = (*req)->dev.next;
    else
        prev_req->dev.next = (*req)->dev.next;
    if (recvq_posted_tail == *req)
        recvq_posted_tail = prev_req;
    
    /* set error and complete */
    (*req)->status.MPI_ERROR = *error;
    MPIDI_CH3U_Request_complete(*req);
    *req = next;
}



#undef FUNCNAME
#define FUNCNAME MPIDU_Complete_posted_with_error
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3U_Complete_posted_with_error(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req, *prev_req;
    int error = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_COMPLETE_POSTED_WITH_ERROR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_COMPLETE_POSTED_WITH_ERROR);

    MPIU_THREAD_CS_ENTER(MSGQUEUE,);

    /* check each req to see if the VC is part of that communicator */
    req = recvq_posted_head;
    prev_req = NULL;
    while (req) {
        if (req->dev.match.parts.rank != MPI_ANY_SOURCE && req_uses_vc(req, vc)) {
            /* this req is expected on the VC */
            dequeue_and_set_error(&req, prev_req, &error, vc->pg_rank);
        } else if (req->dev.match.parts.rank == MPI_ANY_SOURCE && is_vc_in_comm(vc, req->comm)) {
            /* This req is an ANY_SOURCE and is expected on a communicator that includes the VC.
               We need to dequeue all anysources posted in a communicator with a failed VC.  We
               check whether the VC is in the communicator by iterating over the comm's VC table.
               Since this may be expensive, now that we know the VC is in comm, we take the
               opportunity to scan the rest of the posted recv queue for other anysources with
               the same communicator.  Note that in the worst case this is O(N*M), where N is the
               number of posted requests and M is the number of communicators.  This can happen
               if every req is an anysource and uses a different communicator.  We can possibly
               conditionally execute the optimization based on number of comms, number of posted
               requests and communicator size. */
            MPID_Request *as_req = req->dev.next;
            MPID_Request *prev_as_req = req;
            /* First remove any AS recvs on this comm that were posted AFTER this req */
            while (as_req) {
                if (as_req->comm == req->comm && as_req->dev.match.parts.rank == MPI_ANY_SOURCE) {
                    dequeue_and_set_error(&as_req, prev_as_req, &error, vc->pg_rank);
                } else {
                    prev_as_req = as_req;
                    as_req = as_req->dev.next;
                }
            }
            /* Now remove this req.  We do this in this order to make it easier to keep track of
               req and prev_req pointers */
            dequeue_and_set_error(&req, prev_req, &error, vc->pg_rank);
        } else {
            prev_req = req;
            req = req->dev.next;
        }
    }
    
 fn_exit:
    MPIU_THREAD_CS_EXIT(MSGQUEUE,);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_COMPLETE_POSTED_WITH_ERROR);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* --BEGIN ERROR HANDLING-- */
/* pretty prints tag, returns out for calling convenience */
static char *tag_val_to_str(int tag, char *out, int max)
{
    if (tag == MPI_ANY_TAG) {
        MPIU_Strncpy(out, "MPI_ANY_TAG", max);
    }
    else {
        MPIU_Snprintf(out, max, "%d", tag);
    }
    return out;
}

/* pretty prints rank, returns out for calling convenience */
static char *rank_val_to_str(int rank, char *out, int max)
{
    if (rank == MPI_ANY_SOURCE) {
        MPIU_Strncpy(out, "MPI_ANY_SOURCE", max);
    }
    else {
        MPIU_Snprintf(out, max, "%d", rank);
    }
    return out;
}

/* satisfy the compiler */
void MPIDI_CH3U_Dbg_print_recvq(FILE *stream);

/* This function can be called by a debugger to dump the recvq state to the
 * given stream. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Dbg_print_recvq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Dbg_print_recvq(FILE *stream)
{
    MPID_Request * rreq;
    int i;
    char tag_buf[128];
    char rank_buf[128];

    fprintf(stream, "========================================\n");
    fprintf(stream, "MPI_COMM_WORLD  ctx=%#x rank=%d\n", MPIR_Process.comm_world->context_id, MPIR_Process.comm_world->rank);
    fprintf(stream, "MPI_COMM_SELF   ctx=%#x\n", MPIR_Process.comm_self->context_id);
    if (MPIR_Process.comm_parent) {
        fprintf(stream, "MPI_COMM_PARENT ctx=%#x recvctx=%#x\n",
                MPIR_Process.comm_self->context_id,
                MPIR_Process.comm_parent->recvcontext_id);
    }
    else {
        fprintf(stream, "MPI_COMM_PARENT (NULL)\n");
    }

    fprintf(stream, "CH3 Posted RecvQ:\n");
    rreq = recvq_posted_head;
    i = 0;
    while (rreq != NULL) {
        fprintf(stream, "..[%d] rreq=%p ctx=%#x rank=%s tag=%s\n", i, rreq,
                        rreq->dev.match.parts.context_id,
                        rank_val_to_str(rreq->dev.match.parts.rank, rank_buf, sizeof(rank_buf)),
                        tag_val_to_str(rreq->dev.match.parts.tag, tag_buf, sizeof(tag_buf)));
        ++i;
        rreq = rreq->dev.next;
    }

    fprintf(stream, "CH3 Unexpected RecvQ:\n");
    rreq = recvq_unexpected_head;
    i = 0;
    while (rreq != NULL) {
        fprintf(stream, "..[%d] rreq=%p ctx=%#x rank=%s tag=%s\n", i, rreq,
                        rreq->dev.match.parts.context_id,
                        rank_val_to_str(rreq->dev.match.parts.rank, rank_buf, sizeof(rank_buf)),
                        tag_val_to_str(rreq->dev.match.parts.tag, tag_buf, sizeof(tag_buf)));
        fprintf(stream, "..    status.src=%s status.tag=%s\n",
                        rank_val_to_str(rreq->status.MPI_SOURCE, rank_buf, sizeof(rank_buf)),
                        tag_val_to_str(rreq->status.MPI_TAG, tag_buf, sizeof(tag_buf)));
        ++i;
        rreq = rreq->dev.next;
    }
    fprintf(stream, "========================================\n");
}
/* --END ERROR HANDLING-- */

/* returns the number of elements in the unexpected queue */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_count_unexp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Recvq_count_unexp(void)
{
    int count = 0;
    MPID_Request *req = recvq_unexpected_head;

    MPIU_THREAD_CS_ASSERT_HELD(MSGQUEUE);

    while (req)
    {
        ++count;
        req = req->dev.next;
    }

    return count;
}
