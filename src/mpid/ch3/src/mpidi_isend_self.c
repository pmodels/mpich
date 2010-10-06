/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: Explain this function */

/* FIXME: should there be a simpler version of this for short messages, 
   particularly contiguous ones?  See also the FIXME about buffering
   short messages */

#undef FUNCNAME
#define FUNCNAME MPIDI_Isend_self
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Isend_self(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		     int type, MPID_Request ** request)
{
    MPIDI_Message_match match;
    MPID_Request * sreq;
    MPID_Request * rreq;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int found;
    int mpi_errno = MPI_SUCCESS;
	
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending message to self");
	
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, type);
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_SELF_MSG);
    
    match.parts.rank = rank;
    match.parts.tag = tag;
    match.parts.context_id = comm->context_id + context_offset;

    MPIU_THREAD_CS_ENTER(MSGQUEUE,);

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&match, &found);
    /* --BEGIN ERROR HANDLING-- */
    if (rreq == NULL)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", 
		      "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    MPIDI_Request_set_seqnum(rreq, seqnum);
    
    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    
    if (found)
    {
	MPIDI_msg_sz_t data_sz;
	
        /* we found a posted req, which we now own, so we can release the CS */
        MPIU_THREAD_CS_EXIT(MSGQUEUE,);

	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,
		     "found posted receive request; copying data");
	    
	MPIDI_CH3U_Buffer_copy(buf, count, datatype, &sreq->status.MPI_ERROR,
			       rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &data_sz, &rreq->status.MPI_ERROR);
	rreq->status.count = (int)data_sz;
	MPID_REQUEST_SET_COMPLETED(rreq);
	MPID_Request_release(rreq);
	/* sreq has never been seen by the user or outside this thread, so it is safe to reset ref_count and cc */
	MPIU_Object_set_ref(sreq, 1);
        MPID_cc_set(&sreq->cc, 0);
    }
    else
    {
	if (type != MPIDI_REQUEST_TYPE_RSEND)
	{
	    int dt_sz;
	
	    /* FIXME: Insert code here to buffer small sends in a temporary buffer? */

	    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,
          "added receive request to unexpected queue; attaching send request");
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
		MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
	    }
	    rreq->partner_request = sreq;
	    rreq->dev.sender_req_id = sreq->handle;
	    MPID_Datatype_get_size_macro(datatype, dt_sz);
	    rreq->status.count = count * dt_sz;
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIU_DBG_MSG(CH3_OTHER,TYPICAL,
			 "ready send unable to find matching recv req");
	    sreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
							  "**rsendnomatch", "**rsendnomatch %d %d", rank, tag);
	    rreq->status.MPI_ERROR = sreq->status.MPI_ERROR;
	    
	    rreq->partner_request = NULL;
	    rreq->dev.sender_req_id = MPI_REQUEST_NULL;
	    rreq->status.count = 0;
	    
	    /* sreq has never been seen by the user or outside this thread, so it is safe to reset ref_count and cc */
	    MPIU_Object_set_ref(sreq, 1);
            MPID_cc_set(&sreq->cc, 0);
	    /* --END ERROR HANDLING-- */
	}
	    
	MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_SELF_MSG);

        /* can release now that we've set fields in the unexpected request */
        MPIU_THREAD_CS_EXIT(MSGQUEUE,);

        /* kick the progress engine in case another thread that is performing a
           blocking recv or probe is waiting in the progress engine */
	MPIDI_CH3_Progress_signal_completion();
    }

  fn_exit:
    *request = sreq;

    return mpi_errno;
}
