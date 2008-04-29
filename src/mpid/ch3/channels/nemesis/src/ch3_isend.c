/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* #include "mpidpre.h" */
#include "mpidi_ch3_impl.h"
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend (MPIDI_VC_t *vc, MPID_Request *sreq, void * hdr, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    if (((MPIDI_CH3I_VC *)vc->channel_private)->iSendContig)
    {
        mpi_errno = ((MPIDI_CH3I_VC *)vc->channel_private)->iSendContig(vc, sreq, hdr, hdr_sz, NULL, 0);
        goto fn_exit;
    }

    /* MPIU_Assert(((MPIDI_CH3I_VC *)vc->channel_private)->is_local); */
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which
     * is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
        /* MT */
    {
	MPIU_DBG_MSG_D (CH3_CHANNEL, VERBOSE, "iSend %d", (int) hdr_sz);
	mpi_errno = MPID_nem_mpich2_send_header (hdr, hdr_sz, vc, &again);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	if (again)
	{
	    goto enqueue_it;
	}
	else
	{
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert (MPIDI_Request_get_type (sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete (sreq);
            }
            else
            {
                int complete = 0;
                mpi_errno = reqFn (vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            }
	}
    }
    else
    {
	goto enqueue_it;
    }


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

 enqueue_it:
    MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
    sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
    sreq->dev.iov_count = 1;
    sreq->dev.iov_offset = 0;
    sreq->ch.noncontig = FALSE;
    sreq->ch.vc = vc;
    MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);

    goto fn_exit;
}

