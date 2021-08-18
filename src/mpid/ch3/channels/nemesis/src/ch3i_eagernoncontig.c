/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif


/* MPIDI_CH3I_SendNoncontig - Sends a message by packing
   directly into cells.  The caller must initialize sreq->dev.segment
   as well as msg_offset and msgsize. */
int MPIDI_CH3I_SendNoncontig( MPIDI_VC_t *vc, MPIR_Request *sreq, void *header, intptr_t hdr_sz,
                              struct iovec *hdr_iov, int n_hdr_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    intptr_t orig_msg_offset = sreq->dev.msg_offset;

    MPIR_FUNC_ENTER;

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)header);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    if (n_hdr_iov > 0) {
        /* translate segments to iovs and combine with the extended header iov. */
        mpi_errno = MPIDI_CH3_SendNoncontig_iov(vc, sreq, header, hdr_sz,
                                                hdr_iov, n_hdr_iov);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    if (!MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq)) /* MT */
    {
        /* send queue is not empty, enqueue the request then check to
           see if we can send any now */

        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER, TERSE, "enqueuing");

	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)header;
        sreq->ch.noncontig    = TRUE;
        sreq->ch.header_sz    = hdr_sz;
	sreq->ch.vc           = vc;

        MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
        mpi_errno = MPIDI_CH3I_Shm_send_progress();
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* send as many cells of data as you can */
    MPID_nem_mpich_send_seg_header(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype,
                                   &sreq->dev.msg_offset, sreq->dev.msgsize,
                                   header, hdr_sz, vc, &again);
    while(!again && sreq->dev.msg_offset < sreq->dev.msgsize)
        MPID_nem_mpich_send_seg(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype,
                                &sreq->dev.msg_offset, sreq->dev.msgsize, vc, &again);

    if (again)
    {
        /* we didn't finish sending everything */
        sreq->ch.noncontig = TRUE;
        sreq->ch.vc = vc;
        if (sreq->dev.msg_offset == orig_msg_offset) /* nothing was sent, save header */
        {
            sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *)header;
            sreq->ch.header_sz    = hdr_sz;
        }
        else
        {
            /* part of message was sent, make this req an active send */
            MPIR_Assert(MPIDI_CH3I_shm_active_send == NULL);
            MPIDI_CH3I_shm_active_send = sreq;
        }
        MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
        goto fn_exit;
    }

    /* finished sending all data, complete the request */
    if (!sreq->dev.OnDataAvail)
    {
        MPIR_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
        mpi_errno = MPID_Request_complete(sreq);
        MPIR_ERR_CHECK(mpi_errno);
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete %d bytes", (int) (sreq->dev.msgsize));
    }
    else
    {
        int complete = 0;
        mpi_errno = sreq->dev.OnDataAvail(vc, sreq, &complete);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(complete); /* all data has been sent, we should always complete */

        MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete %d bytes", (int) (sreq->dev.msgsize));
    }

 fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
