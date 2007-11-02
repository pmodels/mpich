/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iWrite()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iWrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iWrite(MPIDI_VC_t * vc, MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    MPID_IOV tmp_iov;
    MPID_IOV *iov = req->dev.iov;
    int n_iov = req->dev.iov_count;
    int iov_offset = req->gasnet.iov_offset;
    int i, j;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    if (n_iov == 0)
    {
	goto fn_exit;
    }
    
    /* get an iov that has no more than MPIDI_CH3_packet_len of data */
    msg_sz = 0;
    for (i = iov_offset; i < n_iov + iov_offset; ++i)
    {
	if (msg_sz + iov[i].MPID_IOV_LEN > MPIDI_CH3_packet_len)
	    break;
	msg_sz += iov[i].MPID_IOV_LEN;
    }
    if (i < n_iov + iov_offset)
    {
	tmp_iov = iov[i];
	iov[i].MPID_IOV_LEN = MPIDI_CH3_packet_len - msg_sz;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
					    MPIDI_CH3_continue_packet_handler_id,
					    &iov[iov_offset], i+1 - iov_offset);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}

	/* update iov to reflect sent data */
	req->dev.iov[i].MPID_IOV_BUF = (void *) ((char *)tmp_iov.MPID_IOV_BUF +
						 iov[i].MPID_IOV_LEN);
	req->dev.iov[i].MPID_IOV_LEN = tmp_iov.MPID_IOV_LEN -
	    iov[i].MPID_IOV_LEN;
	    
	req->gasnet.iov_offset = i;
	req->dev.iov_count = n_iov + iov_offset - i;
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
					    MPIDI_CH3_continue_packet_handler_id,
					    &iov[iov_offset], i - iov_offset);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}
	req->gasnet.iov_offset = 0;
	req->dev.iov_count = 0;
    }
 fn_exit:
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
    return mpi_errno;
}
