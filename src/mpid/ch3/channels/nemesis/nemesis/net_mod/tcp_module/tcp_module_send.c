/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_module_impl.h"

#define DO_PAPI3(x) /*x */

/* #define TRACE */

#undef FUNCNAME
#define FUNCNAME send_cell
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int
send_cell (int dest, MPID_nem_cell_ptr_t cell, int datalen)
{
    int             mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_t *pkt       = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */
    int             len       = MPID_NEM_PACKET_OPT_LEN(pkt);
    int             offset    = 0;
    node_t         *nodes     = MPID_nem_tcp_internal_vars.nodes ;

    MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN + MPID_NEM_MPICH2_HEAD_LEN);

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    do
    {
        offset = write(nodes[dest].desc, pkt,len);
    }
    while (offset == -1 && errno == EINTR);

    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues4));

    if( offset == len )
    {
	nodes[dest].left2write = 0;
#ifdef TRACE
	{
            int index;
            fprintf(stderr,"[%i] -- TCP DIRECT SEND : sent ALL MSG (%i len, offset %i, payload is %i , datalen %i)\n",
                    MPID_nem_mem_region.rank, len, offset, pkt->mpich2.datalen,datalen);
            for(index = 0 ; index < ((pkt->mpich2.datalen)/sizeof(int)); index ++)
            {
		fprintf(stderr,"[%i] --- cell[%i] : %i\n",MPID_nem_mem_region.rank,index,((int *)&(cell->pkt.mpich2))[index] );
            }

	}
#endif
	MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
    }
    else if (offset != -1)
    {
#ifdef TRACE
	fprintf(stderr,"[%i] -- TCP DIRECT SEND : sent PARTIAL  MSG (%i offset, payload is %i)\n",MPID_nem_mem_region.rank,offset, pkt->mpich2.datalen);
#endif

	cell->pkt.mpich2.source = MPID_nem_mem_region.rank;
	cell->pkt.mpich2.dest   = dest;
	nodes[dest].left2write  = offset;
	MPID_nem_tcp_internal_queue_enqueue (&nodes[dest].internal_recv_queue, cell);
	MPID_nem_tcp_internal_vars.n_pending_send++;
        MPID_nem_tcp_internal_vars.n_pending_sends[dest]++;
    }
    else
    {
	if (errno == EAGAIN)
	{

#ifdef TRACE
	    fprintf(stderr,"[%i] -- TCP DIRECT SEND : Direct EnQ \n",MPID_nem_mem_region.rank );
#endif
	    cell->pkt.mpich2.source = MPID_nem_mem_region.rank;
	    cell->pkt.mpich2.dest   = dest;
	    nodes[dest].left2write  = 0;
	    MPID_nem_tcp_internal_queue_enqueue (&nodes[dest].internal_recv_queue, cell);
	    MPID_nem_tcp_internal_vars.n_pending_send++;
	    MPID_nem_tcp_internal_vars.n_pending_sends[dest]++;
	}
	else
	{
            /* write() returned an error */
            MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror (errno));
	}
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    int dest = vc->lpid;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_SEND);

    DO_PAPI3 (PAPI_reset (PAPI_EventSet));
    cell->pkt.mpich2.datalen = datalen;
    if (  MPID_nem_tcp_internal_vars.n_pending_sends[dest] == 0 )
    {
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	mpi_errno = send_cell (dest, cell, datalen);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues16));
    }
    else
    {
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	cell->pkt.mpich2.source  = MPID_nem_mem_region.rank;
	cell->pkt.mpich2.dest    = dest;
	MPID_nem_tcp_internal_queue_enqueue ( &(MPID_nem_tcp_internal_vars.nodes)[dest].internal_recv_queue, cell);
        MPID_nem_tcp_internal_vars.n_pending_send++;
        MPID_nem_tcp_internal_vars.n_pending_sends[dest]++;
	mpi_errno = MPID_nem_tcp_module_poll_send();
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues17));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
