/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Issend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Issend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPIR_Comm * comm, int context_offset,
		MPIR_Request ** request)
{
    intptr_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype* dt_ptr;
    MPIR_Request * sreq;
    MPIDI_VC_t * vc=0;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int eager_threshold = -1;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISSEND);

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
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_SSEND, &sreq);
	goto fn_exit;
    }

    if (rank != MPI_PROC_NULL)
    {
       MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
        /* this needs to come before the sreq is created, since the override */
        /* function is responsible for creating its own request */       
#ifdef ENABLE_COMM_OVERRIDES
       if (vc->comm_ops && vc->comm_ops->issend)
       {
	  mpi_errno = vc->comm_ops->issend( vc, buf, count, datatype, rank, tag, comm, context_offset, &sreq);
	  goto fn_exit;
       }
#endif
    }   
   
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    
    if (rank == MPI_PROC_NULL)
    {
	MPIR_Object_set_ref(sreq, 1);
        MPIR_cc_set(&sreq->cc, 0);
	goto fn_exit;
    }
    
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    
    if (data_sz == 0)
    {
	mpi_errno = MPIDI_CH3_EagerSyncZero( &sreq, rank, tag, comm, 
					     context_offset );
	goto fn_exit;
    }

    MPIDI_CH3_GET_EAGER_THRESHOLD(&eager_threshold, comm, vc);

    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_sync_send_t) <= eager_threshold)
    {
	mpi_errno = MPIDI_CH3_EagerSyncNoncontigSend( &sreq, buf, count,
                                                      datatype, data_sz, 
                                                      dt_contig, dt_true_lb,
                                                      rank, tag, comm, 
                                                      context_offset );
	/* If we're not complete and this is a derived datatype
         * communication, then add a reference to the datatype */
	if (sreq && (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)) {
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPIR_Datatype_add_ref(dt_ptr);
	}
    }
    else
    {
	/* Note that the sreq was created above */
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_RNDV_MSG);
	mpi_errno = vc->rndvSend_fn( &sreq, buf, count, datatype, dt_contig,
                                     data_sz, dt_true_lb, rank, tag, comm, 
                                     context_offset );
	
	/* FIXME: fill temporary IOV or pack temporary buffer after send to 
	   hide some latency.  This requires synchronization
           because the CTS packet could arrive and be processed before the 
	   above iStartmsg completes (depending on the progress
           engine, threads, etc.). */
	
	if (sreq && dt_ptr != NULL)
	{
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPIR_Datatype_add_ref(dt_ptr);
	}
    }

  fn_exit:
    *request = sreq;
    
    MPL_DBG_STMT(MPIDI_CH3_DBG_OTHER,VERBOSE,
    {
	if (sreq != NULL) {
	    MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,
			   "request allocated, handle=0x%08x", sreq->handle);
	}
    }
		  )
    
  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
}
