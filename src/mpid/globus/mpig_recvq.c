/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

MPIG_STATIC mpig_mutex_t mpig_recvq_mutex;

#define mpig_recvq_mutex_construct()	mpig_mutex_construct(&mpig_recvq_mutex)
#define mpig_recvq_mutex_destruct()	mpig_mutex_destruct(&mpig_recvq_mutex)
#define mpig_recvq_mutex_lock()							\
{										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - acquiring mutex"));				\
    mpig_mutex_lock(&mpig_recvq_mutex);                                         \
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - mutex acquired"));				\
}
#define mpig_recvq_mutex_unlock()						\
{										\
    mpig_mutex_unlock(&mpig_recvq_mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - mutex released"));				\
}


/* head and tail pointers for the posted and unexpected receive queues */
MPIG_STATIC MPID_Request * mpig_recvq_posted_head;
MPIG_STATIC MPID_Request * mpig_recvq_posted_tail;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_head;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_tail;


/* receive-any-source operation wait to be unregistered */
MPIG_STATIC mpig_genq_t mpig_recvq_unreg_rasq;

static MPIG_INLINE void mpig_recvq_unreg_rasq_enqueue_op(mpig_recvq_ras_op_t * ras_op)
{
    mpig_genq_entry_t * q_entry = &ras_op->q_entry;
    mpig_genq_entry_set_value(q_entry, ras_op);
    mpig_genq_enqueue_tail_entry(&mpig_recvq_unreg_rasq, q_entry);
}

static MPIG_INLINE mpig_recvq_ras_op_t * mpig_recvq_unreg_rasq_dequeue_op(void)
{
    mpig_genq_entry_t * q_entry;
    mpig_recvq_ras_op_t * ras_op;
    
    q_entry = mpig_genq_dequeue_head_entry(&mpig_recvq_unreg_rasq);
    if (q_entry)
    {
        ras_op = mpig_genq_entry_get_value(q_entry);
    }
    else
    {
        ras_op = NULL;
    }

    return ras_op;
}

static MPIG_INLINE void mpig_recvq_unreg_rasq_remove_op(mpig_recvq_ras_op_t * ras_op)
{
    mpig_genq_entry_t * q_entry = &ras_op->q_entry;
    mpig_genq_remove_entry(&mpig_recvq_unreg_rasq, q_entry);
}

/* routines to operate on receive-any-source operation description objects */
static MPIG_INLINE void mpig_recvq_ras_op_construct(mpig_recvq_ras_op_t * ras_op, MPID_Request * ras_req)
{
    mpig_genq_entry_t * q_entry = &ras_op->q_entry;

    ras_op->complete = FALSE;
    ras_op->cancelled = FALSE;
    ras_op->req = ras_req;
    ras_op->cond_in_use = FALSE;
    mpig_genq_entry_construct(q_entry);
    mpig_genq_entry_set_value(q_entry, NULL);
}

static MPIG_INLINE void mpig_recvq_ras_op_destruct(mpig_recvq_ras_op_t * ras_op)
{
    mpig_genq_entry_t * q_entry = &ras_op->q_entry;

    ras_op->complete = FALSE;
    ras_op->cancelled = FALSE;
    ras_op->req = MPIG_INVALID_PTR;
    if (ras_op->cond_in_use)
    {
	mpig_cond_destruct(&ras_op->cond);
	ras_op->cond_in_use = FALSE;
    }
    mpig_genq_entry_destruct(q_entry);
}

/* 
 * static MPIG_INLINE bool_t mpig_recvq_ras_op_is_complete(mpig_recvq_ras_op_t * ras_op)
 * {
 *     return ras_op->complete;
 * }
 */

static void mpig_recvq_ras_op_wait_complete(mpig_vc_t * vc, mpig_recvq_ras_op_t * ras_op)
{
    if (ras_op->complete == FALSE)
    {
	mpig_cond_construct(&ras_op->cond);
	ras_op->cond_in_use = TRUE;
	ras_op->req->dev.recvq_unreg_ras_op = ras_op;
	mpig_vc_mutex_unlock_conditional(vc, (vc != NULL));
	{
	    do
	    {
		mpig_cond_wait(&ras_op->cond, &mpig_recvq_mutex);
	    }
	    while(ras_op->complete == FALSE);
	}
	mpig_vc_mutex_lock_conditional(vc, (vc != NULL));
	/* (ras_op_)->req->recvq_unreg_ras_op = NULL; -- req may have completed and thus be invalid */
    }
}

static MPIG_INLINE bool_t mpig_recvq_ras_op_is_enqueued(mpig_recvq_ras_op_t * ras_op)
{
    mpig_genq_entry_t * q_entry = &ras_op->q_entry;
    bool_t is_enq = (mpig_genq_entry_get_value(q_entry) != NULL) ? TRUE : FALSE;

    return is_enq;
}


/*
 * <mpi_errno> mpig_recvq_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_init
int mpig_recvq_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering"));

    mpig_recvq_mutex_construct();
    mpig_recvq_posted_head = NULL;
    mpig_recvq_posted_tail = NULL;
    mpig_recvq_unexp_head = NULL;
    mpig_recvq_unexp_tail = NULL;

    mpig_genq_construct(&mpig_recvq_unreg_rasq);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_init);
    return mpi_errno;
}
/* mpig_recvq_init() */

/*
 * <mpi_errno> mpig_recvq_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_finalize
int mpig_recvq_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_destroy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering"));

    mpig_genq_destruct(&mpig_recvq_unreg_rasq, NULL);

    mpig_recvq_posted_head = MPIG_INVALID_PTR;
    mpig_recvq_posted_tail = MPIG_INVALID_PTR;
    mpig_recvq_unexp_head = MPIG_INVALID_PTR;
    mpig_recvq_unexp_tail = MPIG_INVALID_PTR;
    mpig_recvq_mutex_destruct();

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_destroy);
    return mpi_errno;
}
/* mpig_recvq_finalize() */

/*
 * <found flag> mpig_recvq_find_unexp_and_extract_status()
 *
 * find a request in the unexpected queue matching the supplied envelope and extract the status.
 *
 * Returns: TRUE if a matching request was found; FALSE if the no matching request is found
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_find_unexp_and_extract_status
int mpig_recvq_find_unexp_and_extract_status(const int rank, const int tag, MPID_Comm * const comm, const int ctx,
    bool_t * found_p, MPI_Status * const status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * rreq;
    bool_t found = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_find_unexp_and_extract_status);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_find_unexp_and_extract_status);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));

    mpig_recvq_mutex_lock();
    {
	const int rank_mask = (rank == MPI_ANY_SOURCE) ? 0 : ~0;
	const int tag_mask = (tag == MPI_ANY_TAG) ? 0 : ~0;

	rreq = mpig_recvq_unexp_head;
	while (found == FALSE && rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    
	    if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "MATCHED req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", rank=%d"
		    ", tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
		
		/* MT-NOTE: holding the recvq mutex does not mean that the matched request is ready for use.  the routine that
		   enqueues unexpected requests, mpig_recvq_deq_posted_or_enq_unexp(), returns the request to the calling routine
		   with the request's mutex locked.  the calling routine then completes the initialization of the request,
		   including the status structure, before releasing the request's mutex.  the request mutex must be acquired here
		   to insure that the initialization of the status structure is complete prior to extracting the information from
		   it.  it is safe to assume that the request will not be dequeued, completed and destroyed prior to the
		   acquisition of the of the request's mutex since the request cannot be matched and removed from the unexpected
		   queue until we release the recvq mutex. */
		mpig_request_mutex_lock(rreq);
		{
		    MPIR_Request_extract_status(rreq, status);
		}
		mpig_request_mutex_unlock(rreq);

		found = TRUE;
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "skipping req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", rank=%d"
		    ", tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
		rreq = rreq->dev.next;
	    }
	}
	/* end while (found == FALSE && rreq != NULL) */
    }
    mpig_recvq_mutex_unlock();

    *found_p = found;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rank=%d, tag=%d, ctx=%d, found=%s",
	rank, tag, ctx, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_find_unexp_and_extract_status);
    return mpi_errno;
}
/* mpig_recvq_find_unexp_and_extract_status() */

