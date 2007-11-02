/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"

/*#define PRINT_IBU_WAIT*/
#ifdef PRINT_IBU_WAIT
#define MPIU_DBG_PRINTFX(a) MPIU_DBG_PRINTF(a)
#else
#define MPIU_DBG_PRINTFX(a)
#endif

char * op2str(int wc_type)
{
    static char str[20];
    switch(wc_type)
    {
    case IB_WC_SEND:
	return "IB_WC_SEND";
    case IB_WC_RDMA_WRITE:
	return "IB_WC_RDMA_WRITE";
    case IB_WC_RECV:
	return "IB_WC_RECV";
    case IB_WC_RDMA_READ:
	return "IB_WC_RDMA_READ";
    case IB_WC_MW_BIND:
	return "IB_WC_MW_BIND";
    case IB_WC_FETCH_ADD:
	return "IB_WC_FETCH_ADD";
    case IB_WC_COMPARE_SWAP:
	return "IB_WC_COMPARE_SWAP";
    case IB_WC_RECV_RDMA_WRITE:
	return "IB_WC_RECV_RDMA_WRITE";
    }
    MPIU_Snprintf(str, 20, "%d", wc_type);
    return str;
}

void PrintWC(ib_wc_t *p)
{
    MPIU_Msg_printf("Work Completion Descriptor:\n");
    MPIU_Msg_printf(" id: %d\n", (int)p->wr_id);
    MPIU_Msg_printf(" opcode: %u = %s\n",	   p->wc_type, op2str(p->wc_type));
    MPIU_Msg_printf(" opcode: %u \n",  p->wc_type);
    MPIU_Msg_printf(" length: %d\n", p->length);
    MPIU_Msg_printf(" imm_data_valid: %d\n", (IB_RECV_OPT_IMMEDIATE & (int)p->recv.conn.recv_opt)); 
    MPIU_Msg_printf(" imm_data: %u\n", (unsigned int)p->recv.conn.immediate_data);
    MPIU_Msg_printf(" remote_node_addr:\n");
    // TODO where in ibal remote_node_addr?    MPIU_Msg_printf("  type: %u = %s\n",
    //	   p->remote_node_addr.type,
    //	   remote_node_addr_sym(p->remote_node_addr.type));
    MPIU_Msg_printf("  slid: %d\n", (int)p->recv.ud.remote_lid);
    MPIU_Msg_printf("  sl: %d\n", (int)p->recv.ud.remote_sl);
    MPIU_Msg_printf("  qp: %d\n", (int)p->recv.ud.remote_qp);
    MPIU_Msg_printf("  loc_eecn: %d\n", 0 /*TODO: loc_eecn*/);
    MPIU_Msg_printf(" grh_flag: %d\n", (IB_RECV_OPT_GRH_VALID & (int)p->recv.ud.recv_opt));
    MPIU_Msg_printf(" pkey_ix: %d\n", p->recv.ud.pkey_index);
    MPIU_Msg_printf(" status: %u = %s\n",
	(int)p->status, ib_get_wc_status_str(p->status));
    MPIU_Msg_printf(" vendor_err_syndrome: %d\n", (IB_RECV_OPT_VEND_MASK & (int)p->recv.ud.recv_opt));
    fflush(stdout);
}

