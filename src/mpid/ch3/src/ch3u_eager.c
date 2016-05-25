/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidi_recvq_statistics.h"

/*
 * Send an eager message.  To optimize for the important, short contiguous
 * message case, there are separate routines for the contig and non-contig
 * datatype cases.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_SendNoncontig_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDI_CH3_SendNoncontig_iov - Sends a message by loading an
   IOV and calling iSendv.  The caller must initialize
   sreq->dev.segment as well as segment_first and segment_size. */
int MPIDI_CH3_SendNoncontig_iov( MPIDI_VC_t *vc, MPIR_Request *sreq,
                                 void *header, intptr_t hdr_sz )
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPL_IOV iov[MPL_IOV_LIMIT];
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_SENDNONCONTIG_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_SENDNONCONTIG_IOV);

    iov[0].MPL_IOV_BUF = header;
    iov[0].MPL_IOV_LEN = hdr_sz;

    iov_n = MPL_IOV_LIMIT - 1;

    if (sreq->dev.ext_hdr_sz > 0) {
        /* When extended packet header exists, here we leave one IOV slot
         * before loading data to IOVs, so that there will be enough
         * IOVs for hdr/ext_hdr/data. */
        iov_n--;
    }

    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
    if (mpi_errno == MPI_SUCCESS)
    {
	iov_n += 1;
	
	/* Note this routine is invoked withing a CH3 critical section */
	/* MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex); */
	mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	/* MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex); */
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
            MPIR_Request_free(sreq);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
	}
	/* --END ERROR HANDLING-- */

	/* Note that in the non-blocking case, we need to add a ref to the
	   datatypes */
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
        MPIR_Request_free(sreq);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");
	/* --END ERROR HANDLING-- */
    }


 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_SENDNONCONTIG_IOV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* This function will allocate a segment.  That segment must be freed when
   it is no longer needed */
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerNoncontigSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDI_CH3_EagerNoncontigSend - Eagerly send noncontiguous data */
int MPIDI_CH3_EagerNoncontigSend( MPIR_Request **sreq_p,
				  MPIDI_CH3_Pkt_type_t reqtype, 
				  const void * buf, MPI_Aint count,
				  MPI_Datatype datatype, intptr_t data_sz,
				  int rank, 
				  int tag, MPIR_Comm * comm,
				  int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIR_Request *sreq = *sreq_p;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
                     "sending non-contiguous eager message, data_sz=%" PRIdPTR,
					data_sz));
    sreq->dev.OnDataAvail = 0;
    sreq->dev.OnFinal = 0;

    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.parts.rank	= comm->rank;
    eager_pkt->match.parts.tag	= tag;
    eager_pkt->match.parts.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= MPI_REQUEST_NULL;
    eager_pkt->data_sz		= data_sz;
    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);

    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);

    MPL_DBG_MSGPKT(vc,tag,eager_pkt->match.parts.context_id,rank,data_sz,
                    "Eager");
	    
    sreq->dev.segment_ptr = MPIR_Segment_alloc( );
    MPIR_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");

    MPIR_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;
	    
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = vc->sendNoncontig_fn(vc, sreq, eager_pkt, 
                                     sizeof(MPIDI_CH3_Pkt_eager_send_t));
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    *sreq_p = NULL;
    goto fn_exit;
}

