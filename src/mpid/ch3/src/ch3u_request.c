/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

/* Max depth of recursive calls of MPID_Request_complete */
#define REQUEST_CB_DEPTH 2
#define MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET (-1)

/* See the comments above about request creation.  Some routines will
   use macros in mpidimpl.h *instead* of this routine */
void MPID_Request_create_hook(MPIR_Request *req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_INIT);
    
    req->dev.datatype_ptr	   = NULL;
    req->dev.msg_offset         = 0;
    /* Masks and pkt_flags for channel device state in an MPIR_Request */
    req->dev.state		   = 0;
    req->dev.cancel_pending	   = FALSE;
    /* FIXME: RMA ops shouldn't need to be set except when creating a
     * request for RMA operations */
    req->dev.target_win_handle = MPI_WIN_NULL;
    req->dev.source_win_handle = MPI_WIN_NULL;
    req->dev.target_lock_queue_entry = NULL;
    req->dev.flattened_type = NULL;
    req->dev.iov_offset        = 0;
    req->dev.pkt_flags             = MPIDI_CH3_PKT_FLAG_NONE;
    req->dev.resp_request_handle = MPI_REQUEST_NULL;
    req->dev.user_buf          = NULL;
    req->dev.OnDataAvail       = NULL;
    req->dev.OnFinal           = NULL;
    req->dev.user_buf          = NULL;
    req->dev.drop_data         = FALSE;
    req->dev.tmpbuf            = NULL;
    req->dev.ext_hdr_ptr       = NULL;
    req->dev.ext_hdr_sz        = 0;
    req->dev.rma_target_ptr    = NULL;
    req->dev.request_handle    = MPI_REQUEST_NULL;
    req->dev.orig_msg_offset = MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET;

    req->dev.request_completed_cb  = NULL;
#ifdef MPIDI_CH3_REQUEST_INIT
    MPIDI_CH3_REQUEST_INIT(req);
