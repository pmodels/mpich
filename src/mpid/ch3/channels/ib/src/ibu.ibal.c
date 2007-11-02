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

#ifdef HAVE_WINDOWS_H
#ifdef USE_DEBUG_ALLOCATION_HOOK
#include <crtdbg.h>
#endif
#endif

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"
#include <complib/cl_thread.h>

IBU_Global IBU_Process;

/*============== static functions prototypes ==================*/
static int ibui_get_first_active_ca();

/* utility ibu functions */
#undef FUNCNAME
#define FUNCNAME getMaxInlineSize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ib_api_status_t getMaxInlineSize( ibu_t ibu )
{
    ib_api_status_t status = IB_SUCCESS;
    ib_qp_attr_t qp_prop;
    MPIDI_STATE_DECL(MPID_STATE_IBU_GETMAXINLINESIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_GETMAXINLINESIZE);
    MPIU_DBG_PRINTF(("entering getMaxInlineSize\n"));

    status = ib_query_qp(ibu->qp_handle, &qp_prop);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("getMaxInlineSize: ib_query_qp failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_GETMAXINLINESIZE);
	return status;
    }
    ibu->max_inline_size = qp_prop.sq_max_inline;

    MPIU_DBG_PRINTF(("exiting getMaxInlineSize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_GETMAXINLINESIZE);
    return IBU_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME modifyQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ib_api_status_t modifyQP( ibu_t ibu, ib_qp_state_t qp_state )
{
    ib_api_status_t status;
    ib_qp_mod_t qp_mod;
    MPIDI_STATE_DECL(MPID_STATE_IBU_MODIFYQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_MODIFYQP);

    memset(&qp_mod, 0, sizeof(qp_mod));
    MPIU_DBG_PRINTF(("entering modifyQP\n"));
    if (qp_state == IB_QPS_INIT)
    {
	qp_mod.req_state = IB_QPS_INIT;
	qp_mod.state.init.qkey = 0x0;
	qp_mod.state.init.pkey_index = 0x0;
	qp_mod.state.init.primary_port = IBU_Process.port;

	qp_mod.state.init.access_ctrl = /* Mellanox, dafna April 11th added rdma write and read	for access control */
	    IB_AC_LOCAL_WRITE | 
	    IB_AC_RDMA_WRITE  |
	    IB_AC_RDMA_READ;
    }
    else if (qp_state == IB_QPS_RTR) 
    {
	qp_mod.req_state = IB_QPS_RTR;
	qp_mod.state.rtr.rq_psn = cl_hton32(0x00000000); /*Mellanox, dafna April 11th changed from '1' to '0'*/
	qp_mod.state.rtr.dest_qp = ibu->dest_qp_num;
	qp_mod.state.rtr.resp_res = 1;
	qp_mod.state.rtr.rnr_nak_timeout = 7;
	qp_mod.state.rtr.pkey_index = 0x0;
	qp_mod.state.rtr.opts = IB_MOD_QP_PRIMARY_AV;
	qp_mod.state.rtr.primary_av.sl = 0;
	qp_mod.state.rtr.primary_av.dlid = ibu->dlid;
	qp_mod.state.rtr.primary_av.grh_valid = 0;
	/* static rate: IB_PATH_RECORD_RATE_10_GBS for 4x link width, 30GBS for 12x*/
	qp_mod.state.rtr.primary_av.static_rate = IB_PATH_RECORD_RATE_10_GBS; /* Mellanox dafna April 11th, TODO - set to IBU_Process.port_static_rate*/
	qp_mod.state.rtr.primary_av.path_bits = 0;
	if (IBU_Process.dev_id == 0x5A44) /* Tavor Based */
	{
	    qp_mod.state.rtr.primary_av.conn.path_mtu = IB_MTU_1024;
	}
	else
	{
	    qp_mod.state.rtr.primary_av.conn.path_mtu = IB_MTU_2048;
	}		
	qp_mod.state.rtr.primary_av.conn.local_ack_timeout = 7;
	qp_mod.state.rtr.primary_av.conn.seq_err_retry_cnt = 7;
	qp_mod.state.rtr.primary_av.conn.rnr_retry_cnt = 7;
	/*qp_mod.state.rtr.primary_av.port_num = IBU_Process.port;*/
    }
    else if (qp_state == IB_QPS_RTS)
    {
	qp_mod.req_state = IB_QPS_RTS;
	qp_mod.state.rts.sq_psn = cl_hton32(0x00000000);/*Mellanox, dafna April 11th changed from '1' to '0'*/
	qp_mod.state.rts.retry_cnt = 1; /*Mellanox, dafna April 11th changed from '7' to '1'*/
	qp_mod.state.rts.rnr_retry_cnt = 3; /*Mellanox, dafna April 11th changed from '7' to '3'*/
	qp_mod.state.rts.rnr_nak_timeout = 7;
	qp_mod.state.rts.local_ack_timeout = 10; /*Mellanox, dafna April 11th changed from '20' to '10'*/
	qp_mod.state.rts.init_depth = 7;
	qp_mod.state.rts.opts = 0;
    }
    else if (qp_state == IB_QPS_RESET)
    {
	qp_mod.req_state = IB_QPS_RESET;
	/* Mellanox, dafna April 11th no RESET union which holdes timewait anymore/ removed qp_mod.state.reset.timewait = 0*/
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return IBU_FAIL;
    }

    status = ib_modify_qp(ibu->qp_handle, &qp_mod);
    if ( status != IB_SUCCESS )
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return status;
    }

    MPIU_DBG_PRINTF(("exiting modifyQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME createQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ib_api_status_t createQP(ibu_t ibu, ibu_set_t set)
{
    ib_api_status_t status;
    ib_qp_create_t qp_init_attr;
    ib_qp_attr_t qp_prop;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATEQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATEQP);
    MPIU_DBG_PRINTF(("entering createQP\n"));
    qp_init_attr.qp_type = IB_QPT_RELIABLE_CONN;
    /* Mellanox, dafna April 11th h_rdd not a member of qp_create anymore qp_init_attr.h_rdd = 0 */
    qp_init_attr.sq_depth = IBU_DEFAULT_MAX_WQE; /* Mellanox, dafna April 11th 10000;*/
    qp_init_attr.rq_depth = IBU_DEFAULT_MAX_WQE; /* Mellanox, dafna April 11th 10000;*/
    qp_init_attr.sq_sge = 8;
    qp_init_attr.rq_sge = 8;
    qp_init_attr.h_sq_cq = set;
    qp_init_attr.h_rq_cq = set;
    qp_init_attr.sq_signaled = FALSE; /*TRUE;*/

    status = ib_create_qp(IBU_Process.pd_handle, &qp_init_attr, NULL, NULL, &ibu->qp_handle);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_create_qp failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    status = ib_query_qp(ibu->qp_handle, &qp_prop);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ib_query_qp failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    MPIU_DBG_PRINTF(("qp: num = %d, dest_num = %d\n",
	cl_ntoh32(qp_prop.num),
	cl_ntoh32(qp_prop.dest_num)));
    ibu->qp_num = qp_prop.num;

    MPIU_DBG_PRINTF(("exiting createQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
    return IBU_SUCCESS;
}


/* * Added by Mellanox, dafna April 11th * *
args:
ibu - pointer to ibu_state_t
retrun value:
p - memory hndl of the buffers , NULL if fail.

Allocates local RDMA buffers and registers them to the HCA
*/
#undef FUNCNAME
#define FUNCNAME ibui_update_remote_RDMA_buf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_update_remote_RDMA_buf(ibu_t ibu, ibu_rdma_buf_t* buf, uint32_t rkey)
{
    MPIDI_STATE_DECL(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);
    MPIU_DBG_PRINTF(("entering ibui_update_remote_RDMA_buf\n"));
    ibu->remote_RDMA_buf_base = buf;
    ibu->remote_RDMA_buf_hndl.rkey = rkey;
    ibu->remote_RDMA_head = 0;
    ibu->remote_RDMA_limit = IBU_NUM_OF_RDMA_BUFS - 1;

    MPIU_DBG_PRINTF(("ibu_update_RDMA_buf  rkey = %x  buf= %p\n",rkey,buf));
    MPIU_DBG_PRINTF(("ibu_update_RDMA_buf  ************** remote_RDMA_head = %d  remote_RDMA_limit= %d\n",
	ibu->remote_RDMA_head,
	ibu->remote_RDMA_limit));

    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);
    return IBU_SUCCESS;
}

/* * Added by Mellanox, dafna April 11th  * *
args:
ibu - pointer to ibu_state_t 
retrun value:
p - memory hndl of the buffers , NULL if fail.

Allocates local RDMA buffers and registers them to the HCA
*/
#undef FUNCNAME
#define FUNCNAME ibui_RDMA_buf_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_rdma_buf_t* ibui_RDMA_buf_init(ibu_t ibu, uint32_t* rkey)
{
    ibu_rdma_buf_t *buf;
    ibu_mem_t mem_handle;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_RDMA_BUF_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_RDMA_BUF_INIT);
    MPIU_DBG_PRINTF(("entering ibui_RDMA_buf_init\n"));

    buf = ibuRDMAAllocInitIB(&mem_handle); 

    if (buf == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_RDMA_BUF_INIT);
	return NULL;
    }

    ibu->local_RDMA_buf_base = buf;
    ibu->local_RDMA_buf_hndl = mem_handle; 
    ibu->local_RDMA_head =  0;
    ibu->local_last_updated_RDMA_limit = IBU_NUM_OF_RDMA_BUFS - 1;

    *rkey = ibu->local_RDMA_buf_hndl.rkey;
    MPIU_DBG_PRINTF(("ibui_RDMA_buf_init rkey = %x buf= %p\n", *rkey, buf));
    MPIU_DBG_PRINTF(("ibui_RDMA_buf_init ************* ibu->local_RDMA_head = %d ibu->local_last_updated_RDMA_limit= %d\n", 
	ibu->local_RDMA_head, ibu->local_last_updated_RDMA_limit));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_RDMA_BUF_INIT);
    return buf;
}


#undef FUNCNAME
#define FUNCNAME ibu_start_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_t ibu_start_qp(ibu_set_t set, int *qp_num_ptr)
{
    ib_api_status_t status;
    ibu_t p;
    MPIDI_STATE_DECL(MPID_STATE_IBU_START_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_START_QP);
    MPIU_DBG_PRINTF(("entering ibu_start_qp\n"));

    p = (ibu_t)MPIU_Malloc(sizeof(ibu_state_t));
    if (p == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
	return NULL;
    }

    memset(p, 0, sizeof(ibu_state_t));
    p->state = 0;
    p->allocator = ibuBlockAllocInitIB();

    /*MPIDI_DBG_PRINTF((60, FCNAME, "creating the queue pair\n"));*/
    /* Create the queue pair */
    status = createQP(p, set);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_qp: createQP failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
	return NULL;
    }
    *qp_num_ptr = p->qp_num;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_START_QP);
    return p;
}

