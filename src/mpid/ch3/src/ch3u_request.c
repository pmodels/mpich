/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* This file contains two types of routines associated with requests: 
 * Routines to allocate and free requests
 * Routines to manage iovs on requests 
 *
 * Note that there are a number of macros that also manage requests defined
 * in mpidimpl.h ; these are intended to optimize request creation for
 * specific types of requests.  See the comments in mpidimpl.h for more
 * details.
 */

/* Routines and data structures for request allocation and deallocation */
#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC] = {{0}};
MPIU_Object_alloc_t MPID_Request_mem = {
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request), MPID_Request_direct,
    MPID_REQUEST_PREALLOC };

/* See the comments above about request creation.  Some routines will
   use macros in mpidimpl.h *instead* of this routine */
#undef FUNCNAME
#define FUNCNAME MPID_Request_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPID_Request_create(void)
{
    MPID_Request * req;
    MPIDI_STATE_DECL(MPID_STATE_MPID_REQUEST_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_REQUEST_CREATE);
    
    req = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (req != NULL)
    {
	MPIU_DBG_MSG_P(CH3_CHANNEL,VERBOSE,
		       "allocated request, handle=0x%08x", req->handle);
#ifdef MPICH_DBG_OUTPUT
	/*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
	if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
	{
	    int mpi_errno;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
		       FCNAME, __LINE__, MPI_ERR_OTHER, 
		       "**invalid_handle", "**invalid_handle %d", req->handle);
	    MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
	}
#endif
	/* FIXME: This makes request creation expensive.  We need to trim
	   this to the basics, with additional setup for special-purpose 
	   requests (think base class and inheritance).  For example, do we 
	   *really* want to set the kind to UNDEFINED? And should the RMA 
	   values be set only for RMA requests? */
	MPIU_Object_set_ref(req, 1);
	req->kind		   = MPID_REQUEST_UNDEFINED;
        MPID_cc_set(&req->cc, 1);
	req->cc_ptr		   = &req->cc;
	/* FIXME: status fields meaningful only for receive, and even then
	   should not need to be set. */
	req->status.MPI_SOURCE	   = MPI_UNDEFINED;
	req->status.MPI_TAG	   = MPI_UNDEFINED;
	req->status.MPI_ERROR	   = MPI_SUCCESS;
	req->status.count	   = 0;
	req->status.cancelled	   = FALSE;
	req->comm		   = NULL;
	req->dev.datatype_ptr	   = NULL;
	req->dev.segment_ptr	   = NULL;
	/* Masks and flags for channel device state in an MPID_Request */
	req->dev.state		   = 0;
	req->dev.cancel_pending	   = FALSE;
	/* FIXME: RMA ops shouldn't need to be set except when creating a
	   request for RMA operations */
	req->dev.target_win_handle = MPI_WIN_NULL;
	req->dev.source_win_handle = MPI_WIN_NULL;
	req->dev.single_op_opt	   = 0;
	req->dev.lock_queue_entry  = NULL;
	req->dev.dtype_info	   = NULL;
	req->dev.dataloop	   = NULL;
	req->dev.iov_offset        = 0;
#ifdef MPIDI_CH3_REQUEST_INIT
	MPIDI_CH3_REQUEST_INIT(req);
#endif
    }
    else
    {
	/* FIXME: This fails to fail if debugging is turned off */
	MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"unable to allocate a request");
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_REQUEST_CREATE);
    return req;
}

/* FIXME: We need a lighter-weight version of this to avoid all of the
   extra checks.  One posibility would be a single, no special case (no 
   comm, datatype, or srbuf to check) and a more general (check everything)
   version.  */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Request_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Request_destroy(MPID_Request * req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
    MPIU_DBG_MSG_P(CH3_CHANNEL,VERBOSE,
		   "freeing request, handle=0x%08x", req->handle);
    
#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);*/
    if (HANDLE_GET_MPI_KIND(req->handle) != MPID_REQUEST)
    {
	int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
                      FCNAME, __LINE__, MPI_ERR_OTHER, 
                      "**invalid_handle", "**invalid_handle %d", req->handle);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
    /* XXX DJG FIXME should we be checking this? */
    /*MPIU_Assert(req->ref_count == 0);*/
    if (req->ref_count != 0)
    {
	int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                       FCNAME, __LINE__, MPI_ERR_OTHER, 
              "**invalid_refcount", "**invalid_refcount %d", req->ref_count);
	MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif

    /* FIXME: We need a better way to handle these so that we
       do not always need to initialize these fields and check them
       when we destroy a request */
    /* FIXME: We need a way to call these routines ONLY when the 
       related ref count has become zero. */
    if (req->comm != NULL) {
	MPIR_Comm_release(req->comm, 0);
    }

    if (req->dev.datatype_ptr != NULL) {
	MPID_Datatype_release(req->dev.datatype_ptr);
    }

    if (req->dev.segment_ptr != NULL) {
	MPID_Segment_free(req->dev.segment_ptr);
    }

    if (MPIDI_Request_get_srbuf_flag(req)) {
	MPIDI_CH3U_SRBuf_free(req);
    }

    MPIU_Handle_obj_free(&MPID_Request_mem, req);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQUEST_DESTROY);
}