#endif
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
int MPIDI_CH3U_Request_load_send_iov(MPIR_Request * const sreq,
				     struct iovec * const iov, int * const iov_n)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);

    last = sreq->dev.msgsize;
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,(MPL_DBG_FDEST,
     "pre-pv: first=%" PRIdPTR ", last=%" PRIdPTR ", iov_n=%d",
		      sreq->dev.msg_offset, last, *iov_n));
    MPIR_Assert(sreq->dev.msg_offset < last);
    MPIR_Assert(last > 0);
    MPIR_Assert(*iov_n > 0 && *iov_n <= MPL_IOV_LIMIT);

    int max_iov_len = *iov_n;
    MPI_Aint actual_iov_bytes, actual_iov_len;
    MPIR_Typerep_to_iov(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype,
                     sreq->dev.msg_offset, iov, (MPI_Aint) max_iov_len,
                     sreq->dev.msgsize - sreq->dev.msg_offset, &actual_iov_len, &actual_iov_bytes);
    *iov_n = (int) actual_iov_len;
    last = sreq->dev.msg_offset + actual_iov_bytes;

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,(MPL_DBG_FDEST,
    "post-pv: first=%" PRIdPTR ", last=%" PRIdPTR ", iov_n=%d",
		      sreq->dev.msg_offset, last, *iov_n));
    MPIR_Assert(*iov_n > 0 && *iov_n <= MPL_IOV_LIMIT);
    
    if (last == sreq->dev.msgsize)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"remaining data loaded into IOV");
	sreq->dev.OnDataAvail = sreq->dev.OnFinal;
    }
    else if ((last - sreq->dev.msg_offset) / *iov_n >= MPIDI_IOV_DENSITY_MIN)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"more data loaded into IOV");
	sreq->dev.msg_offset = last;
	sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SendReloadIOV;
    }
    else
    {
	intptr_t data_sz;
	int i, iov_data_copied;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"low density.  using SRBuf.");
	    
	data_sz = sreq->dev.msgsize - sreq->dev.msg_offset;
	if (!MPIDI_Request_get_srbuf_flag(sreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (sreq->dev.tmpbuf_sz == 0)
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,TYPICAL,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
                                __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 
						 "**nomem %d", data_sz);
		sreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	iov_data_copied = 0;
	for (i = 0; i < *iov_n; i++) {
	    MPIR_Memcpy((char*) sreq->dev.tmpbuf + iov_data_copied,
		   iov[i].iov_base, iov[i].iov_len);
	    iov_data_copied += iov[i].iov_len;
	}
	sreq->dev.msg_offset = last;

        MPI_Aint max_pack_bytes;
        MPI_Aint actual_pack_bytes;

        if (data_sz > sreq->dev.tmpbuf_sz - iov_data_copied)
            max_pack_bytes = sreq->dev.tmpbuf_sz - iov_data_copied;
        else
            max_pack_bytes = sreq->dev.msgsize - sreq->dev.msg_offset;

        MPIR_Typerep_pack(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype,
                       sreq->dev.msg_offset, (char*) sreq->dev.tmpbuf + iov_data_copied,
                       max_pack_bytes, &actual_pack_bytes);
        last = sreq->dev.msg_offset + actual_pack_bytes;

	iov[0].iov_base = (void *)sreq->dev.tmpbuf;
	iov[0].iov_len = actual_pack_bytes + iov_data_copied;
	*iov_n = 1;
	if (last == sreq->dev.msgsize)
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"remaining data packed into SRBuf");
	    sreq->dev.OnDataAvail = sreq->dev.OnFinal;
	}
	else 
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"more data packed into SRBuf");
	    sreq->dev.msg_offset = last;
	    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SendReloadIOV;
	}
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
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
int MPIDI_CH3U_Request_load_recv_iov(MPIR_Request * const rreq)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);

    if (rreq->dev.orig_msg_offset == MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET) {
        rreq->dev.orig_msg_offset = rreq->dev.msg_offset;
    }

    if (rreq->dev.msg_offset < rreq->dev.msgsize)
    {
	/* still reading data that needs to go into the user buffer */
	
	if (MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_ACCUM_RECV &&
            MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_ACCUM_RECV &&
            MPIDI_Request_get_srbuf_flag(rreq))
	{
	    intptr_t data_sz;
	    intptr_t tmpbuf_sz;

	    /* Once a SRBuf is in use, we continue to use it since a small 
	       amount of data may already be present at the beginning
	       of the buffer.  This data is left over from the previous unpack,
	       most like a result of alignment issues.  NOTE: we
	       could force the use of the SRBuf only 
	       when (rreq->dev.tmpbuf_off > 0)... */
	    
	    data_sz = rreq->dev.msgsize - rreq->dev.msg_offset - 
		rreq->dev.tmpbuf_off;
	    MPIR_Assert(data_sz > 0);
	    tmpbuf_sz = rreq->dev.tmpbuf_sz - rreq->dev.tmpbuf_off;
	    if (data_sz > tmpbuf_sz)
	    {
		data_sz = tmpbuf_sz;
	    }
	    rreq->dev.iov[0].iov_base =
		(void *)((char *) rreq->dev.tmpbuf +
				    rreq->dev.tmpbuf_off);
	    rreq->dev.iov[0].iov_len = data_sz;
            rreq->dev.iov_offset = 0;
	    rreq->dev.iov_count = 1;
	    MPIR_Assert(rreq->dev.msg_offset - rreq->dev.orig_msg_offset + data_sz +
			rreq->dev.tmpbuf_off <= rreq->dev.recv_data_sz);
	    if (rreq->dev.msg_offset - rreq->dev.orig_msg_offset + data_sz + rreq->dev.tmpbuf_off ==
		rreq->dev.recv_data_sz)
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
		  "updating rreq to read the remaining data into the SRBuf");
		rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackSRBufComplete;
                rreq->dev.orig_msg_offset = MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET;
	    }
	    else
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
		       "updating rreq to read more data into the SRBuf");
		rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV;
	    }
	    goto fn_exit;
	}
	
	last = rreq->dev.msgsize;
	rreq->dev.iov_count = MPL_IOV_LIMIT;
	rreq->dev.iov_offset = 0;
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,(MPL_DBG_FDEST,
   "pre-upv: first=%" PRIdPTR ", last=%" PRIdPTR ", iov_n=%d",
			  rreq->dev.msg_offset, last, rreq->dev.iov_count));
	MPIR_Assert(rreq->dev.msg_offset < last);
	MPIR_Assert(last > 0);

        MPI_Aint actual_iov_bytes, actual_iov_len;
        MPIR_Typerep_to_iov(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype,
                         rreq->dev.msg_offset, &rreq->dev.iov[0], MPL_IOV_LIMIT,
                         rreq->dev.msgsize - rreq->dev.msg_offset,
                         &actual_iov_len, &actual_iov_bytes);
        rreq->dev.iov_count = (int) actual_iov_len;
        last = rreq->dev.msg_offset + actual_iov_bytes;

	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,(MPL_DBG_FDEST,
   "post-upv: first=%" PRIdPTR ", last=%" PRIdPTR ", iov_n=%d, iov_offset=%lld",
			  rreq->dev.msg_offset, last, rreq->dev.iov_count, (long long)rreq->dev.iov_offset));
	MPIR_Assert(rreq->dev.iov_count >= 0 && rreq->dev.iov_count <=
		    MPL_IOV_LIMIT);

	/* --BEGIN ERROR HANDLING-- */
	if (rreq->dev.iov_count == 0)
	{
	    /* If the data can't be unpacked, the we have a mismatch between
	       the datatype and the amount of data received.  Adjust
	       the segment info so that the remaining data is received and 
	       thrown away. */
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		       MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE,
		       "**dtypemismatch", 0);
            MPIR_STATUS_SET_COUNT(rreq->status, rreq->dev.msg_offset);
	    rreq->dev.msgsize = rreq->dev.msg_offset;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    goto fn_exit;
	}
        else
        {
            MPIR_Assert(rreq->dev.iov_offset < rreq->dev.iov_count);
        }
	/* --END ERROR HANDLING-- */

	if (last == rreq->dev.recv_data_sz + rreq->dev.orig_msg_offset)
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
     "updating rreq to read the remaining data directly into the user buffer");
	    /* Eventually, use OnFinal for this instead */
	    rreq->dev.OnDataAvail = rreq->dev.OnFinal;
            rreq->dev.orig_msg_offset = MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET;
	}
	else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RECV ||
                 MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RECV ||
                 (last == rreq->dev.msgsize ||
                  (last - rreq->dev.msg_offset) / rreq->dev.iov_count >= MPIDI_IOV_DENSITY_MIN))
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
	     "updating rreq to read more data directly into the user buffer");
	    rreq->dev.msg_offset = last;
	    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReloadIOV;
	}
	else
	{
	    /* Too little data would have been received using an IOV.  
	       We will start receiving data into a SRBuf and unpacking it
	       later. */
	    MPIR_Assert(MPIDI_Request_get_srbuf_flag(rreq) == FALSE);
	    
	    MPIDI_CH3U_SRBuf_alloc(rreq, 
			    rreq->dev.msgsize - rreq->dev.msg_offset);
	    rreq->dev.tmpbuf_off = 0;
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		/* FIXME - we should drain the data off the pipe here, but we 
		   don't have a buffer to drain it into.  should this be
		   a fatal error? */
		MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
			      __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 
			 "**nomem %d", 
			 rreq->dev.msgsize - rreq->dev.msg_offset);
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
	intptr_t data_sz;

	data_sz = rreq->dev.recv_data_sz - rreq->dev.msg_offset;
	if (!MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(rreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,TYPICAL,"SRBuf allocation failure");
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
			       __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	if (data_sz <= rreq->dev.tmpbuf_sz)
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
	    "updating rreq to read overflow data into the SRBuf and complete");
	    rreq->dev.iov[0].iov_len = data_sz;
	    MPIR_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV);
	    /* Eventually, use OnFinal for this instead */
	    rreq->dev.OnDataAvail = rreq->dev.OnFinal;
            rreq->dev.orig_msg_offset = MPIDI_LOAD_RECV_IOV_ORIG_MSG_OFFSET_UNSET;
	}
	else
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,VERBOSE,
	  "updating rreq to read overflow data into the SRBuf and reload IOV");
	    rreq->dev.iov[0].iov_len = rreq->dev.tmpbuf_sz;
	    rreq->dev.msg_offset += rreq->dev.tmpbuf_sz;
	    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReloadIOV;
	}
	
	rreq->dev.iov[0].iov_base = (void *)rreq->dev.tmpbuf;
	rreq->dev.iov_count = 1;
    }
    
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_unpack_srbuf
 *
 * Unpack data from a send/receive buffer into the user buffer.
 */
