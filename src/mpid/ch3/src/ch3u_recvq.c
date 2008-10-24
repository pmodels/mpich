/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* These are needed for nemesis to know from where to expect a message */
#ifndef MPIDI_POSTED_RECV_ENQUEUE_HOOK
#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(x)
#endif
#ifndef MPIDI_POSTED_RECV_DEQUEUE_HOOK
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(x)
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
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Recvq_FU(int source, int tag, int context_id, MPI_Status *s)
{
    MPID_Request * rreq;
    int found = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FU);

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
	rreq = recvq_unexpected_head;
	/* FIXME: If the match data fits in an int64_t, we should try
	   to use a single test here */
	while(rreq != NULL)
	{
	    if (rreq->dev.match.context_id == context_id && 
		rreq->dev.match.rank == source && rreq->dev.match.tag == tag)
	    {
		break;
	    }
	    
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
	
	rreq = recvq_unexpected_head;
	while (rreq != NULL)
	{
	    if (rreq->dev.match.context_id == match.context_id && 
		(rreq->dev.match.rank & mask.rank) == match.rank &&
		(rreq->dev.match.tag & mask.tag) == match.tag)
	    {
		break;
	    }
	    
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
 * This routine is used only in the case of send_cancel
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

    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;

    /* Note that since this routine is used only in the case of send_cancel,
       there can be only one match if at all. */
    cur_rreq = recvq_unexpected_head;
    while(cur_rreq != NULL) {
	if (cur_rreq->dev.sender_req_id == sreq_id && 
	    cur_rreq->dev.match.context_id == match->context_id &&
	    cur_rreq->dev.match.rank == match->rank && 
	    cur_rreq->dev.match.tag == match->tag)
	    {
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
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Recvq_FDU_or_AEP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int source, int tag, 
					   int context_id, int * foundp)
{
    int found;
    MPID_Request *rreq, *prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP);

    /* Optimize this loop for an empty unexpected receive queue */
    rreq = recvq_unexpected_head;
    if (rreq) {
	prev_rreq = NULL;
	if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE) {
	    do { 
		if (rreq->dev.match.context_id == context_id && 
		    rreq->dev.match.rank == source && 
		    rreq->dev.match.tag == tag) {
		    if (prev_rreq != NULL) {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else {
			recvq_unexpected_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL) {
			recvq_unexpected_tail = prev_rreq;
		    }
		    found = TRUE;
		    goto lock_exit;
		} 
		
		prev_rreq = rreq;
		rreq      = rreq->dev.next;
	    } while (rreq);
	}
	else {
	    MPIDI_Message_match match;
	    MPIDI_Message_match mask;
	    
	    match.context_id = context_id;
	    mask.context_id = ~0;
	    if (tag == MPI_ANY_TAG) {
		match.tag = 0;
		mask.tag = 0;
	    }
	    else {
		match.tag = tag;
		mask.tag = ~0;
	    }
	    if (source == MPI_ANY_SOURCE) {
		match.rank = 0;
		mask.rank = 0;
	    }
	    else {
		match.rank = source;
		mask.rank = ~0;
	    }
	    
	    do {
		if (rreq->dev.match.context_id == match.context_id && 
		    (rreq->dev.match.rank & mask.rank) == match.rank &&
		    (rreq->dev.match.tag & mask.tag) == match.tag) {
		    if (prev_rreq != NULL) {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else {
			recvq_unexpected_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL) {
			recvq_unexpected_tail = prev_rreq;
		    }
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
	int mpi_errno=0;
	MPIDI_Request_create_rreq( rreq, mpi_errno, 
				   found = FALSE;goto lock_exit );
	rreq->dev.match.tag	   = tag;
	rreq->dev.match.rank	   = source;
	rreq->dev.match.context_id = context_id;
	rreq->dev.next		   = NULL;
	if (recvq_posted_tail != NULL) {
	    recvq_posted_tail->dev.next = rreq;
	}
	else {
	    recvq_posted_head = rreq;
	}
	recvq_posted_tail = rreq;
	/* This is for nemesis to know from where to expect a message */
	MPIDI_POSTED_RECV_ENQUEUE_HOOK (rreq);
    }
    
    found = FALSE;

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
	    /* This is for nemesis to know from where to expect a message */
	    MPIDI_POSTED_RECV_DEQUEUE_HOOK (rreq);
	    found = TRUE;
	    break;
	}
	
	prev_rreq = cur_rreq;
	cur_rreq = cur_rreq->dev.next;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECVQ_DP);
    return found;
}

/*
 * MPIDI_CH3U_Recvq_FDP_or_AEU()
 *
 * Locate a request in the posted queue and dequeue it, or allocate a new 
 * request and enqueue it in the unexpected queue
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
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU);
    
    prev_rreq = NULL;

    rreq = recvq_posted_head;
    while (rreq != NULL) {
	if ((rreq->dev.match.context_id == match->context_id) &&
	    (rreq->dev.match.rank == match->rank || 
	     rreq->dev.match.rank == MPI_ANY_SOURCE) &&
	    (rreq->dev.match.tag == match->tag || 
	     rreq->dev.match.tag == MPI_ANY_TAG)) {
	    if (prev_rreq != NULL) {
		prev_rreq->dev.next = rreq->dev.next;
	    }
	    else {
		recvq_posted_head = rreq->dev.next;
	    }
	    if (rreq->dev.next == NULL) {
		recvq_posted_tail = prev_rreq;
	    }
	    found = TRUE;
	    /* This is for nemesis to know from where to expect a message */
	    MPIDI_POSTED_RECV_DEQUEUE_HOOK (rreq);
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
                        rreq->dev.match.context_id,
                        rank_val_to_str(rreq->dev.match.rank, rank_buf, sizeof(rank_buf)),
                        tag_val_to_str(rreq->dev.match.tag, tag_buf, sizeof(tag_buf)));
        ++i;
        rreq = rreq->dev.next;
    }

    fprintf(stream, "CH3 Unexpected RecvQ:\n");
    rreq = recvq_unexpected_head;
    i = 0;
    while (rreq != NULL) {
        fprintf(stream, "..[%d] rreq=%p ctx=%#x rank=%s tag=%s\n", i, rreq,
                        rreq->dev.match.context_id,
                        rank_val_to_str(rreq->dev.match.rank, rank_buf, sizeof(rank_buf)),
                        tag_val_to_str(rreq->dev.match.tag, tag_buf, sizeof(tag_buf)));
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

    while (req)
    {
        ++count;
        req = req->dev.next;
    }

    return count;
}