/* ------------------------------------------------------------------------- */
/* Here are the routines to manipulate the iovs in the requests              */
/* ------------------------------------------------------------------------- */



/*
 * MPIDI_CH3U_Request_load_send_iov()
 *
 * Fill the provided IOV with the next (or remaining) portion of data described
 * by the segment contained in the request structure.
 * If the density of IOV is not sufficient, pack the data into a send/receive 
 * buffer and point the IOV at the buffer.
 *
 * Expects sreq->dev.OnFinal to be initialized (even if it's NULL).
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_send_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_send_iov(MPID_Request * const sreq, 
				     MPID_IOV * const iov, int * const iov_n)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    MPIU_Assert(sreq->dev.segment_ptr != NULL);
    last = sreq->dev.segment_size;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
     "pre-pv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    MPIU_Assert(sreq->dev.segment_first < last);
    MPIU_Assert(last > 0);
    MPIU_Assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    MPID_Segment_pack_vector(sreq->dev.segment_ptr, sreq->dev.segment_first, 
			     &last, iov, iov_n);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
    "post-pv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    MPIU_Assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    
    if (last == sreq->dev.segment_size)
    {
	MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"remaining data loaded into IOV");
	sreq->dev.OnDataAvail = sreq->dev.OnFinal;
    }
    else if ((last - sreq->dev.segment_first) / *iov_n >= MPIDI_IOV_DENSITY_MIN)
    {
	MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"more data loaded into IOV");
	sreq->dev.segment_first = last;
	sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SendReloadIOV;
    }
    else
    {
	MPIDI_msg_sz_t data_sz;
	int i, iov_data_copied;
	
	MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"low density.  using SRBuf.");
	    
	data_sz = sreq->dev.segment_size - sreq->dev.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(sreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (sreq->dev.tmpbuf_sz == 0)
	    {
		MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
                                FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 
						 "**nomem %d", data_sz);
		sreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	iov_data_copied = 0;
	for (i = 0; i < *iov_n; i++) {
	    MPIU_Memcpy((char*) sreq->dev.tmpbuf + iov_data_copied, 
		   iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN);
	    iov_data_copied += iov[i].MPID_IOV_LEN;
	}
	sreq->dev.segment_first = last;

	last = (data_sz <= sreq->dev.tmpbuf_sz - iov_data_copied) ? 
	    sreq->dev.segment_size :
	    sreq->dev.segment_first + sreq->dev.tmpbuf_sz - iov_data_copied;
	MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
               "pre-pack: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT,
			  sreq->dev.segment_first, last));
	MPID_Segment_pack(sreq->dev.segment_ptr, sreq->dev.segment_first, 
			  &last, (char*) sreq->dev.tmpbuf + iov_data_copied);
	MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
              "post-pack: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT,
			   sreq->dev.segment_first, last));
	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.tmpbuf;
	iov[0].MPID_IOV_LEN = last - sreq->dev.segment_first + iov_data_copied;
	*iov_n = 1;
	if (last == sreq->dev.segment_size)
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"remaining data packed into SRBuf");
	    sreq->dev.OnDataAvail = sreq->dev.OnFinal;
	}
	else 
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"more data packed into SRBuf");
	    sreq->dev.segment_first = last;
	    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SendReloadIOV;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_load_recv_iov()
 *
 * Fill the request's IOV with the next (or remaining) portion of data 
 * described by the segment (also contained in the request
 * structure).  If the density of IOV is not sufficient, allocate a 
 * send/receive buffer and point the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_recv_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_recv_iov(MPID_Request * const rreq)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);
    if (rreq->dev.segment_first < rreq->dev.segment_size)
    {
	/* still reading data that needs to go into the user buffer */
	
	if (MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_msg_sz_t data_sz;
	    MPIDI_msg_sz_t tmpbuf_sz;

	    /* Once a SRBuf is in use, we continue to use it since a small 
	       amount of data may already be present at the beginning
	       of the buffer.  This data is left over from the previous unpack,
	       most like a result of alignment issues.  NOTE: we
	       could force the use of the SRBuf only 
	       when (rreq->dev.tmpbuf_off > 0)... */
	    
	    data_sz = rreq->dev.segment_size - rreq->dev.segment_first - 
		rreq->dev.tmpbuf_off;
	    MPIU_Assert(data_sz > 0);
	    tmpbuf_sz = rreq->dev.tmpbuf_sz - rreq->dev.tmpbuf_off;
	    if (data_sz > tmpbuf_sz)
	    {
		data_sz = tmpbuf_sz;
	    }
	    rreq->dev.iov[0].MPID_IOV_BUF = 
		(MPID_IOV_BUF_CAST)((char *) rreq->dev.tmpbuf + 
				    rreq->dev.tmpbuf_off);
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
            rreq->dev.iov_offset = 0;
	    rreq->dev.iov_count = 1;
	    MPIU_Assert(rreq->dev.segment_first + data_sz + 
			rreq->dev.tmpbuf_off <= rreq->dev.recv_data_sz);
	    if (rreq->dev.segment_first + data_sz + rreq->dev.tmpbuf_off == 
		rreq->dev.recv_data_sz)
	    {
		MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
		  "updating rreq to read the remaining data into the SRBuf");
		rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackSRBufComplete;
	    }
	    else
	    {
		MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
		       "updating rreq to read more data into the SRBuf");
		rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV;
	    }
	    goto fn_exit;
	}
	
	last = rreq->dev.segment_size;
	rreq->dev.iov_count = MPID_IOV_LIMIT;
	rreq->dev.iov_offset = 0;
	MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
   "pre-upv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count));
	MPIU_Assert(rreq->dev.segment_first < last);
	MPIU_Assert(last > 0);
	MPID_Segment_unpack_vector(rreq->dev.segment_ptr, 
				   rreq->dev.segment_first,
				   &last, &rreq->dev.iov[0], &rreq->dev.iov_count);
	MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
   "post-upv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d, iov_offset=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count, rreq->dev.iov_offset));
	MPIU_Assert(rreq->dev.iov_count >= 0 && rreq->dev.iov_count <= 
		    MPID_IOV_LIMIT);

	/* --BEGIN ERROR HANDLING-- */
	if (rreq->dev.iov_count == 0)
	{
	    /* If the data can't be unpacked, the we have a mis-match between
	       the datatype and the amount of data received.  Adjust
	       the segment info so that the remaining data is received and 
	       thrown away. */
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
		       "**dtypemismatch", 0);
	    rreq->status.count = (int)rreq->dev.segment_first;
	    rreq->dev.segment_size = rreq->dev.segment_first;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    goto fn_exit;
	}
        else
        {
            MPIU_Assert(rreq->dev.iov_offset < rreq->dev.iov_count);
        }
	/* --END ERROR HANDLING-- */

	if (last == rreq->dev.recv_data_sz)
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
     "updating rreq to read the remaining data directly into the user buffer");
	    /* Eventually, use OnFinal for this instead */
	    rreq->dev.OnDataAvail = 0;
	}
	else if (last == rreq->dev.segment_size || 
		 (last - rreq->dev.segment_first) / rreq->dev.iov_count >= MPIDI_IOV_DENSITY_MIN)
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
	     "updating rreq to read more data directly into the user buffer");
	    rreq->dev.segment_first = last;
	    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReloadIOV;
	}
	else
	{
	    /* Too little data would have been received using an IOV.  
	       We will start receiving data into a SRBuf and unpacking it
	       later. */
	    MPIU_Assert(MPIDI_Request_get_srbuf_flag(rreq) == FALSE);
	    
	    MPIDI_CH3U_SRBuf_alloc(rreq, 
			    rreq->dev.segment_size - rreq->dev.segment_first);
	    rreq->dev.tmpbuf_off = 0;
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		/* FIXME - we should drain the data off the pipe here, but we 
		   don't have a buffer to drain it into.  should this be
		   a fatal error? */
		MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
			      FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 
			 "**nomem %d", 
			 rreq->dev.segment_size - rreq->dev.segment_first);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    /* fill in the IOV using a recursive call */
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	}
    }
    else
    {
	/* receive and toss any extra data that does not fit in the user's 
	   buffer */
	MPIDI_msg_sz_t data_sz;

	data_sz = rreq->dev.recv_data_sz - rreq->dev.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(rreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
			       FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	if (data_sz <= rreq->dev.tmpbuf_sz)
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
	    "updating rreq to read overflow data into the SRBuf and complete");
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV);
	    /* Eventually, use OnFinal for this instead */
	    rreq->dev.OnDataAvail = 0;
	}
	else
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
	  "updating rreq to read overflow data into the SRBuf and reload IOV");
	    rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.tmpbuf_sz;
	    rreq->dev.segment_first += rreq->dev.tmpbuf_sz;
	    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReloadIOV;
	}
	
	rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.tmpbuf;
	rreq->dev.iov_count = 1;
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_unpack_srbuf
 *
 * Unpack data from a send/receive buffer into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_unpack_srbuf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_unpack_srbuf(MPID_Request * rreq)
{
    MPI_Aint last;
    int tmpbuf_last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);

    tmpbuf_last = (int)(rreq->dev.segment_first + rreq->dev.tmpbuf_sz);
    if (rreq->dev.segment_size < tmpbuf_last)
    {
	tmpbuf_last = (int)rreq->dev.segment_size;
    }
    last = tmpbuf_last;
    MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, 
			&last, rreq->dev.tmpbuf);
    if (last == 0 || last == rreq->dev.segment_first)
    {
	/* --BEGIN ERROR HANDLING-- */
	/* If no data can be unpacked, then we have a datatype processing 
	   problem.  Adjust the segment info so that the remaining
	   data is received and thrown away. */
	rreq->status.count = (int)rreq->dev.segment_first;
	rreq->dev.segment_size = rreq->dev.segment_first;
	rreq->dev.segment_first += tmpbuf_last;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
		       "**dtypemismatch", 0);
	/* --END ERROR HANDLING-- */
    }
    else if (tmpbuf_last == rreq->dev.segment_size)
    {
	/* --BEGIN ERROR HANDLING-- */
	if (last != tmpbuf_last)
	{
	    /* received data was not entirely consumed by unpack() because too
	       few bytes remained to fill the next basic datatype.
	       Note: the segment_first field is set to segment_last so that if
	       this is a truncated message, extra data will be read
	       off the pipe. */
	    rreq->status.count = (int)last;
	    rreq->dev.segment_size = last;
	    rreq->dev.segment_first = tmpbuf_last;
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		  MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							  "**dtypemismatch", 0);
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	rreq->dev.tmpbuf_off = (int)(tmpbuf_last - last);
	if (rreq->dev.tmpbuf_off > 0)
	{
	    /* move any remaining data to the beginning of the buffer.  
	       Note: memmove() is used since the data regions could
               overlap. */
	    memmove(rreq->dev.tmpbuf, (char *) rreq->dev.tmpbuf + 
		    (last - rreq->dev.segment_first), rreq->dev.tmpbuf_off);
	}
	rreq->dev.segment_first = last;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_unpack_uebuf
 *
 * Copy/unpack data from an "unexpected eager buffer" into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_unpack_uebuf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_unpack_uebuf(MPID_Request * rreq)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t unpack_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, 
			    dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
    
    if (rreq->dev.recv_data_sz <= userbuf_sz)
    {
	unpack_sz = rreq->dev.recv_data_sz;
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,(MPIU_DBG_FDEST,
      "receive buffer overflow; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT 
	      ", buf_sz=" MPIDI_MSG_SZ_FMT, 
                rreq->dev.recv_data_sz, userbuf_sz));
	unpack_sz = userbuf_sz;
	rreq->status.count = (int)userbuf_sz;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		 MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
		 "**truncate", "**truncate %d %d", 
                 rreq->dev.recv_data_sz, userbuf_sz);
	/* --END ERROR HANDLING-- */
    }

    if (unpack_sz > 0)
    {
	if (dt_contig)
	{
	    /* TODO - check that amount of data is consistent with datatype.  
	       In other words, if we were to use Segment_unpack()
	       would last = unpack?  If not we should return an error 
	       (unless configured with --enable-fast) */
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    MPIU_Memcpy((char *)rreq->dev.user_buf + dt_true_lb, rreq->dev.tmpbuf,
		   unpack_sz);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	}
	else
	{
	    MPID_Segment seg;
	    MPI_Aint last;

	    MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, 
			      rreq->dev.datatype, &seg, 0);
	    last = unpack_sz;
	    MPID_Segment_unpack(&seg, 0, &last, rreq->dev.tmpbuf);
	    if (last != unpack_sz)
	    {
		/* --BEGIN ERROR HANDLING-- */
		/* received data was not entirely consumed by unpack() 
		   because too few bytes remained to fill the next basic
		   datatype */
		rreq->status.count = (int)last;
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
                         MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
			 "**dtypemismatch", 0);
		/* --END ERROR HANDLING-- */
	    }
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    return mpi_errno;
}

/* 
 * Export the function to set a request as completed for use by
 * the generalized request functions in mpich2/src/pt2pt/greq_complete.c
 */
void MPID_Request_set_completed( MPID_Request *req )
{
    MPID_REQUEST_SET_COMPLETED(req);
}