/*
 * <rreq> mpig_recvq_find_unexp_and_deq()
 *
 * find a request in the unexpected queue matching the supplied envelope, and dequeue that request.  Return a pointer to the request object if a matching request
 * was found; otherwise return NULL.  The primary purpose of this routine is to support canceling an already received but not
 * matched message.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_find_unexp_and_deq
MPID_Request * mpig_recvq_find_unexp_and_deq(const int rank, const int tag, const int ctx, const MPI_Request sreq_id)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_find_unexp_and_deq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_find_unexp_and_deq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rank=%d, tag=%d, ctx=%d, sreq_id="
	MPIG_HANDLE_FMT, rank, tag, ctx, sreq_id));

    mpig_recvq_mutex_lock();
    {
	MPID_Request * prev_rreq = NULL;
	MPID_Request * cur_rreq = mpig_recvq_unexp_head;
	MPID_Request * matching_prev_rreq = NULL;
	MPID_Request * matching_cur_rreq = NULL;
	
	/* since this routine is intended for cancelling an already received unexpected message, we must look for the last entry
	   in the unexpected queue that matches the (rank, tag, context id, and send request id) tuple.  the reason we look for
	   the last entry is that it will be the most recently sent and thus must match the currently active send attempting to
	   be cancelled by the remote process. */
	while(cur_rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(cur_rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    if (rreq_ctx == ctx && rreq_rank == rank && rreq_tag == tag && mpig_request_get_remote_req_id(cur_rreq) == sreq_id)
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
		mpig_recvq_unexp_head = matching_cur_rreq->dev.next;
	    }
	
	    if (matching_cur_rreq->dev.next == NULL)
	    {
		mpig_recvq_unexp_tail = matching_prev_rreq;
	    }

	    rreq = matching_cur_rreq;
	    rreq->dev.next = NULL;
	}
	else
	{
	    rreq = NULL;
	}
    }
    mpig_recvq_mutex_unlock();

    /* If the request is still being initialized by another thread, wait for that process to complete */
    mpig_request_mutex_lock_conditional(rreq, (rreq != NULL));
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
	MPIG_HANDLE_VAL(rreq), MPIG_PTR_CAST(rreq)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_find_unexp_and_deq);
    return rreq;
}
/* mpig_recvq_find_unexp_and_deq() */


