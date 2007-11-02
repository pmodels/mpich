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

#ifdef USE_IB_VAPI

#include "ibuimpl.vapi.h"

IBU_Global IBU_Process;

/*============== static functions prototypes ==================*/
static VAPI_ret_t ibui_get_list_of_hcas(VAPI_hca_id_t **hca_id_buf_p, u_int32_t *num_of_hcas);

/* utility ibu functions */
#undef FUNCNAME
#define FUNCNAME getMaxInlineSize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
VAPI_ret_t getMaxInlineSize( ibu_t ibu )
{
    VAPI_ret_t status;
    VAPI_qp_attr_t qp_attr;
    VAPI_qp_init_attr_t qp_init_attr;
    VAPI_qp_attr_mask_t qp_attr_mask;
    MPIDI_STATE_DECL(MPID_STATE_IBU_GETMAXINLINESIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_GETMAXINLINESIZE);

    MPIU_DBG_PRINTF(("entering getMaxInlineSize\n"));

    QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
    status = VAPI_query_qp(IBU_Process.hca_handle, 
	ibu->qp_handle, 
	&qp_attr,
	&qp_attr_mask, 
	&qp_init_attr );
    if ( status != VAPI_OK )
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_GETMAXINLINESIZE);
	return status;
    }
    ibu->max_inline_size = qp_attr.cap.max_inline_data_sq;

    MPIU_DBG_PRINTF(("exiting getMaxInlineSize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_GETMAXINLINESIZE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME modifyQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
VAPI_ret_t modifyQP( ibu_t ibu, VAPI_qp_state_t qp_state )
{
    VAPI_ret_t status;
    VAPI_qp_attr_t qp_attr;
    VAPI_qp_cap_t qp_cap;
    VAPI_qp_attr_mask_t qp_attr_mask;
    MPIDI_STATE_DECL(MPID_STATE_IBU_MODIFYQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_MODIFYQP);

    MPIU_DBG_PRINTF(("entering modifyQP\n"));
    if (qp_state == VAPI_INIT)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state = VAPI_INIT;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.pkey_ix = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PKEY_IX);
	qp_attr.port = IBU_Process.port;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PORT);
	qp_attr.remote_atomic_flags = VAPI_EN_REM_WRITE | VAPI_EN_REM_READ;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_REMOTE_ATOMIC_FLAGS);
	MPIU_DBG_PRINTF(("moving to INIT on port %d\n", IBU_Process.port));
    }
    else if (qp_state == VAPI_RTR) 
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state         = VAPI_RTR;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.qp_ous_rd_atom   = 1;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_OUS_RD_ATOM);
	qp_attr.path_mtu         = MTU1024;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PATH_MTU);
	qp_attr.rq_psn           = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RQ_PSN);
	qp_attr.pkey_ix          = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_PKEY_IX);
	qp_attr.min_rnr_timer    = 0;/*5;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_MIN_RNR_TIMER);
	qp_attr.dest_qp_num = ibu->dest_qp_num;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_DEST_QP_NUM);
	qp_attr.av.sl            = 0;
	qp_attr.av.grh_flag      = 0;
	qp_attr.av.static_rate   = 0;
	qp_attr.av.src_path_bits = 0;
	qp_attr.av.dlid = ibu->dlid;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_AV);
	MPIU_DBG_PRINTF(("moving to RTR qp(%d:%d => %d:%d)\n", IBU_Process.lid, ibu->qp_num, ibu->dlid, ibu->dest_qp_num));
    }
    else if (qp_state == VAPI_RTS)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state         = VAPI_RTS;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
	qp_attr.sq_psn           = 0;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_SQ_PSN);
	qp_attr.timeout          = /*0x20;*/ 10;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_TIMEOUT);
	qp_attr.retry_count      = 1; /*5;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RETRY_COUNT);
	qp_attr.rnr_retry        = 3; /*1;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_RNR_RETRY);
	qp_attr.ous_dst_rd_atom  = 1; /*255;*/
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_OUS_DST_RD_ATOM);
	MPIU_DBG_PRINTF(("moving to RTS\n"));
    }
    else if (qp_state == VAPI_RESET)
    {
	QP_ATTR_MASK_CLR_ALL(qp_attr_mask);
	qp_attr.qp_state = VAPI_RESET;
	QP_ATTR_MASK_SET(qp_attr_mask, QP_ATTR_QP_STATE);
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return IBU_FAIL;
    }

    status = VAPI_modify_qp(IBU_Process.hca_handle, 
	ibu->qp_handle, 
	&qp_attr,
	&qp_attr_mask, 
	&qp_cap );
    if ( status != VAPI_OK )
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
VAPI_ret_t createQP(ibu_t ibu, ibu_set_t set)
{
    VAPI_ret_t status;
    VAPI_qp_init_attr_t qp_init_attr;
    VAPI_qp_prop_t qp_prop;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATEQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATEQP);
    MPIU_DBG_PRINTF(("entering createQP\n"));
    qp_init_attr.cap.max_oust_wr_rq = IBU_DEFAULT_MAX_WQE; /*10000;*/
    qp_init_attr.cap.max_oust_wr_sq = IBU_DEFAULT_MAX_WQE; /*10000;*/
    qp_init_attr.cap.max_sg_size_rq = 8;
    qp_init_attr.cap.max_sg_size_sq = 8;
    qp_init_attr.pd_hndl = IBU_Process.pd_handle;
    qp_init_attr.rdd_hndl = 0;
    qp_init_attr.rq_cq_hndl = set;
    qp_init_attr.sq_cq_hndl = set;
    qp_init_attr.rq_sig_type = /*VAPI_SIGNAL_ALL_WR;*/ VAPI_SIGNAL_REQ_WR;
    qp_init_attr.sq_sig_type = /*VAPI_SIGNAL_ALL_WR;*/ VAPI_SIGNAL_REQ_WR;
    qp_init_attr.ts_type = IB_TS_RC;

    status = VAPI_create_qp(IBU_Process.hca_handle, &qp_init_attr, &ibu->qp_handle, &qp_prop);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("VAPI_create_qp failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    ibu->qp_num = qp_prop.qp_num;
    MPIU_DBG_PRINTF(("exiting createQP\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
    return IBU_SUCCESS;
}


/*Mellanox - sleybo 
args:			
ibu - pointer to ibu_state_t 
retrun value:
p - memory hndl of the buffers , NULL if fail.

function that allocate local RDMA buffers and register them to the HCA


*/
#undef FUNCNAME
#define FUNCNAME ibui_update_remote_RDMA_buf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_update_remote_RDMA_buf(ibu_t ibu, ibu_rdma_buf_t* buf, VAPI_rkey_t rkey)
{
    MPIDI_STATE_DECL(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);
    MPIU_DBG_PRINTF(("entering ibui_update_remote_RDMA_buf\n"));
    ibu->remote_RDMA_buf_base = buf;
    ibu->remote_RDMA_buf_hndl.rkey = rkey;
    ibu->remote_RDMA_head = 0;
    ibu->remote_RDMA_limit = IBU_NUM_OF_RDMA_BUFS - 1;

    MPIU_DBG_PRINTF(("ibu_update_RDMA_buf  rkey = %x  buf= %p\n",rkey,buf));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_UPDATE_REMOTE_RDMA_BUF);
    return IBU_SUCCESS;
}




/*Mellanox - sleybo 
args:			
ibu - pointer to ibu_state_t 
retrun value:
p - memory hndl of the buffers , NULL if fail.

function that allocate local RDMA buffers and register them to the HCA
*/
#undef FUNCNAME
#define FUNCNAME ibui_RDMA_buf_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_rdma_buf_t* ibui_RDMA_buf_init(ibu_t ibu, VAPI_rkey_t* rkey)
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
    MPIU_DBG_PRINTF(("ibui_RDMA_buf_init   rkey = %x  buf= %p\n", *rkey, buf));
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_RDMA_BUF_INIT);
    return buf;
}

