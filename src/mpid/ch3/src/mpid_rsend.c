/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/* FIXME: How does this differ from eager send?  It should differ in 
   only a few bits (e.g., indicate that the send is ready and should
   fail if there is no matching receive) */

/*
 * MPID_Rsend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Rsend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
	       MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_Request * sreq = NULL;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_RSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RSEND);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
					"rank=%d, tag=%d, context=%d", 
                              rank, tag, comm->context_id + context_offset));
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_RSEND, &sreq);
	goto fn_exit;
    }

    if (rank == MPI_PROC_NULL)
    {
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

	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending zero length message");
    
	MPIDI_Pkt_init(ready_pkt, MPIDI_CH3_PKT_READY_SEND);
	ready_pkt->match.parts.rank = comm->rank;
	ready_pkt->match.parts.tag = tag;
	ready_pkt->match.parts.context_id = comm->context_id + context_offset;
	ready_pkt->sender_req_id = MPI_REQUEST_NULL;
	ready_pkt->data_sz = data_sz;

	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(ready_pkt, seqnum);
	
	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
	mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, ready_pkt, 
						  sizeof(*ready_pkt), &sreq));
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
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
    
    if (dt_contig)
    {
	mpi_errno = MPIDI_CH3_EagerContigSend( &sreq, 
					       MPIDI_CH3_PKT_READY_SEND,
					       (char *)buf + dt_true_lb,
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
                                                  data_sz, rank, tag, 
                                                  comm, context_offset );
    }

  fn_exit:
    *request = sreq;

    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,
    {
	if (mpi_errno == MPI_SUCCESS) {
	    if (sreq) {
		MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,"request allocated, handle=0x%08x", sreq->handle);
	    }
	    else
	    {
		MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"operation complete, no requests allocated");
	    }
	}
    }
		  );
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RSEND);
    return mpi_errno;
}