/*
 * <bool: found> mpig_recvq_deq_unexp_rreq()
 *
 * Given a pointer to a reuqest object, attempt to find that request in the posted queue and dequeue it.  Return TRUE if the
 * request was located and dequeued, or FALSE if the request was not found.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_rreq
bool_t mpig_recvq_deq_unexp_rreq(MPID_Request * const rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t found = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_rreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_rreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
	rreq->handle, MPIG_PTR_CAST(rreq)));
    
    mpig_recvq_mutex_lock();
    {
	MPID_Request * cur_rreq = mpig_recvq_unexp_head;
	MPID_Request * prev_rreq = NULL;

	while (found == FALSE && cur_rreq != NULL)
	{
	    if (cur_rreq == rreq)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = cur_rreq->dev.next;
		}
		else
		{
		    mpig_recvq_unexp_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    mpig_recvq_unexp_tail = prev_rreq;
		}
	    
		cur_rreq->dev.next = NULL;
		found = TRUE;
	    }
	    else
	    {
		prev_rreq = cur_rreq;
		cur_rreq = cur_rreq->dev.next;
	    }
	}
	/* end while (found == FALSE && cur_rreq != NULL) */
    }
    mpig_recvq_mutex_unlock();

    /* wait for any pending initialization to complete */
    if (found)
    {
	mpig_request_mutex_lock(rreq);
	mpig_request_mutex_unlock(rreq);
    }
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
	", found=%s", rreq->handle, MPIG_PTR_CAST(rreq), MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_rreq);
    return found;
}
/* mpig_recvq_deq_unexp_rreq() */