/* Send a contiguous eager message.  We'll want to optimize (and possibly
   inline) this.

   Make sure that buf is at the beginning of the data to send; 
   adjust by adding dt_true_lb if necessary 
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerContigSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_EagerContigSend( MPIR_Request **sreq_p,
			       MPIDI_CH3_Pkt_type_t reqtype, 
			       const void * buf, intptr_t data_sz, int rank,
			       int tag, MPIR_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    MPIR_Request *sreq = *sreq_p;
    MPL_IOV iov[2];
    
    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.parts.rank	= comm->rank;
    eager_pkt->match.parts.tag	= tag;
    eager_pkt->match.parts.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= MPI_REQUEST_NULL;
    eager_pkt->data_sz		= data_sz;
    
    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)eager_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*eager_pkt);
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
	       "sending contiguous eager message, data_sz=%" PRIdPTR,
					data_sz));
	    
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) buf;
    iov[1].MPL_IOV_LEN = data_sz;
    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
    
    MPL_DBG_MSGPKT(vc,tag,eager_pkt->match.parts.context_id,rank,data_sz,"EagerContig");
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, sreq_p);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|eagermsg");
    }

    sreq = *sreq_p;
    if (sreq != NULL)
    {
	MPIDI_Request_set_seqnum(sreq, seqnum);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    }

 fn_fail:
    return mpi_errno;
}

#ifdef USE_EAGER_SHORT
/* Send a short contiguous eager message.  We'll want to optimize (and possibly
   inline) this 

   Make sure that buf is at the beginning of the data to send; 
   adjust by adding dt_true_lb if necessary 

   We may need a nonblocking (cancellable) version of this, which will 
   have a smaller payload.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerContigShortSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_EagerContigShortSend( MPIR_Request **sreq_p,
				    MPIDI_CH3_Pkt_type_t reqtype,
				    const void * buf, intptr_t data_sz, int rank,
				    int tag, MPIR_Comm * comm,
				    int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eagershort_send_t * const eagershort_pkt = 
	&upkt.eagershort_send;
    MPIR_Request *sreq = *sreq_p;
    
    /*    printf( "Sending short eager\n"); fflush(stdout); */
    MPIDI_Pkt_init(eagershort_pkt, reqtype);
    eagershort_pkt->match.parts.rank	     = comm->rank;
    eagershort_pkt->match.parts.tag	     = tag;
    eagershort_pkt->match.parts.context_id = comm->context_id + context_offset;
    eagershort_pkt->data_sz	     = data_sz;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
       "sending contiguous short eager message, data_sz=%" PRIdPTR,
					data_sz));
	    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eagershort_pkt, seqnum);

    /* Copy the payload. We could optimize this if data_sz & 0x3 == 0 
       (copy (data_sz >> 2) ints, inline that since data size is 
       currently limited to 4 ints */
    {
	unsigned char * restrict p = 
	    (unsigned char *)eagershort_pkt->data;
	unsigned char const * restrict bufp = (unsigned char *)buf;
	int i;
	for (i=0; i<data_sz; i++) {
	    *p++ = *bufp++;
	}
    }

    MPL_DBG_MSGPKT(vc,tag,eagershort_pkt->match.parts.context_id,rank,data_sz,
		    "EagerShort");
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, eagershort_pkt, sizeof(*eagershort_pkt), sreq_p);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|eagermsg");
    }
    sreq = *sreq_p;
    if (sreq != NULL) {
	/*printf( "Surprise, did not complete send of eagershort (starting connection?)\n" ); 
	  fflush(stdout); */
        /* MT FIXME setting fields in the request after it has been given to the
         * progress engine is racy.  The start call above is protected by
         * vc CS, but the progress engine is protected by MPIDCOMM.  So
         * we can't just extend the CS type below this point... what's the fix? */
	MPIDI_Request_set_seqnum(sreq, seqnum);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
    }

 fn_fail:    
    return mpi_errno;
}

