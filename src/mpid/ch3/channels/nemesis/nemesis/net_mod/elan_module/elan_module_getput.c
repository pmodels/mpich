/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include <elan/elan.h>

/*
static void
getput_callback (struct gm_port *p, void *completion_ctr, gm_status_t status)
{
    if (status != GM_SUCCESS)
    {
	gm_perror ("Send error", status);
    }

    ++MPID_nem_module_gm_num_send_tokens;

    MPID_NEM_ATOMIC_DEC ((int *)completion_ctr);
}
*/

/*
void
MPID_nem_gm_module_do_get (void *target_p, void *source_p, int len, int node_id, int port_id, int *completion_ctr)
{
    //    gm_get (MPID_nem_module_gm_port, (long)source_p, target_p, len, GM_LOW_PRIORITY, node_id, port_id, getput_callback, completion_ctr);
    
    --MPID_nem_module_gm_num_send_tokens;
}
*/
/*
int
MPID_nem_gm_module_get (void *target_p, void *source_p, int len, MPIDI_VC_t *source_vc, int *completion_ctr)
{
    int ret;
    
    MPIU_Assert (VC_FIELD(source_vc, node_id) >= 0 && VC_FIELD(source_vc, node_id) < MPID_nem_mem_region.num_procs);
    MPIU_Assert (len >= 0);
    
    if (len == 0)
    {
	MPID_NEM_ATOMIC_DEC (completion_ctr);
	return 0;
    }
#if 0
    {
	gm_status_t status;
	
	status = gm_directcopy_get (MPID_nem_module_gm_port, source_p, target_p, len, 0, VC_FIELD(source_vc, port_id));
	if (status != GM_SUCCESS)
	{
	    gm_perror ("directcopy", status);
	    exit (-1);
	}
	MPID_NEM_ATOMIC_DEC (completion_ctr);
    return 0;
    }
#endif
    if (MPID_nem_module_gm_num_send_tokens)
    {
	MPID_nem_gm_module_do_get (target_p, source_p, len, VC_FIELD(source_vc, node_id), VC_FIELD(source_vc, port_id), completion_ctr);
    }
    else
    {
	MPID_nem_gm_module_send_queue_t *e = MPID_nem_gm_module_queue_alloc (send);

	e->type = SEND_TYPE_RDMA;
	e->u.rdma.type = RDMA_TYPE_GET;
	e->u.rdma.target_p = target_p;
	e->u.rdma.source_p = source_p;
	e->u.rdma.len = len;
	e->node_id = VC_FIELD(source_vc, node_id);
	e->port_id = VC_FIELD(source_vc, port_id);
	e->u.rdma.completion_ctr = completion_ctr;
	MPID_nem_gm_module_queue_enqueue (send, e);
    }
    
    return 0;
}
*/

/*
void
MPID_nem_gm_module_do_put (void *target_p, void *source_p, int len, int node_id, int port_id, int *completion_ctr)
{

    --MPID_nem_module_gm_num_send_tokens;
}
*/

/*
int
MPID_nem_gm_module_put (void *target_p, void *source_p, int len, MPIDI_VC_t *target_vc, int *completion_ctr)
{
    int ret;
    
    MPIU_Assert (VC_FIELD(target_vc, node_id) >= 0 && VC_FIELD(target_vc, node_id) < MPID_nem_mem_region.num_procs);
    MPIU_Assert (len >= 0);
    
    if (len == 0)
    {
	MPID_NEM_ATOMIC_DEC (completion_ctr);
	return 0;
    }
    
    if (MPID_nem_module_gm_num_send_tokens)
    {
	MPID_nem_gm_module_do_put (target_p, source_p, len, VC_FIELD(target_vc, node_id), VC_FIELD(target_vc, port_id), completion_ctr);
    }
    else
    {
	MPID_nem_gm_module_send_queue_t *e = MPID_nem_gm_module_queue_alloc (send);

	e->type = SEND_TYPE_RDMA;
	e->u.rdma.type = RDMA_TYPE_PUT;
	e->u.rdma.target_p = target_p;
	e->u.rdma.source_p = source_p;
	e->u.rdma.len = len;
	e->node_id = VC_FIELD(target_vc, node_id);
	e->port_id = VC_FIELD(target_vc, port_id);
	e->u.rdma.completion_ctr = completion_ctr;
	MPID_nem_gm_module_queue_enqueue (send, e);
    }
    return 0;
}
*/