/*
 * <int: mpi_errno> mpig_recvq_cancel_posted_rreq([IN/MOD] rreq, [OUT] cancelled)
 *
 * given a pointer to a request object, attempt to find that request in the posted queue and dequeue it.  if the request's source
 * process is MPI_ANY_SOURCE, vendor MPI is in use, and an associated vendor MPI request exists, then initiate the cancellation
 * of the vendor MPI request.
 * 
 * Parameters:
 *
 *   rreq - posted receive request to be cancelled
 *
 *   cancelled [OUT] - TRUE if the request was located and dequeued; FALSE if the request was not found or the cancellation of an
 *   associated vendor MPI request is pending
 *
 * Returns: MPICH2 error code
 *
 * MPI-MT-NOTE: since the vendor MPI may only be capable of MPI_THREAD_SINGLE, and the vendor MPI_Cancel routine be called, this
 * routine may only be called from the main thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_cancel_posted_rreq
int mpig_recvq_cancel_posted_rreq(MPID_Request * const rreq, bool_t * cancelled_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t found = FALSE;
    bool_t cancelled = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_cancel_posted_rreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_cancel_posted_rreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
	rreq->handle, MPIG_PTR_CAST(rreq)));
    
    mpig_recvq_mutex_lock();
    {
	MPID_Request * cur_rreq = mpig_recvq_posted_head;
	MPID_Request * prev_rreq = FALSE;
	while (found == FALSE && cur_rreq != NULL)
	{
	    if (cur_rreq == rreq)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = cur_rreq->dev.next;
		}
		else
		{
		    mpig_recvq_posted_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    mpig_recvq_posted_tail = prev_rreq;
		}
	    
		cur_rreq->dev.next = NULL;
		found = TRUE;

#		if defined(MPIG_VMPI)
		{
		    if (mpig_request_get_rank(rreq) == MPI_ANY_SOURCE)
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "receive any source request found and removed from the posted "
			    "queued: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

			/* only perform the vendor MPI_Cancel() if the vendor request is valid and has not completed */
			if (mpig_cm_vmpi_comm_has_remote_vprocs(rreq->comm) == FALSE)
			{
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "communicator associated with the request does not contain "
				"any vendor MPI processes; skipping cancellation of vendor request: rreq=" MPIG_HANDLE_FMT
				", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
			    cancelled = TRUE;
			}
			else
			{
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "communicator associated with the request contains vendor "
				"MPI processes; cancelling vendor request: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
				rreq->handle, MPIG_PTR_CAST(rreq)));
			    
			    mpi_errno = mpig_cm_vmpi_cancel_recv_any_source(rreq, &cancelled);
			    if (mpi_errno)
			    {   /* --BEGIN ERROR HANDLING-- */
				MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_RECVQ, "ERROR: attempt to cancel the "
				    "vendor MPI request failed: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle,
				    MPIG_PTR_CAST(rreq)));
				MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|cancel_ras",
				    "**globus|cm_vmpi|cancel_ras %R", rreq);
			    }  /* --END ERROR HANDLING-- */
			}
		    }
		    else
		    {
			cancelled = TRUE;
		    }
		}
#		else
		{
		    cancelled = TRUE;
		}
#		endif
	    }
	    else
	    {
		prev_rreq = cur_rreq;
		cur_rreq = cur_rreq->dev.next;
	    }
	}
	/* end while (found == FALSE && cur_rreq != NULL) */
    }
    mpig_recvq_mutex_unlock();
	
    /* NOTE: attempting to cancel a partially initialized request is not legal; therefore, it is unnecessary to wait for any
       pending initialization to complete */

    *cancelled_p = cancelled;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
	", found=%s, cancelled=%s, mpi_errno=" MPIG_ERRNO_FMT, rreq->handle, MPIG_PTR_CAST(rreq), MPIG_BOOL_STR(found),
	MPIG_BOOL_STR(cancelled), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_cancel_posted_rreq);
    return mpi_errno;
}
/* mpig_recvq_cancel_posted_rreq() */