#undef FUNCNAME
#define FUNCNAME ibu_start_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_t ibu_start_qp(ibu_set_t set, int *qp_num_ptr)
{
    VAPI_ret_t status;
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
	MPIU_Internal_error_printf("ibu_create_qp: createQP failed, error %s\n", VAPI_strerror(status));
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
int ibu_finish_qp(ibu_t p, ibu_lid_t dest_lid, int dest_qp_num)
{
    VAPI_ret_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINISH_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINISH_QP);
    MPIU_DBG_PRINTF(("entering ibu_finish_qp\n"));

    p->dest_qp_num = dest_qp_num;
    p->dlid = dest_lid;
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(INIT)"));*/
    status = modifyQP(p, VAPI_INIT);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(INIT) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTR)"));*/
    status = modifyQP(p, VAPI_RTR);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTR) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    status = modifyQP(p, VAPI_RTS);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: modifyQP(RTS) failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    status = getMaxInlineSize(p);
    if (status != IBU_SUCCESS)
    {
	MPIU_Internal_error_printf("ibu_finish_qp: getMaxInlineSize() failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
	return -1;
    }

    /* Mellanox dafna July 11th*/
    p->send_wqe_info_fifo.head = 0;
    p->send_wqe_info_fifo.tail = 0;	
    /* Mellanox END dafna July 11th*/

    MPIU_DBG_PRINTF(("exiting ibu_create_qp\n"));    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINISH_QP);
    return IBU_SUCCESS;
}

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