/* This is the matching handler for the EagerShort message defined above */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_EagerShortSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EagerShortSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
					 intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_eagershort_send_t * eagershort_pkt = &pkt->eagershort_send;
    MPIR_Request * rreq;
    int found;
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

    /* printf( "Receiving short eager!\n" ); fflush(stdout); */
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
	"received eagershort send pkt, rank=%d, tag=%d, context=%d",
	eagershort_pkt->match.parts.rank, 
	eagershort_pkt->match.parts.tag, 
	eagershort_pkt->match.parts.context_id));
	    
    MPL_DBG_MSGPKT(vc,eagershort_pkt->match.parts.tag,
		    eagershort_pkt->match.parts.context_id,
		    eagershort_pkt->match.parts.rank,eagershort_pkt->data_sz,
		    "ReceivedEagerShort");
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&eagershort_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_fail;
    }

    (rreq)->status.MPI_SOURCE = (eagershort_pkt)->match.parts.rank;
    (rreq)->status.MPI_TAG    = (eagershort_pkt)->match.parts.tag;
    MPIR_STATUS_SET_COUNT((rreq)->status, (eagershort_pkt)->data_sz);
    (rreq)->dev.recv_data_sz  = (eagershort_pkt)->data_sz;
    MPIDI_Request_set_seqnum((rreq), (eagershort_pkt)->seqnum);
    /* FIXME: Why do we set the message type? */
    MPIDI_Request_set_msg_type((rreq), MPIDI_REQUEST_EAGER_MSG);

    /* This packed completes the reception of the indicated data.
       The packet handler returns null for a request that requires
       no further communication */
    *rreqp = NULL;
    *buflen = 0;

    /* Extract the data from the packet */
    /* Note that if the data size if zero, we're already done */
    if (rreq->dev.recv_data_sz > 0) {
	if (found) {
	    int            dt_contig;
	    MPI_Aint       dt_true_lb;
	    intptr_t userbuf_sz;
	    MPIR_Datatype *dt_ptr;
	    intptr_t data_sz;

	    /* Make sure that we handle the general (non-contiguous)
	       datatypes correctly while optimizing for the 
	       special case */
	    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, 
				    dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
		
	    if (rreq->dev.recv_data_sz <= userbuf_sz) {
		data_sz = rreq->dev.recv_data_sz;
	    }
	    else {
		MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
		    "receive buffer too small; message truncated, msg_sz=%"
					  PRIdPTR ", userbuf_sz=%"
				          PRIdPTR,
				 rreq->dev.recv_data_sz, userbuf_sz));
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
		     "**truncate", "**truncate %d %d %d %d", 
		     rreq->status.MPI_SOURCE, rreq->status.MPI_TAG, 
		     rreq->dev.recv_data_sz, userbuf_sz );
		MPIR_STATUS_SET_COUNT(rreq->status, userbuf_sz);
		data_sz = userbuf_sz;
	    }

	    if (dt_contig && data_sz == rreq->dev.recv_data_sz) {
		/* user buffer is contiguous and large enough to store the
		   entire message.  We can just copy the code */

		/* Copy the payload. We could optimize this 
		   if data_sz & 0x3 == 0 
		   (copy (data_sz >> 2) ints, inline that since data size is 
		   currently limited to 4 ints */
		{
		    unsigned char const * restrict p = 
			(unsigned char *)eagershort_pkt->data;
		    unsigned char * restrict bufp = 
			(unsigned char *)(char*)(rreq->dev.user_buf) + 
			dt_true_lb;
		    int i;
		    for (i=0; i<data_sz; i++) {
			*bufp++ = *p++;
		    }
		}
		/* FIXME: We want to set the OnDataAvail to the appropriate 
		   function, which depends on whether this is an RMA 
		   request or a pt-to-pt request. */
		rreq->dev.OnDataAvail = 0;
		/* The recv_pending_count must be one here (!) because of
		   the way the pending count is queried.  We may want 
		   to fix this, but it will require a sweep of the code */
	    }
	    else {
		intptr_t recv_data_sz;
		MPI_Aint last;
		/* user buffer is not contiguous.  Use the segment
		   code to unpack it, handling various errors and 
		   exceptional cases */
		/* FIXME: The MPICH tests do not exercise this branch */
		/* printf( "Surprise!\n" ); fflush(stdout);*/
		rreq->dev.segment_ptr = MPIR_Segment_alloc( );
                MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");

		MPIR_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, 
				  rreq->dev.datatype, rreq->dev.segment_ptr, 0);

		recv_data_sz = rreq->dev.recv_data_sz;
		last    = recv_data_sz;
		MPIR_Segment_unpack( rreq->dev.segment_ptr, 0, 
				     &last, eagershort_pkt->data );
		if (last != recv_data_sz) {
		    /* --BEGIN ERROR HANDLING-- */
		    /* There are two cases:  a datatype mismatch (could
		       not consume all data) or a too-short buffer. We
		       need to distinguish between these two types. */
		    MPIR_STATUS_SET_COUNT(rreq->status, last);
		    if (rreq->dev.recv_data_sz <= userbuf_sz) {
			MPIR_ERR_SETSIMPLE(rreq->status.MPI_ERROR,MPI_ERR_TYPE,
					   "**dtypemismatch");
		    }
		    /* --END ERROR HANDLING-- */
		}
		rreq->dev.OnDataAvail = 0;
	    }
	}
	else { /* (!found) */
            /* MT note: unexpected rreq is currently protected by MSGQUEUE CS */

            /* FIXME the AEU branch gives (rreq->cc==1) but the complete at the
             * bottom of this function will decr it.  Is everything going to be
             * cool in this case?  No upper layer has a pointer to rreq yet
             * (it's unexpected and freshly allocated) 
             */
	    intptr_t recv_data_sz;
	    /* This is easy; copy the data into a temporary buffer.
	       To begin with, we use the same temporary location as
	       is used in receiving eager unexpected data.
	     */
	    /* FIXME: When eagershort is enabled, provide a preallocated
               space for short messages (which is used even if eager short
	       is not used), since we don't want to have a separate check
	       to figure out which buffer we're using (or perhaps we should 
	       have a free-buffer-pointer, which can be null if it isn't
               a buffer that we've allocated). */
	    /* printf( "Allocating into tmp\n" ); fflush(stdout); */
	    recv_data_sz = rreq->dev.recv_data_sz;
        MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_buffer_size, recv_data_sz);
	    rreq->dev.tmpbuf = MPL_malloc(recv_data_sz);
	    if (!rreq->dev.tmpbuf) {
		MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	    }
	    rreq->dev.tmpbuf_sz = recv_data_sz;
 	    /* Copy the payload. We could optimize this if recv_data_sz & 0x3 == 0 
	       (copy (recv_data_sz >> 2) ints, inline that since data size is 
	       currently limited to 4 ints */
            /* We actually could optimize this a lot of ways, including just
             * putting a memcpy here.  Modern compilers will inline fast
             * versions of the memcpy here (__builtin_memcpy, etc).  Another
             * option is a classic word-copy loop with a switch block at the end
             * for a remainder.  Alternatively a Duff's device loop could work.
             * Any replacement should be profile driven, and no matter what
             * we're likely to pick something suboptimal for at least one
             * compiler out there. [goodell@ 2012-02-10] */
	    {
		unsigned char const * restrict p = 
		    (unsigned char *)eagershort_pkt->data;
		unsigned char * restrict bufp = 
		    (unsigned char *)rreq->dev.tmpbuf;
		int i;
		for (i=0; i<recv_data_sz; i++) {
		    *bufp++ = *p++;
		}
	    }
	    /* printf( "Unexpected eager short\n" ); fflush(stdout); */
	    /* These next two indicate that once matched, there is
	       one more step (the unpack into the user buffer) to perform. */
	    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackUEBufComplete;

            /* normally starts at 2, but we are implicitly decrementing it
             * because all of the data arrived in the pkt (see mpidpre.h) */
	    rreq->dev.recv_pending_count = 1;
	}

	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
		     "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGERSHORT_SEND");
	}
    }

    /* The request is still complete (in the sense of having all data), decr the
     * cc and kick the progress engine. */
    /* MT note: when multithreaded, completing a request (cc==0) also signifies
     * that an upper layer may acquire exclusive ownership of the request, so
     * all rreq field modifications must be complete at this point.  This macro
     * also kicks the progress engine, which was previously done here via
     * MPIDI_CH3_Progress_signal_completion(). */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

 fn_fail:
    /* MT note: it may be possible to narrow this CS after careful
     * consideration.  Note though that the (!found) case must be wholly
     * protected by this CS. */
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    return mpi_errno;
}

