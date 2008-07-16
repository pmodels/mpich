 /* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "psm_module_impl.h"
#include "psm.h"
#include "psm_module.h"
#include "my_papi_defs.h"

/*
void psm__print_queue(MPID_nem_psm_req_queue_ptr_t qhead, int sens)
{
   MPID_nem_psm_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [CELL %i @%p]: [REQUEST is %i @%p][NEXT @ %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->psm_request,&(curr->psm_request),curr->next);
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
#define FUNCNAME MPID_nem_psm_module_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_psm_module_send_from_queue()
{
   int         mpi_errno = MPI_SUCCESS;
   psm_error_t ret;
   psm_mq_status_t status;
   uint32_t    result;
   
   if (MPID_nem_module_psm_pendings_sends > 0)   
   {	
       MPID_nem_psm_cell_ptr_t curr_cell = MPID_nem_module_psm_send_pending_req_queue->head;
       psm_mq_req_t *req;

       while( curr_cell != NULL )
       {
           req = MPID_NEM_PSM_CELL_TO_REQUEST(curr_cell);

           ret = psm_poll(MPID_nem_module_psm_local_endpoint);
           MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_OK_NO_PROGRESS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
           
           ret = psm_mq_test(req, &status);
           
           MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
           if((ret == PSM_OK) && (*req == PSM_MQ_REQINVALID))
           {
               MPID_nem_psm_cell_ptr_t cell;
               MPID_nem_psm_req_queue_dequeue(MPID_nem_module_psm_send_pending_req_queue,&cell);
               MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_send_free_req_queue,cell);
               MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context);		  
               MPID_nem_module_psm_pendings_sends--;
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
#define FUNCNAME MPID_nem_psm_module_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_psm_module_recv()
{
   int                 mpi_errno = MPI_SUCCESS;
   psm_error_t         ret;
   psm_mq_status_t         status;
   uint32_t            result;
   
   if (MPID_nem_module_psm_recv_outstanding_request_num > 0)
     {
	MPID_nem_psm_cell_ptr_t curr_cell = MPID_nem_module_psm_recv_pending_req_queue->head;
        psm_mq_req_t *req;
        
	
	while( curr_cell != NULL )
	  {
              req = MPID_NEM_PSM_CELL_TO_REQUEST(curr_cell);
              
              ret = psm_poll(MPID_nem_module_psm_local_endpoint);
              MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_OK_NO_PROGRESS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
              
              ret = psm_mq_test(req, &status);
              MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_MQ_NO_COMPLETIONS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
              if((ret == PSM_OK) && (*req == PSM_MQ_REQINVALID))
              {		  
                  MPID_nem_psm_cell_ptr_t cell_req;
                  MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, (MPID_nem_cell_ptr_t)status.context);		      
                  MPID_nem_psm_req_queue_dequeue(MPID_nem_module_psm_recv_pending_req_queue,&cell_req);
                  MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_recv_free_req_queue,cell_req);
                  MPID_nem_module_psm_recv_outstanding_request_num--;
                  curr_cell = curr_cell->next;	     		  
              }
              else
              {
                  goto next_step;
              }
	  }
     }
   
   next_step:   
   if (MPID_nem_module_psm_recv_outstanding_request_num == 0)
   {	
       MPID_nem_cell_ptr_t cell = NULL;	
       if (!MPID_nem_queue_empty(MPID_nem_module_psm_free_queue) && !MPID_nem_psm_req_queue_empty(MPID_nem_module_psm_recv_free_req_queue))
       {	
           MPID_nem_psm_cell_ptr_t cell_req;		       
           psm_mq_req_t            *request;
           uint32_t                num_seg ;
	   
           MPID_nem_psm_req_queue_dequeue(MPID_nem_module_psm_recv_free_req_queue,&cell_req);	     
           request = MPID_NEM_PSM_CELL_TO_REQUEST(cell_req);		
	   
           MPID_nem_queue_dequeue (MPID_nem_module_psm_free_queue, &cell);	    	     
	   
           ret = psm_mq_irecv(MPID_nem_module_psm_mq, 
                           MQ_TAG,
                           MQ_TAGSEL_ALL,		       
                           MQ_FLAGS_NONE,
                           MPID_NEM_CELL_TO_PACKET (cell),
                           MPID_NEM_CELL_PAYLOAD_LEN,
                           (void *)cell,
                           request);
           MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_irecv", "**psm_irecv %s", psm_error_get_string (ret));

           ret = psm_poll(MPID_nem_module_psm_local_endpoint);
           MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_OK_NO_PROGRESS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
           ret = psm_mq_test(request, &status);
           
           MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_MQ_NO_COMPLETIONS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
           if((ret == PSM_OK) && (*request == PSM_MQ_REQINVALID))
           {	     
               MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);
               MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_recv_free_req_queue,cell_req);
           }
           else 
           {
               MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_recv_pending_req_queue,cell_req);
               MPID_nem_module_psm_recv_outstanding_request_num++;
           }	
       }
   } 
 fn_exit:
   return mpi_errno;
 fn_fail:
   goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_poll(MPID_nem_poll_dir_t in_or_out)
{
   int mpi_errno = MPI_SUCCESS;
   int ret;
   
   
   if (!MPID_nem_module_psm_connected)
   {
       ret = MPID_nem_psm_module_connect();
       MPIU_ERR_CHKANDJUMP1 (ret != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**psm_module_connect", "**psm_module_connect %d", ret);
   }
   
   
   if (in_or_out == MPID_NEM_POLL_OUT)
     {
	MPID_nem_psm_module_send_from_queue();
	MPID_nem_psm_module_recv();
     }
   else
     {
	MPID_nem_psm_module_recv();
	MPID_nem_psm_module_send_from_queue();
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}
