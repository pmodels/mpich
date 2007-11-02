/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* STATES:NO WARNINGS */

/*static void update_request(MPID_Request * sreq, MPID_IOV * iov, int count, int offset, int nb)*/
#undef update_request
#ifdef MPICH_DBG_OUTPUT
#define update_request(sreq, iov, count, offset, nb) \
{ \
    int i; \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    for (i = 0; i < count; i++) \
    { \
	sreq->dev.iov[i] = iov[i]; \
    } \
    if (offset == 0) \
    { \
	if (iov[0].MPID_IOV_LEN != sizeof(MPIDI_CH3_Pkt_t)) \
	{ \
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0); \
	    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV); \
	    return mpi_errno; \
	} \
	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) iov[0].MPID_IOV_BUF; \
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.pending_pkt; \
    } \
    sreq->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[offset].MPID_IOV_BUF + nb); \
    sreq->dev.iov[offset].MPID_IOV_LEN -= nb; \
    sreq->dev.iov_count = count; \
    sreq->dev.iov_offset = offset; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#else
#define update_request(sreq, iov, count, offset, nb) \
{ \
    int i; \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    for (i = 0; i < count; i++) \
    { \
	sreq->dev.iov[i] = iov[i]; \
    } \
    if (offset == 0) \
    { \
	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) iov[0].MPID_IOV_BUF; \
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.pending_pkt; \
    } \
    sreq->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[offset].MPID_IOV_BUF + nb); \
    sreq->dev.iov[offset].MPID_IOV_LEN -= nb; \
    sreq->dev.iov_count = count; \
    sreq->dev.iov_offset = offset; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov, 
		     int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);

#ifdef MPICH_DBG_OUTPUT
    if (n_iov > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
	return mpi_errno;
    }
    if (iov[0].MPID_IOV_LEN > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
	return mpi_errno;
    }
#endif
    /* The SHM implementation uses a fixed length header, the size of which is 
       the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);
    
    /* Connection already formed.  If send queue is empty attempt to send data,
       queuing any unsent data. */
    if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
    {
	int nb;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	
	/* MT - need some signalling to lock down our right to use the channel,
	   thus insuring that the progress engine does
	   also try to write */
	
	/* FIXME: the current code only agressively writes the first IOV.  
	   Eventually it should be changed to agressively write
	   as much as possible.  Ideally, the code would be shared between the
	   send routines and the progress engine. */
	
	mpi_errno = (n_iov > 1) ?
	    MPIDI_CH3I_SHM_writev(vc, iov, n_iov, &nb) :
	    MPIDI_CH3I_SHM_write(vc, iov->MPID_IOV_BUF, iov->MPID_IOV_LEN, &nb);
	if (mpi_errno == MPI_SUCCESS)
	{
	    int offset = 0;
	    
	    MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));
	    
	    while (offset < n_iov)
	    {
		if ((int)iov[offset].MPID_IOV_LEN <= nb)
		{
		    nb -= iov[offset].MPID_IOV_LEN;
		    offset++;
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "partial write, enqueuing at head"));
		    update_request(sreq, iov, n_iov, offset, nb);
		    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
		    vcch->send_active = sreq;
		    break;
		}
	    }
	    if (offset == n_iov)
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
	}
	else
	{
	    /* Connection just failed.  Mark the request complete and return 
	       an error. */
	    /*vcch->state = MPIDI_CH3I_VC_STATE_FAILED;*/
	    /* TODO: Create an appropriate error message based on the value of 
	       errno */
	    sreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmwrite", 0);
	    /* MT - CH3U_Request_complete performs write barrier */
	    MPIDI_CH3U_Request_complete(sreq);
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	update_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
}