/* Mellanox August 25th*/
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
Given the next posted index, determines weather this description posting
is singnalled or not.
returns signal bit according to VAPI definitions.
*/
#undef FUNCNAME
#define FUNCNAME ibui_signaled_completion
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_signaled_completion(ibu_t ibu)
{
    int signaled = VAPI_UNSIGNALED;	
    if (ibu->send_wqe_info_fifo.head % (IBU_PACKET_COUNT >> 1) == 0)
    {
	signaled = VAPI_SIGNALED;
    }
    return signaled;
}
/* Mellanox END ibui_signaled_completion July 11th*/

#undef FUNCNAME
#define FUNCNAME ibu_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr)
{
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_sr_desc_t work_req;
    void* mem_ptr;
    unsigned int length, msg_size;
    int signaled_type = VAPI_UNSIGNALED;
    int total = 0;
    ibu_work_id_handle_t *id_ptr = NULL;
    ibu_rdma_type_t entry_type;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITE);
    MPIU_DBG_PRINTF(("entering ibu_write\n"));
    while (len)
    {
	length = min(len, IBU_EAGER_PACKET_SIZE);
	len -= length;

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

	/*((ibu_rdma_buf_t*)mem_ptr)->footer.cur_offset = 0;*/
	((ibu_rdma_buf_t*)mem_ptr)->footer.RDMA_buf_valid_flag = IBU_VALID_RDMA_BUF;
	((ibu_rdma_buf_t*)mem_ptr)->footer.updated_remote_RDMA_recv_limit = 
	    ((ibu->local_RDMA_head + IBU_NUM_OF_RDMA_BUFS) - 1) % IBU_NUM_OF_RDMA_BUFS; /* Piggybacked update of remote Q state */
	((ibu_rdma_buf_t*)mem_ptr)->footer.total_size = msg_size;

	/* Mellanox - END adding the RDMA header */
	memcpy(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size), buf, length);
	total += length;
	data.len = msg_size; /*Mellanox descriptor holds additional header */
	/* Data.addr points to Beginning of original buffer */
	data.addr = (VAPI_virt_addr_t)(MT_virt_addr_t)(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size));
	data.lkey = GETLKEY(mem_ptr);
	/*MPIU_Assert(data.lkey == s_lkey);*/

	work_req.opcode = VAPI_RDMA_WRITE; /* Mellanox dafna changed from SEND July 11th*/
	signaled_type = ibui_signaled_completion(ibu); /* Mellanox dafna July 11th*/
	work_req.comp_type = signaled_type;
	work_req.sg_lst_p = &data;
	work_req.sg_lst_len = 1;
	work_req.imm_data = 0;
	work_req.fence = 0;
	work_req.remote_ah = 0;
	work_req.remote_qp = 0;
	work_req.remote_qkey = 0;
	work_req.ethertype = 0;
	work_req.eecn = 0;
	work_req.set_se = 0;
	work_req.remote_addr	= (VAPI_virt_addr_t)(MT_virt_addr_t)(ibu->remote_RDMA_buf_base + (ibu->remote_RDMA_head + 1));
	work_req.remote_addr   -= msg_size;
	work_req.r_key		= ibu->remote_RDMA_buf_hndl.rkey;
	work_req.compare_add = 0;
	work_req.swap = 0;

	MPIU_DBG_PRINTF((" work_req remote_addr = %p  \n",work_req.remote_addr ));

	/* Mellanox Allocate id_ptr only if wqe is signaled  */
	if (signaled_type == VAPI_SIGNALED)
	{
	    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator);
	    *((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
	    if (id_ptr == NULL)
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
		return IBU_FAIL;
	    }
	    id_ptr->ibu = ibu;
	    id_ptr->mem = (void*)mem_ptr;
	    id_ptr->length = msg_size;
	}

	/*sanity_check_send(&work_req);*/
	if (msg_size < ibu->max_inline_size)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "calling EVAPI_post_inline_sr(%d bytes)", msg_size));
	    status = EVAPI_post_inline_sr( IBU_Process.hca_handle,
		ibu->qp_handle, 
		&work_req);
	}
	else
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "calling VAPI_post_sr(%d bytes)", msg_size));
	    status = VAPI_post_sr( IBU_Process.hca_handle,
		ibu->qp_handle, 
		&work_req);
	}
	if (status != VAPI_OK)
	{
	    /* Free id_ptr if was signaled and VAPI posting failed */
	    if (signaled_type == VAPI_SIGNALED)
	    {
		ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
	    }
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, VAPI_strerror(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return IBU_FAIL;
	}

	/* Mellanox push entry to send_wqe_fifo */
	entry_type = (signaled_type == VAPI_SIGNALED)? IBU_RDMA_EAGER_SIGNALED : IBU_RDMA_EAGER_UNSIGNALED;
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
    VAPI_ret_t status;
    VAPI_sg_lst_entry_t data;
    VAPI_sr_desc_t work_req;
    void* mem_ptr;
    unsigned int len, msg_size;
    int cur_msg_total, total = 0;
    unsigned int num_avail;
    unsigned char *buf;
    int cur_index, msg_calc_cur_index; /* Mellanox added */
    unsigned int cur_len, msg_calc_cur_len;
    char *cur_buf;
    int signaled_type = VAPI_UNSIGNALED;
    ibu_work_id_handle_t *id_ptr = NULL;
    ibu_rdma_type_t entry_type;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITEV);
    MPIU_DBG_PRINTF(("entering ibu_writev\n"));

    cur_index          = 0;
    msg_calc_cur_index = 0;
    cur_len            = iov[cur_index].MPID_IOV_LEN;
    cur_buf            = iov[cur_index].MPID_IOV_BUF;
    msg_calc_cur_len   = iov[cur_index].MPID_IOV_LEN;

    do
    {
	cur_msg_total = 0;
	if ((((ibu->remote_RDMA_limit - ibu->remote_RDMA_head) + IBU_NUM_OF_RDMA_BUFS) % IBU_NUM_OF_RDMA_BUFS) < 2)
	{
	    /*printf("ibu_writev: no remote packets available\n");fflush(stdout);*/
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
	data.len = msg_size;
	MPIU_Assert(data.len);

	/* Data.addr points to Beginning of original buffer */
	data.addr = (VAPI_virt_addr_t)(MT_virt_addr_t)(((ibu_rdma_buf_t*)mem_ptr)->alignment + (IBU_RDMA_BUF_SIZE - msg_size)); 
	data.lkey = GETLKEY(mem_ptr);
	/*MPIU_Assert(data.lkey == s_lkey);*/

	work_req.opcode = VAPI_RDMA_WRITE; /* Mellanox dafna changed from SEND July 11th*/
	signaled_type = ibui_signaled_completion(ibu); /* Mellanox dafna July 11th*/
	work_req.comp_type = signaled_type;
	work_req.sg_lst_p = &data;
	work_req.sg_lst_len = 1;
	work_req.imm_data = 0;
	work_req.fence = 0;
	work_req.remote_ah = 0;
	work_req.remote_qp = 0;
	work_req.remote_qkey = 0;
	work_req.ethertype = 0;
	work_req.eecn = 0;
	work_req.set_se = 0;
	work_req.remote_addr	= (VAPI_virt_addr_t)(MT_virt_addr_t)(ibu->remote_RDMA_buf_base + (ibu->remote_RDMA_head + 1));
	work_req.remote_addr   -= msg_size;
	work_req.r_key		= ibu->remote_RDMA_buf_hndl.rkey;
	work_req.compare_add = 0;
	work_req.swap = 0;

	MPIU_DBG_PRINTF((" work_req remote_addr = %p  \n",work_req.remote_addr ));

	/* Mellanox Allocate id_ptr only if wqe is signaled  */
	if (signaled_type == VAPI_SIGNALED)
	{
	    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator);
	    *((ibu_work_id_handle_t**)&work_req.id) = id_ptr;
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

	/*sanity_check_send(&work_req);*/
	if (msg_size < (unsigned int)ibu->max_inline_size)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "EVAPI_post_inline_sr(%d bytes)", msg_size));
	    status = EVAPI_post_inline_sr( IBU_Process.hca_handle,
		ibu->qp_handle, 
		&work_req);
	}
	else
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "VAPI_post_sr(%d bytes)", msg_size));
	    status = VAPI_post_sr( IBU_Process.hca_handle,
		ibu->qp_handle, 
		&work_req);
	}

	if (status != VAPI_OK)
	{
	    /* Free id_ptr if was signaled and VAPI posting failed */
	    if (signaled_type == VAPI_SIGNALED)
	    {
		ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
	    }
	    MPIU_Internal_error_printf("%s: Error: failed to post ib send, status = %s\n", FCNAME, VAPI_strerror(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return IBU_FAIL;
	}

	/* Mellanox push entry to send_wqe_fifo */
	entry_type = (signaled_type == VAPI_SIGNALED)? IBU_RDMA_EAGER_SIGNALED : IBU_RDMA_EAGER_UNSIGNALED;
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
#define FUNCNAME ibui_get_list_of_hcas
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static VAPI_ret_t ibui_get_list_of_hcas(VAPI_hca_id_t **hca_id_buf_p, u_int32_t *num_of_hcas)
{
    VAPI_hca_id_t *hca_id_buf;
    VAPI_ret_t rc;
    u_int32_t local_num_of_hcas;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_GET_LIST_OF_HCAS);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
    MPIU_DBG_PRINTF(("entering ibui_get_list_of_hcas\n"));

    *hca_id_buf_p = NULL;
    rc = EVAPI_list_hcas(0, &local_num_of_hcas, NULL);
    switch ( rc )
    {
    case VAPI_OK:
	*num_of_hcas = local_num_of_hcas;
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
	return VAPI_OK;
    case VAPI_EAGAIN:
	hca_id_buf = MPIU_Malloc(sizeof(VAPI_hca_id_t)*local_num_of_hcas);
	if ( !hca_id_buf )
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
	    return VAPI_ERR;
	}
	rc = EVAPI_list_hcas(local_num_of_hcas, &local_num_of_hcas, hca_id_buf);
	if ( rc != VAPI_OK )
	{
	    MPIU_Free(hca_id_buf);
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
	    return VAPI_ERR;
	}
	*num_of_hcas = local_num_of_hcas;
	*hca_id_buf_p = hca_id_buf;
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
	return VAPI_OK;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_GET_LIST_OF_HCAS);
    return VAPI_ERR;
}


