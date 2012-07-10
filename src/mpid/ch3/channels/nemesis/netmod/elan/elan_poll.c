/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_impl.h"
#include <elan/elan.h>
#include "my_papi_defs.h"

/*
void elan__print_queue(MPID_nem_elan_event_queue_ptr_t qhead, int sens)
{
   MPID_nem_elan_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [ELAN_CELL %i is @ %p]: [EVENT is @%p][CELL PTR is @ %p][NEXT is %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->elan_event,curr->cell_ptr,curr->next);
	curr = curr->next;
	index++;
     }
   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
}
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_elan_send_from_queue( void )
{   
   int mpi_errno = MPI_SUCCESS;
   
   if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
     {	
	MPID_nem_elan_cell_ptr_t elan_event_cell  = NULL;
	int                      num_tries        = MPID_NEM_NUM_CELLS ;
	
	elan_event_cell = MPID_nem_module_elan_pending_event_queue->head ;  	
	while ( num_tries > 0)
	  {
	     if(elan_event_cell->to_proceed)
	       {
		  MPID_nem_pkt_t *pkt  = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET(elan_event_cell->cell_ptr);
		  int             dest = MPID_NEM_CELL_DEST(elan_event_cell->cell_ptr);
		  
		  elan_event_cell->elan_event = 
		    elan_queueTx(rxq_ptr_array[dest],MPID_nem_elan_vpids[dest],(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);
		  elan_event_cell->to_proceed = 0;		  
	       }

	     if(elan_poll(elan_event_cell->elan_event,MPID_NEM_ELAN_LOOPS_SEND) == TRUE)
	       {
		  MPID_nem_elan_cell_ptr_t elan_event_cell2 = NULL;
		  
		  MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell2);
		  MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell2->cell_ptr);	 
		  MPID_NEM_ELAN_RESET_CELL( elan_event_cell2 );
		  MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell2);
		  
		  if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
		    {			    
		       elan_event_cell = MPID_nem_module_elan_pending_event_queue->head ;
		       --num_tries;
		    }		       
		  else
		    {			    
		       goto fn_exit;
		    }		       
	       }
	     else
	       {
		  goto fn_exit;
	       }		  
	  }
     }

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_elan_recv( void )
{
   int mpi_errno = MPI_SUCCESS;
   
   if((!MPID_nem_queue_empty(MPID_nem_module_elan_free_queue)) && 
      (elan_queueRxPoll(mpid_nem_elan_recv_queue_ptr,MPID_NEM_ELAN_LOOPS_RECV) == TRUE ))
     {
	MPID_nem_cell_ptr_t  cell = NULL;
	
	MPID_nem_queue_dequeue (MPID_nem_module_elan_free_queue, &cell);
	elan_queueRxWait(mpid_nem_elan_recv_queue_ptr,(char *)(MPID_NEM_CELL_TO_PACKET (cell)),elan_base->waitType);	     	     
	MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	     	     
     }	
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_poll(int in_blocking_poll)
{
   int mpi_errno = MPI_SUCCESS;
   
   if (MPID_nem_elan_freq > 0)
   {	
       MPID_nem_elan_recv();
       MPID_nem_elan_send_from_queue();
   }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}
