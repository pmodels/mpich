/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_impl.h"
#include "myriexpress.h"
#include "my_papi_defs.h"

/*
void mx__print_queue(MPID_nem_mx_req_queue_ptr_t qhead, int sens)
{
   MPID_nem_mx_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [CELL %i @%p]: [REQUEST is %i @%p][NEXT @ %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->mx_request,&(curr->mx_request),curr->next);
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
#define FUNCNAME MPID_nem_mx_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_mx_send_from_queue()
{
   int         mpi_errno = MPI_SUCCESS;
   mx_return_t ret;
   mx_status_t status;
   uint32_t    result;
   
   if (MPID_nem_module_mx_pendings_sends > 0)   
     {	
	MPID_nem_mx_cell_ptr_t curr_cell = MPID_nem_module_mx_send_pending_req_queue->head;
	while( curr_cell != NULL )
	  {
	     ret = mx_test(MPID_nem_module_mx_local_endpoint,
			   MPID_NEM_MX_CELL_TO_REQUEST(curr_cell),
			   &status,
			   &result);
	     MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_test", "**mx_test %s", mx_strerror (ret));
	     if((result != 0) && (status.code == MX_STATUS_SUCCESS))
	       {
		  MPID_nem_mx_cell_ptr_t cell;
		  MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_send_pending_req_queue,&cell);
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_free_req_queue,cell);
		  MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context);		  
		  MPID_nem_module_mx_pendings_sends--;
		  curr_cell = curr_cell->next;
	       }
	     else
	       {
		  return;
	       }
	  }	
     }
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_mx_recv()
{
   int                 mpi_errno = MPI_SUCCESS;
   mx_segment_t        seg;
   mx_return_t         ret;
   mx_status_t         status;
   uint32_t            result;
   
   if (MPID_nem_module_mx_recv_outstanding_request_num > 0)
     {
	MPID_nem_mx_cell_ptr_t curr_cell = MPID_nem_module_mx_recv_pending_req_queue->head;
	
	while( curr_cell != NULL )
	  {	
	     ret = mx_test(MPID_nem_module_mx_local_endpoint,
			   MPID_NEM_MX_CELL_TO_REQUEST(curr_cell),
			   &status,
			   &result);
	     MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_test", "**mx_test %s", mx_strerror (ret));
	     if((result != 0) && (status.code == MX_STATUS_SUCCESS))
	       {		  
		  MPID_nem_mx_cell_ptr_t cell_req;
		  MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, (MPID_nem_cell_ptr_t)status.context);		      
		  MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_recv_pending_req_queue,&cell_req);
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_recv_free_req_queue,cell_req);
		  MPID_nem_module_mx_recv_outstanding_request_num--;
		  curr_cell = curr_cell->next;	     		  
	       }
	     else
	       {
		  goto next_step;
	       }
	  }
     }
   
   next_step:   
   if (MPID_nem_module_mx_recv_outstanding_request_num == 0)
     {	
	MPID_nem_cell_ptr_t cell = NULL;	
	if (!MPID_nem_queue_empty(MPID_nem_module_mx_free_queue) && !MPID_nem_mx_req_queue_empty(MPID_nem_module_mx_recv_free_req_queue))
	  {	
	     MPID_nem_mx_cell_ptr_t cell_req;		       
	     mx_request_t          *request;
	     uint32_t               num_seg ;
	     
	     MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_recv_free_req_queue,&cell_req);	     
	     request = MPID_NEM_MX_CELL_TO_REQUEST(cell_req);		
	     
	     MPID_nem_queue_dequeue (MPID_nem_module_mx_free_queue, &cell);	    	     
	     seg.segment_ptr    = (void *)(MPID_NEM_CELL_TO_PACKET (cell));
	     seg.segment_length = MPID_NEM_CELL_PAYLOAD_LEN ;
	     
	     ret = mx_irecv(MPID_nem_module_mx_local_endpoint,
			    &seg,1,		       
			    MPID_NEM_MX_MATCH,
			    MPID_NEM_MX_MASK,
			    (void *)cell,
			    request);
	     MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_irecv", "**mx_irecv %s", mx_strerror (ret));	       	     

	     
	     ret = mx_test(MPID_nem_module_mx_local_endpoint,
				request,
				&status,
				&result);
	     MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_test", "**mx_test %s", mx_strerror (ret));
	     if((result != 0) && (status.code == MX_STATUS_SUCCESS))
	       {	     
		  MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_recv_free_req_queue,cell_req);
	       }
	     else 
	       {
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_recv_pending_req_queue,cell_req);
		  MPID_nem_module_mx_recv_outstanding_request_num++;
	       }	
	  }
     } 
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_poll(MPID_nem_poll_dir_t in_or_out)
{
   int mpi_errno = MPI_SUCCESS;
   
   if (in_or_out == MPID_NEM_POLL_OUT)
     {
	MPID_nem_mx_send_from_queue();
	MPID_nem_mx_recv();
     }
   else
     {
	MPID_nem_mx_recv();
	MPID_nem_mx_send_from_queue();
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}