int MPIDI_CH3U_Request_unpack_srbuf(MPIR_Request * rreq)
{
    MPI_Aint last;
    int tmpbuf_last;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);

    tmpbuf_last = (int)(rreq->dev.msg_offset + rreq->dev.tmpbuf_sz);
    if (rreq->dev.msgsize < tmpbuf_last)
    {
	tmpbuf_last = (int)rreq->dev.msgsize;
    }

    MPI_Aint actual_unpack_bytes;
    MPIR_Typerep_unpack(rreq->dev.tmpbuf, tmpbuf_last - rreq->dev.msg_offset,
                     rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype,
                     rreq->dev.msg_offset, &actual_unpack_bytes);
    last = rreq->dev.msg_offset + actual_unpack_bytes;

    if (last == 0 || last == rreq->dev.msg_offset)
    {
	/* --BEGIN ERROR HANDLING-- */
	/* If no data can be unpacked, then we have a datatype processing 
	   problem.  Adjust the segment info so that the remaining
	   data is received and thrown away. */
	MPIR_STATUS_SET_COUNT(rreq->status, rreq->dev.msg_offset);
	rreq->dev.msgsize = rreq->dev.msg_offset;
	rreq->dev.msg_offset += tmpbuf_last;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		       MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE,
		       "**dtypemismatch", 0);
	/* --END ERROR HANDLING-- */
    }
    else if (tmpbuf_last == rreq->dev.msgsize)
    {
	/* --BEGIN ERROR HANDLING-- */
	if (last != tmpbuf_last)
	{
	    /* received data was not entirely consumed by unpack() because too
	       few bytes remained to fill the next basic datatype.
	       Note: the msg_offset field is set to segment_last so that if
	       this is a truncated message, extra data will be read
	       off the pipe. */
	    MPIR_STATUS_SET_COUNT(rreq->status, last);
	    rreq->dev.msgsize = last;
	    rreq->dev.msg_offset = tmpbuf_last;
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		  MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE,
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
		    (last - rreq->dev.msg_offset), rreq->dev.tmpbuf_off);
	}
	rreq->dev.msg_offset = last;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_unpack_uebuf
 *
 * Copy/unpack data from an "unexpected eager buffer" into the user buffer.
 */
