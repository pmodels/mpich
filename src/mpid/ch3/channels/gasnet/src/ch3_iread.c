/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iRead() -- will read the rest of the packet
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iRead
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iRead(MPIDI_VC_t * vc, MPID_Request * rreq)
{
    int i;
    int complete;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    do
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "  rreq->gasnet.iov_offset = %d\n",
			  rreq->gasnet.iov_offset));
	MPIDI_DBG_PRINTF((55, FCNAME, "  rreq->dev.iov_count = %d\n",
			  rreq->dev.iov_count));
	MPIDI_DBG_PRINTF((55, FCNAME, "  vc->gasnet.data_sz = %d\n",
			  vc->gasnet.data_sz));
    
	for (i = rreq->gasnet.iov_offset;
	     i < rreq->dev.iov_count + rreq->gasnet.iov_offset; ++i)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "  rreq->dev.iov[%d] = (%p, %d)\n", i,
		      rreq->dev.iov[i].MPID_IOV_BUF,
		      rreq->dev.iov[i].MPID_IOV_LEN));
	
	    if (rreq->dev.iov[i].MPID_IOV_LEN <= vc->gasnet.data_sz)
	    {
		memcpy (rreq->dev.iov[i].MPID_IOV_BUF, vc->gasnet.data,
			rreq->dev.iov[i].MPID_IOV_LEN);
		vc->gasnet.data = (void *)((char *)vc->gasnet.data +
		    rreq->dev.iov[i].MPID_IOV_LEN);
		vc->gasnet.data_sz -= rreq->dev.iov[i].MPID_IOV_LEN;
	    }
	    else
	    {
		memcpy (rreq->dev.iov[i].MPID_IOV_BUF, vc->gasnet.data,
			vc->gasnet.data_sz);
		rreq->dev.iov[i].MPID_IOV_BUF =
		    (void *)((char *) rreq->dev.iov[i].MPID_IOV_BUF +
			     vc->gasnet.data_sz);
		rreq->dev.iov[i].MPID_IOV_LEN -= vc->gasnet.data_sz;
		rreq->dev.iov_count -= i - rreq->gasnet.iov_offset;
		rreq->gasnet.iov_offset = i;
		/* vc->gasnet.data += vc->gasnet.data_sz; */
		vc->gasnet.data_sz = 0;
		MPIU_Assert (vc->gasnet.recv_active == NULL ||
			vc->gasnet.recv_active == rreq);
		vc->gasnet.recv_active = rreq;
		goto fn_exit;
	    }
	}

	rreq->gasnet.iov_offset = 0;
	
	MPIDI_CH3U_Handle_recv_req(vc, rreq, &complete);
    }
    while (!complete);
    
    vc->gasnet.recv_active = NULL;

fn_exit:
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
    return MPI_SUCCESS;
}