#undef FUNCNAME
#define FUNCNAME ibu_finish_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_finish_qp(ibu_t p, int dest_lid, int dest_qp_num)
{
    ib_api_status_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINISH_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINISH_QP);

    MPIU_DBG_PRINTF(("entering ibu_finish_qp\n"));

    p->dest_qp_num = dest_qp_num;
    p->dlid = dest_lid;
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(INIT)"));*/
    status = modifyQP(p, IB_QPS_INIT);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(INIT) failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTR)"));*/
    status = modifyQP(p, IB_QPS_RTR);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTR) failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTS)"));*/
    status = modifyQP(p, IB_QPS_RTS);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTS) failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    /* Added by Mellanox, dafna April 11th */ 
    status = getMaxInlineSize(p);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: getMaxInlineSize() failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }
    p->send_wqe_info_fifo.head = 0;
    p->send_wqe_info_fifo.tail = 0;	

    /* Finished scope added by Mellanox, dafna April 11th; removed receive posting.  */ 

    MPIU_DBG_PRINTF(("exiting ibu_finish_qp\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
    return MPI_SUCCESS;
}
/* * Mellanox, dafna April 11th changed post_ack_write upside down to match new methodology * */
#undef FUNCNAME
#define FUNCNAME ibui_post_ack_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_post_ack_write(ibu_t ibu)
{
    int mpi_errno = MPI_SUCCESS;
    int num_bytes;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_rdma_limit_upt_t * const ack_pkt = &upkt.limit_upt;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_ACK_WRITE);
    MPIU_DBG_PRINTF(("entering ibui_post_ack_write\n"));

    ack_pkt->iov_len = 0;
    ack_pkt->type = MPIDI_CH3_PKT_LMT_UPT;

#ifdef MPICH_DBG_OUTPUT
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)ack_pkt);
#endif
    mpi_errno = ibu_write(ibu, ack_pkt, (int)sizeof(MPIDI_CH3_Pkt_t), &num_bytes); /* Mellanox - write with special ack pkt header */

    if (mpi_errno != MPI_SUCCESS || num_bytes != sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return mpi_errno;
    }

    MPIU_DBG_PRINTF(("exiting ibui_post_ack_write\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
    return mpi_errno;
}
/* ibu functions */

//Mellanox, dafna April 11th added functions (up to ibu_write)

#undef FUNCNAME
#define FUNCNAME ibui_post_rndv_cts_iov_reg_err
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_post_rndv_cts_iov_reg_err(ibu_t ibu, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int num_bytes;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_rndv_reg_error_t* const cts_iov_reg_err_pkt = &upkt.rndv_reg_error;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RNDV_CTS_IOV_REG_ERR);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RNDV_CTS_IOV_REG_ERR);
    MPIU_DBG_PRINTF(("entering ibui_post_rndv_cts_iov_reg_err\n"));

    cts_iov_reg_err_pkt->iov_len = 0;
    cts_iov_reg_err_pkt->type = MPIDI_CH3_PKT_RNDV_CTS_IOV_REG_ERR;
    cts_iov_reg_err_pkt->sreq = rreq->dev.sender_req_id;
    cts_iov_reg_err_pkt->rreq = rreq->handle;

