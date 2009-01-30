/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_impl.h"
#include "myriexpress.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_mx_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
   MPID_nem_mx_cell_ptr_t cell_req;
   mx_request_t          *request;   
   mx_segment_t           seg;   
   mx_return_t            ret;	   
   mx_status_t            status;
   uint32_t               result;
   int                    data_size;
   int                    dest = vc->lpid;
   int                    mpi_errno = MPI_SUCCESS;
   
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);
   
   if ( MPID_nem_mx_req_queue_empty(MPID_nem_module_mx_send_free_req_queue))
     {
	MPID_nem_mx_cell_ptr_t curr_cell = MPID_nem_module_mx_send_pending_req_queue->head;
	ret = mx_wait(MPID_nem_module_mx_local_endpoint,
		      MPID_NEM_MX_CELL_TO_REQUEST(curr_cell),
		      MX_INFINITE,
		      &status,
		      &result);
	MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_wait", "**mx_wait %s", mx_strerror (ret));
	if((result != 0) && (status.code == MX_STATUS_SUCCESS))
	  {	
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context); 
	     MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_send_pending_req_queue,&cell_req);	     	   
	     MPID_nem_module_mx_pendings_sends--;
	     goto regular_step;
	  }
     }
   else     
     {
	MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_send_free_req_queue,&cell_req);	     
	regular_step:	
	  {	     
	     request   = MPID_NEM_MX_CELL_TO_REQUEST(cell_req);
	     data_size = datalen + MPID_NEM_MPICH2_HEAD_LEN;	     
	     seg.segment_ptr    = (void *)(MPID_NEM_CELL_TO_PACKET (cell));	       	
	     seg.segment_length = data_size ;  
	     
	     ret = mx_isend(MPID_nem_module_mx_local_endpoint,
			    &seg,1,
			    MPID_nem_module_mx_endpoints_addr[dest],
			    MPID_NEM_MX_MATCH,
			    (void *)cell,
			    request);	
	     MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_isend", "**mx_isend %s", mx_strerror (ret));
	     	     
	     if(MPID_nem_module_mx_pendings_sends == 0)
	       {	
		  ret = mx_test(MPID_nem_module_mx_local_endpoint,
				request,
				&status,
				&result);
		  MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_test", "**mx_test %s", mx_strerror (ret));		  				  
		  if((result != 0) && (status.code == MX_STATUS_SUCCESS))		    
		    {	
		       MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context);
		       MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_free_req_queue,cell_req);
		    }  
		  else
		    {
		       MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_pending_req_queue,cell_req);
		       MPID_nem_module_mx_pendings_sends++;
		    }  
	       }	
	     else
	       {
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_pending_req_queue,cell_req);
		  MPID_nem_module_mx_pendings_sends++;
	       }	
	  }	
     }   
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;   
}