#endif

/* Send a contiguous eager message that can be cancelled (e.g., 
   a nonblocking eager send).  We'll want to optimize (and possibly
   inline) this 

   Make sure that buf is at the beginning of the data to send; 
   adjust by adding dt_true_lb if necessary 
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerContigIsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_EagerContigIsend( MPIR_Request **sreq_p,
				MPIDI_CH3_Pkt_type_t reqtype, 
				const void * buf, intptr_t data_sz, int rank,
				int tag, MPIR_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    MPIR_Request *sreq = *sreq_p;
    MPL_IOV iov[MPL_IOV_LIMIT];

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
	       "sending contiguous eager message, data_sz=%" PRIdPTR,
					data_sz));
	    
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.parts.rank	= comm->rank;
    eager_pkt->match.parts.tag	= tag;
    eager_pkt->match.parts.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= sreq->handle;
    eager_pkt->data_sz		= data_sz;
    
    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)eager_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*eager_pkt);
    
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) buf;
    iov[1].MPL_IOV_LEN = data_sz;
    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    MPL_DBG_MSGPKT(vc,tag,eager_pkt->match.parts.context_id,rank,data_sz,"EagerIsend");
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, 2);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        MPIR_Request_free(sreq);
	*sreq_p = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
    }
    /* --END ERROR HANDLING-- */
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* 
 * Here are the routines that are called by the progress engine to handle
 * the various rendezvous message requests (cancel of sends is in 
 * mpid_cancel_send.c).
 */    

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.parts.rank;	\
    (rreq_)->status.MPI_TAG = (pkt_)->match.parts.tag;		\
    MPIR_STATUS_SET_COUNT((rreq_)->status, (pkt_)->data_sz);		\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

