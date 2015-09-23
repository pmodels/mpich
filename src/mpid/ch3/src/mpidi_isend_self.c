/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Isend_self(const void * buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
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

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&match, &found);
    /* --BEGIN ERROR HANDLING-- */
    if (rreq == NULL)
    {
        /* We release the send request twice, once to release the
         * progress engine reference and the second to release the
         * user reference since the user will never have a chance to
         * release their reference. */
        MPID_Request_release(sreq);
        MPID_Request_release(sreq);
	sreq = NULL;
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", 
		      "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPID_cc_get(rreq->cc) == 0) {
        /* We release the send request twice, once to release the
         * progress engine reference and the second to release the
         * user reference since the user will never have a chance to
         * release their reference. */
        MPID_Request_release(sreq);
        MPID_Request_release(sreq);
        sreq = NULL;
        goto fn_exit;
    }

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
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,
		     "found posted receive request; copying data");
	    
	MPIDI_CH3U_Buffer_copy(buf, count, datatype, &sreq->status.MPI_ERROR,
			       rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &data_sz, &rreq->status.MPI_ERROR);
	MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        mpi_errno = MPID_Request_complete(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_Request_complete(sreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else
    {
	if (type != MPIDI_REQUEST_TYPE_RSEND)
	{
	    MPI_Aint dt_sz;
	
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
	    MPIR_STATUS_SET_COUNT(rreq->status, count * dt_sz);
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
	    MPIR_STATUS_SET_COUNT(rreq->status, 0);
	    
	    /* sreq has never been seen by the user or outside this thread, so it is safe to reset ref_count and cc */
            mpi_errno = MPID_Request_complete(sreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
	    /* --END ERROR HANDLING-- */
	}
	    
	MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_SELF_MSG);

        /* can release now that we've set fields in the unexpected request */
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

        /* kick the progress engine in case another thread that is performing a
           blocking recv or probe is waiting in the progress engine */
	MPIDI_CH3_Progress_signal_completion();
    }

  fn_exit:
    *request = sreq;

    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