#undef FUNCNAME
#define FUNCNAME ibu_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ib_api_status_t ibu_poll(ib_wc_t *completion_data)
{
    int i, RDMA_buf_water_mark;
    ibu_t ibu;
    MPIDI_VC_t *vc;
    ib_wc_t *p_in, *p_complete;
    ibu_work_id_handle_t *id_ptr;
    ibui_send_wqe_info_t entry;
    static cur_vc = 0;
    static vc_cq  = 0;
    volatile void* mem_ptr;
    volatile int valid_flag;
    ibu_set_t set = MPIDI_CH3I_Process.set;
    MPIDI_PG_t  *pg = MPIDI_CH3I_Process.pg;
    ib_api_status_t status = IB_NOT_FOUND;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POLL);
    MPIU_DBG_PRINTFX(("entering ibu_poll\n"));

    if (IBU_Process.num_send_cqe || (vc_cq == IBU_CQ_POLL_WATER_MARK))
    {		
	vc_cq = 0;
	MPIU_DBG_PRINTF(("calling poll cq pg->ch.rank =%d \n", pg->ch.rank));
	p_complete = NULL;
	completion_data->p_next = NULL;
	p_in = completion_data;

	status = ib_poll_cq(set, &p_in, &p_complete); 

	if (status == IB_SUCCESS)
	{
	    if (completion_data->status != IB_WCS_SUCCESS) 
	    {
		/* Create id_ptr for this completion data, so that code outside will remain the same */
		id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator);
		id_ptr->ibu = NULL;
		id_ptr->length = 0;
		id_ptr->mem = NULL;
		/* Set id for completion data */
		completion_data->wr_id = (uint64_t)id_ptr;
		completion_data->recv.ud.recv_opt = completion_data->recv.ud.recv_opt & (!IB_RECV_OPT_IMMEDIATE); /* Mellanox, dafna April 11th: set immd valid to 0 */

		MPIU_DBG_PRINTFX(("exiting ibu_poll upon bad completion status \n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_POLL);
		return status;
	    }
	    id_ptr = *((ibu_work_id_handle_t**)&completion_data->wr_id);
	    ibu = id_ptr->ibu;
	    mem_ptr = (void*)(id_ptr->mem);		
	    send_wqe_info_fifo_pop(ibu, &entry);

	    while ((entry.mem_ptr) != mem_ptr)
	    {
		ibuBlockFreeIB(ibu->allocator, entry.mem_ptr);	
		send_wqe_info_fifo_pop(ibu, &entry);
	    }
	    if (IBU_RDMA_EAGER_SIGNALED == entry.RDMA_type)
	    {
		/* Mellanox - always using IB_WC_SEND since there is never a real RECV */
		completion_data->wc_type = IB_WC_SEND;
	    }
	    MPIU_DBG_PRINTFX(("exiting ibu_poll \n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POLL);
	    return status;
	}
	if (status != IB_INSUFFICIENT_RESOURCES && status != IB_NOT_FOUND)
	{
	    MPIU_DBG_PRINTFX(("ib_poll_cq return  != IB_SUCCESS/NOT_FOUND/INSUFFECIENT_RESOURCES\n"));
	    MPIU_DBG_PRINTFX(("exiting ibu_poll 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POLL);
	    return status;
	}
    }


    MPIU_DBG_PRINTF(("calling poll buffers pg->ch.rank =%d \n", pg->ch.rank));
    vc_cq++;
    for (i=cur_vc; i<MPIDI_PG_Get_size(pg); i++)
    {    
	cur_vc = (i==(MPIDI_PG_Get_size(pg)-1)) ? 0 : i;
	MPIDI_PG_Get_vc(pg, i, &vc);

	if (vc->pg_rank == pg->ch.rank)
	{
	    MPIU_DBG_PRINTF(("cont. pg->ch.rank =%d vc->pg_rank = %d\n", pg->ch.rank,vc->pg_rank));
	    continue;
	}		    		
	/* get the RDMA buf valid flag address */
	mem_ptr = vc->ch.ibu->local_RDMA_buf_base + vc->ch.ibu->local_RDMA_head;
	valid_flag = ((ibu_rdma_buf_t*)mem_ptr)->footer.RDMA_buf_valid_flag;
	MPIU_DBG_PRINTF(("poll on buffer vc->pg_rank = %d  mem_ptr = %p \n", vc->pg_rank,mem_ptr));
	MPIU_DBG_PRINTF(("poll on buffer local_RDMA_buf_base = %p  local_RDMA_head = %d \n",vc->ch.ibu->local_RDMA_buf_base, vc->ch.ibu->local_RDMA_head ));
	if (IBU_VALID_RDMA_BUF == valid_flag)
	{
	    ((ibu_rdma_buf_t*)mem_ptr)->footer.RDMA_buf_valid_flag = IBU_INVALID_RDMA_BUF;
	    /* Set id_ptr*/
	    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator);
	    id_ptr->mem = (void*)(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE -((ibu_rdma_buf_t*)mem_ptr)->footer.total_size));
	    id_ptr->ibu = vc->ch.ibu;
	    id_ptr->length = ((ibu_rdma_buf_t*)mem_ptr)->footer.total_size - sizeof(ibu_rdma_buf_footer_t); 

	    vc->ch.ibu->remote_RDMA_limit = ((ibu_rdma_buf_t*)mem_ptr)->footer.updated_remote_RDMA_recv_limit;
	    /* Set pseudo completion data */
	    completion_data->wr_id = (uint64_t)id_ptr;
	    completion_data->recv.ud.recv_opt  = completion_data->recv.ud.recv_opt & (!IB_RECV_OPT_IMMEDIATE); 
	    completion_data->length = ((ibu_rdma_buf_t*)mem_ptr)->footer.total_size - sizeof(ibu_rdma_buf_footer_t);
	    completion_data->status	= IB_WCS_SUCCESS;
	    completion_data->wc_type = IB_WC_RECV;
	    if (completion_data->length) 
	    {
		status = IB_SUCCESS;
	    } 
	    /* else status remains IB_NOT_FOUND */
	    RDMA_buf_water_mark = (vc->ch.ibu->local_last_updated_RDMA_limit - vc->ch.ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS;
	    if (RDMA_buf_water_mark < (IBU_NUM_OF_RDMA_BUFS /2))
	    {
		ibui_post_ack_write(vc->ch.ibu);
	    }
	    /* update head to point to next buffer */
	    vc->ch.ibu->local_RDMA_head = (vc->ch.ibu->local_RDMA_head + 1) % IBU_NUM_OF_RDMA_BUFS;
	    break; /* Finished this poll */
	}
    }
    MPIU_DBG_PRINTFX(("exiting ibu_poll 4\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POLL);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr)
{
    ib_api_status_t status;
    ib_wc_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    long offset;
    ibu_work_id_handle_t *id_ptr;
    int send_length;
    int ibu_reg_status = IBU_SUCCESS;
    MPIDI_VC_t *recv_vc_ptr;
    void *mem_ptr_orig;
    int mpi_errno;
    int pkt_offset;
#ifdef MPIDI_CH3_CHANNEL_RNDV
    MPID_Request *sreq, *rreq;
    int complete;
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);
    MPIU_DBG_PRINTFX(("entering ibu_wait\n"));

    for (;;)
    {
	if (IBU_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes",
		IBU_Process.unex_finished_list->read.total));
	    /* remove this ibu from the finished list */
	    ibu = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = IBU_Process.unex_finished_list->unex_finished_queue;
	    ibu->unex_finished_queue = NULL;
	    *num_bytes_ptr = ibu->read.total;
	    *op_ptr = IBU_OP_READ;
	    *vc_pptr = ibu->vc_ptr;
	    ibu->pending_operations--;
	    if (ibu->closing && ibu->pending_operations == 0)
	    {
		ibu = IBU_INVALID_QP;
	    }
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 1\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return MPI_SUCCESS;
	}

	status = ibu_poll(&completion_data);
	if (status == IB_INSUFFICIENT_RESOURCES || status == IB_NOT_FOUND)
	{
	    /*usleep(1);*/
	    /* poll until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		*num_bytes_ptr = 0;
		*vc_pptr = NULL;
		*op_ptr = IBU_OP_TIMEOUT;
		MPIU_DBG_PRINTFX(("exiting ibu_wait 2\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return MPI_SUCCESS;
	    }
	    continue;
	}
	if (status != IB_SUCCESS)
	{
	    /*MPIU_Internal_error_printf("%s: error: ib_poll_cq did not return IB_SUCCESS, %s\n", FCNAME, ib_get_err_str(status));*/
	    /*MPIU_dump_dbg_memlog_to_stdout();*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_err_str(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return mpi_errno;
	}
	/*
	if (completion_data.status != IB_WCS_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: status = %s != IB_WCS_SUCCESS\n", 
		FCNAME, ib_get_wc_status_str(completion_data.status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	id_ptr = *((ibu_work_id_handle_t**)&completion_data.wr_id);
	ibu = id_ptr->ibu;
	mem_ptr = (void*)(id_ptr->mem);
	send_length = id_ptr->length;
	ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
	mem_ptr_orig = mem_ptr;


	switch (completion_data.wc_type)
	{
#ifdef MPIDI_CH3_CHANNEL_RNDV
	case IB_WC_RDMA_WRITE:
	    if (completion_data.status != IB_WCS_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
		    FCNAME, ib_get_wc_status_str(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_wc_status_str(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    /*if (ibu->state & IBU_RDMA_WRITING)*/
	    {
		MPI_Request rreq_cached;
		int complete = 0;
		ibu->state &= ~IBU_RDMA_WRITING;
		sreq = (MPID_Request*)mem_ptr;
		MPIU_DBG_PRINTF(("sreq after rdma write: sreq=0x%x, rreq=0x%x\n", sreq->handle, sreq->dev.rdma_request));
		rreq_cached = sreq->dev.rdma_request;
		if (sreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER)
		{
		    MPIU_DBG_PRINTF(("unregistering and reloading the sender's iov.\n"));
		    /* unpin the sender's iov */
		    sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
		    for (i=0; i<sreq->dev.iov_count; i++)
		    {
			ibu_reg_status = ibu_deregister_memory(
			    sreq->dev.iov[i].MPID_IOV_BUF, 
			    sreq->dev.iov[i].MPID_IOV_LEN, 
			    &sreq->ch.local_iov_mem[i]);
			/* --BEGIN ERROR HANDLING-- */
			if (ibu_reg_status != IBU_SUCCESS) 
			{						
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed a\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
		    }				
		    /* update the sender's request */
		    mpi_errno = MPIDI_CH3U_Handle_send_req(ibu->vc_ptr, sreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma write");
			MPIU_DBG_PRINTFX(("exiting ibu_wait a\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if ((sreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER) || complete)
		{
		    MPIDI_CH3_Pkt_t pkt;
		    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
		    MPID_Request *reload_sreq = NULL;

		    MPIU_DBG_PRINTF(("sending a reload/done packet (sreq=0x%x, rreq=0x%x).\n", sreq->handle, rreq_cached));
		    /* send the reload/done packet to the receiver */
		    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
		    reload_pkt->rreq = rreq_cached/*sreq->dev.rdma_request*/;
		    reload_pkt->sreq = sreq->handle;
		    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
		    mpi_errno = MPIDI_CH3_iStartMsg(ibu->vc_ptr, reload_pkt, sizeof(*reload_pkt), &reload_sreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait b\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		    if (reload_sreq != NULL)
		    {
			/* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
			MPID_Request_release(reload_sreq);
		    }
		}
		if (sreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER && !complete)
		{
		    /* pin the sender's iov */
		    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
		    sreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
		    for (i=0; i<sreq->dev.iov_count; i++)
		    {
			sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
			ibu_reg_status = ibu_register_memory(
			    sreq->dev.iov[i].MPID_IOV_BUF, 
			    sreq->dev.iov[i].MPID_IOV_LEN, 
			    &sreq->ch.local_iov_mem[i]);
			if (ibu_reg_status != IBU_SUCCESS) break;
		    }
		    if (ibu_reg_status != IBU_SUCCESS)
		    {
			sreq->ch.rndv_status = IBU_RNDV_RTS_FAIL;
			/* Deregister all register buffers we are not going to use */ 
			MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
			if (i > 0) 
			{
			    for (i-=1; i==0; i--) /* take last i's value one down, since last did not succeed*/
			    {
				ibu_reg_status = ibu_deregister_memory(
				    sreq->dev.iov[i].MPID_IOV_BUF, 
				    sreq->dev.iov[i].MPID_IOV_LEN, 
				    &sreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS)
				{								
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed b\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			}					
		    }
		}
		if ((!complete) && !(sreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER))
		{
		    sreq->ch.reload_state = 0;				
		    if (sreq->ch.rndv_status == IBU_RNDV_SUCCESS) 
		    {
			/* do some more rdma writes */
			mpi_errno = MPIDI_CH3I_rdma_writev(ibu->vc_ptr, sreq);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait c\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
		    }
		    else 
		    {
			/* switch to EAGER sending with special packet type (set in switch routine)*/
			MPIDI_CH3_Pkt_t pkt;
			MPIDI_CH3_Pkt_rndv_eager_send_t * rndv_eager_pkt = &pkt.rndv_eager_send;
			MPIU_DBG_PRINTF(("sending eager packet instead of rndv failed sender reloading\n"));
			/* send new eager packet to the receiver */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
			rndv_eager_pkt->seqnum			= ibu->vc_ptr->seqnum_send;
#endif    
			rndv_eager_pkt->match.rank		= sreq->comm->rank;
			rndv_eager_pkt->match.tag		= sreq->dev.match.tag;
			rndv_eager_pkt->match.context_id	= sreq->dev.match.context_id;
			pkt.rndv_eager_send.sender_req_id	= sreq->handle;
			pkt.rndv_eager_send.receiver_req_id	= rreq_cached;

			pkt.rndv_eager_send.type = MPIDI_CH3_PKT_RNDV_EAGER_SEND;
			mpi_errno = MPIDI_CH3I_Switch_rndv_to_eager(ibu->vc_ptr, sreq, &pkt);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait k\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
		    }
		}
		else
		{
		    sreq->ch.reload_state = 0;
		    /* return from the wait */
		    *num_bytes_ptr = 0;
		    *vc_pptr = ibu->vc_ptr;
		    *op_ptr = IBU_OP_WAKEUP;
		    MPIU_DBG_PRINTFX(("exiting ibu_wait d\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	    break;
	case IB_WC_RDMA_READ:
	    if (completion_data.status != IB_WCS_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
		    FCNAME, ib_get_wc_status_str(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_wc_status_str(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    /*if (ibu->state & IBU_RDMA_READING)*/
	    {
		MPI_Request sreq_cached;
		int complete = 0;
		ibu->state &= ~IBU_RDMA_READING;
		rreq = (MPID_Request*)mem_ptr;
		MPIU_DBG_PRINTF(("rreq after rdma read: rreq=0x%x, sreq=0x%x\n", rreq->handle, rreq->dev.rdma_request));
		sreq_cached = rreq->dev.rdma_request;
		if (rreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER)
		{
		    MPIU_DBG_PRINTF(("unregistering and reloading the receiver's iov.\n"));
		    /* unpin the receiver's iov */
		    rreq->ch.rndv_status = IBU_RNDV_SUCCESS;
		    for (i=0; i<rreq->dev.iov_count; i++)
		    {
			ibu_reg_status = ibu_deregister_memory(
			    rreq->dev.iov[i].MPID_IOV_BUF,
			    rreq->dev.iov[i].MPID_IOV_LEN,
			    &rreq->ch.local_iov_mem[i]);
			/* --BEGIN ERROR HANDLING-- */
			if (ibu_reg_status != IBU_SUCCESS) 
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed c\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

		    }
		    /* update the receiver's request */
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(ibu->vc_ptr, rreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma read");
			MPIU_DBG_PRINTFX(("exiting ibu_wait e\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if ((rreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER) || complete)
		{
		    MPIDI_CH3_Pkt_t pkt;
		    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
		    MPID_Request *reload_rreq = NULL;

		    MPIU_DBG_PRINTF(("sending a reload/done packet (sreq=0x%x, rreq=0x%x).\n", rreq->handle, sreq_cached));
		    /* send the reload/done packet to the sender */
		    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
		    reload_pkt->sreq = sreq_cached/*rreq->dev.rdma_request*/;
		    reload_pkt->rreq = rreq->handle;				
		    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
		    mpi_errno = MPIDI_CH3_iStartMsg(ibu->vc_ptr, reload_pkt, sizeof(*reload_pkt), &reload_rreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait f\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		    if (reload_rreq != NULL)
		    {
			/* The sender doesn't need to know when the packet has been sent.
			So release the request immediately */
			MPID_Request_release(reload_rreq);
		    }
		}
		if (rreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER && !complete)
		{
		    /* pin the receiver's iov */
		    MPIU_DBG_PRINTF(("registering the receiver's iov.\n"));
		    rreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
		    for (i=0; i<rreq->dev.iov_count; i++)
		    {
			rreq->ch.rndv_status = IBU_RNDV_SUCCESS;
			ibu_reg_status = ibu_register_memory(
			    rreq->dev.iov[i].MPID_IOV_BUF,
			    rreq->dev.iov[i].MPID_IOV_LEN,
			    &rreq->ch.local_iov_mem[i]);
			if (ibu_reg_status != IBU_SUCCESS) break;
		    }
		    if (ibu_reg_status != IBU_SUCCESS)
		    {
			rreq->ch.rndv_status = IBU_RNDV_CTS_IOV_FAIL;				
			/* Deregister all register buffers we are not going to use */ 
			MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
			if (i > 0) 
			{
			    for (i-=1; i==0; i--) /* take last i's value one down, since last did not succeed*/
			    {
				ibu_reg_status = ibu_deregister_memory(
				    rreq->dev.iov[i].MPID_IOV_BUF, 
				    rreq->dev.iov[i].MPID_IOV_LEN, 
				    &rreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed d\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			}
		    }
		}
		if ((!complete) && !(rreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER))
		{
		    rreq->ch.reload_state = 0;
		    if (rreq->ch.rndv_status == IBU_RNDV_SUCCESS) 
		    {				
			/* do some more rdma reads */
			mpi_errno = MPIDI_CH3I_rdma_readv(ibu->vc_ptr, rreq);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait g\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
		    }
		    else 
		    {
			/* Send a CTS_IOV_REG_ERROR packet. */
			rreq->dev.sender_req_id = sreq_cached;
			mpi_errno = ibui_post_rndv_cts_iov_reg_err(ibu, rreq);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait l\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
		    }
		}
		else
		{
		    rreq->ch.reload_state = 0;
		    /* return from the wait */
		    *num_bytes_ptr = 0;
		    *vc_pptr = ibu->vc_ptr;
		    *op_ptr = IBU_OP_WAKEUP;
		    MPIU_DBG_PRINTFX(("exiting ibu_wait h\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	    break;
#endif /* MPIDI_CH3_CHANNEL_RNDV */
	case IB_WC_SEND:
	    if (completion_data.status != IB_WCS_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
		    FCNAME, ib_get_wc_status_str(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_wc_status_str(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTFX(("exiting ibu_wait 40\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    if (mem_ptr == (void*)-1)
	    {
		MPIU_DBG_PRINTF(("ack sent\n"));
		/* flow control ack completed, no user data so break out here */
		MPIU_DBG_PRINTF(("ack sent.\n"));
		break;
	    }
	    num_bytes = send_length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    ibuBlockFreeIB(ibu->allocator, mem_ptr);

	    *num_bytes_ptr = num_bytes;
	    *op_ptr = IBU_OP_TIMEOUT;
	    *vc_pptr = ibu->vc_ptr;
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 5\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return MPI_SUCCESS;
	    break;
	case IB_WC_RECV:
	    if (completion_data.status != IB_WCS_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: recv completion status = %s\n",
		    FCNAME, ib_get_wc_status_str(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_wc_status_str(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTFX(("exiting ibu_wait 41\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    if (completion_data.recv.ud.recv_opt & IB_RECV_OPT_IMMEDIATE)
	    {
		MPIU_Assert((completion_data.recv.ud.recv_opt & IB_RECV_OPT_IMMEDIATE) != 1); /* Error! */
		break;
	    }
	    num_bytes = completion_data.length;
	    recv_vc_ptr = ibu->vc_ptr;
	    pkt_offset = 0;
	    if (recv_vc_ptr->ch.reading_pkt)
	    {
#ifdef MPIDI_CH3_CHANNEL_RNDV
		if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type > MPIDI_CH3_PKT_END_CH3)
		{
		    if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RTS_IOV)
		    {
			MPIU_DBG_PRINTF(("received rts packet(sreq=0x%x).\n",
			    ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->sreq));
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait h\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_RTS_IOV_READ_REQUEST;
			rreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->sreq;
			rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->iov_len;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.remote_iov_mem[0];
			rreq->dev.iov[1].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			rreq->dev.iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.pkt;
			rreq->dev.iov[2].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
			rreq->dev.iov_count = 3;		
			rreq->ch.req = NULL;
			recv_vc_ptr->ch.recv_active = rreq;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RTS_PUT)
		    {
			int found;
			MPIU_DBG_PRINTF(("received rts put packet(sreq=0x%x).\n",
			    ((MPIDI_CH3_Pkt_rndv_req_to_send_t*)mem_ptr)->sender_req_id));

			mpi_errno = MPIDI_CH3U_Handle_recv_rndv_pkt(recv_vc_ptr,
			    (MPIDI_CH3_Pkt_t*)mem_ptr,
			    &rreq, &found);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(put) packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			if (found)
			{
			    mpi_errno = MPIDI_CH3U_Post_data_receive(found, &rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			    mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}

			recv_vc_ptr->ch.recv_active = NULL;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_CTS_IOV)
		    {
			MPIU_DBG_PRINTF(("received cts packet(sreq=0x%x, rreq=0x%x).\n",
			    ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->sreq,
			    ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->rreq));
			MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->sreq, sreq);
			sreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->rreq;
			sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->iov_len;
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait i\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.remote_iov_mem[0];
			rreq->dev.iov[1].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			rreq->dev.iov_count = 2;
			rreq->ch.req = sreq;
			recv_vc_ptr->ch.recv_active = rreq;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_IOV)
		    {
			if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND )
			{
			    MPIU_DBG_PRINTF(("received sender's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, sreq);
			    sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    rreq = MPID_Request_create();
			    if (rreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait j\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(rreq, 1);
			    rreq->kind = MPIDI_CH3I_IOV_READ_REQUEST;
			    rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			    rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.remote_iov_mem[0];
			    rreq->dev.iov[1].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			    rreq->dev.iov_count = 2;
			    rreq->ch.req = sreq;
			    recv_vc_ptr->ch.recv_active = rreq;
			}
			else if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV )
			{
			    MPIU_DBG_PRINTF(("received receiver's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, rreq);
			    rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    sreq = MPID_Request_create();
			    if (sreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait k\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(sreq, 1);
			    sreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			    sreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    sreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.remote_iov_mem[0];
			    sreq->dev.iov[1].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			    sreq->dev.iov_count = 2;
			    sreq->ch.req = rreq;
			    recv_vc_ptr->ch.recv_active = sreq;
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "received invalid MPIDI_CH3_PKT_IOV packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait l\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RELOAD)
		    {
			if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND)
			{
			    MPIU_DBG_PRINTF(("received reload send packet (sreq=0x%x).\n", ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq, sreq);
			    MPIU_DBG_PRINTF(("unregistering the sender's iov.\n"));
			    sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
			    for (i=0; i<sreq->dev.iov_count; i++)
			    {
				ibu_reg_status = ibu_deregister_memory(
				    sreq->dev.iov[i].MPID_IOV_BUF, 
				    sreq->dev.iov[i].MPID_IOV_LEN,
				    &sreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed e\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			    mpi_errno = MPIDI_CH3U_Handle_send_req(recv_vc_ptr, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update send request after receiving a reload packet");
				MPIU_DBG_PRINTFX(("exiting ibu_wait m\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * rts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
				sreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
				for (i=0; i<sreq->dev.iov_count; i++)
				{
				    sreq->ch.rndv_status = IBU_RNDV_SUCCESS;
				    ibu_reg_status = ibu_register_memory(
					sreq->dev.iov[i].MPID_IOV_BUF, 
					sreq->dev.iov[i].MPID_IOV_LEN,
					&sreq->ch.local_iov_mem[i]);
				    if (ibu_reg_status != IBU_SUCCESS) break;
				}

				if (ibu_reg_status != IBU_SUCCESS)
				{
				    sreq->ch.rndv_status = IBU_RNDV_RTS_IOV_FAIL;
				    /* Deregister all register buffers we are not going to use */ 
				    MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
				    if (i > 0) 
				    {
					for (i-=1; i==0; i--) /* take last i's value one down, since last did not succeed*/
					{
					    ibu_reg_status = ibu_deregister_memory(
						sreq->dev.iov[i].MPID_IOV_BUF, 
						sreq->dev.iov[i].MPID_IOV_LEN, 
						&sreq->ch.local_iov_mem[i]);
					    /* --BEGIN ERROR HANDLING-- */
					    if (ibu_reg_status != IBU_SUCCESS)
					    {
						mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
						MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed f\n"));
						MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
						return mpi_errno;
					    }
					    /* --END ERROR HANDLING-- */
					}
				    }								
				}
				if (sreq->ch.rndv_status == IBU_RNDV_SUCCESS) 
				{
				    MPIU_DBG_PRINTF(("sending reloaded send iov of length %d\n", sreq->dev.iov_count));								
				    MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				    pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
				    pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq;
				    pkt.iov.iov_len = sreq->dev.iov_count;

				    sreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				    sreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				    sreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.iov;
				    sreq->dev.rdma_iov[1].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(MPID_IOV);
				    sreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.local_iov_mem[0];
				    sreq->dev.rdma_iov[2].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(ibu_mem_t);

				    mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, sreq->dev.rdma_iov, 3, &rts_sreq);
				    /* --BEGIN ERROR HANDLING-- */
				    if (mpi_errno != MPI_SUCCESS)
				    {
					MPIU_Object_set_ref(sreq, 0);
					MPIDI_CH3_Request_destroy(sreq);
					sreq = NULL;
					mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
					MPIU_DBG_PRINTFX(("exiting ibu_wait n\n"));
					MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
					return mpi_errno;
				    }
				    /* --END ERROR HANDLING-- */
				    if (rts_sreq != NULL)
				    {
					/* The sender doesn't need to know when the message has been sent.  So release the request immediately */
					MPID_Request_release(rts_sreq);
				    }
				}
				else 
				{
				    /* switch to EAGER sending with special packet type (set in switch routine)*/
				    MPIDI_CH3_Pkt_t pkt;
				    MPIDI_CH3_Pkt_rndv_eager_send_t* rndv_eager_pkt = &pkt.rndv_eager_send;
				    MPIU_DBG_PRINTF(("sending eager packet instead of rndv failed sender reloading\n"));

				    /* send new eager packet to the receiver */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
				    rndv_eager_pkt->seqnum		= recv_vc_ptr->seqnum_send;
#endif    
				    rndv_eager_pkt->match.rank		= sreq->comm->rank;
				    rndv_eager_pkt->match.tag		= sreq->dev.match.tag;
				    rndv_eager_pkt->match.context_id	= sreq->dev.match.context_id;
				    pkt.rndv_eager_send.sender_req_id	= sreq->handle;
				    pkt.rndv_eager_send.receiver_req_id	= ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq; 

				    pkt.rndv_eager_send.type = MPIDI_CH3_PKT_RNDV_EAGER_SEND;
				    mpi_errno = MPIDI_CH3I_Switch_rndv_to_eager(recv_vc_ptr, sreq, &pkt);

				    /* --BEGIN ERROR HANDLING-- */
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
					MPIU_DBG_PRINTFX(("exiting ibu_wait k\n"));
					MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
					return mpi_errno;
				    }
				    /* --END ERROR HANDLING-- */								
				}
			    }
			}
			else if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV)
			{
			    MPIU_DBG_PRINTF(("received reload recv packet (rreq=0x%x).\n", ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq, rreq);
			    MPIU_DBG_PRINTF(("unregistering the receiver's iov.\n"));
			    rreq->ch.rndv_status = IBU_RNDV_SUCCESS;
			    for (i=0; i<rreq->dev.iov_count; i++)
			    {
				ibu_reg_status = ibu_deregister_memory(
				    rreq->dev.iov[i].MPID_IOV_BUF, 
				    rreq->dev.iov[i].MPID_IOV_LEN,
				    &rreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed g\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			    mpi_errno = MPIDI_CH3U_Handle_recv_req(recv_vc_ptr, rreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after receiving a reload packet");
				MPIU_DBG_PRINTFX(("exiting ibu_wait o\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * cts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				MPIU_DBG_PRINTF(("registering the receiver's iov.\n"));
				rreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
				for (i=0; i<rreq->dev.iov_count; i++)
				{
				    rreq->ch.rndv_status = IBU_RNDV_SUCCESS;
				    ibu_reg_status = ibu_register_memory(
					rreq->dev.iov[i].MPID_IOV_BUF, 
					rreq->dev.iov[i].MPID_IOV_LEN,
					&rreq->ch.local_iov_mem[i]);
				    if (ibu_reg_status != IBU_SUCCESS) break;
				}
				if (ibu_reg_status != IBU_SUCCESS)
				{
				    rreq->ch.rndv_status = IBU_RNDV_CTS_IOV_FAIL;									
				    /* Deregister all register buffers we are not going to use */ 
				    MPIU_DBG_PRINTF(("ibu_register_memory failed. deregistering the sender's iov.\n"));
				    if (i > 0) 
				    {
					for (i-=1; i==0; i--) { /* take last i's value one down, since last did not succeed*/
					    ibu_reg_status = ibu_deregister_memory(
						rreq->dev.iov[i].MPID_IOV_BUF, 
						rreq->dev.iov[i].MPID_IOV_LEN, 
						&rreq->ch.local_iov_mem[i]);
					    /* --BEGIN ERROR HANDLING-- */
					    if (ibu_reg_status != IBU_SUCCESS)
					    {
						mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
						MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed h\n"));
						MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
						return mpi_errno;
					    }
					    /* --END ERROR HANDLING-- */
					}
				    }								
				}
				if (rreq->ch.rndv_status == IBU_RNDV_SUCCESS)
				{
				    MPIU_DBG_PRINTF(("sending reloaded recv iov of length %d\n", rreq->dev.iov_count));								
				    MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				    pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
				    pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq;
				    pkt.iov.iov_len = rreq->dev.iov_count;

				    rreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				    rreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				    rreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.iov;
				    rreq->dev.rdma_iov[1].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(MPID_IOV);
				    rreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.local_iov_mem[0];
				    rreq->dev.rdma_iov[2].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(ibu_mem_t);

				    mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, rreq->dev.rdma_iov, 3, &cts_sreq);
				    /* --BEGIN ERROR HANDLING-- */
				    if (mpi_errno != MPI_SUCCESS)
				    {
					/* This destruction probably isn't correct. */
					/* I think it needs to save the error in the request, complete the request and return */
					MPIU_Object_set_ref(rreq, 0);
					MPIDI_CH3_Request_destroy(rreq);
					rreq = NULL;
					mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
					MPIU_DBG_PRINTFX(("exiting ibu_wait p\n"));
					MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
					return mpi_errno;
				    }
				    /* --END ERROR HANDLING-- */
				    if (cts_sreq != NULL)
				    {
					/* The sender doesn't need to know when the message has been sent.  So release the request immediately */
					MPID_Request_release(cts_sreq);
				    }
				}
				else
				{
				    /* Send a CTS_IOV_REG_ERROR packet. */
				    mpi_errno = ibui_post_rndv_cts_iov_reg_err(ibu, rreq);
				    /* --BEGIN ERROR HANDLING-- */
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
					MPIU_DBG_PRINTFX(("exiting ibu_wait k\n"));
					MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
					return mpi_errno;
				    }
				    /* --END ERROR HANDLING-- */	
				}
			    }
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait q\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;
			if (num_bytes > sizeof(MPIDI_CH3_Pkt_t))
			{
			    MPIU_DBG_PRINTF(("pkt handled with %d bytes remaining to be buffered.\n", num_bytes));
			    ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
			}
			else
			{
			    /* Added by Mellanox, dafna April 11th removed post_receive */
			    /* ibuBlockFreeIB(ibu->allocator, mem_ptr_orig); 
			    ibui_post_receive(ibu);*/
			}
			/* return from the wait */
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			*op_ptr = IBU_OP_WAKEUP;
			MPIU_DBG_PRINTFX(("exiting ibu_wait q\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RNDV_CTS_IOV_REG_ERR)
		    {
			MPIDI_CH3_Pkt_t pkt;
			MPIDI_CH3_Pkt_rndv_eager_send_t* rndv_eager_pkt = &pkt.rndv_eager_send;
			MPIU_DBG_PRINTF(("received cts iov registration failure packet(sreq=0x%x).\n", ((MPIDI_CH3_Pkt_rndv_reg_error_t*)mem_ptr)->sreq));					
			MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rndv_reg_error_t*)mem_ptr)->sreq, sreq);
			sreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rndv_reg_error_t*)mem_ptr)->rreq;

			MPIU_DBG_PRINTF(("sending eager packet instead of rndv that failed at receiver\n"));

			/* send new eager packet to the receiver */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
			rndv_eager_pkt->seqnum		    = recv_vc_ptr->seqnum_send;
#endif    					
			rndv_eager_pkt->match.rank	    = sreq->comm->rank;
			rndv_eager_pkt->match.tag	    = sreq->dev.match.tag;
			rndv_eager_pkt->match.context_id    = sreq->dev.match.context_id;
			pkt.rndv_eager_send.sender_req_id   = sreq->handle;
			pkt.rndv_eager_send.receiver_req_id = sreq->dev.rdma_request;
#ifndef USE_RDMA_GET
			/* only in USE_RDMA_PUT sreq registration may have failed when receiving this packet type,
			otherwise - previous registration has definitely succeeded */
			if (sreq->ch.rndv_status == IBU_RNDV_SUCCESS)
#endif
			{
			    MPIU_DBG_PRINTF(("unregistering the sender's iov.\n"));
			    /* unpin the sender's iov */
			    for (i=0; i<sreq->dev.iov_count; i++)
			    {
				ibu_reg_status = ibu_deregister_memory(
				    sreq->dev.iov[i].MPID_IOV_BUF, 
				    sreq->dev.iov[i].MPID_IOV_LEN, &
				    sreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed i\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			    sreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
			}
			pkt.rndv_eager_send.type = MPIDI_CH3_PKT_RNDV_EAGER_SEND;
			mpi_errno = MPIDI_CH3I_Switch_rndv_to_eager(recv_vc_ptr, sreq, &pkt);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    MPIU_Object_set_ref(sreq, 0);
			    MPIDI_CH3_Request_destroy(sreq);
			    sreq = NULL;
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait failed swithching to rndv a\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RNDV_EAGER_SEND)
		    {
			MPIDI_CH3_Pkt_rndv_eager_send_t * rndv_eager_pkt = &((MPIDI_CH3_Pkt_t*)mem_ptr)->rndv_eager_send;
			MPIU_DBG_PRINTF(("received rndv eager packet(sreq=0x%x).\n",
			    ((MPIDI_CH3_Pkt_rndv_eager_send_t*)mem_ptr)->sender_req_id));

			MPID_Request_get_ptr(rndv_eager_pkt->receiver_req_id, rreq);

			if (rreq->ch.rndv_status == IBU_RNDV_SUCCESS) 
			{
			    MPIU_DBG_PRINTF(("deregistering buffers.\n"));
			    /* unpin the receiver's iov */
			    for (i=0; i<rreq->dev.iov_count; i++)
			    {							
				ibu_reg_status = ibu_deregister_memory(
				    rreq->dev.iov[i].MPID_IOV_BUF, 
				    rreq->dev.iov[i].MPID_IOV_LEN, &
				    rreq->ch.local_iov_mem[i]);
				/* --BEGIN ERROR HANDLING-- */
				if (ibu_reg_status != IBU_SUCCESS) 
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to deregister");
				    MPIU_DBG_PRINTFX(("exiting ibu_wait deregister failed j\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
			    }
			    rreq->ch.rndv_status = IBU_RNDV_NO_DEREG;
			}
			/* Need to receive the data on this rndv_eager_message */

			rreq->status.MPI_SOURCE = rndv_eager_pkt->match.rank;
			rreq->status.MPI_TAG	= rndv_eager_pkt->match.tag;
			rreq->status.count	= rndv_eager_pkt->data_sz;
			rreq->dev.sender_req_id = rndv_eager_pkt->sender_req_id;
			rreq->dev.recv_data_sz	= rndv_eager_pkt->data_sz;
			MPIDI_Request_set_seqnum(rreq, rndv_eager_pkt->seqnum);
			MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_EAGER_MSG);
			recv_vc_ptr->ch.recv_active = rreq;
			mpi_errno = MPIDI_CH3U_Post_data_receive(TRUE, &recv_vc_ptr->ch.recv_active);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "infiniband read progress unable to handle incoming packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait rs\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle unknown rdma packet");
			MPIU_DBG_PRINTFX(("exiting ibu_wait r\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		else
#endif /* MPIDI_CH3_CHANNEL_RNDV */
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "infiniband read progress unable to handle incoming packet");
			MPIU_DBG_PRINTFX(("exiting ibu_wait s\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    MPIU_DBG_PRINTF(("packet with no data handled.\n"));
		    recv_vc_ptr->ch.reading_pkt = TRUE;
		}
		else
		{
		    /*
		    int z;
		    printf("posting a read of %d buffers\n",
		    recv_vc_ptr->ch.recv_active->dev.iov_count);
		    for (z=0; z<recv_vc_ptr->ch.recv_active->dev.iov_count; z++)
		    {
			printf(" [%d].len = %d\n", z, recv_vc_ptr->ch.recv_active->dev.iov[z].MPID_IOV_LEN);
		    }
		    fflush(stdout);
		    */
		    /*mpi_errno =*/ ibu_post_readv(ibu, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count);
		}
		mem_ptr = (unsigned char *)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		num_bytes -= sizeof(MPIDI_CH3_Pkt_t);

		if (num_bytes == 0)
		{
		    *num_bytes_ptr = num_bytes;
		    *op_ptr = IBU_OP_TIMEOUT;
		    *vc_pptr = ibu->vc_ptr;
		    MPIU_DBG_PRINTFX(("exiting ibu_wait num_byte == 0\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return MPI_SUCCESS;
		    break;
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    MPIU_DBG_PRINTF(("pkt handled with %d bytes remaining to be buffered.\n", num_bytes));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
		    break;
		}
		pkt_offset = sizeof(MPIDI_CH3_Pkt_t);
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "ibu_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(ibu->state & IBU_READING))
	    {
		MPIU_DBG_PRINTF(("a:buffering %d bytes.\n", num_bytes));
		ibui_buffer_unex_read(ibu, mem_ptr_orig, pkt_offset, num_bytes);
		break;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", ibu->read.total, num_bytes, ibu->read.total + num_bytes));
	    if (ibu->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && ibu->read.iovlen > 0)
		{
		    if ((int)ibu->read.iov[ibu->read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr,
			    ibu->read.iov[ibu->read.index].MPID_IOV_LEN);
			iter_ptr += ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			ibu->read.index++;
			ibu->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			ibu->read.iov[ibu->read.index].MPID_IOV_LEN -= num_bytes;
			ibu->read.iov[ibu->read.index].MPID_IOV_BUF = 
			    (char*)(ibu->read.iov[ibu->read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (long)((unsigned char*)iter_ptr - (unsigned char*)mem_ptr);
		ibu->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: read mem_ptr == NULL\n");
		    MPIU_Assert(mem_ptr != NULL);
		}
		else
		{
		    /* save the unused but received data */
		    MPIU_DBG_PRINTF(("b:buffering %d bytes (offset,pkt = %d,%d).\n", num_bytes, offset, pkt_offset));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, offset + pkt_offset, num_bytes);
		}
		if (ibu->read.iovlen == 0)
		{
		    if (recv_vc_ptr->ch.recv_active->kind < MPID_LAST_REQUEST_KIND)
		    {
			ibu->state &= ~IBU_READING;
			*num_bytes_ptr = ibu->read.total;
			*op_ptr = IBU_OP_READ;
			*vc_pptr = ibu->vc_ptr;
			ibu->pending_operations--;
			if (ibu->closing && ibu->pending_operations == 0)
			{
			    MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after iov read completed."));
			    ibu = IBU_INVALID_QP;
			}
			MPIU_DBG_PRINTFX(("exiting ibu_wait 6\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
		    }
#ifdef MPIDI_CH3_CHANNEL_RNDV
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_RTS_IOV_READ_REQUEST)
		    {
			int found;
			MPIU_DBG_PRINTF(("received rts iov_read.\n"));

			mpi_errno = MPIDI_CH3U_Handle_recv_rndv_pkt(recv_vc_ptr,
			    &recv_vc_ptr->ch.recv_active->ch.pkt,
			    &rreq, &found);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(get) packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			for (i=0; i<recv_vc_ptr->ch.recv_active->dev.rdma_iov_count; i++)
			{
			    rreq->dev.rdma_iov[i].MPID_IOV_BUF = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_BUF;
			    rreq->dev.rdma_iov[i].MPID_IOV_LEN = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_LEN;
			    rreq->ch.remote_iov_mem[i] = recv_vc_ptr->ch.recv_active->ch.remote_iov_mem[i];
			}
			rreq->dev.rdma_iov_count = recv_vc_ptr->ch.recv_active->dev.rdma_iov_count;
			rreq->dev.rdma_request = recv_vc_ptr->ch.recv_active->dev.rdma_request;

			if (found)
			{
			    mpi_errno = MPIDI_CH3U_Post_data_receive(found, &rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			    mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}
			rreq = recv_vc_ptr->ch.recv_active;
			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;

			/* free the request used to receive the rts packet and iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_READ_REQUEST)
		    {
			MPIU_DBG_PRINTF(("received iov_read.\n"));
			rreq = recv_vc_ptr->ch.recv_active;

			/* A new sender's iov has arrived so set the offset back to zero. */
			rreq->ch.req->ch.siov_offset = 0;

			mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq->ch.req);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(get) iov");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;

			/* free the request used to receive the iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_WRITE_REQUEST)
		    {
			MPIU_DBG_PRINTF(("received iov_write.\n"));

			/* A new receiver's iov has arrived so set the offset back to zero. */
			recv_vc_ptr->ch.recv_active->ch.req->ch.riov_offset = 0;

			/* Check rndv status. If failed - need to send CANCEL_IOV packet and wait for confirmation */
			if (recv_vc_ptr->ch.recv_active->ch.req->ch.rndv_status == IBU_RNDV_SUCCESS)
			{
			    mpi_errno = MPIDI_CH3I_rdma_writev(recv_vc_ptr, recv_vc_ptr->ch.recv_active->ch.req);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait w\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}
			else 
			{
			    /* switch to EAGER sending with special packet type (set in switch routine)*/
			    MPIDI_CH3_Pkt_t pkt;
			    MPIDI_CH3_Pkt_rndv_eager_send_t * rndv_eager_pkt = &pkt.rndv_eager_send;
			    MPIU_DBG_PRINTF(("sending eager packet instead of rndv failed sender reloading\n"));

			    /* send new eager packet to the receiver */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
			    rndv_eager_pkt->seqnum              = recv_vc_ptr->seqnum_send;
#endif    
			    rndv_eager_pkt->match.rank          = (recv_vc_ptr->ch.recv_active->ch.req)->comm->rank;
			    rndv_eager_pkt->match.tag           = (recv_vc_ptr->ch.recv_active->ch.req)->dev.match.tag;
			    rndv_eager_pkt->match.context_id	= (recv_vc_ptr->ch.recv_active->ch.req)->dev.match.context_id;
			    pkt.rndv_eager_send.sender_req_id	= (recv_vc_ptr->ch.recv_active->ch.req)->handle;
			    pkt.rndv_eager_send.receiver_req_id	= (recv_vc_ptr->ch.recv_active->ch.req)->dev.rdma_request;

			    pkt.rndv_eager_send.type = MPIDI_CH3_PKT_RNDV_EAGER_SEND;
			    mpi_errno = MPIDI_CH3I_Switch_rndv_to_eager(recv_vc_ptr, recv_vc_ptr->ch.recv_active->ch.req, &pkt);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait w\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}
			/* return from the wait */
			MPID_Request_release(recv_vc_ptr->ch.recv_active);
			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			*op_ptr = IBU_OP_WAKEUP;
			MPIU_DBG_PRINTFX(("exiting ibu_wait x\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
			break;
		    }
#endif /* MPIDI_CH3_CHANNEL_RNDV */
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "invalid request type", recv_vc_ptr->ch.recv_active->kind);
			MPIU_DBG_PRINTFX(("exiting ibu_wait y\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > ibu->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, ibu->read.bufflen);
		    ibu->read.total = ibu->read.bufflen;
		    MPIU_DBG_PRINTF(("c:buffering %d bytes.\n", num_bytes - ibu->read.bufflen));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig,
			ibu->read.bufflen + pkt_offset,
			num_bytes - ibu->read.bufflen);
		    ibu->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, num_bytes);
		    ibu->read.total += num_bytes;
		    /* advance the user pointer */
		    ibu->read.buffer = (char*)(ibu->read.buffer) + num_bytes;
		    ibu->read.bufflen -= num_bytes;
		}
		if (ibu->read.bufflen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    *num_bytes_ptr = ibu->read.total;
		    *op_ptr = IBU_OP_READ;
		    *vc_pptr = ibu->vc_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after simple read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    MPIU_DBG_PRINTFX(("exiting ibu_wait 7\n"));
		    return MPI_SUCCESS;
		}
	    }
	    break;
	default:
	    if (completion_data.status != IB_WCS_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: unknown completion status = %s != IB_WCS_SUCCESS\n", 
		    FCNAME, ib_get_wc_status_str(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", ib_get_wc_status_str(completion_data.status));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 42\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    MPIU_Internal_error_printf("%s: unknown ib opcode: %s\n", FCNAME, op2str(completion_data.wc_type));
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", op2str(completion_data.wc_type));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait z\n"));
	    return mpi_errno;
	    break;
	}
    }

    MPIU_DBG_PRINTFX(("exiting ibu_wait 8\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
    return MPI_SUCCESS;
}

#endif /* USE_IB_IBAL */
