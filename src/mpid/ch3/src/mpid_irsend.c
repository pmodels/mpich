/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Irsend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Irsend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int context_offset,
		MPIR_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;
    intptr_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype* dt_ptr;
    MPIR_Request * sreq;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;    
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IRSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IRSEND);

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
                "rank=%d, tag=%d, context=%d", 
                rank, tag, comm->context_id + context_offset));

    /* Check to make sure the communicator hasn't already been revoked */
    if (comm->revoked &&
            MPIR_AGREE_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_Process.tagged_coll_mask) &&
            MPIR_SHRINK_TAG != MPIR_TAG_MASK_ERROR_BITS(tag & ~MPIR_Process.tagged_coll_mask)) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }
    
    if (rank == comm->rank && comm->comm_kind != MPIR_COMM_KIND__INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_RSEND, &sreq);
	goto fn_exit;
    }

    if (rank != MPI_PROC_NULL) {
        MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
#ifdef ENABLE_COMM_OVERRIDES
        /* this needs to come before the sreq is created, since the override
         * function is responsible for creating its own request */
        if (vc->comm_ops && vc->comm_ops->irsend)
        {
            mpi_errno = vc->comm_ops->irsend( vc, buf, count, datatype, rank, tag, comm, context_offset, &sreq);
            goto fn_exit;
        }
#endif
    }
    
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
    
    if (rank == MPI_PROC_NULL)
    {
	MPIR_Object_set_ref(sreq, 1);
        MPIR_cc_set(&sreq->cc, 0);
	goto fn_exit;
    }
    
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Pkt_init(ready_pkt, MPIDI_CH3_PKT_READY_SEND);
    ready_pkt->match.parts.rank = comm->rank;
    ready_pkt->match.parts.tag = tag;
    ready_pkt->match.parts.context_id = comm->context_id + context_offset;
    ready_pkt->sender_req_id = MPI_REQUEST_NULL;
    ready_pkt->data_sz = data_sz;

    if (data_sz == 0)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending zero length message");

	sreq->dev.OnDataAvail = 0;
	
	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(ready_pkt, seqnum);
	MPIDI_Request_set_seqnum(sreq, seqnum);
	
	MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = MPIDI_CH3_iSend(vc, sreq, ready_pkt, sizeof(*ready_pkt));
	MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
            MPIR_Request_free(sreq);
	    sreq = NULL;
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	goto fn_exit;
    }
    
    if (vc->ready_eager_max_msg_sz < 0 || data_sz + sizeof(MPIDI_CH3_Pkt_ready_send_t) <= vc->ready_eager_max_msg_sz) {
        if (dt_contig) {
            mpi_errno = MPIDI_CH3_EagerContigIsend( &sreq,
                                                    MPIDI_CH3_PKT_READY_SEND,
                                                    (char*)buf + dt_true_lb,
                                                    data_sz, rank, tag,
                                                    comm, context_offset );
            
        }
        else {
            mpi_errno = MPIDI_CH3_EagerNoncontigSend( &sreq,
                                                      MPIDI_CH3_PKT_READY_SEND,
                                                      buf, count, datatype,
                                                      data_sz, rank, tag,
                                                      comm, context_offset );
            /* If we're not complete, then add a reference to the datatype */
            if (sreq) {
                sreq->dev.datatype_ptr = dt_ptr;
                MPIR_Datatype_add_ref(dt_ptr);
            }
        }
    } else {
 	/* Do rendezvous.  This will be sent as a regular send not as
           a ready send, so the receiver won't know to send an error
           if the receive has not been posted */
	MPIDI_Request_set_msg_type( sreq, MPIDI_REQUEST_RNDV_MSG );
	mpi_errno = vc->rndvSend_fn( &sreq, buf, count, datatype, dt_contig,
                                     data_sz, dt_true_lb, rank, tag, comm,
                                     context_offset );
	if (sreq && dt_ptr != NULL) {
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPIR_Datatype_add_ref(dt_ptr);
	}
    }

  fn_exit:
    *request = sreq;

    MPL_DBG_STMT(MPIDI_CH3_DBG_OTHER,VERBOSE,{
	if (sreq != NULL)
	{
	    MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,"request allocated, handle=0x%08x", sreq->handle);
	}
    }
		  );
    
  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
}
