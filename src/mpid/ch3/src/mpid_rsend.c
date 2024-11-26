/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* FIXME: How does this differ from eager send?  It should differ in 
   only a few bits (e.g., indicate that the send is ready and should
   fail if there is no matching receive) */

/*
 * MPID_Rsend()
 */
int MPID_Rsend(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int attr,
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
    int mpi_errno = MPI_SUCCESS;    

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
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
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_RSEND, &sreq);
	goto fn_exit;
    }

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);

#ifdef ENABLE_COMM_OVERRIDES
    if (vc->comm_ops && vc->comm_ops->rsend)
    {
	mpi_errno = vc->comm_ops->rsend( vc, buf, count, datatype, rank, tag, comm, context_offset, &sreq);
	goto fn_exit;
    }
#endif

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (data_sz == 0)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;

	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending zero length message");
    
	MPIDI_Pkt_init(ready_pkt, MPIDI_CH3_PKT_READY_SEND);
	ready_pkt->match.parts.rank = (MPIDI_Rank_t) comm->rank;
	ready_pkt->match.parts.tag = tag;
	ready_pkt->match.parts.context_id = (MPIR_Context_id_t) (comm->context_id + context_offset);
	ready_pkt->sender_req_id = MPI_REQUEST_NULL;
	ready_pkt->data_sz = data_sz;

	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(ready_pkt, seqnum);
	
	mpi_errno = MPIDI_CH3_iStartMsg(vc, ready_pkt, sizeof(*ready_pkt), &sreq);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	if (sreq != NULL)
	{
	    MPIDI_Request_set_seqnum(sreq, seqnum);
	    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
	    /* sreq->comm = comm;
	       MPIR_Comm_add_ref(comm); -- not needed for blocking operations */
	}

	goto fn_exit;
    }
    
    if (vc->ready_eager_max_msg_sz < 0 || data_sz + sizeof(MPIDI_CH3_Pkt_ready_send_t) <= vc->ready_eager_max_msg_sz) {
        if (dt_contig)
        {
            mpi_errno = MPIDI_CH3_EagerContigSend( &sreq,
                                                   MPIDI_CH3_PKT_READY_SEND,
                                                   MPIR_get_contig_ptr(buf, dt_true_lb),
                                                   data_sz, rank, tag, comm,
                                                   context_offset );
        }
        else
        {
            MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
            MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
            mpi_errno = MPIDI_CH3_EagerNoncontigSend( &sreq,
                                                      MPIDI_CH3_PKT_READY_SEND,
                                                      buf, count, datatype,
                                                      rank, tag,
                                                      comm, context_offset );
        }
    } else {
 	/* Do rendezvous.  This will be sent as a regular send not as
           a ready send, so the receiver won't know to send an error
           if the receive has not been posted */
 	MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	mpi_errno = vc->rndvSend_fn( &sreq, buf, count, datatype, dt_contig,
                                     data_sz, dt_true_lb, rank, tag, comm,
                                     context_offset );
	/* Note that we don't increase the ref count on the datatype
	   because this is a blocking call, and the calling routine
	   must wait until sreq completes */
   }

  fn_exit:
    *request = sreq;
    if (sreq) {
        MPII_SENDQ_REMEMBER(sreq, rank, tag, comm->recvcontext_id, buf, count);
    }

    MPL_DBG_STMT(MPIDI_CH3_DBG_OTHER,VERBOSE,
    {
	if (mpi_errno == MPI_SUCCESS) {
	    if (sreq) {
		MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,"request allocated, handle=0x%08x", sreq->handle);
	    }
	    else
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"operation complete, no requests allocated");
	    }
	}
    }
		  );
    
  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
