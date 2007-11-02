/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "psm_module_impl.h"
#include "psm.h"
#include "psm_module.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_psm_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    MPID_nem_psm_cell_ptr_t cell_req;
    psm_mq_req_t            *request;   
    //psm_segment_t         seg;   
    psm_error_t             ret;	   
    psm_mq_status_t         status;
    uint32_t                result;
    int                     data_size;
    int                     dest = vc->lpid;
    int                     mpi_errno = MPI_SUCCESS;
    
    MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);

    if (!MPID_nem_module_psm_connected)
    {
        ret = MPID_nem_psm_module_connect();
        MPIU_ERR_CHKANDJUMP1 (ret != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**psm_module_connect", "**psm_module_connect %d", ret);
    }

    
    if ( MPID_nem_psm_req_queue_empty(MPID_nem_module_psm_send_free_req_queue))
    {
	MPID_nem_psm_cell_ptr_t curr_cell = MPID_nem_module_psm_send_pending_req_queue->head;
        psm_mq_req_t *req;

        req = MPID_NEM_PSM_CELL_TO_REQUEST(curr_cell);

        ret = psm_poll(MPID_nem_module_psm_local_endpoint);
        MPIU_ERR_CHKANDJUMP1 ((ret != PSM_OK && ret != PSM_OK_NO_PROGRESS), mpi_errno,
                                 MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
           
        ret = psm_mq_test(req, &status);
	//ret = psm_mq_wait(req, &status);
        
	//MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_wait", "**psm_wait %s", psm_error_get_string (ret));
	if((ret == PSM_OK) && (*req == PSM_MQ_REQINVALID))
        {	
            MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context); 
            MPID_nem_psm_req_queue_dequeue(MPID_nem_module_psm_send_pending_req_queue,&cell_req);	     	   
            MPID_nem_module_psm_pendings_sends--;
            goto regular_step;
        }
    }
    else     
    {
/*         char buf[1024]; */
/*         int i; */

/*         for(i=0;i<1024;i++) */
/*             buf[i] = 'G'; */

        MPID_nem_psm_req_queue_dequeue(MPID_nem_module_psm_send_free_req_queue,&cell_req);	     
    regular_step:	
        {	     
            request   = MPID_NEM_PSM_CELL_TO_REQUEST(cell_req);
            data_size = datalen + MPID_NEM_MPICH2_HEAD_LEN;
            //strcpy(buf, (&cell)->pkt);
            
            //seg.segment_ptr    = (void *)(MPID_NEM_CELL_TO_PACKET (cell));	       	
            //seg.segment_length = data_size ;  
	    
            ret = psm_mq_isend(MPID_nem_module_psm_mq,
			    MPID_nem_module_psm_endpoint_addrs[dest],
			    MQ_FLAGS_NONE,
                            MQ_TAG,
                            MPID_NEM_CELL_TO_PACKET (cell),
                            data_size,
			    (void *)cell,
			    request);
            
/*             ret = psm_mq_send(MPID_nem_module_psm_mq, */
/*                               MPID_nem_module_psm_endpoint_addrs[dest], */
/*                               MQ_FLAGS_NONE, */
/*                               MQ_TAG, */
/*                               buf, */
/*                               data_size); */
            
            MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_isend", "**psm_isend %s", psm_error_get_string (ret));
            
            if(MPID_nem_module_psm_pendings_sends == 0)
            {	
                ret = psm_mq_test(request, &status);

                //MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_test", "**psm_test %s", psm_error_get_string (ret));
                if((ret == PSM_OK) && (*request == PSM_MQ_REQINVALID))		    
                {
                    cell_req->psm_request = *request;
                    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context);
                    MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_send_free_req_queue,cell_req);
                }  
                else
                {
                    cell_req->psm_request = *request;
                    MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_send_pending_req_queue,cell_req);
                    MPID_nem_module_psm_pendings_sends++;
                }  
            }	
            else
            {
                MPID_nem_psm_req_queue_enqueue(MPID_nem_module_psm_send_pending_req_queue,cell_req);
                MPID_nem_module_psm_pendings_sends++;
            }	
        }	
    }   
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;   
}
