/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

    MPIR_FUNC_ENTER;
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
	*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
    }
    /* --END ERROR HANDLING-- */
    
    if (sdata_sz == 0)
    {
	*rsz = 0;
	goto fn_exit;
    }
    
    if (sdt_contig && rdt_contig)
    {
	MPIR_Memcpy(MPIR_get_contig_ptr(rbuf, rdt_true_lb), MPIR_get_contig_ptr(sbuf, sdt_true_lb), sdata_sz);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(MPIR_get_contig_ptr(sbuf, sdt_true_lb), sdata_sz, rbuf, rcount, rdt, 0,
                            &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        /* --BEGIN ERROR HANDLING-- */
        if (actual_unpack_bytes != sdata_sz)
        {
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
        /* --END ERROR HANDLING-- */
        *rsz = actual_unpack_bytes;
    }
    else if (rdt_contig)
    {
	MPI_Aint actual_pack_bytes;
    MPIR_Typerep_pack(sbuf, scount, sdt, 0, MPIR_get_contig_ptr(rbuf, rdt_true_lb), sdata_sz,
                      &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
	/* --BEGIN ERROR HANDLING-- */
	if (actual_pack_bytes != sdata_sz)
	{
	    *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
	}
	/* --END ERROR HANDLING-- */
	*rsz = actual_pack_bytes;
    }
    else
    {
	char * buf;
	intptr_t sfirst;
	intptr_t rfirst;

	buf = MPL_malloc(MPIDI_COPY_BUFFER_SZ, MPL_MEM_BUFFER);
	/* --BEGIN ERROR HANDLING-- */
	if (buf == NULL)
	{
	    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"SRBuf allocation failure");
	    *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

	sfirst = 0;
	rfirst = 0;
	
	for(;;)
	{
	    MPI_Aint max_pack_bytes;
	    MPI_Aint actual_pack_bytes;
	    MPI_Aint actual_unpack_bytes;

	    /* rdata_sz is allowed to be larger than sdata_sz, so if
	     * we copied everything from the source buffer to the
	     * receive buffer, we are done */

	    if (sdata_sz - sfirst > MPIDI_COPY_BUFFER_SZ) {
		max_pack_bytes = MPIDI_COPY_BUFFER_SZ;
	    } else {
		max_pack_bytes = sdata_sz - sfirst;
	    }

	    /* nothing left to copy, break out */
	    if (max_pack_bytes == 0)
		break;

        MPIR_Typerep_pack(sbuf, scount, sdt, sfirst, buf, max_pack_bytes, &actual_pack_bytes,
                          MPIR_TYPEREP_FLAG_NONE);
        MPIR_Typerep_unpack(buf, actual_pack_bytes, rbuf, rcount, rdt, rfirst, &actual_unpack_bytes,
                            MPIR_TYPEREP_FLAG_NONE);
	    MPIR_Assert(actual_pack_bytes == actual_unpack_bytes);

	    sfirst += actual_pack_bytes;
	    rfirst += actual_unpack_bytes;

	    /* --BEGIN ERROR HANDLING-- */
	    if (rfirst == sdata_sz && sfirst != sdata_sz) {
		/* datatype mismatch -- remaining bytes could not be unpacked */
		*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
		break;
	    }
	    /* --END ERROR HANDLING-- */
	}

	*rsz = rfirst;
	MPL_free(buf);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
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
        MPIR_ERR_CHECK(mpi_errno);
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
    MPIR_ERR_CHECK(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
