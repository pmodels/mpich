/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "mpidi_ch3_impl.h"
#include "mpidu_process_locks.h"

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    /* There is no post_close for ib connections so handle them as closed immediately. */
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
    int offset = req->ch.iov_offset;
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
	    req->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char*)req->dev.iov[offset].MPID_IOV_BUF + nb);
	    req->dev.iov[offset].MPID_IOV_LEN -= nb;
	    req->ch.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }

    req->ch.iov_offset = 0;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

#undef FUNCNAME
#define FUNCNAME handle_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_read(MPIDI_VC_t *vc, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * req;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    req = vc->ch.recv_active;
    if (req == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
	return mpi_errno;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
	    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    }
	    if (complete)
	    {
		post_pkt_recv(vc);
		vc->ch.recv_active = NULL; /* Mellanox: Dafna, August 29th*/
	    }
	    else
	    {
		vc->ch.recv_active = req;
		mpi_errno = ibu_post_readv(vc->ch.ibu, req->dev.iov, req->dev.iov_count);
	    }
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->dev.iov + req->ch.iov_offset, req->dev.iov_count - req->ch.iov_offset));
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME handle_written
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_written(MPIDI_VC_t * vc)
{
    int nb, mpi_errno = MPI_SUCCESS;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_WRITTEN);

    /*MPIDI_DBG_PRINTF((60, FCNAME, "entering"));*/
    while (vc->ch.send_active != NULL)
    {
	MPID_Request * req = vc->ch.send_active;

	/*MPIDI_DBG_PRINTF((60, FCNAME, "calling ibu_post_writev"));*/
	mpi_errno = ibu_writev(vc->ch.ibu, req->dev.iov + req->ch.iov_offset, req->dev.iov_count - req->ch.iov_offset, &nb);
	if (mpi_errno != IBU_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibwrite", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
	    return mpi_errno;
	}
	MPIDI_DBG_PRINTF((60, FCNAME, "ibu_post_writev returned %d", nb));

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		mpi_errno = MPIDI_CH3U_Handle_send_req(vc, req, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
		    return mpi_errno;
		}
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(vc);
		}
		vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "ibu_post_writev returned %d bytes", nb));
	    break;
	}
    }

    /*MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));*/

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
    return mpi_errno;
}

#ifndef MPIDI_CH3_Progress_start
void MPIDI_CH3_Progress_start(MPID_Progress_state *state)
{
    /* MT - This function is empty for the single-threaded implementation */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking, MPID_Progress_state *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc_ptr;
    int num_bytes;
    ibu_op_t wait_result;
#ifdef MPICH_DBG_OUTPUT
    unsigned register count;
#endif
    unsigned completions = MPIDI_CH3I_progress_completion_count;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));

    do
    {
	mpi_errno = ibu_wait(0, (void*)&vc_ptr, &num_bytes, &wait_result);
	if (mpi_errno != IBU_SUCCESS)
	{
	    /*MPIU_Internal_error_printf("ibu_wait returned IBU_FAIL\n");*/
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibu_wait", "**ibu_wait %d", mpi_errno);
	    goto fn_exit;
	}
	switch (wait_result)
	{
	case IBU_OP_TIMEOUT:
	    MPIDU_Yield();
	    /*sched_yield();*/
	    break;
	case IBU_OP_READ:
	    MPIDI_DBG_PRINTF((50, FCNAME, "ibu_wait reported %d bytes read", num_bytes));
	    mpi_errno = handle_read(vc_ptr, num_bytes);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
	    }
	    break;
	case IBU_OP_WRITE:
	    MPIDI_DBG_PRINTF((50, FCNAME, "ibu_wait reported %d bytes written", num_bytes));
	    mpi_errno = handle_written(vc_ptr);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
	    }
	    break;
	case IBU_OP_CLOSE:
	    break;
	case IBU_OP_WAKEUP:
	    break;
	default:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibu_op", "**ibu_op %d", wait_result);
	    goto fn_exit;
	}

	/* pound on the write queues since ibu_wait currently does not return IBU_OP_WRITE */
	for (i=0; i<MPIDI_PG_Get_size(MPIDI_Process.my_pg); i++)
	{
	    MPIDI_VC_t *vc;
	    MPIDI_PG_Get_vc(MPIDI_Process.my_pg, i, &vc);
	    if (vc->ch.send_active != NULL)
		/*if (i != MPIR_Process.comm_world->rank)*/
	    {
		mpi_errno = handle_written(vc);
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

#ifndef MPIDI_CH3_Progress_poke
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_poke()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    mpi_errno = MPIDI_CH3I_Progress(0, NULL);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}
#endif

#ifndef MPIDI_CH3_Progress_end
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end(MPID_Progress_state *state)
{
    /* MT - This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_END);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_END);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_END);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    /* With the new close protocol you can't do a MPI_Barrier here because the VC's have already
       been torn down.
    */

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return mpi_errno;
}
