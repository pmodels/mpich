/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* STATES:NO WARNINGS */

#undef update_request
#ifdef MPICH_DBG_OUTPUT
#define update_request(sreq, pkt, pkt_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    if (pkt_sz != sizeof(MPIDI_CH3_Pkt_t)) \
    { \
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0); \
	MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND); \
	return mpi_errno; \
    } \
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) pkt; \
    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb); \
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb; \
    sreq->dev.iov_count = 1; \
    sreq->dev.iov_offset = 0; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#else
#define update_request(sreq, pkt, pkt_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) pkt; \
    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb); \
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb; \
    sreq->dev.iov_count = 1; \
    sreq->dev.iov_offset = 0; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPID_Request * sreq, void * pkt,
		    MPIDI_msg_sz_t pkt_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

#ifdef MPICH_DBG_OUTPUT
    if (pkt_sz > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
	return mpi_errno;
    }
#endif

    /* The SHM implementation uses a fixed length header, the size of which is the maximum of all possible packet headers */
    pkt_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)pkt);
    
    if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
    {
	int nb;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	
	/* MT: need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
	   also try to write */
	
	MPIU_DBG_PRINTF(("shm_write(%d bytes)\n", pkt_sz));
	mpi_errno = MPIDI_CH3I_SHM_write(vc, pkt, pkt_sz, &nb);
	if (mpi_errno == MPI_SUCCESS)
	{	
	    MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));

	    if (nb == pkt_sz)
	    {
		int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

		reqFn = sreq->dev.OnDataAvail;
		if (!reqFn) {
		    MPIDI_CH3U_Request_complete(sreq);
		    vcch->send_active = MPIDI_CH3I_SendQ_head(vcch);
		}
		else {
		    int complete;
		    mpi_errno = reqFn( vc, sreq, &complete );
		    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		    if (!complete)
		    {
			sreq->dev.iov_offset = 0;
			MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
			vcch->send_active = sreq;
		    }
		    else
		    {
			vcch->send_active = MPIDI_CH3I_SendQ_head(vcch);
		    }
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "partial write, enqueuing at head"));
		update_request(sreq, pkt, pkt_sz, nb);
		MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
		vcch->send_active = sreq;
	    }
	}
	else
	{
	    /*vcch->state = MPIDI_CH3I_VC_STATE_FAILED;*/
	    /* TODO: Create an appropriate error message based on the return value (rc) */
	    sreq->status.MPI_ERROR = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmwrite", 0);
	    /* MT -CH3U_Request_complete() performs write barrier */
	    MPIDI_CH3U_Request_complete(sreq);
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	update_request(sreq, pkt, pkt_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
}
