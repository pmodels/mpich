/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#if !defined(MPIDI_COPY_BUFFER_SZ)
#define MPIDI_COPY_BUFFER_SZ 16384
#endif

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
    MPIDI_msg_sz_t sdata_sz;
    MPIDI_msg_sz_t rdata_sz;
    MPID_Datatype * sdt_ptr;
    MPID_Datatype * rdt_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    MPIDI_CH3U_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz, sdt_ptr);
    MPIDI_CH3U_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz, rdt_ptr);

    if (sdata_sz > rdata_sz)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "message truncated, sdata_sz=" MPIDI_MSG_SZ_FMT " rdata_sz=" MPIDI_MSG_SZ_FMT,
			  sdata_sz, rdata_sz));
	sdata_sz = rdata_sz;
	*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
    }
    
    if (sdata_sz == 0)
    {
	*rsz = 0;
	goto fn_exit;
    }
    
    if (sdt_contig && rdt_contig)
    {
	memcpy(rbuf, sbuf, sdata_sz);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
	MPID_Segment seg;
	MPIDI_msg_sz_t last;

	MPID_Segment_init(rbuf, rcount, rdt, &seg, 0);
	last = sdata_sz;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-unpack last=" MPIDI_MSG_SZ_FMT, last ));
	MPID_Segment_unpack(&seg, 0, &last, sbuf);
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-unpack last=" MPIDI_MSG_SZ_FMT, last ));
	if (last != sdata_sz)
	{
	    *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
	}

	*rsz = last;
    }
    else if (rdt_contig)
    {
	MPID_Segment seg;
	MPIDI_msg_sz_t last;

	MPID_Segment_init(sbuf, scount, sdt, &seg, 0);
	last = sdata_sz;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-pack last=" MPIDI_MSG_SZ_FMT, last ));
	MPID_Segment_pack(&seg, 0, &last, rbuf);
	MPIDI_DBG_PRINTF((40, FCNAME, "post-pack last=" MPIDI_MSG_SZ_FMT, last ));
	if (last != sdata_sz)
	{
	    *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
	}

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
	if (buf == NULL)
	{
	    MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
	    *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_exit;
	}

	MPID_Segment_init(sbuf, scount, sdt, &sseg, 0);
	MPID_Segment_init(rbuf, rcount, rdt, &rseg, 0);

	sfirst = 0;
	rfirst = 0;
	buf_off = 0;
	
	for(;;)
	{
	    MPIDI_msg_sz_t last;
	    char * buf_end;

	    if (sdata_sz - sfirst > MPIDI_COPY_BUFFER_SZ - buf_off)
	    {
		last = sfirst + (MPIDI_COPY_BUFFER_SZ - buf_off);
	    }
	    else
	    {
		last = sdata_sz;
	    }
	    
	    MPIDI_DBG_PRINTF((40, FCNAME, "pre-pack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, sfirst, last ));
	    MPID_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
	    MPIDI_DBG_PRINTF((40, FCNAME, "post-pack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, sfirst, last ));
	    assert(last > sfirst);
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPIDI_DBG_PRINTF((40, FCNAME, "pre-unpack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, rfirst, last ));
	    MPID_Segment_unpack(&rseg, rfirst, &last, buf);
	    MPIDI_DBG_PRINTF((40, FCNAME, "post-unpack first=" MPIDI_MSG_SZ_FMT ", last=" MPIDI_MSG_SZ_FMT, rfirst, last ));
	    assert(last > rfirst);

	    rfirst = last;

	    if (rfirst == sdata_sz)
	    {
		/* successful completion */
		break;
	    }
	    
	    if (sfirst == sdata_sz)
	    {
		/* datatype mismatch -- remaining bytes could not be unpacked */
		*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
		break;
	    }
	    
	    buf_off = sfirst - rfirst;
	    if (buf_off > 0)
	    {
		MPIDI_DBG_PRINTF((40, FCNAME, "moved " MPIDI_MSG_SZ_FMT " bytes to the beginning of the tmp buffer", buf_off));
		memmove(buf, buf_end - buf_off, buf_off);
	    }
	}

	*rsz = rfirst;
	MPIU_Free(buf);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
}
