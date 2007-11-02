/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"
#include "pmi.h"

#ifdef WITH_METHOD_IB

IB_PerProcess IB_Process;
MPIDI_VC_functions g_ib_vc_functions = 
{
    ib_post_read,
    ib_enqueue_read_at_head,
    ib_merge_with_unexpected,
    ib_merge_with_posted,
    ib_merge_unexpected_data,
    ib_post_write,
    ib_enqueue_write_at_head,
    ib_reset_car,
    ib_setup_packet_car,
    ib_post_read_pkt
};

/*@
   ib_init - initialize the ib method

   Notes:
   Let's assume there is one HCA per node with one port available.
@*/
int ib_init()
{
    ib_uint32_t status;
    char key[100], value[100];
    ib_uint32_t max_cq_entries = IB_MAX_CQ_ENTRIES+1;
    ib_uint32_t attr_size;
    MPIDI_STATE_DECL(MPID_STATE_IB_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_INIT);

    /*ib_init_us();*/

    /* Initialize globals */
    /* get a handle to the host channel adapter */
    status = ib_hca_open_us(0 , &IB_Process.hca_handle);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_hca_open_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
	return status;
    }
    /* get a protection domain handle */
    status = ib_pd_allocate_us(IB_Process.hca_handle, &IB_Process.pd_handle);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_pd_allocate_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
	return status;
    }
    /* get a completion queue domain handle */
    status = ib_cqd_create_us(IB_Process.hca_handle, &IB_Process.cqd_handle);
#if 0 /* for some reason this function fails when it really is ok */
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_cqd_create_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
	return status;
    }
#endif
    /* create the completion queue */
    status = ib_cq_create_us(IB_Process.hca_handle, 
	IB_Process.cqd_handle,
	&max_cq_entries,
	&IB_Process.cq_handle,
	NULL);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_cq_create_us failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
	return -1;
    }

    /* get the lid */
    attr_size = 0;
    status = ib_hca_query_us(IB_Process.hca_handle, NULL, 
        HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    IB_Process.attr_p = calloc(attr_size, sizeof(ib_uint8_t));
    /*MPIU_DBG_PRINTF(("MALLOC %d bytes\n", attr_size));*/
    status = ib_hca_query_us(IB_Process.hca_handle, IB_Process.attr_p, 
	HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_hca_query_us(HCA_QUERY_HCA_STATIC) failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
	return status;
    }
    IB_Process.lid = IB_Process.attr_p->port_dynamic_info_p->lid;

    sprintf(key, "ib_lid_%d", MPIR_Process.comm_world->rank);
    sprintf(value, "%d", IB_Process.lid);
    MPIU_DBG_PRINTF(("ib lid %d\n", IB_Process.lid));
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
    PMI_Barrier();

    ib_setup_connections();
    MPIU_DBG_PRINTF(("ib_setup_connections returned\n"));

    MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
    return MPI_SUCCESS;
}

/*@
   ib_finalize - finalize the ib method

   Notes:
@*/
int ib_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_FINALIZE);

    /*ib_release_us();*/

    MPIDI_FUNC_EXIT(MPID_STATE_IB_FINALIZE);
    return MPI_SUCCESS;
}

#endif