/* FIXME: This is not optimized for short messages, which 
   should have the data in the same packet when the data is
   particularly short (e.g., one 8 byte long word) */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_EagerSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EagerSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data,
				    intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_eager_send_t * eager_pkt = &pkt->eager_send;
    MPIR_Request * rreq;
    int found;
    int complete;
    intptr_t data_len;
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
	"received eager send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
	eager_pkt->sender_req_id, eager_pkt->match.parts.rank, 
	eager_pkt->match.parts.tag, eager_pkt->match.parts.context_id));
    MPL_DBG_MSGPKT(vc,eager_pkt->match.parts.tag,
		    eager_pkt->match.parts.context_id,
		    eager_pkt->match.parts.rank,eager_pkt->data_sz,
		    "ReceivedEager");
	    
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&eager_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_fail;
    }
    
    set_request_info(rreq, eager_pkt, MPIDI_REQUEST_EAGER_MSG);
    
    data_len = ((*buflen >= rreq->dev.recv_data_sz)
                ? rreq->dev.recv_data_sz : *buflen);
    
    if (rreq->dev.recv_data_sz == 0) {
        /* return the number of bytes processed in this function */
        *buflen = 0;
        mpi_errno = MPID_Request_complete(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
	*rreqp = NULL;
    }
    else {
	if (found) {
	    mpi_errno = MPIDI_CH3U_Receive_data_found( rreq, data,
                                                       &data_len, &complete );
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_unexpected( rreq, data,
                                                            &data_len, &complete );
	}

	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
			     "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SEND");
	}

        /* return the number of bytes processed in this function */
        *buflen = data_len;

        if (complete) 
        {
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
            *rreqp = NULL;
        }
        else
        {
            *rreqp = rreq;
        }
    }

 fn_fail:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_ReadySend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_ReadySend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data,
				    intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_ready_send_t * ready_pkt = &pkt->ready_send;
    MPIR_Request * rreq;
    int found;
    int complete;
    intptr_t data_len;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
	"received ready send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			ready_pkt->sender_req_id, 
			ready_pkt->match.parts.rank, 
                        ready_pkt->match.parts.tag, 
			ready_pkt->match.parts.context_id));
    MPL_DBG_MSGPKT(vc,ready_pkt->match.parts.tag,
		    ready_pkt->match.parts.context_id,
		    ready_pkt->match.parts.rank,ready_pkt->data_sz,
		    "ReceivedReady");
	    
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&ready_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_fail;
    }
    
    set_request_info(rreq, ready_pkt, MPIDI_REQUEST_EAGER_MSG);
    
    data_len = ((*buflen >= rreq->dev.recv_data_sz)
                ? rreq->dev.recv_data_sz : *buflen);
    
    if (found) {
	if (rreq->dev.recv_data_sz == 0) {
            /* return the number of bytes processed in this function */
            *buflen = data_len;;
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
	    *rreqp = NULL;
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_found(rreq, data, &data_len,
                                                      &complete);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
				     "**ch3|postrecv",
				     "**ch3|postrecv %s", 
				     "MPIDI_CH3_PKT_READY_SEND");
	    }

            /* return the number of bytes processed in this function */
            *buflen = data_len;

            if (complete) 
            {
                mpi_errno = MPID_Request_complete(rreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
                *rreqp = NULL;
            }
            else
            {
                *rreqp = rreq;
            }
	}
    }
    else
    {
	/* FIXME: an error packet should be sent back to the sender 
	   indicating that the ready-send failed.  On the send
	   side, the error handler for the communicator can be invoked
	   even if the ready-send request has already
	   completed. */
	
	/* We need to consume any outstanding associated data and 
	   mark the request with an error. */
	
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
				      MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
				      MPI_ERR_OTHER, "**rsendnomatch", 
				      "**rsendnomatch %d %d", 
				      ready_pkt->match.parts.rank,
				      ready_pkt->match.parts.tag);
	MPIR_STATUS_SET_COUNT(rreq->status, 0);
	if (rreq->dev.recv_data_sz > 0)
	{
	    /* force read of extra data */
	    *rreqp = rreq;
	    rreq->dev.segment_first = 0;
	    rreq->dev.segment_size = 0;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				    "**ch3|loadrecviov");
	    }
	}
	else
	{
	    /* mark data transfer as complete and decrement CC */
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
	    *rreqp = NULL;
	}
        /* we didn't process anything but the header in this case */
        *buflen = 0;
    }
 fn_fail:
    return mpi_errno;
}


