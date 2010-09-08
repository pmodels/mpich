/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Irsend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_Request * sreq;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_IRSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IRSEND);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                "rank=%d, tag=%d, context=%d", 
                rank, tag, comm->context_id + context_offset));
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_RSEND, &sreq);
	goto fn_exit;
    }
    
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
    
    if (rank == MPI_PROC_NULL)
    {
	MPIU_Object_set_ref(sreq, 1);
        MPID_cc_set(&sreq->cc, 0);
	goto fn_exit;
    }
    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);

#ifdef ENABLE_COMM_OVERRIDES
    if (vc->comm_ops && vc->comm_ops->irsend)
    {
	mpi_errno = vc->comm_ops->irsend( vc, buf, count, datatype, rank, tag, comm, context_offset, &sreq);
	goto fn_exit;
    }
#endif
    
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_Pkt_init(ready_pkt, MPIDI_CH3_PKT_READY_SEND);
    ready_pkt->match.parts.rank = comm->rank;
    ready_pkt->match.parts.tag = tag;
    ready_pkt->match.parts.context_id = comm->context_id + context_offset;
    ready_pkt->sender_req_id = MPI_REQUEST_NULL;
    ready_pkt->data_sz = data_sz;

    if (data_sz == 0)
    {
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending zero length message");

	sreq->dev.OnDataAvail = 0;
	
	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(ready_pkt, seqnum);
	MPIDI_Request_set_seqnum(sreq, seqnum);
	
	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
	mpi_errno = MPIU_CALL(MPIDI_CH3,iSend(vc, sreq, ready_pkt, sizeof(*ready_pkt)));
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	goto fn_exit;
    }
    
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
	if (sreq && sreq->dev.OnDataAvail) {
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPID_Datatype_add_ref(dt_ptr);
	}
    }
 
  fn_exit:
    *request = sreq;

    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,{
	if (sreq != NULL)
	{
	    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,"request allocated, handle=0x%08x", sreq->handle);
	}
    }
		  );
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
}
