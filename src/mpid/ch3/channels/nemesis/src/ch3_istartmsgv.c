/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"
/*#include "mpidpre.h"*/
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.
 * If the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the iovec and the first buffer
 * pointed to by the iovec (which is assumed to be a MPIDI_CH3_Pkt_t)
 * are copied into the request, and a pointer to the request is
 * returned.  An error condition also results in a request be
 * allocated and the errror being returned in the status field of the
 * request.
 */

/* NOTE - The completion action associated with a request created by CH3_iStartMsgv() is alway null (onDataAvail = 0).  This
   implies that CH3_iStartMsgv() can only be used when the entire message can be described by a single iovec of size
   MPL_IOV_LIMIT. */

int MPIDI_CH3_iStartMsgv (MPIDI_VC_t *vc, struct iovec *iov, int n_iov, MPIR_Request **sreq_ptr)
{
    MPIR_Request * sreq = *sreq_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int in_cs = FALSE;
    int again = 0;
    int j;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIR_ERR_CHKANDJUMP1(vc->state == MPIDI_VC_STATE_MORIBUND, mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);

    if (vc->ch.iStartContigMsg)
    {
        MPIR_Assert (n_iov > 0);
        switch (n_iov)
        {
        case 1:
            mpi_errno = vc->ch.iStartContigMsg(vc, iov[0].iov_base, iov[0].iov_len, NULL, 0, sreq_ptr);
            break;
        case 2:
            mpi_errno = vc->ch.iStartContigMsg(vc, iov[0].iov_base, iov[0].iov_len,
                                               iov[1].iov_base, iov[1].iov_len, sreq_ptr);
            break;
        default:
            mpi_errno = MPID_nem_send_iov(vc, &sreq, iov, n_iov);
            *sreq_ptr = sreq;
            break;
        }
        goto fn_exit;
    }

    MPIR_Assert (n_iov <= MPL_IOV_LIMIT);
    MPIR_Assert (iov[0].iov_len <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].iov_len = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].iov_base);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    in_cs = TRUE;

    if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq))
        /* MT */
    {
	struct iovec *remaining_iov = iov;
	int remaining_n_iov = n_iov;

        MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iStartMsgv");
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
                int total = 0;
                int i;
                for (i = 0; i < n_iov; ++i)
                    total += iov[i].iov_len;

                MPL_DBG_MSG_D (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "   + len=%d ", total);
            });
	mpi_errno = MPID_nem_mpich_sendv_header (&remaining_iov, &remaining_n_iov, vc, &again);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
	while (!again && (remaining_n_iov > 0))
	{
            MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
                    int total = 0;
                    int i;
                    for (i = 0; i < remaining_n_iov; ++i)
                        total += remaining_iov[i].iov_len;
                    MPL_DBG_MSG_D (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "   + len=%d ", total);
                });

	    mpi_errno = MPID_nem_mpich_sendv (&remaining_iov, &remaining_n_iov, vc, &again);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);
	}
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
                int total = 0;
                int i;
                for (i = 0; i < remaining_n_iov; ++i)
                    total += remaining_iov[i].iov_len;
                MPL_DBG_MSG_D (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "   - len=%d ", total);
            });

	if (again)
	{
            /* Create a new request and save remaining portions of the
	     * iov in it. */
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED, 0);
	    MPIR_Assert(sreq != NULL);
	    MPIR_Object_set_ref(sreq, 2);
	    sreq->kind = MPIR_REQUEST_KIND__SEND;
	    for (j = 0; j < remaining_n_iov; ++j)
	    {
		sreq->dev.iov[j] = remaining_iov[j];
	    }
	    sreq->dev.iov_offset = 0;
	    sreq->dev.iov_count = remaining_n_iov;
	    sreq->dev.OnDataAvail = 0;
            sreq->ch.noncontig = FALSE;
	    sreq->ch.vc = vc;
	    if ( iov == remaining_iov )
	    {
		/* header was not sent, so iov[0] might point to something on the stack */
		sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].iov_base;
		sreq->dev.iov[0].iov_base = (char *) &sreq->dev.pending_pkt;
		sreq->dev.iov[0].iov_len = iov[0].iov_len;
	    }
	    MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
	    MPIR_Assert (MPIDI_CH3I_shm_active_send == NULL);
	    MPIDI_CH3I_shm_active_send = sreq;
	}
    }
    else
    {
	int i;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER, TERSE, "request enqueued");
	/* create a request */
	sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED, 0);
	MPIR_Assert(sreq != NULL);
	MPIR_Object_set_ref(sreq, 2);
	sreq->kind = MPIR_REQUEST_KIND__SEND;

	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].iov_base;
	sreq->dev.iov[0].iov_base = (char *) &sreq->dev.pending_pkt;
	sreq->dev.iov[0].iov_len = iov[0].iov_len;

	/* copy iov */
	for (i = 1; i < n_iov; ++i)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->dev.iov_offset = 0;
	sreq->dev.iov_count = n_iov;
	sreq->dev.OnDataAvail = 0;
        sreq->ch.noncontig = FALSE;
	sreq->ch.vc = vc;

        if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq)) {  
            MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
        } else {
            /* this is not the first send on the queue, enqueue it then
               check to see if we can send any now */
            MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
            mpi_errno = MPIDI_CH3I_Shm_send_progress();
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    *sreq_ptr = sreq;

 fn_exit:
    if (in_cs) {
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

