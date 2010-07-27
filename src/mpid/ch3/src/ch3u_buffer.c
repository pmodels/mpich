/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: Explain the contents of this file */
/*
 * A first guess.  This file contains routines to manage memory-to-memory
 * copies of buffers described by MPI datatypes
 */

/* MPIDI_COPY_BUFFER_SZ is the size of the buffer that is allocated when 
   copying from one non-contiguous buffer to another.  In this case, an 
   intermediate contiguous buffer is used of this size */
#if !defined(MPIDI_COPY_BUFFER_SZ)
#define MPIDI_COPY_BUFFER_SZ 16384
#endif

/* FIXME: Explain this routine .
Used indirectly by mpid_irecv, mpid_recv (through MPIDI_CH3_RecvFromSelf) and 
 in mpidi_isend_self.c */

/* This routine appears to handle copying data from one buffer (described by
 the usual MPI triple (buf,count,datatype) to another, as is needed in send 
 and receive from self.  We may want to put all of the "from self" routines
 into a single file, and make MPIDI_CH3U_Buffer_copy static to this file. */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Buffer_copy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Buffer_copy(
    const void * const sbuf, int scount, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rsz,
    int * rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPI_Aint sdt_true_lb, rdt_true_lb;
    MPIDI_msg_sz_t sdata_sz;
    MPIDI_msg_sz_t rdata_sz;
    MPID_Datatype * sdt_ptr;
    MPID_Datatype * rdt_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    MPIDI_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz, sdt_ptr, sdt_true_lb);
    MPIDI_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz, rdt_ptr, rdt_true_lb);

    /* --BEGIN ERROR HANDLING-- */
    if (sdata_sz > rdata_sz)
    {
	MPIU_DBG_MSG_FMT(CH3_OTHER,TYPICAL,(MPIU_DBG_FDEST,
	    "message truncated, sdata_sz=" MPIDI_MSG_SZ_FMT " rdata_sz=" MPIDI_MSG_SZ_FMT,
			  sdata_sz, rdata_sz));
	sdata_sz = rdata_sz;
	*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
    }
    /* --END ERROR HANDLING-- */
    
    if (sdata_sz == 0)
    {
	*rsz = 0;
	goto fn_exit;
    }
    
    if (sdt_contig && rdt_contig)
    {
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	MPIU_Memcpy((char *)rbuf + rdt_true_lb, (const char *)sbuf + sdt_true_lb, sdata_sz);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
	MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(rbuf, rcount, rdt, &seg, 0);
	last = sdata_sz;
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST, 
                          "pre-unpack last=" MPIDI_MSG_SZ_FMT, last ));
	MPID_Segment_unpack(&seg, 0, &last, (char*)sbuf + sdt_true_lb);
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
			 "pre-unpack last=" MPIDI_MSG_SZ_FMT, last ));
	/* --BEGIN ERROR HANDLING-- */
	if (last != sdata_sz)
	{
	    *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
	}
	/* --END ERROR HANDLING-- */

	*rsz = last;
    }
    else if (rdt_contig)
    {
	MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(sbuf, scount, sdt, &seg, 0);
	last = sdata_sz;
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
			       "pre-pack last=" MPIDI_MSG_SZ_FMT, last ));
	MPID_Segment_pack(&seg, 0, &last, (char*)rbuf + rdt_true_lb);
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
			    "post-pack last=" MPIDI_MSG_SZ_FMT, last ));
	/* --BEGIN ERROR HANDLING-- */
	if (last != sdata_sz)
	{
	    *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
	}
	/* --END ERROR HANDLING-- */

	*rsz = last;
    }
    else
    {
	char * buf;
	MPIDI_msg_sz_t buf_off;
	MPID_Segment sseg;
	MPIDI_msg_sz_t sfirst;
	MPID_Segment rseg;
	MPIDI_msg_sz_t rfirst;

	buf = MPIU_Malloc(MPIDI_COPY_BUFFER_SZ);
	/* --BEGIN ERROR HANDLING-- */
	if (buf == NULL)
	{
	    MPIU_DBG_MSG(CH3_OTHER,TYPICAL,"SRBuf allocation failure");
	    *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

	MPID_Segment_init(sbuf, scount, sdt, &sseg, 0);
	MPID_Segment_init(rbuf, rcount, rdt, &rseg, 0);

	sfirst = 0;
	rfirst = 0;
	buf_off = 0;
	
	for(;;)
	{
	    MPI_Aint last;
	    char * buf_end;

	    if (sdata_sz - sfirst > MPIDI_COPY_BUFFER_SZ - buf_off)
	    {
		last = sfirst + (MPIDI_COPY_BUFFER_SZ - buf_off);
	    }
	    else
	    {
		last = sdata_sz;
	    }
	    
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
               "pre-pack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, 
						sfirst, last ));
	    MPID_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
               "post-pack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, 
               sfirst, last ));
	    /* --BEGIN ERROR HANDLING-- */
	    MPIU_Assert(last > sfirst);
	    /* --END ERROR HANDLING-- */
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
             "pre-unpack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, 
						rfirst, last ));
	    MPID_Segment_unpack(&rseg, rfirst, &last, buf);
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
             "post-unpack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, 
						rfirst, last ));
	    /* --BEGIN ERROR HANDLING-- */
	    MPIU_Assert(last > rfirst);
	    /* --END ERROR HANDLING-- */

	    rfirst = last;

	    if (rfirst == sdata_sz)
	    {
		/* successful completion */
		break;
	    }

	    /* --BEGIN ERROR HANDLING-- */
	    if (sfirst == sdata_sz)
	    {
		/* datatype mismatch -- remaining bytes could not be unpacked */
		*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
		break;
	    }
	    /* --END ERROR HANDLING-- */

	    buf_off = sfirst - rfirst;
	    if (buf_off > 0)
	    {
		MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE, (MPIU_DBG_FDEST,
                  "moved " MPIDI_MSG_SZ_FMT " bytes to the beginning of the tmp buffer", buf_off));
		memmove(buf, buf_end - buf_off, buf_off);
	    }
	}

	*rsz = rfirst;
	MPIU_Free(buf);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
}


/*
 * This routine is called by mpid_recv and mpid_irecv when a request
 * matches a send-to-self message 
 */
int MPIDI_CH3_RecvFromSelf( MPID_Request *rreq, void *buf, int count, 
			    MPI_Datatype datatype )
{
    MPID_Request * const sreq = rreq->partner_request;

    if (sreq != NULL)
    {
	MPIDI_msg_sz_t data_sz;
	
	MPIDI_CH3U_Buffer_copy(sreq->dev.user_buf, sreq->dev.user_count,
			       sreq->dev.datatype, &sreq->status.MPI_ERROR,
			       buf, count, datatype, &data_sz, 
			       &rreq->status.MPI_ERROR);
	rreq->status.count = (int)data_sz;
	MPID_REQUEST_SET_COMPLETED(sreq);
	MPID_Request_release(sreq);
    }
    else
    {
	/* The sreq is missing which means an error occurred.  
	   rreq->status.MPI_ERROR should have been set when the
	   error was detected. */
    }
    
    /* no other thread can possibly be waiting on rreq, so it is safe to 
       reset ref_count and cc */
    /* FIXME DJG really? shouldn't we at least decr+assert? */
    MPID_cc_set(rreq->cc_ptr, 0);
    MPIU_Object_set_ref(rreq, 1);

    return MPI_SUCCESS;
}
