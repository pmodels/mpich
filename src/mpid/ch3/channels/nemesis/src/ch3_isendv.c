/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"
/*#include "mpidpre.h" */
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif

extern void *MPIDI_CH3_packet_buffer;

int MPIDI_CH3_iSendv (MPIDI_VC_t *vc, MPIR_Request *sreq, struct iovec *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    int j;
    int in_cs = FALSE;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;

    MPIR_FUNC_ENTER;

    if (vc->state == MPIDI_VC_STATE_MORIBUND) {
        sreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_ERR_SET1(sreq->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        MPIR_ERR_SET1(mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        MPID_Request_complete(sreq);
        goto fn_fail;
    }

    if (vc_ch->iSendContig)
    {
        MPIR_Assert(n_iov > 0);
        switch (n_iov)
        {
        case 1:
            mpi_errno = vc_ch->iSendContig(vc, sreq, iov[0].iov_base, iov[0].iov_len, NULL, 0);
            break;
        case 2:
            mpi_errno = vc_ch->iSendContig(vc, sreq, iov[0].iov_base, iov[0].iov_len, iov[1].iov_base, iov[1].iov_len);
            break;
        default:
            mpi_errno = MPID_nem_send_iov(vc, &sreq, iov, n_iov);
            break;
        }
        goto fn_exit;
    }

    /*MPIR_Assert(vc_ch->is_local); */
    MPIR_Assert(n_iov <= MPL_IOV_LIMIT);
    MPIR_Assert(iov[0].iov_len <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].iov_len = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].iov_base);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    in_cs = TRUE;

    if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq))
    {
	struct iovec *remaining_iov = iov;
	int remaining_n_iov = n_iov;

        MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "iSendv");
	mpi_errno = MPID_nem_mpich_sendv_header (&remaining_iov, &remaining_n_iov,
	                                         vc, &again);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
	while (!again && remaining_n_iov > 0)
	{
	    mpi_errno = MPID_nem_mpich_sendv (&remaining_iov, &remaining_n_iov, vc, &again);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);
	}

	if (again)
	{
	    if (remaining_iov == iov)
	    {
		/* header was not sent */
		sreq->dev.pending_pkt =
		    *(MPIDI_CH3_Pkt_t *) iov[0].iov_base;
		sreq->dev.iov[0].iov_base = (char *) &sreq->dev.pending_pkt;
		sreq->dev.iov[0].iov_len = iov[0].iov_len;
	    }
	    else
	    {
		sreq->dev.iov[0] = remaining_iov[0];
	    }
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "  out of cells. remaining iov:");
            MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "    %ld", (long int)sreq->dev.iov[0].iov_len);

	    for (j = 1; j < remaining_n_iov; ++j)
	    {
		sreq->dev.iov[j] = remaining_iov[j];
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "    %ld", (long int)remaining_iov[j].iov_len);
	    }
	    sreq->dev.iov_offset = 0;
	    sreq->dev.iov_count = remaining_n_iov;
            sreq->ch.noncontig = FALSE;
	    sreq->ch.vc = vc;
	    MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
	    MPIR_Assert (MPIDI_CH3I_shm_active_send == NULL);

            if (remaining_iov != iov) {
                /* headers are sent, mark current sreq as active_send req */
                MPIDI_CH3I_shm_active_send = sreq;
            }

            MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "  enqueued");
	}
	else
	{
            int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIR_Assert (MPIDI_Request_get_type (sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                mpi_errno = MPID_Request_complete (sreq);
                MPIR_ERR_CHECK(mpi_errno);
                MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
            }
            else
            {
                int complete = 0;

                mpi_errno = reqFn (vc, sreq, &complete);
                if (mpi_errno) MPIR_ERR_POP (mpi_errno);

                if (!complete)
                {
                    sreq->dev.iov_offset = 0;
                    sreq->ch.noncontig = FALSE;
                    sreq->ch.vc = vc;
                    MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
                    MPIR_Assert (MPIDI_CH3I_shm_active_send == NULL);
                    MPIDI_CH3I_shm_active_send = sreq;
                    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... reloaded and enqueued");
                }
                else
                {
                    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
                }
            }
        }
    }
    else
    {
	int i;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER, TERSE, "enqueuing");
	
	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].iov_base;
	sreq->dev.iov[0].iov_base = (char *) &sreq->dev.pending_pkt;
	sreq->dev.iov[0].iov_len = iov[0].iov_len;

	for (i = 1; i < n_iov; i++)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->dev.iov_count = n_iov;
	sreq->dev.iov_offset = 0;
        sreq->ch.noncontig = FALSE;
	sreq->ch.vc = vc;

        /* this is not the first send on the queue, enqueue it then
           check to see if we can send any now */
        MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
        mpi_errno = MPIDI_CH3I_Shm_send_progress();
        MPIR_ERR_CHECK(mpi_errno);
    }

 fn_exit:
    if (in_cs) {
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

