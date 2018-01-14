/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
/*#include "mpidpre.h"*/
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif


/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If
 * the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the header is copied into the
 * request, and a pointer to the request is returned.  An error
 * condition also results in a request be allocated and the errror
 * being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg (MPIDI_VC_t *vc, void *hdr, intptr_t hdr_sz, MPIR_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    int in_cs = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIR_ERR_CHKANDJUMP1(vc->state == MPIDI_VC_STATE_MORIBUND, mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);

    if (vc->ch.iStartContigMsg)
    {
        mpi_errno = vc->ch.iStartContigMsg(vc, hdr, hdr_sz, NULL, 0, sreq_ptr);
        goto fn_exit;
    }

    /*MPIR_Assert(vc->ch.is_local);*/
    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    in_cs = 1;

    if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq))
       /* MT */
    {
        MPIR_Assert(hdr_sz <= INT_MAX);
	MPL_DBG_MSG_D (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iStartMsg %d", (int) hdr_sz);
	mpi_errno = MPID_nem_mpich_send_header (hdr, (int)hdr_sz, vc, &again);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
	if (again)
	{
	    goto enqueue_it;
	}
	else
	{
	    *sreq_ptr = NULL;
	}
    }
    else
    {
	goto enqueue_it;
    }

 fn_exit:
    if (in_cs) {
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

 enqueue_it:
    {
	MPIR_Request * sreq = NULL;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER, TERSE, "enqueuing");

	/* create a request */
    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
	MPIR_Assert (sreq != NULL);
	MPIR_Object_set_ref (sreq, 2);

	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
	sreq->dev.iov[0].MPL_IOV_BUF = (char *) &sreq->dev.pending_pkt;
	sreq->dev.iov[0].MPL_IOV_LEN = hdr_sz;
	sreq->dev.iov_count = 1;
	sreq->dev.iov_offset = 0;
        sreq->ch.noncontig = FALSE;
	sreq->ch.vc = vc;
	sreq->dev.OnDataAvail = 0;

        if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq)) {
            MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
        } else {
            /* this is not the first send on the queue, enqueue it then
               check to see if we can send any now */
            MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
            /* FIXME we are sometimes called from within the progress engine, we
             * shouldn't be calling the progress engine again */
            mpi_errno = MPIDI_CH3I_Shm_send_progress();
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    
	*sreq_ptr = sreq;
	
	goto fn_exit;
    }
}