/*
 * Define the routines that can print out the cancel packets if 
 * debugging is enabled.
 */
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_EagerSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... EAGER_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->eager_send.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->eager_send.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->eager_send.match.parts.rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," data_sz ...... %d\n", pkt->eager_send.data_sz);
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," seqnum ....... %d\n", pkt->eager_send.seqnum);
#endif
}

#if defined(USE_EAGER_SHORT)
int MPIDI_CH3_PktPrint_EagerShortSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    int datalen;
    unsigned char *p = (unsigned char *)pkt->eagershort_send.data;
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... EAGERSHORT_SEND\n");
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->eagershort_send.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->eagershort_send.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->eagershort_send.match.parts.rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," data_sz ...... %d\n", pkt->eagershort_send.data_sz);
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," seqnum ....... %d\n", pkt->eagershort_send.seqnum);
#endif
    datalen = pkt->eagershort_send.data_sz;
    if (datalen > 0) {
	char databytes[64+1];
	int i;
	if (datalen > 32) datalen = 32;
	for (i=0; i<datalen; i++) {
	    MPL_snprintf( &databytes[2*i], 64 - 2*i, "%2x", p[i] );
	}
	MPL_DBG_MSG_S(MPIDI_CH3_DBG_OTHER,TERSE," data ......... %s\n", databytes);
    }
}
#endif /* defined(USE_EAGER_SHORT) */

int MPIDI_CH3_PktPrint_ReadySend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... READY_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->ready_send.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->ready_send.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->ready_send.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->ready_send.match.parts.rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," data_sz ...... %d\n", pkt->ready_send.data_sz);
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," seqnum ....... %d\n", pkt->ready_send.seqnum);
#endif
}

#endif /* MPICH_DBG_OUTPUT */