#if defined(MPIG_VMPI)
/*
 * <bool: ras_req_valid> mpig_recvq_deq_posted_ras_req()
 *
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_ras_req
bool_t mpig_recvq_deq_posted_ras_req(MPID_Request * const ras_req, bool_t vcancelled)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_recvq_ras_op_t * ras_op = ras_req->dev.recvq_unreg_ras_op;
    bool_t found = FALSE;
    bool_t ras_req_valid = TRUE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_ras_req);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_ras_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: ras_req=" MPIG_HANDLE_FMT ", ras_reqp="
	MPIG_PTR_FMT ", vcancelled=%s", ras_req->handle, MPIG_PTR_CAST(ras_req), MPIG_BOOL_STR(vcancelled)));
    
    mpig_recvq_mutex_lock();
    {
	MPID_Request * cur_rreq = mpig_recvq_posted_head;
	MPID_Request * prev_rreq = FALSE;
	while (found == FALSE && cur_rreq != NULL)
	{
	    if (cur_rreq == ras_req)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = cur_rreq->dev.next;
		}
		else
		{
		    mpig_recvq_posted_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    mpig_recvq_posted_tail = prev_rreq;
		}
	    
		ras_req->dev.next = NULL;

		found = TRUE;
	    }
	    else
	    {
		prev_rreq = cur_rreq;
		cur_rreq = cur_rreq->dev.next;
	    }
	}
	/* end while (found == FALSE && cur_rreq != NULL) */

	/* if a thread is waiting for the status of a paired vendor MPI request, then send it the status. */
	if (ras_op != NULL)
	{
	    MPIU_Assert(found == FALSE);
	    if (mpig_recvq_ras_op_is_enqueued(ras_op))
	    {
		/* if an RAS unregister operation for this request is enqueued, then the unregister has yet to be performed.
	           this can happen if the operation is enqueued by mpig_recvq_deq_posted_or_enq_unexp() after the progress engine
	           calls mpig_recvq_unregister_ras_vreqs().  we must dequeue the operation, otherwise the an attempt will be made
	           to cancel an invalid request the next time mpig_recvq_unregister_ras_vreqs() is called.

                   since the unregister has to be performed, the message could not possibly have been cancelled and so the
                   cancelled state is left set to false. */
		mpig_recvq_unreg_rasq_remove_op(ras_op);
		/* mpig_recvq_unexp_op_set_cancelled_state(ras_op, FALSE); -- already set when the object is constructed */
	    }
	    else
	    {
		/* the RAS unregister operation has been performed for this request.  set the cancel state of the vendor MPI
		   request. */
		mpig_recvq_ras_op_set_cancelled_state(ras_op, vcancelled);
		ras_req_valid = !vcancelled;
	    }
	    
	    /* signal the thread waiting in mpig_recvq_deq_posted_or_enq_unexp() so that it may proceed */
	    mpig_recvq_ras_op_signal_complete(ras_op);
	}
    }
    mpig_recvq_mutex_unlock();
    
    /* it is possible for a progress engine to attempt dequeue the posted request before the initialization request is
       completed; therefore, it is necessary to the wait for any pending initialization to complete */
    if (found)
    {
	mpig_request_mutex_lock(ras_req);
	mpig_request_mutex_unlock(ras_req);
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: ras_req=" MPIG_HANDLE_FMT ", ras_reqp="
	MPIG_PTR_FMT ", found=%s, valid=%s", ras_req->handle, MPIG_PTR_CAST(ras_req), MPIG_BOOL_STR(found),
	MPIG_BOOL_STR(ras_req_valid)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_ras_req);
    return ras_req_valid;
}
/* mpig_recvq_deq_posted_ras_req() */
#endif

/*
 * <mpi_errno> mpig_recvq_deq_unexp_or_enq_posted()
 *
 * Find a matching request in the unexpected queue and dequeue it; otherwise allocate a new request and enqueue it in the posted
 * queue.  This is an atomic operation.
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_or_enq_posted
int mpig_recvq_deq_unexp_or_enq_posted(const int rank, const int tag, const int ctx,
    const mpig_msg_op_params_t * const ras_params, bool_t * const found_p, MPID_Request ** const rreq_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t recvq_locked = FALSE;
    bool_t found = FALSE;
    MPID_Request * rreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
    
    mpig_recvq_mutex_lock();
    recvq_locked = TRUE;
    {
	const int rank_mask = (rank == MPI_ANY_SOURCE) ? 0 : ~0;;
	const int tag_mask = (tag == MPI_ANY_TAG) ? 0 : ~0;;
	MPID_Request * prev_rreq = NULL;

	rreq = mpig_recvq_unexp_head;
	while (found == FALSE && rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
	    
	    mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    
	    if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = rreq->dev.next;
		}
		else
		{
		    mpig_recvq_unexp_head = rreq->dev.next;
		}
		if (rreq->dev.next == NULL)
		{
		    mpig_recvq_unexp_tail = prev_rreq;
		}
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "MATCHED UNEXP: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
		    ", rank=%d, tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
		
		found = TRUE;
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "skipping: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, "
		    "tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}
	/* end while (found == FALSE && rreq != NULL) */
	
	if (found == FALSE)
	{
	    /* if a matching request was not found in the unexpected queue, then allocate and initialize a new request */
	    mpig_request_alloc(&rreq);
	    if (rreq == NULL)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_RECVQ | MPIG_DEBUG_LEVEL_REQ,
		    "ERROR: malloc failed when attempting to allocate a new receive request"));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	    
	    mpig_request_construct(rreq);
	    mpig_request_set_envelope(rreq, rank, tag, ctx);
	    
	    /* if vendor MPI is enabled, and receive any source parameters have been passed in, then first register the receive
	       any source operation with with the VMPI module.  besides an error condition, two possible things can occur.  the
	       operation is successfully registered but no matching message existed.  alternatively, the vendor MPI receive
	       operation matched a message.  in the latter case, a request is created for the vendor MPI operation, which may
	       have already completed, and returned. */
