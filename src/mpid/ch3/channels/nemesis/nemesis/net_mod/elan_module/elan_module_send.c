/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include <elan/elan.h>
#include "elan_module.h"
#include "my_papi_defs.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_elan_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{   
   MPID_nem_elan_cell_ptr_t elan_event_cell;
   int                      mpi_errno  = MPI_SUCCESS;
   
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);
   if ( MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
     {
	ELAN_EVENT     *elan_event_ptr = NULL; 
	MPID_nem_pkt_t *pkt            = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */
	int             dest           = vc->lpid;
	
	
	elan_event_ptr =
	  elan_queueTx(rxq_ptr_array[dest],MPID_nem_elan_vpids[dest],(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);
	//elan_queueTx(VC_FIELD(vc, rxq_ptr_array)[dest],MPID_nem_elan_vpids[dest],(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);

	if (elan_poll(elan_event_ptr,MPID_NEM_ELAN_LOOPS_SEND) == TRUE)
	  {
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,cell);	     
	  }
	else
	  {		  
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_free_event_queue,&elan_event_cell);
	     MPID_NEM_ELAN_SET_CELL( elan_event_cell , elan_event_ptr , cell, 0);
	     MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_pending_event_queue,elan_event_cell);
	  }
     }
   else 
     {
	if (!MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_free_event_queue))
	  {
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_free_event_queue,&elan_event_cell);
	  }	
	else
	  {
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
	     if (elan_event_cell->to_proceed)
	       {
		  MPID_nem_pkt_t *pkt  = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (elan_event_cell->cell_ptr); /* cast away volatile */
		  int             dest = vc->lpid;

		  elan_event_cell->elan_event = 
		    elan_queueTx(rxq_ptr_array[dest],MPID_nem_elan_vpids[dest],(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);
		  //elan_queueTx(VC_FIELD(vc, rxq_ptr_array)[dest],MPID_nem_elan_vpids[dest],(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);
	       }
	     elan_wait(elan_event_cell->elan_event,elan_base->waitType);
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);	     	     
	  }		
	MPID_NEM_ELAN_SET_CELL( elan_event_cell , NULL , cell, 1);
	MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_pending_event_queue,elan_event_cell);	
     }   
   
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;   
}
