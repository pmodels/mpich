/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
/*#include "mpidpre.h" */
#include "mpid_nem_impl.h"
#if defined (MPID_NEM_INLINE) && MPID_NEM_INLINE
#include "mpid_nem_inline.h"
#endif

extern void *MPIDI_CH3_packet_buffer;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv (MPIDI_VC_t *vc, MPID_Request *sreq, MPL_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int again = 0;
    int j;
    int in_cs = FALSE;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);

    if (vc->state == MPIDI_VC_STATE_MORIBUND) {
        sreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_ERR_SET1(sreq->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        MPIR_ERR_SET1(mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        MPID_Request_complete(sreq);
        goto fn_fail;
    }

    if (vc_ch->iSendContig)
    {
        MPIU_Assert(n_iov > 0);
        switch (n_iov)
        {
        case 1:
            mpi_errno = vc_ch->iSendContig(vc, sreq, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN, NULL, 0);
            break;
        case 2:
            mpi_errno = vc_ch->iSendContig(vc, sreq, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN, iov[1].MPL_IOV_BUF, iov[1].MPL_IOV_LEN);
            break;
        default:
            mpi_errno = MPID_nem_send_iov(vc, &sreq, iov, n_iov);
            break;
        }
        goto fn_exit;
    }

    /*MPIU_Assert(vc_ch->is_local); */
    MPIU_Assert(n_iov <= MPL_IOV_LIMIT);
    MPIU_Assert(iov[0].MPL_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].MPL_IOV_BUF);

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    in_cs = TRUE;

    if (MPIDI_CH3I_Sendq_empty(MPIDI_CH3I_shm_sendq))
    {
	MPL_IOV *remaining_iov = iov;
	int remaining_n_iov = n_iov;

        MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "iSendv");
	mpi_errno = MPID_nem_mpich_sendv_header (&remaining_iov, &remaining_n_iov,
	                                         sreq->dev.ext_hdr_ptr, sreq->dev.ext_hdr_sz,
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
		    *(MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF;
		sreq->dev.iov[0].MPL_IOV_BUF = (char *) &sreq->dev.pending_pkt;
		sreq->dev.iov[0].MPL_IOV_LEN = iov[0].MPL_IOV_LEN;
	    }
	    else
	    {
		sreq->dev.iov[0] = remaining_iov[0];
	    }
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "  out of cells. remaining iov:");
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "    %ld", (long int)sreq->dev.iov[0].MPL_IOV_LEN);

	    for (j = 1; j < remaining_n_iov; ++j)
	    {
		sreq->dev.iov[j] = remaining_iov[j];
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "    %ld", (long int)remaining_iov[j].MPL_IOV_LEN);
	    }
	    sreq->dev.iov_offset = 0;
	    sreq->dev.iov_count = remaining_n_iov;
            sreq->ch.noncontig = FALSE;
	    sreq->ch.vc = vc;
	    MPIDI_CH3I_Sendq_enqueue(&MPIDI_CH3I_shm_sendq, sreq);
	    MPIU_Assert (MPIDI_CH3I_shm_active_send == NULL);

            if (remaining_iov != iov) {
                /* headers are sent, mark current sreq as active_send req */
                MPIDI_CH3I_shm_active_send = sreq;
            }

            MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "  enqueued");
	}
	else
	{
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert (MPIDI_Request_get_type (sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                mpi_errno = MPID_Request_complete (sreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
                MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, ".... complete");
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
                    MPIU_Assert (MPIDI_CH3I_shm_active_send == NULL);
                    MPIDI_CH3I_shm_active_send = sreq;
                    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, ".... reloaded and enqueued");
                }
                else
                {
                    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, ".... complete");
                }
            }
        }
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));
	
	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF;
	sreq->dev.iov[0].MPL_IOV_BUF = (char *) &sreq->dev.pending_pkt;
	sreq->dev.iov[0].MPL_IOV_LEN = iov[0].MPL_IOV_LEN;

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
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    if (in_cs) {
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