#           if defined(MPIG_VMPI)
	    {
		if (rank == MPI_ANY_SOURCE && mpig_cm_vmpi_comm_has_remote_vprocs(ras_params->comm))
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "registering any source receive with the VMPI CM"));

		    mpi_errno = mpig_cm_vmpi_register_recv_any_source(ras_params, &found, rreq);
		    if (mpi_errno)
		    {   /* --BEGIN ERROR HANDLING-- */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_RECVQ, "ERROR: registration of receive any "
			    "source with the VMPI CM failed: rank=%d, tag=%d, ctx=%d, comm=" MPIG_HANDLE_FMT ", commp="
			    MPIG_PTR_FMT ", ctxoff=%d", ras_params->rank, ras_params->tag, ras_params->comm->context_id +
			    ras_params->ctxoff, MPIG_PTR_CAST(ras_params->comm), ras_params->ctxoff));
			MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|reg_ras");
			goto fn_fail;
		    }   /* --END ERROR HANDLING-- */

		    if (found)
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "VMPI CM receive any source completed; returning found=TRUE: "
			    "rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
		    }
		    else
		    {
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "registered receive any source with the VMPI CM: rreq="
			    MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
		    }
		}
	    }
#           endif
	}
	/* end if (found == FALSE) */

	/* if a matching request was not found, either in the global receive queue or by the vendor MPI, then insert the request
	   into the posted queue */
	if (found == FALSE)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "ENQUEUE POSTED: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
		", rank=%d, tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rank, tag, ctx));
	
	    /* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it
	       and attempt to use it before the calling routine has a chance to initialized it */
	    mpig_request_mutex_lock(rreq);

	    /* add the request to the posted queue */
	    if (mpig_recvq_posted_tail != NULL)
	    {
		mpig_recvq_posted_tail->dev.next = rreq;
	    }
	    else
	    {
		mpig_recvq_posted_head = rreq;
	    }
	    mpig_recvq_posted_tail = rreq;
		
	    rreq->dev.next = NULL;
	}
	/* end if (found == FALSE) */
    }
    mpig_recvq_mutex_unlock();
    recvq_locked = FALSE;

    if (found)
    {
	/* wait until the request has been initialized before returning */
	mpig_request_mutex_lock(rreq);
    }

    *found_p = found;
    *rreq_p = rreq;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rank=%d, tag=%d, ctx=%d, rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", found=%s", rank, tag, ctx, MPIG_HANDLE_VAL(rreq), MPIG_PTR_CAST(rreq), MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (recvq_locked)
    {
	mpig_recvq_mutex_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_unexp_or_enq_posted() */

/*
 * <mpi_errno> mpig_recvq_deq_posted_or_enq_unexp()
 *
 * Find a matching request in the posted queue and dequeue it; otherwise allocate a new request and enqueue it in the expected
 * queue.  This is an atomic operation.  
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_or_enq_unexp
int mpig_recvq_deq_posted_or_enq_unexp(mpig_vc_t * const vc, const int rank, const int tag, const int ctx, bool_t * const found_p,
    MPID_Request ** const rreq_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t recvq_locked = FALSE;
    bool_t found;
    MPID_Request * rreq;
    int rreq_rank;
    int rreq_tag;
    int rreq_ctx;
    int rank_mask;
    int tag_mask;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));

    mpig_recvq_mutex_lock();
    recvq_locked = TRUE;
    {
	do /* while (MPIG_USING_VMPI && rreq == NULL) */
	{
	    MPID_Request * prev_rreq = NULL;

	    found = FALSE;
	    rreq = mpig_recvq_posted_head;
	    while (rreq != NULL && found == FALSE)
	    {
		mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
		rank_mask = (rreq_rank == MPI_ANY_SOURCE) ? 0 : ~0;
		tag_mask = (rreq_tag == MPI_ANY_TAG) ? 0 : ~0;
		
		if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
		{
		    if (prev_rreq != NULL)
		    {
			prev_rreq->dev.next = rreq->dev.next;
		    }
		    else
		    {
			mpig_recvq_posted_head = rreq->dev.next;
		    }
		    if (rreq->dev.next == NULL)
		    {
			mpig_recvq_posted_tail = prev_rreq;
		    }

		    found = TRUE;

#		    if defined(MPIG_VMPI)
		    {
			bool_t comm_has_remote_vprocs;
			
			/* MT-NOTE: holding the recvq mutex does not mean that the matched request is ready for use.  only the
			   envelope information in the request is protected by the recvq mustex.  the routine that enqueues
			   posted requests, mpig_recvq_deq_unexp_or_enq_posted(), returns the request to the calling routine with
			   the request's mutex locked.  the calling routine then completes the initialization of the request
			   before releasing the request's mutex.  the request mutex must be acquired here to insure that the
			   initialization process is complete. */
			mpig_request_mutex_lock(rreq);
			{
			    comm_has_remote_vprocs = mpig_cm_vmpi_comm_has_remote_vprocs(rreq->comm);
			}
			mpig_request_mutex_unlock(rreq);
			
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "consider posted request: rreq=" MPIG_HANDLE_FMT ", rreqp="
			    MPIG_PTR_FMT "rank=%d, tag=%d, ctx=%d, comm_has_remote_vprocs=%s", rreq->handle, MPIG_PTR_CAST(rreq),
			    rreq_rank, rreq_tag, rreq_ctx, MPIG_BOOL_STR(comm_has_remote_vprocs)));
			
			if (rreq_rank == MPI_ANY_SOURCE && comm_has_remote_vprocs)
			{
			    /*
			     * if the request source was MPI_ANY_SOURCE, then the associated vendor MPI request must be cancelled
			     * before the request is returned to the calling routine.  if the cancellation of the vendor MPI
			     * request fails because it has already been matched by an incoming vendor MPI message, then the
			     * request, which has already been removed from the queue, is treated as though it had never been
			     * found.  in this case, the search for a matching request in the posted queue must be restarted from
			     * the beginning of the queue.
			     *
			     * the problem with performing a call to the vendor MPI to attempt the cancellation of the associated
			     * vendor request is that this function (mpig_recvq_deq_posted_or_enq_unexp) may be called from any
			     * thread, whereas the vendor MPI_Cancel may only be called from the main thread.  this can be solved
			     * by having the VMPI CM cancellation routine queue up the request as one that must be marked for
			     * cancellation during the next polling cycle by the main thread.  the current thread will be block
			     * until the main thread determines if the vendor MPI request has been cancelled or completed.  since
			     * the current thread may be blocked for some time, it is important that the cancellation process
			     * release the recvq mutex befor it blocks.  if the current thread were to continue to hold the recvq
			     * mutex, the main thread may block in another recvq routine resulting in deadlock.
			     *
			     * MT-NOTE: this algorithm will preserve message ordering provided no other thread attempts to
			     * simultaneously dequeue a request from the same source processs that matches by the tag and context
			     * supplied to the current thread when it called this routine.
			     */
			    bool_t vcancelled;
			    mpig_recvq_ras_op_t ras_op;
		
			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "unregistering receieve any source from the VMPI CM: "
				"rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

			    mpig_recvq_ras_op_construct(&ras_op, rreq);

			    if (mpig_cm_vmpi_thread_is_main())
			    {
				mpi_errno = mpig_cm_vmpi_unregister_recv_any_source(&ras_op);
				MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|unreg_ras");
			    }
			    else
			    {
				mpig_recvq_unreg_rasq_enqueue_op(&ras_op);
			    }
			    
			    mpig_recvq_ras_op_wait_complete(vc, &ras_op);
			    vcancelled = mpig_recvq_ras_op_get_cancelled_state(&ras_op);
			    
			    mpig_recvq_ras_op_destruct(&ras_op);
			    
			    if (vcancelled == FALSE)
			    {
				rreq = NULL;
			    }

			    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "receive any source unregistered from the VMPI CM: "
				"vcancelled=%s, rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, MPIG_BOOL_STR(vcancelled),
				MPIG_HANDLE_VAL(rreq), MPIG_PTR_CAST(rreq)));
			}
			/* end if (rreq_rank == MPI_ANY_SOURCE && mpig_cm_vmpi_comm_has_remote_vprocs(rreq->comm)) */
		    }
