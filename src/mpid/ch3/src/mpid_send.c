/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/*
 * MPID_Send()
 */
int MPID_Send(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank,
	      int tag, MPIR_Comm * comm, int attr,
	      MPIR_Request ** request)
{
    intptr_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype* dt_ptr;
    MPIR_Request * sreq = NULL;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int eager_threshold = -1;
    int mpi_errno = MPI_SUCCESS;    

    if (MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr)) {
        return MPID_Ssend(buf, count, datatype, rank, tag, comm, attr, request);
    }

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);

    switch (errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
                "rank=%d, tag=%d, context=%d", 
		rank, tag, comm->context_id + context_offset));

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm->revoked &&
            MPIR_AGREE_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_TAG_COLL_BIT) &&
            MPIR_SHRINK_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_TAG_COLL_BIT)) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }

    if (rank == comm->rank && comm->comm_kind != MPIR_COMM_KIND__INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, 
				     context_offset, MPIDI_REQUEST_TYPE_SEND, 
				     &sreq);

	/* In the single threaded case, sending to yourself will cause 
	   deadlock.  Note that in the runtime-thread case, this check
	   will not be made (long-term FIXME) */
#       ifndef MPICH_IS_THREADED
	{
	    if (sreq != NULL && MPIR_cc_get(sreq->cc) != 0) {
		MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				    "**dev|selfsenddeadlock");
	    }
	}
#	endif
	if (mpi_errno != MPI_SUCCESS) { MPIR_ERR_POP(mpi_errno); }
	goto fn_exit;
    }

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIR_ERR_CHKANDJUMP1(vc->state == MPIDI_VC_STATE_MORIBUND, mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", rank);

#ifdef ENABLE_COMM_OVERRIDES
    if (vc->comm_ops && vc->comm_ops->send)
    {
	mpi_errno = vc->comm_ops->send( vc, buf, count, datatype, rank, tag, comm, context_offset, &sreq);
	goto fn_exit;
    }
#endif

    if (count == 0)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;

	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending zero length message");
	MPIDI_Pkt_init(eager_pkt, MPIDI_CH3_PKT_EAGER_SEND);
	eager_pkt->match.parts.rank = (MPIDI_Rank_t) comm->rank;
	eager_pkt->match.parts.tag = tag;
	eager_pkt->match.parts.context_id = (MPIR_Context_id_t) (comm->context_id + context_offset);
	eager_pkt->sender_req_id = MPI_REQUEST_NULL;
	eager_pkt->data_sz = 0;
	
	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
	
	mpi_errno = MPIDI_CH3_iStartMsg(vc, eager_pkt, sizeof(*eager_pkt), &sreq);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|eagermsg");
	}
	/* --END ERROR HANDLING-- */
	if (sreq != NULL)
	{
	    MPIDI_Request_set_seqnum(sreq, seqnum);
	    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	    /* sreq->comm = comm;
	      MPIR_Comm_add_ref(comm); -- not necessary for blocking functions */
	}
	
	goto fn_exit;
    }

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_CH3_GET_EAGER_THRESHOLD(&eager_threshold, comm, vc);

    /* FIXME: flow control: limit number of outstanding eager messages
       containing data and need to be buffered by the receiver */
#ifdef USE_EAGER_SHORT
    if (dt_contig && data_sz <= MPIDI_EAGER_SHORT_SIZE) {
	mpi_errno = MPIDI_CH3_EagerContigShortSend( &sreq, 
					       MPIDI_CH3_PKT_EAGERSHORT_SEND,
                                               MPIR_get_contig_ptr(buf, dt_true_lb),
					       data_sz, rank, tag, comm, 
					       context_offset );
    }
    else
#endif

    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <= eager_threshold)
    {
	if (dt_contig)
        {
 	    mpi_errno = MPIDI_CH3_EagerContigSend( &sreq, 
						   MPIDI_CH3_PKT_EAGER_SEND,
                                                   MPIR_get_contig_ptr(buf, dt_true_lb),
						   data_sz, rank, tag, comm, 
						   context_offset );
	}
	else
        {
	    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
	    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	    mpi_errno = MPIDI_CH3_EagerNoncontigSend( &sreq, 
                                                      MPIDI_CH3_PKT_EAGER_SEND,
                                                      buf, count, datatype,
                                                      rank, tag,
                                                      comm, context_offset );
	}
    }
    else
    {
	MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	mpi_errno = vc->rndvSend_fn( &sreq, buf, count, datatype, dt_contig,
                                     data_sz, dt_true_lb, rank, tag, comm, 
                                     context_offset );
	/* Note that we don't increase the ref count on the datatype
	   because this is a blocking call, and the calling routine 
	   must wait until sreq completes */
    }

 fn_fail:
 fn_exit:
    *request = sreq;
    if (sreq) {
        MPII_SENDQ_REMEMBER(sreq, rank, tag, comm->recvcontext_id, buf, count);
    }

    MPL_DBG_STMT(MPIDI_CH3_DBG_OTHER,VERBOSE,
    {
	if (mpi_errno == MPI_SUCCESS) {
	    if (sreq) {
		MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,
			 "request allocated, handle=0x%08x", sreq->handle);
	    }
	    else
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,
			     "operation complete, no requests allocated");
	    }
	}
    }
		  );
    
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