int MPIDI_CH3U_Request_unpack_uebuf(MPIR_Request * rreq)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    intptr_t userbuf_sz;
    MPIR_Datatype * dt_ptr;
    intptr_t unpack_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MEMCPY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, 
			    dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
    
    if (rreq->dev.recv_data_sz <= userbuf_sz)
    {
	unpack_sz = rreq->dev.recv_data_sz;
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL,VERBOSE,(MPL_DBG_FDEST,
      "receive buffer overflow; message truncated, msg_sz=%" PRIdPTR
	      ", buf_sz=%" PRIdPTR,
                rreq->dev.recv_data_sz, userbuf_sz));
	unpack_sz = userbuf_sz;
	MPIR_STATUS_SET_COUNT(rreq->status, userbuf_sz);
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
		 MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TRUNCATE,
		 "**truncate", "**truncate %d %d", 
                 rreq->dev.recv_data_sz, userbuf_sz);
	/* --END ERROR HANDLING-- */
    }

    if (unpack_sz > 0)
    {
	if (dt_contig)
	{
	    /* TODO - check that amount of data is consistent with
	       datatype.  If not we should return an error (unless
	       configured with --enable-fast) */
	    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MEMCPY);
	    MPIR_Memcpy((char *)rreq->dev.user_buf + dt_true_lb, rreq->dev.tmpbuf,
		   unpack_sz);
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MEMCPY);
	}
	else
	{
	    MPI_Aint actual_unpack_bytes;
	    MPIR_Typerep_unpack(rreq->dev.tmpbuf, unpack_sz,
			     rreq->dev.user_buf, rreq->dev.user_count,
			     rreq->dev.datatype, 0, &actual_unpack_bytes);

	    if (actual_unpack_bytes != unpack_sz)
	    {
		/* --BEGIN ERROR HANDLING-- */
		/* received data was not entirely consumed by unpack() 
		   because too few bytes remained to fill the next basic
		   datatype */
		MPIR_STATUS_SET_COUNT(rreq->status, actual_unpack_bytes);
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
                         MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE,
			 "**dtypemismatch", 0);
		/* --END ERROR HANDLING-- */
	    }
	}
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    return mpi_errno;
}

int MPID_Request_complete(MPIR_Request *req)
{
    int incomplete;
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH3U_Request_decrement_cc(req, &incomplete);
    if (!incomplete) {
        /* decrement completion_notification counter */
        if (req->completion_notification)
            MPIR_cc_dec(req->completion_notification);

	MPIR_Request_free(req);
    }

    return mpi_errno;
}

void MPID_Request_free_hook(MPIR_Request *req)
{
    static int called_cnt = 0;

    MPIR_Assert(called_cnt <= REQUEST_CB_DEPTH);
    called_cnt++;

    /* trigger request_completed callback function */
    if (req->dev.request_completed_cb != NULL && MPIR_Request_is_complete(req)) {
        int mpi_errno = req->dev.request_completed_cb(req);
        MPIR_Assert(mpi_errno == MPI_SUCCESS);

        req->dev.request_completed_cb = NULL;
    }

    MPIDI_CH3_Progress_signal_completion();

    called_cnt--;
}

void MPID_Request_destroy_hook(MPIR_Request *req)
{
    if (req->dev.datatype_ptr != NULL) {
        MPIR_Datatype_ptr_release(req->dev.datatype_ptr);
    }

    if (MPIDI_Request_get_srbuf_flag(req)) {
        MPIDI_CH3U_SRBuf_free(req);
    }

    MPL_free(req->dev.ext_hdr_ptr);
    MPL_free(req->dev.flattened_type);
}