#ifdef MPICH_DBG_OUTPUT
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)cts_iov_reg_err_pkt);
#endif
    mpi_errno = ibu_write(ibu, cts_iov_reg_err_pkt, (int)sizeof(MPIDI_CH3_Pkt_t), &num_bytes); /* Mellanox - write with special cts registration errorr pkt header */

    if (mpi_errno != MPI_SUCCESS || num_bytes != sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RNDV_CTS_IOV_REG_ERR);
	return mpi_errno;
    }
    MPIU_DBG_PRINTF(("exiting ibui_post_rndv_cts_iov_reg_err\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RNDV_CTS_IOV_REG_ERR);
    return mpi_errno;
}

/* Mellanox July 12th 
send_wqe_info_fifo_empty - check if fifo is emptry or not:
return TRUE if empty, FALSE otherwise
*/
#undef FUNCNAME
#define FUNCNAME send_wqe_info_fifo_empty
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int send_wqe_info_fifo_empty(int head, int tail)
{	
    if (head == tail)
    {
	return TRUE;
    }
    return FALSE;
}

/* Mellanox July 12th 
send_wqe_info_fifo_pop - pop entry to send_wqe_fifo:
Update signaled_wqes counter, and advance tail
*/
#undef FUNCNAME
#define FUNCNAME send_wqe_info_fifo_pop
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void send_wqe_info_fifo_pop(ibu_t ibu, ibui_send_wqe_info_t* entry)
{
    int head, tail;
    head = ibu->send_wqe_info_fifo.head;
    tail = ibu->send_wqe_info_fifo.tail;
    if (!send_wqe_info_fifo_empty(head, tail))
    {
	*entry = ibu->send_wqe_info_fifo.entries[tail];
	if (entry->RDMA_type == IBU_RDMA_EAGER_SIGNALED || 
	    entry->RDMA_type == IBU_RDMA_RDNV_SIGNALED)
	{
	    ibu->send_wqe_info_fifo.num_of_signaled_wqes--;
	    IBU_Process.num_send_cqe--;
	}
    }
    else
    {
	entry = NULL;
    }
    ibu->send_wqe_info_fifo.tail = (tail + 1) % IBU_DEFAULT_MAX_WQE;
}
/* Mellanox July 12th 
check if send_wqe_fifo if full and cannot be pushed into:
return TRUE if full, FALSE otherwise
*/
#undef FUNCNAME
#define FUNCNAME send_wqe_info_fifo_full
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int send_wqe_info_fifo_full(int head, int tail)
{
    if (((head + 1) % IBU_DEFAULT_MAX_WQE) == tail)
    {
	return TRUE;
    }
    return FALSE;
}

/* Mellanox July 12th 
push entry to send_wqe_fifo:
Update RDMA type, signaled_wqes counter, length, mem_ptr and advance header
*/
#undef FUNCNAME
#define FUNCNAME send_wqe_info_fifo_push
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void send_wqe_info_fifo_push(ibu_t ibu, ibu_rdma_type_t entry_type, void* mem_ptr, int length)
{
    int head,tail;
    head = ibu->send_wqe_info_fifo.head;
    tail = ibu->send_wqe_info_fifo.tail;
    if (!send_wqe_info_fifo_full(head,tail))
    {
	ibu->send_wqe_info_fifo.entries[head].RDMA_type = entry_type;
	if (entry_type == IBU_RDMA_EAGER_SIGNALED ||
	    entry_type == IBU_RDMA_RDNV_SIGNALED ) 
	{
	    ibu->send_wqe_info_fifo.num_of_signaled_wqes++;
	    IBU_Process.num_send_cqe++;
	}
	ibu->send_wqe_info_fifo.entries[head].length = length;
	ibu->send_wqe_info_fifo.entries[head].mem_ptr = mem_ptr;

	ibu->send_wqe_info_fifo.head = (head + 1) % IBU_DEFAULT_MAX_WQE;
    }
}

/* Mellanox July 11th 
Given the next posted index, determines whether this description posting
is singnalled or not.
returns signal bit according to API definitions.
*/
#undef FUNCNAME
#define FUNCNAME ibui_signaled_completion
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ib_send_opt_t ibui_signaled_completion(ibu_t ibu)
{
    int signaled = 0x00000000 /* unsignalled */;	
    if (ibu->send_wqe_info_fifo.head % (IBU_PACKET_COUNT >> 1) == 0)
    {
	signaled = IB_SEND_OPT_SIGNALED;
    }
    return signaled;
}


//Mellanox, dafna April 11th finished added funcions

#undef FUNCNAME
#define FUNCNAME ibu_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
    void *mem_ptr;
    //Mellanox, dafna April 11th added msg_size 
    unsigned int length, msg_size;	
    int signaled_type = 0x00000000; /*UNSIGNALED*/ 
    int total = 0;
    ibu_work_id_handle_t *id_ptr = NULL;	
    ibu_rdma_type_t entry_type; /* Mellanox, dafna April 11th added entry_type */
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITE);
    MPIU_DBG_PRINTF(("entering ibu_write\n"));

    while (len)
    {
	length = min(len, IBU_EAGER_PACKET_SIZE); /*Mellanox, dafna April 11th replaced IBU_PACKET_SIZE*/
	len -= length;

	/* Mellanox, dafna April 11th replaced nAvailRemote with rmote_RDMA_limit  */
	if ((((ibu->remote_RDMA_limit - ibu->remote_RDMA_head) + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS) < 2)
	{
	    /* Mellanox - check if packet is update limit packet. if not - return and enqueue */
	    if (((MPIDI_CH3_Pkt_t*)buf)->type != MPIDI_CH3_PKT_LMT_UPT)
	    {
		/*printf("ibu_write: no remote packets available\n");fflush(stdout);*/			

		MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available"));
		MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head = %d ibu->remote_RDMA_limit = %d .",
		    ibu->remote_RDMA_head ,ibu->remote_RDMA_limit));
		MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head - limit = %d ",
		    ((ibu->remote_RDMA_head - ibu->remote_RDMA_limit + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS)));

		*num_bytes_ptr = total;
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
		return IBU_SUCCESS;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "Going to send update limit pkt"));
	    /* Mellanox - check for update limit packet if sending is available*/
	    if ((((ibu->remote_RDMA_limit - ibu->remote_RDMA_head)  + IBU_NUM_OF_RDMA_BUFS ) % IBU_NUM_OF_RDMA_BUFS) < 1)
	    {
		/* No send is available. Pretend as if sent and ignore ibu_write request */
		*num_bytes_ptr = len;

		MPIDI_DBG_PRINTF((60, FCNAME, "update limit is not available. exiting ibu_write"));
		MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head = %d ibu->remote_RDMA_limit = %d .",
		    ibu->remote_RDMA_head ,ibu->remote_RDMA_limit));
		MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head - limit = %d ",
		    ((ibu->remote_RDMA_head - ibu->remote_RDMA_limit + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS)));

		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
		return IBU_SUCCESS;
	    }
	}
	MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head = %d ibu->remote_RDMA_limit = %d .",ibu->remote_RDMA_head ,ibu->remote_RDMA_limit));

	mem_ptr = ibuBlockAllocIB(ibu->allocator);
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned address %p\n",mem_ptr));

	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAllocIB returned NULL\n"));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_SUCCESS;
	}

	/* Mellanox - adding the RDMA header */
	if (((MPIDI_CH3_Pkt_t*)buf)->type == MPIDI_CH3_PKT_LMT_UPT)
	{
	    length = 0; /* Only footer will be sent, no other body message */
	}
	msg_size = length + sizeof(ibu_rdma_buf_footer_t); /* Added size of additional header*/

	((ibu_rdma_buf_t*)mem_ptr)->footer.RDMA_buf_valid_flag = IBU_VALID_RDMA_BUF;
	((ibu_rdma_buf_t*)mem_ptr)->footer.updated_remote_RDMA_recv_limit = 
	    ((ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS) - 1) % IBU_NUM_OF_RDMA_BUFS; /* Piggybacked update of remote Q state */
	((ibu_rdma_buf_t*)mem_ptr)->footer.total_size = msg_size;

	/* Mellanox - END adding the RDMA header */
	memcpy(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size), buf, length);
	total += length;

	data.length = msg_size; /*Mellanox descriptor holds additional header */ /*Mellanox, dafna April 11th length*/;
	/* Data.vaddr points to Beginning of original buffer */
	data.vaddr = (uint64_t)(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size)) /*Mellanox, dafna April 11th mem_ptr*/;
	data.lkey = GETLKEY(mem_ptr);

	work_req.p_next = NULL;
	work_req.wr_type = WR_RDMA_WRITE /*Mellanox, dafna April 11th SEND*/;
	signaled_type = ibui_signaled_completion(ibu); 
	work_req.send_opt = signaled_type /*Mellanox, dafna April 11th IB_SEND_OPT_SIGNALED*/;
	work_req.num_ds = 1;
	work_req.ds_array = &data;
	work_req.immediate_data = 0;
	/* Mellanox, dafna April 11th added remote_ops parameters  */
	work_req.remote_ops.vaddr	= (uint64_t)(ibu->remote_RDMA_buf_base + (ibu->remote_RDMA_head + 1));
	work_req.remote_ops.vaddr	-= msg_size;
	work_req.remote_ops.rkey = ibu->remote_RDMA_buf_hndl.rkey;
	work_req.remote_ops.atomic1 = 0x0;
	work_req.remote_ops.atomic2 = 0x0;

	MPIU_DBG_PRINTF((" work_req remote_addr = %p  \n",work_req.remote_ops.vaddr ));

	/* Mellanox, dafna April 11th added signaling type */
	/* Allocate id_ptr only if wqe is signaled  */
	if (signaled_type == IB_SEND_OPT_SIGNALED)
	{
	    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator/*Mellanox, dafna April 11th g_workAllocator*/);
	    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
	    if (id_ptr == NULL)
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
		return IBU_FAIL;
	    }
	    id_ptr->ibu = ibu;
	    id_ptr->mem = (void*)mem_ptr;
	    id_ptr->length = msg_size; /* Mellanox, dafna April 11th added msg_size*/
	}

	if (msg_size < ibu->max_inline_size)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_inline_sr(%d bytes)", msg_size));
	    work_req.send_opt |= IB_SEND_OPT_INLINE;
	    status = ib_post_send(ibu->qp_handle, &work_req, NULL);
	}
	else
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_send(%d bytes)", length));
	    status = ib_post_send(ibu->qp_handle, &work_req, NULL);
	}
	if (status != IB_SUCCESS)
	{
	    if (signaled_type == IB_SEND_OPT_SIGNALED)
	    {
		ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
	    }
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_FAIL;
	}
	/* Mellanox push entry to send_wqe_fifo */
	entry_type = (signaled_type == IB_SEND_OPT_SIGNALED)? IBU_RDMA_EAGER_SIGNALED : IBU_RDMA_EAGER_UNSIGNALED;
	send_wqe_info_fifo_push(ibu, entry_type, mem_ptr, msg_size);

	/* Mellanox update head of remote RDMA buffer to write to */
	ibu->remote_RDMA_head = (ibu->remote_RDMA_head + 1) % IBU_NUM_OF_RDMA_BUFS;

	/* Mellanox update remote RDMA limit to what was sent in the packet */
	ibu->local_last_updated_RDMA_limit = (( ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS )- 1) % IBU_NUM_OF_RDMA_BUFS;

	/* Mellanox change of print. use remote_RDMA_head/remote_RMDA_limit instead of nAvailRemote 
	use new local RDMA limit for nUnacked */
	MPIU_DBG_PRINTF(("send posted, nAvailRemote: %d, local_last_updated_RDMA_limit: %d\n", 
	    (ibu->remote_RDMA_limit - ibu->remote_RDMA_head + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS, 
	    ((ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS)- 1) % IBU_NUM_OF_RDMA_BUFS));

	//Mellanox, dafna April 11th removed ibu->nAvailRemote--;
	buf = (char*)buf + length;

    }

    *num_bytes_ptr = total;

    MPIU_DBG_PRINTF(("exiting ibu_write\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_writev(ibu_t ibu, MPID_IOV *iov, int n, int *num_bytes_ptr)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
    void *mem_ptr;
    unsigned int len, msg_size;
    int cur_msg_total, total = 0;
    unsigned int num_avail;
    unsigned char *buf;
    int cur_index, msg_calc_cur_index; /* Added by Mellanox, dafna April 11th (msg_calc)*/
    unsigned int cur_len, msg_calc_cur_len; /* Added by Mellanox, dafna April 11th (msg_calc)*/
    unsigned char *cur_buf;
    int signaled_type = 0x00000000; /*UNSIGNALED*/ 
    ibu_work_id_handle_t *id_ptr = NULL;
    ibu_rdma_type_t entry_type; /* Added by Mellanox, dafna April 11th */
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITEV);
    MPIU_DBG_PRINTF(("entering ibu_writev\n"));

    cur_index = 0;
    msg_calc_cur_index = 0;
    cur_len = iov[cur_index].MPID_IOV_LEN;
    cur_buf = iov[cur_index].MPID_IOV_BUF;
    msg_calc_cur_len   = iov[cur_index].MPID_IOV_LEN;
    do
    {
	cur_msg_total = 0;
	if ((((ibu->remote_RDMA_limit - ibu->remote_RDMA_head) + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS) < 2)
	{
	    *num_bytes_ptr = total;

	    MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available."));
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head = %d ibu->remote_RDMA_limit = %d .",
		ibu->remote_RDMA_head ,ibu->remote_RDMA_limit));
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head - limit = %d ",
		((ibu->remote_RDMA_head - ibu->remote_RDMA_limit + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS)));

	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_SUCCESS; 
	}

	MPIDI_DBG_PRINTF((60, FCNAME, "ibu->remote_RDMA_head = %d ibu->remote_RDMA_limit = %d .",ibu->remote_RDMA_head ,ibu->remote_RDMA_limit));
	mem_ptr = ibuBlockAllocIB(ibu->allocator);

	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL."));
	    *num_bytes_ptr = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_SUCCESS;
	}

	/*MPIU_DBG_PRINTF(("iov length: %d\n, n));*/		
	if (2 == n && (iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN) < IBU_EAGER_PACKET_SIZE)  
	{	
	    total = iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN;
	    msg_size = total + sizeof(ibu_rdma_buf_footer_t);
	    buf = ((ibu_rdma_buf_t *)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size);
	    memcpy(buf,iov[0].MPID_IOV_BUF,iov[0].MPID_IOV_LEN);
	    memcpy(buf+iov[0].MPID_IOV_LEN,iov[1].MPID_IOV_BUF,iov[1].MPID_IOV_LEN);
	    cur_index =n;
	}
	else
	{
	    num_avail = IBU_EAGER_PACKET_SIZE;
	    for (; msg_calc_cur_index < n && num_avail; )
	    {
		len = min (num_avail, msg_calc_cur_len);
		num_avail -= len;
		cur_msg_total += len;

		MPIU_DBG_PRINTF((" Cur index IOV[%d] - Adding 0x%x to msg_size. Total is 0x%x \n",msg_calc_cur_index, len, total));
		if (msg_calc_cur_len == len)
		{
		    msg_calc_cur_index++;
		    msg_calc_cur_len = iov[msg_calc_cur_index].MPID_IOV_LEN;
		}			
		else
		{
		    msg_calc_cur_len -= len;
		}

	    }


	    msg_size = cur_msg_total + sizeof(ibu_rdma_buf_footer_t); 
	    /* set buf pointer to where data should start . cpy_index will be equal to cur_index after loop*/
	    buf = ((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size);

	    num_avail = IBU_EAGER_PACKET_SIZE;
	    for (; cur_index < n && num_avail; )
	    {
		len = min (num_avail, cur_len);
		num_avail -= len;
		total += len;
		memcpy(buf, cur_buf, len);
		buf += len;

		if (cur_len == len)
		{
		    cur_index++;
		    cur_len = iov[cur_index].MPID_IOV_LEN;
		    cur_buf = iov[cur_index].MPID_IOV_BUF;
		}
		else
		{
		    cur_len -= len;
		    cur_buf += len;
		}
	    }
	}
	/*((ibu_rdma_buf_t*)mem_ptr)->footer.cur_offset = 0;*/
	((ibu_rdma_buf_t*)mem_ptr)->footer.RDMA_buf_valid_flag = IBU_VALID_RDMA_BUF;
	((ibu_rdma_buf_t*)mem_ptr)->footer.updated_remote_RDMA_recv_limit = 
	    (( ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS) - 1) % IBU_NUM_OF_RDMA_BUFS; /* Piggybacked update of remote Q state */
	((ibu_rdma_buf_t*)mem_ptr)->footer.total_size = msg_size; /* Already added size of additional header*/

	/* Mellanox END copying data */
	data.length = msg_size;
	MPIU_Assert(data.length);

	/* Data.vaddr points to beginning of original buffer */
	data.vaddr = (uint64_t)(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size)); 
	data.lkey = GETLKEY(mem_ptr);

	work_req.p_next = NULL;
	work_req.wr_type = WR_RDMA_WRITE; /* replaced WR_SEND by Mellanox, dafna April 11th */
	signaled_type = ibui_signaled_completion(ibu);
	work_req.send_opt = signaled_type;
	work_req.num_ds = 1;
	work_req.ds_array = &data;
	work_req.immediate_data = 0;

	/* Added and updated remote ops by Mellanox, dafna April 11th */
	work_req.remote_ops.vaddr	= (uint64_t)(ibu->remote_RDMA_buf_base + (ibu->remote_RDMA_head + 1));
	work_req.remote_ops.vaddr	-= msg_size;
	work_req.remote_ops.rkey = ibu->remote_RDMA_buf_hndl.rkey;
	work_req.remote_ops.atomic1 = 0x0;
	work_req.remote_ops.atomic2 = 0x0;

	MPIU_DBG_PRINTF((" work_req remote_addr = %p  \n",work_req.remote_ops.vaddr ));

	if (signaled_type == IB_SEND_OPT_SIGNALED)
	{
	    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator);
	    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
	    if (id_ptr == NULL)
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
		MPIU_Assert(0);
		return IBU_FAIL;
	    }
	    id_ptr->ibu = ibu;
	    id_ptr->mem = (void*)mem_ptr;
	    id_ptr->length = msg_size;
	}
	if (msg_size < (unsigned int)ibu->max_inline_size)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_inline_sr(%d bytes)", msg_size));
	    work_req.send_opt |= IB_SEND_OPT_INLINE;
	}
	else
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send(%d bytes)", msg_size));
	}
	status = ib_post_send(ibu->qp_handle, &work_req, NULL);
	if (status != IB_SUCCESS)
	{
	    /* Free id_ptr if was signaled and VAPI posting failed */
	    if (signaled_type == IB_SEND_OPT_SIGNALED)
	    {
		ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
	    }
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, ib_get_err_str(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_FAIL;
	}

	/* Mellanox push entry to send_wqe_fifo */
	entry_type = (signaled_type == IB_SEND_OPT_SIGNALED)? IBU_RDMA_EAGER_SIGNALED : IBU_RDMA_EAGER_UNSIGNALED;
	send_wqe_info_fifo_push(ibu, entry_type , mem_ptr, msg_size);

	/* Mellanox update head of remote RDMA buffer to write to */
	ibu->remote_RDMA_head = (ibu->remote_RDMA_head + 1) % IBU_NUM_OF_RDMA_BUFS;

	/* Mellanox update remote RDMA limit to what was sent in the packet */
	ibu->local_last_updated_RDMA_limit = (( ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS )- 1) % IBU_NUM_OF_RDMA_BUFS;

	/* Mellanox change of print. use remote_RDMA_head/remote_RMDA_limit instead of nAvailRemote 
	use new local RDMA limit for nUnacked */
	MPIU_DBG_PRINTF(("send posted, nAvailRemote: %d, local_last_updated_RDMA_limit: %d\n", 
	    (ibu->remote_RDMA_limit - ibu->remote_RDMA_head + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS, 
	    ((ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS)- 1) % IBU_NUM_OF_RDMA_BUFS));
    } while (cur_index < n);

    *num_bytes_ptr = total;

    MPIU_DBG_PRINTF(("exiting ibu_writev\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
    return IBU_SUCCESS;
}


#ifdef HAVE_WINDOWS_H
#ifdef USE_DEBUG_ALLOCATION_HOOK
int __cdecl ibu_allocation_hook(int nAllocType, void * pvData, size_t nSize, int nBlockUse, long lRequest, const unsigned char * szFileName, int nLine) 
{
    /*nBlockUse = _FREE_BLOCK, _NORMAL_BLOCK, _CRT_BLOCK, _IGNORE_BLOCK, _CLIENT_BLOCK */
    if ( nBlockUse == _CRT_BLOCK ) /* Ignore internal C runtime library allocations */
	return( TRUE ); 

    /*nAllocType = _HOOK_ALLOC, _HOOK_REALLOC, _HOOK_FREE */
    if (nAllocType == _HOOK_FREE)
    {
	/* remove from cache */
	if ( pvData != NULL )
	{
	    ibu_invalidate_memory(pvData, nSize);
	}
    }

    return( TRUE ); /* Allow the memory operation to proceed */
}
#endif
#endif



#undef FUNCNAME
#define FUNCNAME ibui_get_first_active_ca
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_get_first_active_ca()
{
    int mpi_errno;
    ib_api_status_t	status;
    intn_t		guid_count;
    intn_t		i;
    uint32_t		port;
    ib_net64_t		p_ca_guid_array[12];
    ib_ca_attr_t	*p_ca_attr;
    size_t		bsize;
    ib_port_attr_t      *p_port_attr;
    ib_ca_handle_t      hca_handle;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
    MPIU_DBG_PRINTF(("entering ibui_get_first_active_ca\n"));

    status = ib_get_ca_guids( IBU_Process.al_handle, NULL, &guid_count );
    if (status != IB_INSUFFICIENT_MEMORY)
    {
	MPIU_Internal_error_printf( "[%d] ib_get_ca_guids failed [%s]\n", __LINE__, ib_get_err_str(status));
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**get_guids", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
	return mpi_errno;
    }
    /*printf("Total number of CA's = %d\n", (uint32_t)guid_count);fflush(stdout);*/
    if (guid_count == 0) 
    {
	MPIU_Internal_error_printf("no channel adapters available.\n");
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**noca", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
	return mpi_errno;
    }
    if (guid_count > 12)
    {
	guid_count = 12;
    }

    status = ib_get_ca_guids(IBU_Process.al_handle, p_ca_guid_array, &guid_count);
    if ( status != IB_SUCCESS )
    {
	MPIU_Internal_error_printf("[%d] ib_get_ca_guids failed [%s]\n", __LINE__, ib_get_err_str(status));
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ca_guids", "**ca_guids %s", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
	return mpi_errno;
    }

    /* walk guid table */
    for ( i = 0; i < guid_count; i++ )
    {
	status = ib_open_ca( IBU_Process.al_handle, p_ca_guid_array[i], 
	    NULL, NULL, &hca_handle );
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf( "[%d] ib_open_ca failed [%s]\n", __LINE__, 
		ib_get_err_str(status));
	    continue;
	}		

	/*printf( "GUID = %"PRIx64"\n", p_ca_guid_array[i]);fflush(stdout);*/

	/* Query the CA */
	bsize = 0;
	status = ib_query_ca( hca_handle, NULL, &bsize );
	if (status != IB_INSUFFICIENT_MEMORY)
	{
	    MPIU_Internal_error_printf( "[%d] ib_query_ca failed [%s]\n", __LINE__, 
		ib_get_err_str(status));
	    ib_close_ca(hca_handle, NULL);
	    continue;
	}

	/* Allocate the memory needed for query_ca */
	p_ca_attr = (ib_ca_attr_t *)cl_zalloc( bsize );
	if ( !p_ca_attr )
	{
	    MPIU_Internal_error_printf( "[%d] not enough memory\n", __LINE__); 
	    ib_close_ca(hca_handle, NULL);
	    continue;
	}

	status = ib_query_ca( hca_handle, p_ca_attr, &bsize );
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf( "[%d] ib_query_ca failed [%s]\n", __LINE__,
		ib_get_err_str(status));
	    ib_close_ca(hca_handle, NULL);
	    cl_free( p_ca_attr );
	    continue;
	}

	/* scan for active port */
	for( port = 0; port < p_ca_attr->num_ports; port++ )
	{
	    p_port_attr = &p_ca_attr->p_port_attr[port];

	    /* is there an active port? */
	    if ( p_port_attr->link_state == IB_LINK_ACTIVE )
	    {
		/* yes, is there a port_guid or lid we should attach to? */
		/*
		printf("port %d active with lid %d\n", 
		p_port_attr->port_num,
		cl_ntoh16(p_port_attr->lid));
		fflush(stdout);
		*/
		/* get a protection domain handle */
		status = ib_alloc_pd(hca_handle, IB_PDT_NORMAL,
		    NULL, &IBU_Process.pd_handle);
		if (status != IB_SUCCESS)
		{
		    MPIU_Internal_error_printf("get_first_ca: ib_alloc_pd failed, status %s\n", ib_get_err_str(status));
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pd_alloc", "**pd_alloc %s", ib_get_err_str(status));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
		    return mpi_errno;
		}
		IBU_Process.port = p_port_attr->port_num;				
		IBU_Process.port_static_rate = 
		    ib_port_info_compute_rate(p_port_attr); /* Mellanox dafna April 11th, compute port's static rate according to link width*/
		IBU_Process.lid = p_port_attr->lid;
		MPIU_DBG_PRINTF(("port = %d, lid = %d, mtu = %d, max_cqes = %d, maxmsg = %d, link = %s, static_rate = %d\n",
		    p_port_attr->port_num,
		    cl_ntoh16(p_port_attr->lid),
		    p_port_attr->mtu,
		    p_ca_attr->max_cqes,
		    p_port_attr->max_msg_size,
		    ib_get_port_state_str(p_port_attr->link_state),
		    IBU_Process.port_static_rate));

		IBU_Process.hca_handle = hca_handle;
		IBU_Process.dev_id = p_ca_attr->dev_id;	/* Mellanox dafna April 11th, added dev_id */		
		IBU_Process.cq_size = p_ca_attr->max_cqes;
		cl_free( p_ca_attr );
		MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
		return MPI_SUCCESS;
	    }
	}

	/* free allocated mem */
	cl_free( p_ca_attr );
	ib_close_ca(hca_handle, NULL);

    }

    MPIU_Internal_error_printf("no channel adapters available.\n");
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**noca", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_FIRST_ACTIVE_CA);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME ibu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_init()
{
    ib_api_status_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INIT);
    MPIU_DBG_PRINTF(("entering ibu_init\n"));

    /* FIXME: This is a temporary solution to prevent cached pointers from pointing to old
    physical memory pages.  A better solution might be to add a user hook to free() to remove
    cached pointers at that time.
    */
#ifdef MPIDI_CH3_CHANNEL_RNDV
    /* taken from the OSU mvapich source: */
    /* Set glibc/stdlib malloc options to prevent handing
    * memory back to the system (brk) upon free.
    * Also, dont allow MMAP memory for large allocations.
    */
#ifdef M_TRIM_THRESHOLD
    mallopt(M_TRIM_THRESHOLD, -1);
#endif
#ifdef M_MMAP_MAX
    mallopt(M_MMAP_MAX, 0);
#endif
    /* End of OSU code */
#ifdef HAVE_WINDOWS_H
#ifdef USE_DEBUG_ALLOCATION_HOOK
    _CrtSetAllocHook(ibu_allocation_hook);
#endif
#endif
#endif

    /* Initialize globals */
    IBU_Process.num_send_cqe = 0; /* Added by Mellanox, dafna April 11th */

    /* get a handle to the infiniband access layer */
    status = ib_open_al(&IBU_Process.al_handle);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_init: ib_open_al failed, status %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* wait for 50 ms before querying al. This fixes a potential race 
    condition in al where ib_query is not ready with port information
    on faster systems
    */
    cl_thread_suspend(50);
    status = ibui_get_first_active_ca();
    if (status != MPI_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_init: get_first_active_ca failed.\n");
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }

    /* non infiniband initialization */
    IBU_Process.unex_finished_list = NULL;
    IBU_Process.workAllocator = ibuBlockAllocInit(sizeof(ibu_work_id_handle_t), 256, 256, malloc, free);
    MPIU_DBG_PRINTF(("exiting ibu_init\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINALIZE);
    MPIU_DBG_PRINTF(("entering ibu_finalize\n"));
    ibuBlockAllocFinalize(&IBU_Process.workAllocator); /* replaced g_allocator by Mellanox, dafna April 11th */
    ib_close_ca(IBU_Process.hca_handle, NULL);
    ib_close_al(IBU_Process.al_handle);
    MPIU_DBG_PRINTF(("exiting ibu_finalize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}
void AL_API FooBar(const ib_cq_handle_t h_cq, void *p)
{
    MPIU_Internal_error_printf("FooBar\n");
}


#undef FUNCNAME
#define FUNCNAME ibu_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_create_set(ibu_set_t *set)
{
    ib_api_status_t status;
    ib_cq_create_t cq_attr;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    MPIU_DBG_PRINTF(("entering ibu_create_set\n"));

    /* create the completion queue */
    cq_attr.size = IBU_Process.cq_size;
    cq_attr.pfn_comp_cb = FooBar; /* completion routine */
    cq_attr.h_wait_obj = NULL; /* client specific wait object */

    status = ib_create_cq(IBU_Process.hca_handle, &cq_attr, NULL, NULL, set);

    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_create_set: ib_create_cq failed, error %s\n", ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
	return status;
    }
    /*
    status = ib_rearm_cq(*set, TRUE);
    if (status != IB_SUCCESS)
    {
    MPIU_Internal_error_printf("%s: error: ib_rearm_cq failed, %s\n", FCNAME, ib_get_err_str(status));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
    return IBU_FAIL;
    }
    */

    MPIU_DBG_PRINTF(("exiting ibu_create_set\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibu_destroy_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_destroy_set(ibu_set_t set)
{
    ib_api_status_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    MPIU_DBG_PRINTF(("entering ibu_destroy_set\n"));
    status = ib_destroy_cq(set, NULL);
    MPIU_DBG_PRINTF(("exiting ibu_destroy_set\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DESTROY_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibui_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_buffer_unex_read(ibu_t ibu, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    ibu_unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_BUFFER_UNEX_READ);
    MPIU_DBG_PRINTF(("entering ibui_buffer_unex_read\n"));

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (ibu_unex_read_t *)MPIU_Malloc(sizeof(ibu_unex_read_t));
    p->mem_ptr = mem_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = ibu->unex_list;
    ibu->unex_list = p;

    MPIU_DBG_PRINTF(("exiting ibui_buffer_unex_read\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_BUFFER_UNEX_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_read_unex(ibu_t ibu)
{
    unsigned int len;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READ_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READ_UNEX);
    MPIU_DBG_PRINTF(("entering ibui_read_unex\n"));

    MPIU_Assert(ibu->unex_list);

    /* copy the received data */
    while (ibu->unex_list)
    {
	len = min(ibu->unex_list->length, ibu->read.bufflen);
	memcpy(ibu->read.buffer, ibu->unex_list->buf, len);
	/* advance the user pointer */
	ibu->read.buffer = (char*)(ibu->read.buffer) + len;
	ibu->read.bufflen -= len;
	ibu->read.total += len;
	if (len != ibu->unex_list->length)
	{
	    ibu->unex_list->length -= len;
	    ibu->unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		MPIU_Internal_error_printf("ibui_read_unex: mem_ptr == NULL\n");
	    }
	    MPIU_Assert(ibu->unex_list->mem_ptr != NULL);
	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	}
	/* check to see if the entire message was received */
	if (ibu->read.bufflen == 0)
	{
	    /* place this ibu in the finished list so it will be completed by ibu_wait */
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    /* post another receive to replace the consumed one */
	    /*ibui_post_receive(ibu);*/
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
	    return IBU_SUCCESS;
	}
    }
    MPIU_DBG_PRINTF(("exiting ibui_read_unex\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_readv_unex(ibu_t ibu)
{
    unsigned int num_bytes;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READV_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READV_UNEX);
    MPIU_DBG_PRINTF(("entering ibui_readv_unex"));

    while (ibu->unex_list)
    {
	while (ibu->unex_list->length && ibu->read.iovlen)
	{
	    num_bytes = min(ibu->unex_list->length, ibu->read.iov[ibu->read.index].MPID_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, ibu->unex_list->buf, num_bytes);
	    ibu->read.total += num_bytes;
	    ibu->unex_list->buf += num_bytes;
	    ibu->unex_list->length -= num_bytes;
	    /* update the iov */
	    ibu->read.iov[ibu->read.index].MPID_IOV_LEN -= num_bytes;
	    ibu->read.iov[ibu->read.index].MPID_IOV_BUF = 
		(char*)(ibu->read.iov[ibu->read.index].MPID_IOV_BUF) + num_bytes;
	    if (ibu->read.iov[ibu->read.index].MPID_IOV_LEN == 0)
	    {
		ibu->read.index++;
		ibu->read.iovlen--;
	    }
	}

	if (ibu->unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		MPIU_Internal_error_printf("ibui_readv_unex: mem_ptr == NULL\n");
	    }
	    MPIU_Assert(ibu->unex_list->mem_ptr != NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockFreeIB(mem_ptr)"));

	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	    /* replace the consumed read descriptor */	    
	}

	if (ibu->read.iovlen == 0)
	{
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
	    return IBU_SUCCESS;
	}
    }
    MPIU_DBG_PRINTF(("exiting ibui_readv_unex\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_set_vc_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_set_vc_ptr(ibu_t ibu, void *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_SET_USER_PTR);
    MPIU_DBG_PRINTF(("entering ibu_set_vc_ptr\n"));
    if (ibu == IBU_INVALID_QP)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
	return IBU_FAIL;
    }
    ibu->vc_ptr = vc_ptr;
    MPIU_DBG_PRINTF(("exiting ibu_set_vc_ptr\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
    return IBU_SUCCESS;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME ibu_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_read(ibu_t ibu, void *buf, int len)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READ);
    MPIU_DBG_PRINTF(("entering ibu_post_read\n"));
    ibu->read.total = 0;
    ibu->read.buffer = buf;
    ibu->read.bufflen = len;
    ibu->read.use_iov = FALSE;
    ibu->state |= IBU_READING;
    ibu->vc_ptr->ch.reading_pkt = FALSE;
    ibu->pending_operations++;
    /* copy any pre-received data into the buffer */
    if (ibu->unex_list)
	ibui_read_unex(ibu);
    MPIU_DBG_PRINTF(("exiting ibu_post_read\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_readv(ibu_t ibu, MPID_IOV *iov, int n)
{
#ifdef MPICH_DBG_OUTPUT
    char str[1024] = "ibu_post_readv: ";
    char *s;
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READV);
    MPIU_DBG_PRINTF(("entering ibu_post_readv\n"));
#ifdef MPICH_DBG_OUTPUT
    s = &str[16];
    for (i=0; i<n; i++)
    {
	s += MPIU_Snprintf(s, 1008, "%d,", iov[i].MPID_IOV_LEN);
    }
    MPIDI_DBG_PRINTF((60, FCNAME, "%s\n", str));
#endif
    ibu->read.total = 0;
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->read.iov = iov;*/
    memcpy(ibu->read.iov, iov, sizeof(MPID_IOV) * n);
    ibu->read.iovlen = n;
    ibu->read.index = 0;
    ibu->read.use_iov = TRUE;
    ibu->state |= IBU_READING;
    ibu->vc_ptr->ch.reading_pkt = FALSE;
    ibu->pending_operations++;
    /* copy any pre-received data into the iov */
    if (ibu->unex_list)
	ibui_readv_unex(ibu);
    MPIU_DBG_PRINTF(("exiting ibu_post_readv\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READV);
    return IBU_SUCCESS;
}

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_write(ibu_t ibu, void *buf, int len)
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITE);
    MPIU_DBG_PRINTF(("entering ibu_post_write\n"));
    /*
    ibu->write.total = 0;
    ibu->write.buffer = buf;
    ibu->write.bufflen = len;
    ibu->write.use_iov = FALSE;
    ibu->state |= IBU_WRITING;
    ibu->pending_operations++;
    */
    ibu->state |= IBU_WRITING;

    num_bytes = ibui_post_write(ibu, buf, len, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "returning %d\n", num_bytes));
    return num_bytes;
}
#endif

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_writev(ibu_t ibu, MPID_IOV *iov, int n)
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITEV);
    MPIU_DBG_PRINTF(("entering ibu_post_writev\n"));
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->write.iov = iov;*/
    /*
    memcpy(ibu->write.iov, iov, sizeof(MPID_IOV) * n);
    ibu->write.iovlen = n;
    ibu->write.index = 0;
    ibu->write.use_iov = TRUE;
    */
    ibu->state |= IBU_WRITING;
    /*
    {
    char str[1024], *s = str;
    int i;
    s += sprintf(s, "ibu_post_writev(");
    for (i=0; i<n; i++)
    s += sprintf(s, "%d,", iov[i].MPID_IOV_LEN);
    sprintf(s, ")\n");
    MPIU_DBG_PRINTF(("%s", str));
    }
    */
    num_bytes = ibui_post_writev(ibu, iov, n, wfn);
    MPIU_DBG_PRINTF(("exiting ibu_post_writev\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITEV);
    return IBU_SUCCESS;
}
#endif

/* extended functions */

#undef FUNCNAME
#define FUNCNAME ibu_get_lid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_get_lid()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_GET_LID);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_GET_LID);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_GET_LID);
    return IBU_Process.lid;
}

#undef FUNCNAME
#define FUNCNAME post_pkt_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int post_pkt_recv(MPIDI_VC_t *recv_vc_ptr)
{
    int mpi_errno;
    void *mem_ptr;
    ibu_t ibu;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_PKT_RECV);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_PKT_RECV);

    if (recv_vc_ptr->ch.ibu->unex_list == NULL)
    {
	recv_vc_ptr->ch.reading_pkt = TRUE;
	MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_PKT_RECV);
	return MPI_SUCCESS;
    }
    ibu = recv_vc_ptr->ch.ibu;
    recv_vc_ptr->ch.reading_pkt = TRUE;
    mem_ptr = ibu->unex_list->buf;

    if (ibu->unex_list->length < sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_PKT_RECV);
	return mpi_errno;
    }

    /* This is not correct.  It must handle the same cases that ibu_wait does. */

    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "infiniband read progress unable to handle incoming packet");
	MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_PKT_RECV);
	return mpi_errno;
    }

    ibu->unex_list->buf += sizeof(MPIDI_CH3_Pkt_t);
    ibu->unex_list->length -= sizeof(MPIDI_CH3_Pkt_t);
    if (ibu->unex_list->length == 0)
    {
	/* put the receive packet back in the pool */
	if (ibu->unex_list->mem_ptr == NULL)
	{
	    MPIU_Internal_error_printf("ibui_readv_unex: mem_ptr == NULL\n");
	}
	MPIU_Assert(ibu->unex_list->mem_ptr != NULL);

	/* MPIU_Free the unexpected data node */
	temp = ibu->unex_list;
	ibu->unex_list = ibu->unex_list->next;
	MPIU_Free(temp);
    }

    if (recv_vc_ptr->ch.recv_active == NULL)
    {
	MPIU_DBG_PRINTF(("packet with no data handled.\n"));
	recv_vc_ptr->ch.reading_pkt = TRUE;
    }
    else
    {
	/*mpi_errno =*/ ibu_post_readv(ibu, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_PKT_RECV);
    return mpi_errno;
}

#endif /* USE_IB_IBAL */