#	            endif /* if defined(MPIG_VMPI) */
		
		}
		else
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
			"skipping: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d" ",ctx=%d",
			rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
		    prev_rreq = rreq;
		    rreq = rreq->dev.next;
		}
	    }
	    /* end while (rreq != NULL && found == FALSE) */

	    if (found == FALSE)
	    {
		/* A matching request was not found in the posted queue, so we need to allocate a new request and add it to the
		   unexpected queue */
		mpig_request_alloc(&rreq);
		if (rreq == NULL)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_RECVQ,
			"ERROR: malloc failed when attempting to allocate a new receive request"));
		    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
		    goto fn_fail;
		}   /* --END ERROR HANDLING-- */

		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "ENQUEDED UNEXP: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
		    ", rank=%d, tag=%d ,ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rank, tag, ctx));
	
		/* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it
		   and attempt to use it before the calling routine has a chance to initialized it */
		mpig_request_mutex_lock(rreq);
		mpig_request_construct(rreq);
		mpig_request_set_envelope(rreq, rank, tag, ctx);
	
		if (mpig_recvq_unexp_tail != NULL)
		{
		    mpig_recvq_unexp_tail->dev.next = rreq;
		}
		else
		{
		    mpig_recvq_unexp_head = rreq;
		}
	
		mpig_recvq_unexp_tail = rreq;
	    }
	    /* end if (rreq != NULL && found == FALSE) */
    
	}
	while (MPIG_USING_VMPI && rreq == NULL);
    }
    mpig_recvq_mutex_unlock();
    recvq_locked = FALSE;
	
    if (found)
    { 
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ, "MATCHED POSTED: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
	    ", rank=%d, tag=%d, ctx=%d", rreq->handle, MPIG_PTR_CAST(rreq), rreq_rank, rreq_tag, rreq_ctx));
	
	/* wait until the request has been initialized */
	mpig_request_mutex_lock(rreq);
    }

    rreq->dev.next = NULL;
    *found_p = found;
    *rreq_p = rreq;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: rank=%d, tag=%d, ctx=%d, rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", found=%s", rank, tag, ctx, MPIG_HANDLE_VAL(rreq), MPIG_PTR_CAST(rreq), MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (recvq_locked)
    {
	mpig_recvq_mutex_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_posted_or_enq_unexp() */

#if defined(MPIG_VMPI)
/*
 * <mpi_errno> mpig_recvq_unregister_ras_vreqs()
 *
 * Find a matching request in the posted queue and dequeue it; otherwise allocate a new request and enqueue it in the expected
 * queue.  This is an atomic operation.  
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_unregister_ras_vreqs
int mpig_recvq_unregister_ras_vreqs(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t recvq_locked = FALSE;
    mpig_recvq_ras_op_t * ras_op;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_unregister_ras_vreqs);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_unregister_ras_vreqs);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "entering"));

    mpig_recvq_mutex_lock();
    recvq_locked = TRUE;
    {
	ras_op = mpig_recvq_unreg_rasq_dequeue_op();
	while(ras_op != NULL)
	{
	    mpi_errno = mpig_cm_vmpi_unregister_recv_any_source(ras_op);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|unreg_ras");

	    ras_op = mpig_recvq_unreg_rasq_dequeue_op();
	}
    }
    mpig_recvq_mutex_unlock();
    recvq_locked = FALSE;
	
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_unregister_ras_vreqs);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (recvq_locked)
    {
	mpig_recvq_mutex_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_unregister_ras_vreqs() */
#endif
