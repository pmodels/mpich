/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    /* There is no post_close for shm connections so handle them as closed immediately. */
    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    return mpi_errno;
}

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
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

/* Note the space in MPIDI_S TATE_DECL to keep the state finder from reading
   this commented-out definition */
/*
#define post_pkt_recv(vc) \
{ \
    MPIDI_S TATE_DECL(MPID_STATE_POST_PKT_RECV); \
    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_RECV); \
    vc->ch.req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&vc->ch.req->ch.pkt; \
    vc->ch.req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t); \
    vc->ch.req->dev.iov_count = 1; \
    vc->ch.req->dev.iov_offset = 0; \
    vc->ch.req->dev.OnDataAvail = MPIDI_CH3_SHM_ReqHandler_PktComplete;
    vc->ch.recv_active = vc->ch.req; \
    MPIDI_CH3I_SHM_post_read(vc, &vc->ch.req->ch.pkt, sizeof(MPIDI_CH3_Pkt_t), NULL); \
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_RECV); \
}
*/
/* Because packets are interpreted in-place in the shm queue, posting a pkt read only requires setting a flag */
#define post_pkt_recv(vcch) vcch->shm_reading_pkt = TRUE

#undef FUNCNAME
#define FUNCNAME handle_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_read(MPIDI_VC_t *vc, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * req;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_READ);
    
    req = vcch->recv_active;
    if (req == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */

	    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
	    reqFn = req->dev.OnDataAvail;
	    if (!reqFn) {
		MPIDI_CH3U_Request_complete(req);
		post_pkt_recv(vcch);
	    }
	    else {
		int complete;
		mpi_errno = reqFn( vc, req, &complete );
		if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		if (complete) {
		    post_pkt_recv(vcch);
		}
		else {
		    vcch->recv_active = req;
		    mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, req->dev.iov, req->dev.iov_count, NULL);
		}
	    }
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->dev.iov, req->dev.iov_count));
    }

 fn_fail:    

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME handle_written
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_written(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int nb;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_WRITTEN);
    
    while (vcch->send_active != NULL)
    {
	MPID_Request * req = vcch->send_active;

	mpi_errno = MPIDI_CH3I_SHM_writev(
	    vc, req->dev.iov + req->dev.iov_offset, 
	    req->dev.iov_count - req->dev.iov_offset, &nb);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,
				     "**handle_written");
	}
	MPIDI_DBG_PRINTF((60, FCNAME, "shm_writev returned %d", nb));

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
		/* Write operation complete */

		reqFn = req->dev.OnDataAvail;
		if (!reqFn) {
		    MPIDI_CH3U_Request_complete(req);
		    MPIDI_CH3I_SendQ_dequeue(vcch);
		}
		else {
		    int complete;
		    mpi_errno = reqFn( vc, req, &complete );
		    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
		    if (complete) {
			MPIDI_CH3I_SendQ_dequeue(vcch);
		    }
		}
		vcch->send_active = MPIDI_CH3I_SendQ_head(vcch);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "shm_post_writev returned %d bytes", nb));
	    break;
	}
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking, MPID_Progress_state *state)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_VC_t *vc_ptr;
    MPIDI_CH3I_PG *pgch;
    int num_bytes;
    shm_wait_t wait_result;
#ifdef MPICH_DBG_OUTPUT
    unsigned register count;
#endif
    int spin_count = 1;
    unsigned completions = MPIDI_CH3I_progress_completion_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_YIELD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	mpi_errno = MPIDI_CH3I_SHM_read_progress(MPIDI_CH3I_Process.vc, 0, &vc_ptr, &num_bytes, &wait_result);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_read_progress", 0);
	    goto fn_exit;
	}
	switch (wait_result)
	{
	case SHM_WAIT_TIMEOUT:
	    pgch = (MPIDI_CH3I_PG *)MPIDI_Process.my_pg->channel_private;
	    if (spin_count >= pgch->nShmWaitSpinCount)
	    {
		MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_YIELD);
		MPIDU_Yield();
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_YIELD);
		spin_count = 0;
	    }
	    spin_count++;
	    break;
	case SHM_WAIT_READ:
	    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes read", num_bytes));
	    spin_count = 1;
	    mpi_errno = handle_read(vc_ptr, num_bytes);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
		/*
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		return mpi_errno;
		*/
	    }
	    break;
	case SHM_WAIT_WRITE:
	    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes written", num_bytes));
	    spin_count = 1;
	    mpi_errno = handle_written(vc_ptr);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
		/*
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		return mpi_errno;
		*/
	    }
	    break;
	case SHM_WAIT_WAKEUP:
	    break;
	default:
	    /*MPIDI_err_printf(FCNAME, "MPIDI_CH3I_SHM_read_progress returned an unknown operation code\n");*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_op", "**shm_op %d", wait_result);
	    goto fn_exit;
	    /*
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
	    return mpi_errno;
	    */
	    /*
	    MPID_Abort(MPIR_Process.comm_world, MPI_SUCCESS, 13, NULL);
	    break;
	    */
	}

	/* pound on the write queues since shm_read_progress currently does not return SHM_WAIT_WRITE */
	for (i=0; i<MPIDI_PG_Get_size(MPIDI_CH3I_Process.vc->pg); i++)
	{
	    MPIDI_CH3I_VC *vcch;
	    MPIDI_PG_Get_vc(MPIDI_CH3I_Process.vc->pg, i, &vc_ptr);
	    vcch = (MPIDI_CH3I_VC *)vc_ptr->channel_private;
	    if (/*MPIDI_Process.my_pg->ch.vc_table[i].*/vcch->send_active != NULL)
	    {
		mpi_errno = handle_written(vc_ptr/*&MPIDI_Process.my_pg->ch.vc_table[i]*/);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		    return mpi_errno;
		}
	    }
	}
    }
    while (completions == MPIDI_CH3I_progress_completion_count && is_blocking);

fn_exit:
#ifdef MPICH_DBG_OUTPUT
    count = MPIDI_CH3I_progress_completion_count - completions;
    if (is_blocking)
    {
	MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", count));
    }
    else
    {
	if (count > 0)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting (non-blocking), count=%d", count));
	}
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    mpi_errno = MPIDI_CH3I_SHM_Progress_init();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return mpi_errno;
}

