/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"
#include "psc_iba.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

static int s_cur_receive = 0;
static int s_cur_send = 0;
int g_num_receive_posted = 0;
int g_num_send_posted = 0;

int ibr_post_receive(MPIDI_VC *vc_ptr)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_rcv_t work_req;
    void *mem_ptr;

    mem_ptr = BlockAlloc(vc_ptr->data.ib.info.m_allocator);

    sg_list.data_seg_p = &data;
    sg_list.data_seg_num = 1;
    data.length = IB_PACKET_SIZE;
    data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
    data.l_key = vc_ptr->data.ib.info.m_lkey;
    work_req.op_type = OP_RECEIVE;
    work_req.sg_list = sg_list;
    /* store the VC ptr and the mem ptr in the work id */
    ((ib_work_id_handle_t*)&work_req.work_req_id)->data.vc = (ib_uint32_t)vc_ptr;
    ((ib_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;

    MPIU_DBG_PRINTF(("ib_post_rcv_req_us %d\n", s_cur_receive++));
    g_num_receive_posted++;
    status = ib_post_rcv_req_us(IB_Process.hca_handle, 
				vc_ptr->data.ib.info.m_qp_handle,
				&work_req);
    if (status != IB_SUCCESS)
    {
	err_printf("Error: failed to post ib receive, status = %d\n", status);
	return status;
    }

    return MPI_SUCCESS;
}

typedef struct ibu_num_written_node_t
{
    int num_bytes;
    struct ibu_num_written_node_t *next;
} ibu_num_written_node_t;

ibu_num_written_node_t *g_write_list_head = NULL, *g_write_list_tail = NULL;

int ibr_next_num_written()
{
    ibu_num_written_node_t *p;
    int num_bytes;

    p = g_write_list_head;
    g_write_list_head = g_write_list_head->next;
    if (g_write_list_head == NULL)
	g_write_list_tail = NULL;
    num_bytes = p->num_bytes;
    free(p);
    return num_bytes;
}

int ibr_post_write(MPIDI_VC *vc_ptr, void *buf, int len, int (*write_progress_update)(int, void*))
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_send_t work_req;
    void *mem_ptr;
    ibu_num_written_node_t *p;
    int length;

    while (len)
    {
	length = min(len, IB_PACKET_SIZE);
	len -= length;

	p = malloc(sizeof(ibu_num_written_node_t));
	p->next = NULL;
	p->num_bytes = length;
	if (g_write_list_tail)
	{
	    g_write_list_tail->next = p;
	}
	else
	{
	    g_write_list_head = p;
	}
	g_write_list_tail = p;
	
	mem_ptr = BlockAlloc(vc_ptr->data.ib.info.m_allocator);
	memcpy(mem_ptr, buf, length);
	
	sg_list.data_seg_p = &data;
	sg_list.data_seg_num = 1;
	data.length = length;
	data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
	data.l_key = vc_ptr->data.ib.info.m_lkey;
	
	work_req.dest_address      = 0;
	work_req.dest_q_key        = 0;
	work_req.dest_qpn          = 0; /*var.m_dest_qp_num;  // not needed */
	work_req.eecn              = 0;
	work_req.ethertype         = 0;
	work_req.fence_f           = 0;
	work_req.immediate_data    = 0;
	work_req.immediate_data_f  = 0;
	work_req.op_type           = OP_SEND;
	work_req.remote_addr.va    = 0;
	work_req.remote_addr.key   = 0;
	work_req.se_f              = 0;
	work_req.sg_list           = sg_list;
	work_req.signaled_f        = 0;
	
	/* store the VC ptr and the mem ptr in the work id */
	((ib_work_id_handle_t*)&work_req.work_req_id)->data.vc = (ib_uint32_t)vc_ptr;
	((ib_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;
	
	MPIU_DBG_PRINTF(("ib_post_send_req_us %d\n", s_cur_send++));
	g_num_send_posted++;
	status = ib_post_send_req_us( IB_Process.hca_handle,
	    vc_ptr->data.ib.info.m_qp_handle, 
	    &work_req);
	if (status != IB_SUCCESS)
	{
	    err_printf("Error: failed to post ib send, status = %d, %s\n", status, iba_errstr(status));
	    return status;
	}

	buf = (char*)buf + length;
    }

    return MPI_SUCCESS;
}

int ibr_post_writev(MPIDI_VC *vc_ptr, MPID_IOV *iov, int n, int (*write_progress_update)(int, void*))
{
    int i;
    for (i=0; i<n; i++)
    {
	ibr_post_write(vc_ptr, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN, NULL);
    }
    return MPI_SUCCESS;
}
