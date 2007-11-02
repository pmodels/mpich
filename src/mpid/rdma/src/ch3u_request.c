/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Request_load_send_iov()
 *
 * Fill the provided IOV with the next (or remaining) portion of data described by the segment contained in the request structure.
 * If the density of IOV is not sufficient, pack the data into a send/receive buffer and point the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_send_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_send_iov(MPID_Request * const sreq, MPID_IOV * const iov, int * const iov_n)
{
    MPIDI_msg_sz_t last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    last = sreq->dev.segment_size;
    MPIDI_DBG_PRINTF((40, FCNAME, "pre-pv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    assert(sreq->dev.segment_first < last);
    assert(last > 0);
    assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    MPID_Segment_pack_vector(&sreq->dev.segment, sreq->dev.segment_first, &last, iov, iov_n);
    MPIDI_DBG_PRINTF((40, FCNAME, "post-pv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    
    if (last == sreq->dev.segment_size)
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "remaining data loaded into IOV"));
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
    }
    else if ((last - sreq->dev.segment_first) / *iov_n >= MPIDI_IOV_DENSITY_MIN)
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "more data loaded into IOV"));
	sreq->dev.segment_first = last;
	sreq->dev.ca = MPIDI_CH3_CA_RELOAD_IOV;
    }
    else
    {
	MPIDI_msg_sz_t data_sz;
	
	MPIDI_DBG_PRINTF((40, FCNAME, "low density.  using SRBuf."));
	    
	data_sz = sreq->dev.segment_size - sreq->dev.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(sreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
	    if (sreq->dev.tmpbuf_sz == 0)
	    {
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		sreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	}
		    
	last = (data_sz <= sreq->dev.tmpbuf_sz) ? sreq->dev.segment_size :
	    sreq->dev.segment_first + sreq->dev.tmpbuf_sz;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-pack: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT,
			  sreq->dev.segment_first, last));
	MPID_Segment_pack(&sreq->dev.segment, sreq->dev.segment_first, &last, sreq->dev.tmpbuf);
	MPIDI_DBG_PRINTF((40, FCNAME, "post-pack: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT,
			   sreq->dev.segment_first, last));
	iov[0].MPID_IOV_BUF = sreq->dev.tmpbuf;
	iov[0].MPID_IOV_LEN = last - sreq->dev.segment_first;
	*iov_n = 1;
	if (last == sreq->dev.segment_size)
	{
	    MPIDI_DBG_PRINTF((40, FCNAME, "remaining data packed into SRBuf"));
	    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else 
	{
	    MPIDI_DBG_PRINTF((40, FCNAME, "more data packed into SRBuf"));
	    sreq->dev.segment_first = last;
	    sreq->dev.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_load_recv_iov()
 *
 * Fill the request's IOV with the next (or remaining) portion of data described by the segment (also contained in the request
 * structure).  If the density of IOV is not sufficient, allocate a send/receive buffer and point the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_recv_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_recv_iov(MPID_Request * const rreq)
{
    MPIDI_msg_sz_t last;
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

	    /* Once a SRBuf is in use, we continue to use it since a small amount of data may already be present at the beginning
	       of the buffer.  This data is left over from the previous unpack, most like a result of alignment issues.  NOTE: we
	       could force the use of the SRBuf only when (rreq->dev.tmpbuf_off > 0)... */
	    
	    data_sz = rreq->dev.segment_size - rreq->dev.segment_first - rreq->dev.tmpbuf_off;
	    assert(data_sz > 0);
	    tmpbuf_sz = rreq->dev.tmpbuf_sz - rreq->dev.tmpbuf_off;
	    if (data_sz > tmpbuf_sz)
	    {
		data_sz = tmpbuf_sz;
	    }
	    rreq->dev.iov[0].MPID_IOV_BUF = (char *) rreq->dev.tmpbuf + rreq->dev.tmpbuf_off;
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->dev.iov_count = 1;
	    assert(rreq->dev.segment_first + data_sz + rreq->dev.tmpbuf_off <= rreq->dev.recv_data_sz);
	    if (rreq->dev.segment_first + data_sz + rreq->dev.tmpbuf_off == rreq->dev.recv_data_sz)
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read the remaining data into the SRBuf"));
		rreq->dev.ca = MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE;
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read more data into the SRBuf"));
		rreq->dev.ca = MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV;
	    }
	    goto fn_exit;
	}
	
	last = rreq->dev.segment_size;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-upv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count));
	assert(rreq->dev.segment_first < last);
	assert(last > 0);
	rreq->dev.iov_count = MPID_IOV_LIMIT;
	MPID_Segment_unpack_vector(&rreq->dev.segment, rreq->dev.segment_first, &last, rreq->dev.iov, &rreq->dev.iov_count);
	MPIDI_DBG_PRINTF((40, FCNAME, "post-upv: first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count));
	assert(rreq->dev.iov_count > 0 && rreq->dev.iov_count <= MPID_IOV_LIMIT);
	
	if (rreq->dev.iov_count == 0)
	{
	    /* If the data can't be unpacked, the we have a mis-match between the datatype and the amount of data received.  Adjust
	       the segment info so that the remaining data is received and thrown away. */
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							  "**dtypemismatch", 0);
	    rreq->status.count = rreq->dev.segment_first;
	    rreq->dev.segment_size = rreq->dev.segment_first;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    goto fn_exit;
	}

	if (last == rreq->dev.recv_data_sz)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read the remaining data directly into the user buffer"));
	    rreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else if (last == rreq->dev.segment_size || (last - rreq->dev.segment_first) / rreq->dev.iov_count >= MPIDI_IOV_DENSITY_MIN)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read more data directly into the user buffer"));
	    rreq->dev.segment_first = last;
	    rreq->dev.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
	else
	{
	    /* Too little data would have been received using an IOV.  We will start receiving data into a SRBuf and unpacking it
	       later. */
	    assert(MPIDI_Request_get_srbuf_flag(rreq) == FALSE);
	    
	    MPIDI_CH3U_SRBuf_alloc(rreq, rreq->dev.segment_size - rreq->dev.segment_first);
	    rreq->dev.tmpbuf_off = 0;
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		/* FIXME - we should drain the data off the pipe here, but we don't have a buffer to drain it into.  should this be
		   a fatal error? */
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }

	    /* fill in the IOV using a recursive call */
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	}
    }
    else
    {
	/* receive and toss any extra data that does not fit in the user's buffer */
	MPIDI_msg_sz_t data_sz;

	data_sz = rreq->dev.recv_data_sz - rreq->dev.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(rreq, data_sz);
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	}

	if (data_sz <= rreq->dev.tmpbuf_sz)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow data into the SRBuf and complete"));
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow data into the SRBuf and reload IOV"));
	    rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.tmpbuf_sz;
	    rreq->dev.segment_first += rreq->dev.tmpbuf_sz;
	    rreq->dev.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
	
	rreq->dev.iov[0].MPID_IOV_BUF = rreq->dev.tmpbuf;
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
    MPIDI_msg_sz_t last;
    int tmpbuf_last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);

    tmpbuf_last = rreq->dev.segment_first + rreq->dev.tmpbuf_sz;
    if (rreq->dev.segment_size < tmpbuf_last)
    {
	tmpbuf_last = rreq->dev.segment_size;
    }
    last = tmpbuf_last;
    MPID_Segment_unpack(&rreq->dev.segment, rreq->dev.segment_first, &last, rreq->dev.tmpbuf);
    if (last == 0 || last == rreq->dev.segment_first)
    {
	/* If no data can be unpacked, then we have a datatype processing problem.  Adjust the segment info so that the remaining
	   data is received and thrown away. */
	rreq->status.count = rreq->dev.segment_first;
	rreq->dev.segment_size = rreq->dev.segment_first;
	rreq->dev.segment_first += tmpbuf_last;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
						      "**dtypemismatch", 0);
    }
    else if (tmpbuf_last == rreq->dev.segment_size)
    {
	if (last != tmpbuf_last)
	{
	    /* received data was not entirely consumed by unpack() because too few bytes remained to fill the next basic datatype.
	       Note: the segment_first field is set to segment_last so that if this is a truncated message, extra data will be read
	       off the pipe. */
	    rreq->status.count = last;
	    rreq->dev.segment_size = last;
	    rreq->dev.segment_first = tmpbuf_last;
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							  "**dtypemismatch", 0);
	}
    }
    else
    {
	rreq->dev.tmpbuf_off = tmpbuf_last - last;
	if (rreq->dev.tmpbuf_off > 0)
	{
	    /* move any remaining data to the beginning of the buffer.  Note: memmove() is used since the data regions could
               overlap. */
	    memmove(rreq->dev.tmpbuf, (char *) rreq->dev.tmpbuf + (last - rreq->dev.segment_first), rreq->dev.tmpbuf_off);
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
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t unpack_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    MPIDI_CH3U_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, userbuf_sz, dt_ptr);
    
    if (rreq->dev.recv_data_sz <= userbuf_sz)
    {
	unpack_sz = rreq->dev.recv_data_sz;
    }
    else
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "receive buffer overflow; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", buf_sz="
			  MPIDI_MSG_SZ_FMT, rreq->dev.recv_data_sz, userbuf_sz));
	unpack_sz = userbuf_sz;
	rreq->status.count = userbuf_sz;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
						      "**truncate", "**truncate %d %d", rreq->dev.recv_data_sz, userbuf_sz);
    }

    if (unpack_sz > 0)
    {
	if (dt_contig)
	{
	    /* TODO - check that amount of data is consistent with datatype.  In other words, if we were to use Segment_unpack()
	       would last = unpack?  If not we should return an error (unless configured with --enable-fast) */
	    memcpy(rreq->dev.user_buf, rreq->dev.tmpbuf, unpack_sz);
	}
	else
	{
	    MPID_Segment seg;
	    MPIDI_msg_sz_t last;

	    MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &seg, 0);
	    last = unpack_sz;
	    MPID_Segment_unpack(&seg, 0, &last, rreq->dev.tmpbuf);
	    if (last != unpack_sz)
	    {
		/* received data was not entirely consumed by unpack() because too few bytes remained to fill the next basic
		   datatype */
		rreq->status.count = last;
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							      "**dtypemismatch", 0);
	    }
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    return mpi_errno;
}
