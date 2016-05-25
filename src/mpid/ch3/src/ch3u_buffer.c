/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDI_CH3U_Buffer_copy(
    const void * const sbuf, MPI_Aint scount, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, MPI_Aint rcount, MPI_Datatype rdt, intptr_t * rsz,
    int * rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPI_Aint sdt_true_lb, rdt_true_lb;
    intptr_t sdata_sz;
    intptr_t rdata_sz;
    MPIR_Datatype* sdt_ptr;
    MPIR_Datatype* rdt_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MEMCPY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    MPIDI_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz, sdt_ptr, sdt_true_lb);
    MPIDI_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz, rdt_ptr, rdt_true_lb);

    /* --BEGIN ERROR HANDLING-- */
    if (sdata_sz > rdata_sz)
    {
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TYPICAL,(MPL_DBG_FDEST,
	    "message truncated, sdata_sz=%" PRIdPTR " rdata_sz=%" PRIdPTR,
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
	MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MEMCPY);
	MPIR_Memcpy((char *)rbuf + rdt_true_lb, (const char *)sbuf + sdt_true_lb, sdata_sz);
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MEMCPY);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
	MPIR_Segment seg;
	MPI_Aint last;

	MPIR_Segment_init(rbuf, rcount, rdt, &seg, 0);
	last = sdata_sz;
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
                          "pre-unpack last=%" PRIdPTR, last ));
	MPIR_Segment_unpack(&seg, 0, &last, (char*)sbuf + sdt_true_lb);
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
			 "pre-unpack last=%" PRIdPTR, last ));
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
	MPIR_Segment seg;
	MPI_Aint last;

	MPIR_Segment_init(sbuf, scount, sdt, &seg, 0);
	last = sdata_sz;
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
			       "pre-pack last=%" PRIdPTR, last ));
	MPIR_Segment_pack(&seg, 0, &last, (char*)rbuf + rdt_true_lb);
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
			    "post-pack last=%" PRIdPTR, last ));
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
	intptr_t buf_off;
	MPIR_Segment sseg;
	intptr_t sfirst;
	MPIR_Segment rseg;
	intptr_t rfirst;

	buf = MPL_malloc(MPIDI_COPY_BUFFER_SZ);
	/* --BEGIN ERROR HANDLING-- */
	if (buf == NULL)
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"SRBuf allocation failure");
	    *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

	MPIR_Segment_init(sbuf, scount, sdt, &sseg, 0);
	MPIR_Segment_init(rbuf, rcount, rdt, &rseg, 0);

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
	    
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
               "pre-pack first=%" PRIdPTR ", last=%" PRIdPTR,
						sfirst, last ));
	    MPIR_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
               "post-pack first=%" PRIdPTR ", last=%" PRIdPTR,
               sfirst, last ));
	    /* --BEGIN ERROR HANDLING-- */
	    MPIR_Assert(last > sfirst);
	    /* --END ERROR HANDLING-- */
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
             "pre-unpack first=%" PRIdPTR ", last=%" PRIdPTR,
						rfirst, last ));
	    MPIR_Segment_unpack(&rseg, rfirst, &last, buf);
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
             "post-unpack first=%" PRIdPTR ", last=%" PRIdPTR,
						rfirst, last ));
	    /* --BEGIN ERROR HANDLING-- */
	    MPIR_Assert(last > rfirst);
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
		MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, VERBOSE, (MPL_DBG_FDEST,
                  "moved %" PRIdPTR " bytes to the beginning of the tmp buffer", buf_off));
		memmove(buf, buf_end - buf_off, buf_off);
	    }
	}

	*rsz = rfirst;
	MPL_free(buf);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
}


/*
 * This routine is called by mpid_recv and mpid_irecv when a request
 * matches a send-to-self message 
 */
int MPIDI_CH3_RecvFromSelf( MPIR_Request *rreq, void *buf, MPI_Aint count,
			    MPI_Datatype datatype )
{
    MPIR_Request * const sreq = rreq->dev.partner_request;
    int mpi_errno = MPI_SUCCESS;

    if (sreq != NULL)
    {
	intptr_t data_sz;
	
	MPIDI_CH3U_Buffer_copy(sreq->dev.user_buf, sreq->dev.user_count,
			       sreq->dev.datatype, &sreq->status.MPI_ERROR,
			       buf, count, datatype, &data_sz, 
			       &rreq->status.MPI_ERROR);
	MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
	mpi_errno = MPID_Request_complete(sreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else
    {
	/* The sreq is missing which means an error occurred.  
	   rreq->status.MPI_ERROR should have been set when the
	   error was detected. */
    }
    
    /* no other thread can possibly be waiting on rreq, so it is safe to 
       reset ref_count and cc */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