/* For the style checker: allowing one instance of malloc and free
   (instead of MPIU_Malloc and MPIU_Free) below because they are being
   passed to a function */

/* style: allow:malloc:1 sig:0 */
/* style: allow:free:1 sig:0 */

#undef FUNCNAME
#define FUNCNAME ibu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_init()
{
    VAPI_ret_t status;
    VAPI_hca_id_t id = "blablabla";
    VAPI_hca_id_t *hca_id_buf;
    u_int32_t num_of_hcas;
    VAPI_hca_vendor_t vendor;
    VAPI_hca_cap_t hca_cap;
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
    IBU_Process.num_send_cqe = 0;

    status = ibui_get_list_of_hcas(&hca_id_buf, &num_of_hcas);
    if (status != VAPI_OK || num_of_hcas == 0)
    {
	MPIU_Internal_error_printf("ibu_init: ibui_get_list_of_hcas failed, status %s, num_of_hcas: %d\n", VAPI_strerror(status),num_of_hcas);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    MPIU_Strncpy(id, hca_id_buf[0], sizeof(VAPI_hca_id_t));
    if ( hca_id_buf ) MPIU_Free(hca_id_buf);

    status = EVAPI_get_hca_hndl(id, &IBU_Process.hca_handle);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: EVAPI_get_hca_hndl failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    status = VAPI_query_hca_cap(IBU_Process.hca_handle, &vendor, &hca_cap);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_query_hca_cap failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.port = 1;
    IBU_Process.cq_size = hca_cap.max_num_ent_cq;
    MPIU_DBG_PRINTF(("cq size: %d\n", IBU_Process.cq_size));
    /* get a protection domain handle */
    status = VAPI_alloc_pd(IBU_Process.hca_handle, &IBU_Process.pd_handle);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_alloc_pd failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }

    /* get the lid */
    status = VAPI_query_hca_port_prop(IBU_Process.hca_handle,
	(IB_port_t)1, &IBU_Process.hca_port);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_init: VAPI_query_hca_port_prop failed, status %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.lid = IBU_Process.hca_port.lid;

    /*
    MPIU_DBG_PRINTF(("infiniband:\n mtu: %d\n msg_size: %d\n",
    IBU_Process.attr_p->port_static_info_p->mtu,
    IBU_Process.attr_p->port_static_info_p->msg_size));
    */

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
    ibuBlockAllocFinalize(&IBU_Process.workAllocator);
    MPIU_DBG_PRINTF(("exiting ibu_finalize\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}

void FooBar(VAPI_hca_hndl_t hca_handle, VAPI_cq_hndl_t cq_handle, void* p)
{
    MPIU_Error_printf("FooBar\n");
    fflush(stdout);
}

void FooBar2(VAPI_hca_hndl_t hca_handle, VAPI_event_record_t *event, void *p)
{
    MPIU_Error_printf("FooBar2\n");
    fflush(stdout);
}

#undef FUNCNAME
#define FUNCNAME ibu_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_create_set(ibu_set_t *set)
{
    VAPI_ret_t status;
    VAPI_cqe_num_t max_cq_entries = IBU_Process.cq_size;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    MPIU_DBG_PRINTF(("entering ibu_create_set\n"));
    /* create the completion queue */
    status = VAPI_create_cq(
	IBU_Process.hca_handle, 
	IBU_Process.cq_size,
	set,
	&max_cq_entries);
    if (status != VAPI_OK)
    {
	MPIU_Internal_error_printf("ibu_create_set: VAPI_create_cq failed, error %s\n", VAPI_strerror(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
	return status;
    }

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
    VAPI_ret_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    MPIU_DBG_PRINTF(("entering ibu_destroy_set\n"));
    status = VAPI_destroy_cq(IBU_Process.hca_handle, set);
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
	    /* free the unexpected data node */
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
	    /* free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
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
	/* free the unexpected data node */
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

#endif /* USE_IB_VAPI */
