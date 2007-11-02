/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the 
 * iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->dev.iov_offset;
    const int count = req->dev.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    
    while (offset < count)
    {
	if (req->dev.iov[offset].MPID_IOV_LEN <= (unsigned int)nb)
	{
	    nb -= req->dev.iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    req->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) req->dev.iov[offset].MPID_IOV_BUF + nb);
	    req->dev.iov[offset].MPID_IOV_LEN -= nb;
	    req->dev.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->dev.iov_offset = 0;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write_progress(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int nb;
    int total = 0;
    int complete;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);
    
    while (vcch->send_active != NULL)
    {
	MPID_Request * req = vcch->send_active;

#ifdef MPICH_DBG_OUTPUT
	if (req->dev.iov_offset >= req->dev.iov_count)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->dev.iov_offset, req->dev.iov_count);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);
	    return mpi_errno;
	}
#endif
	/* Check here or inside shm_writev?
	if (vc->ch.write_shmq->packet[vc->ch.write_shmq->tail_index].avail == MPIDI_CH3I_PKT_EMPTY)
	    mpi_errno = MPIDI_CH3I_SHM_writev(vc, req->dev.iov + req->dev.iov_offset, req->dev.iov_count - req->dev.iov_offset, &nb);
	else
	    nb = 0;
	*/
	mpi_errno = MPIDI_CH3I_SHM_writev(vc, req->dev.iov + req->dev.iov_offset, req->dev.iov_count - req->dev.iov_offset, &nb);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);
	    return mpi_errno;
	}

	if (nb > 0)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "shm_writev returned %d", nb));
	    total += nb;
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
#if 1
		{ 
		    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
		    reqFn = req->dev.OnDataAvail;
		    if (!reqFn) {
			MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);
			MPIDI_CH3U_Request_complete(req);
			complete = TRUE;
		    }
		    else {
			mpi_errno = reqFn( vc, req, &complete );
			if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		    }
		}
#else
		mpi_errno = MPIDI_CH3U_Handle_send_req(vc, req, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    goto fn_fail;
		}
#endif
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(vcch);
		}
		vcch->send_active = MPIDI_CH3I_SendQ_head(vcch);
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "iovec updated by %d bytes but not complete", nb));
		MPIU_Assert(req->dev.iov_offset < req->dev.iov_count);
		break;
	    }
	}
	else
	{
#ifdef MPICH_DBG_OUTPUT
	    if (nb != 0)
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "shm_writev returned %d bytes", nb));
	    }
#endif
	    break;
	}
    }


 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);
    return total > 0 ? MPI_SUCCESS : -1 /* SHM_WAIT_TIMEOUT */;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Handle_shm_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Handle_shm_read(MPIDI_VC_t *vc, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * req;
    int complete;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_HANDLE_SHM_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_HANDLE_SHM_READ);

    req = vcch->recv_active;
    if (req == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_HANDLE_SHM_READ);
	return MPI_SUCCESS;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
#if 1
	    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
	    reqFn = req->dev.OnDataAvail;
	    if (!reqFn) {
		MPIDI_CH3U_Request_complete(req);
		complete = TRUE;
	    }
	    else {
		mpi_errno = reqFn( vc, req, &complete );
		if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    }
#else
	    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    }
#endif
	    if (complete)
	    {
		vcch->recv_active = NULL;
		vcch->shm_reading_pkt = TRUE;
	    }
	    else
	    {
		vcch->recv_active = req;
		mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, req->dev.iov, req->dev.iov_count, NULL);
	    }
	}
	else
	{
#ifdef MPICH_DBG_OUTPUT
	    if (req->dev.iov_offset >= req->dev.iov_count)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->dev.iov_offset, req->dev.iov_count);
		goto fn_fail;
	    }
#endif
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_HANDLE_SHM_READ);
	    return MPI_SUCCESS;
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->dev.iov + req->dev.iov_offset, req->dev.iov_count - req->dev.iov_offset));
    }

 fn_fail:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_HANDLE_SHM_READ);
    return mpi_errno;
}
