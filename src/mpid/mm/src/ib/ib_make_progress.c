/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"
#include "psc_iba.h"

#ifdef WITH_METHOD_IB

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_WINDOWS_H
#define usleep Sleep
#endif

extern int g_num_receive_posted;
extern int g_num_send_posted;

int ib_handle_accept()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

/*@
   ib_make_progress - make progress

   Notes:
@*/
int ib_make_progress()
{
    /*static int count = 0;*/
    ib_uint32_t status;
    ib_work_completion_t completion_data;
    MPIDI_VC *vc_ptr;
    void *mem_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MAKE_PROGRESS);

    status = ib_completion_poll_us(
	IB_Process.hca_handle,
	IB_Process.cq_handle,
	&completion_data);
    if (status == IBA_CQ_EMPTY)
    {
	/*
	count++;
	if (count > 500)
	    usleep(1);
	*/
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }
    if (status != IBA_OK)
    {
	err_printf("error: ib_completion_poll_us did not return IBA_OK\n");
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return -1;
    }
    if (completion_data.status != IB_COMP_ST_SUCCESS)
    {
	err_printf("error: status = %d != IB_COMP_ST_SUCCESS, %s\n", 
	    completion_data.status, iba_compstr(completion_data.status));
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return -1;
    }
    /*count = 0;*/

    /* Get the vc_ptr and mem_ptr out of the work_id */
    vc_ptr = (MPIDI_VC*)(((ib_work_id_handle_t*)&completion_data.work_req_id)->data.vc);
    mem_ptr = (void*)(((ib_work_id_handle_t*)&completion_data.work_req_id)->data.mem);

    switch (completion_data.op_type)
    {
    case OP_SEND:
	g_num_send_posted--;
	/*printf("%d:s%d ", MPIR_Process.comm_world->rank, g_num_send_posted);*/
	/*msg_printf("s%d ", g_num_send_posted);*/
	ib_handle_written(vc_ptr, mem_ptr, ibr_next_num_written());
	/* put the send packet back in the pool */
	BlockFree(vc_ptr->data.ib.info.m_allocator, mem_ptr);
	break;
    case OP_RECEIVE:
	g_num_receive_posted--;
	/*printf("%d:r%d ", MPIR_Process.comm_world->rank, g_num_receive_posted);*/
	/*msg_printf("r%d ", g_num_receive_posted);*/
	ib_handle_read(vc_ptr, mem_ptr, completion_data.bytes_num);
	/* put the receive packet back in the pool */
	BlockFree(vc_ptr->data.ib.info.m_allocator, mem_ptr);
	/* post another receive to replace the consumed one */
	ibr_post_receive(vc_ptr);
	break;
    default:
	MPIU_DBG_PRINTF(("unknown ib op_type: %d\n", completion_data.op_type));
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#endif
