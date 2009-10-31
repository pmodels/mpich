/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_impl.h"

#define DO_PAPI3(x) /*x */

static void
send_callback (struct gm_port *p, void *context, gm_status_t status)
{
    MPID_nem_cell_t *cell = (MPID_nem_cell_t *)context;

    if (status != GM_SUCCESS)
    {
	gm_perror ("Send error", status);
    }

    ++MPID_nem_module_gm_num_send_tokens;

    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
}

/*
  requires that pkt is in registered memory, and there are sufficient tokens
 */
static inline int
send_cell (int node_id, int port_id, MPID_nem_cell_ptr_t cell, int datalen)
{
    MPID_nem_pkt_t *pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell);

    MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    gm_send_with_callback (MPID_nem_module_gm_port, pkt, PACKET_SIZE, datalen + MPID_NEM_MPICH2_HEAD_LEN, GM_LOW_PRIORITY, node_id,
			   port_id, send_callback, (void *)cell);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues4));
    return MPI_SUCCESS;
}

/* #define BOUNCE_BUFFER */
#undef FUNCNAME
#define FUNCNAME MPID_nem_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_send_from_queue()
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_gm_send_queue_t *e;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SEND_FROM_QUEUE);
#ifdef BOUNCE_BUFFER
    static MPID_nem_cell_t c;
    static int first = 1;

    if (first)
    {
	first = 0;
	gm_register_memory (MPID_nem_module_gm_port, &c, sizeof (c));
    }
#endif /* BOUNCE_BUFFER */
/*     MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SEND_FROM_QUEUE);     */
    
    while (!MPID_nem_gm_queue_empty (send) && MPID_nem_module_gm_num_send_tokens)
    {
	MPID_nem_gm_queue_dequeue (send, &e);

	switch (e->type)
	{
	case SEND_TYPE_CELL:
	    mpi_errno = send_cell (e->node_id, e->port_id, e->u.cell, e->u.cell->pkt.mpich2.datalen);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	    --MPID_nem_module_gm_num_send_tokens;
            MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Sent pkt from queue len=%d\n", e->u.cell->pkt.mpich2.datalen);
	    break;
	case SEND_TYPE_RDMA:
	    switch (e->u.rdma.type)
	    {
	    case RDMA_TYPE_GET:
		mpi_errno = MPID_nem_gm_do_get (e->u.rdma.target_p, e->u.rdma.source_p, e->u.rdma.len, e->node_id, e->port_id,
                                                       e->u.rdma.completion_ctr);
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                break;
	    case RDMA_TYPE_PUT:
		mpi_errno = MPID_nem_gm_do_put (e->u.rdma.target_p, e->u.rdma.source_p, e->u.rdma.len, e->node_id, e->port_id,
                                                       e->u.rdma.completion_ctr);
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                break;
	    default:
                MPIU_ERR_INTERNALANDJUMP(mpi_errno, "unexpected rdma type");
		break;
	    }
	    break;
	default:
            MPIU_ERR_INTERNALANDJUMP(mpi_errno, "unexpected operation type");
	    break;
	}
	MPID_nem_gm_queue_free (send, e);
    }
 fn_exit:
/*     MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SEND_FROM_QUEUE); */
   return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_GM_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_GM_SEND);    
    
    DO_PAPI3 (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_module_gm_num_send_tokens)
    {
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	mpi_errno = send_cell (VC_FIELD(vc, gm_node_id), VC_FIELD(vc, gm_port_id), cell, datalen);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues16));
	--MPID_nem_module_gm_num_send_tokens;
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sent pkt dst=%d len=%d\n", vc->pg_rank, datalen));
    }
    else
    {
	MPID_nem_gm_send_queue_t *e;

	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	e = MPID_nem_gm_queue_alloc (send);
	e->node_id = VC_FIELD(vc, gm_node_id);
	e->port_id = VC_FIELD(vc, gm_port_id);
	e->type = SEND_TYPE_CELL;
	e->u.cell = (MPID_nem_cell_t *)cell;
	
	cell->pkt.mpich2.source = MPID_nem_mem_region.rank;
	cell->pkt.mpich2.datalen = datalen;
	
	MPID_nem_gm_queue_enqueue (send, e);
	mpi_errno = MPID_nem_send_from_queue();
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues17));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Enqueued pkt dst=%d len=%d\n", vc->pg_rank, datalen));
    }    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_GM_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
