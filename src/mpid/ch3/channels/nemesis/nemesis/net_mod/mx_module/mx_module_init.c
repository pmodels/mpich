/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "myriexpress.h"
#include "mpid_nem_impl.h"
#include "mx_module_impl.h"

MPID_nem_netmod_funcs_t MPIDI_nem_mx_module_funcs = {
    MPID_nem_mx_module_init,
    MPID_nem_mx_module_finalize,
    MPID_nem_mx_module_ckpt_shutdown,
    MPID_nem_mx_module_poll,
    MPID_nem_mx_module_send,
    MPID_nem_mx_module_get_business_card,
    MPID_nem_mx_module_connect_to_root,
    MPID_nem_mx_module_vc_init,
    MPID_nem_mx_module_vc_destroy,
    MPID_nem_mx_module_vc_terminate
};


#define MPIDI_CH3I_ENDPOINT_KEY "endpoint_id"
#define MPIDI_CH3I_NIC_KEY      "nic_id"

static uint64_t local_nic_id;
static uint64_t remote_nic_id;
static uint32_t local_endpoint_id;
static uint32_t remote_endpoint_id;

mx_endpoint_t       MPID_nem_module_mx_local_endpoint;
mx_endpoint_addr_t *MPID_nem_module_mx_endpoints_addr;

MPID_nem_mx_cell_ptr_t MPID_nem_module_mx_send_outstanding_request;
int                    MPID_nem_module_mx_send_outstanding_request_num;
MPID_nem_mx_cell_ptr_t MPID_nem_module_mx_recv_outstanding_request;
int                    MPID_nem_module_mx_recv_outstanding_request_num;

uint32_t        MPID_nem_module_mx_filter  = 0xdeadbeef;
static uint32_t MPID_nem_module_mx_timeout = MX_INFINITE;
int             MPID_nem_module_mx_pendings_sends = 0;
int             MPID_nem_module_mx_pendings_recvs = 0 ;
int            *MPID_nem_module_mx_pendings_sends_array;
int            *MPID_nem_module_mx_pendings_recvs_array;


static MPID_nem_mx_req_queue_t _mx_send_free_req_q;
static MPID_nem_mx_req_queue_t _mx_send_pend_req_q;
static MPID_nem_mx_req_queue_t _mx_recv_free_req_q;
static MPID_nem_mx_req_queue_t _mx_recv_pend_req_q;

MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_free_req_queue    = &_mx_send_free_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_pending_req_queue = &_mx_send_pend_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_free_req_queue    = &_mx_recv_free_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_pending_req_queue = &_mx_recv_pend_req_q;

static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_mx_free_queue = 0;

MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

#undef FUNCNAME
#define FUNCNAME init_mx
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int init_mx( MPIDI_PG_t *pg_p )
{
   mx_return_t ret;
   int         mpi_errno = MPI_SUCCESS;
   int         index;
   MPIU_CHKPMEM_DECL(1);

   ret = mx_init();
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_init", "**mx_init %s", mx_strerror (ret));
   
   /* Allocate more than needed but use only external processes */   
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_mx_endpoints_addr, mx_endpoint_addr_t *, MPID_nem_mem_region.num_procs * sizeof(mx_endpoint_addr_t), mpi_errno, "endpoints addr");   
#if 1
   /* Fix me : mysteriously  MPIU_CHKPMEM_MALLOC fails here, when the regular MPIU_Malloc doesn't ... */
   MPID_nem_module_mx_send_outstanding_request =(MPID_nem_mx_cell_ptr_t)MPIU_Malloc(MPID_NEM_MX_REQ * sizeof(MPID_nem_mx_cell_t));
   MPID_nem_module_mx_recv_outstanding_request =(MPID_nem_mx_cell_ptr_t)MPIU_Malloc(MPID_NEM_MX_REQ * sizeof(MPID_nem_mx_cell_t));
   MPID_nem_module_mx_pendings_recvs_array     =(int *)MPIU_Malloc(MPID_nem_mem_region.num_procs * sizeof(int));
#else 
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_mx_send_outstanding_request, MPID_nem_mx_cell_ptr_t, MPID_NEM_MX_REQ * sizeof(MPID_nem_mx_cell_t), mpi_errno, "send outstanding req");   
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_mx_recv_outstanding_request, MPID_nem_mx_cell_ptr_t, MPID_NEM_MX_REQ * sizeof(MPID_nem_mx_cell_t), mpi_errno, "recv outstanding req");   
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_mx_pendings_recvs_array,int *, MPID_nem_mem_region.num_procs * sizeof(int), mpi_errno, "pending recvs array");
#endif 
   memset(MPID_nem_module_mx_send_outstanding_request,0,MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));  
   memset(MPID_nem_module_mx_recv_outstanding_request,0,MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));     
   for (index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
     {
	MPID_nem_module_mx_pendings_recvs_array[index] = 0;
     }         
   /*
   ret = mx_get_info(NULL, MX_NIC_COUNT, NULL, 0, &nic_count, sizeof(int));
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_info", "**mx_get_info %s", mx_strerror (ret));
   
   count = ++nic_count;
   mx_nics = (uint64_t *)MPIU_Malloc(count*sizeof(uint64_t));
   ret = mx_get_info(NULL, MX_NIC_IDS, NULL, 0, mx_nics, count*sizeof(uint64_t));
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_info", "**mx_get_info %s", mx_strerror (ret));
    
    do{	     
      ret = mx_nic_id_to_board_number(mx_nics[index],&mx_board_num);
      index++;
   }while(ret != MX_SUCCESS);
   */
   
   /* mx_board_num */
   ret = mx_open_endpoint(MX_ANY_NIC,MX_ANY_ENDPOINT,MPID_nem_module_mx_filter,NULL,0,&MPID_nem_module_mx_local_endpoint);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_open_endpoint", "**mx_open_endpoint %s", mx_strerror (ret));
      
   ret = mx_get_endpoint_addr( MPID_nem_module_mx_local_endpoint,&MPID_nem_module_mx_endpoints_addr[MPID_nem_mem_region.rank]);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_get_endpoint_addr", "**mx_get_endpoint_addr %s", mx_strerror (ret));   
   
   ret = mx_decompose_endpoint_addr(MPID_nem_module_mx_endpoints_addr[MPID_nem_mem_region.rank],&local_nic_id, &local_endpoint_id);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_decompose_endpoint_addr", "**mx_decompose_endpoint_addr %s", mx_strerror (ret));
   
   MPID_nem_module_mx_send_free_req_queue->head    = NULL;
   MPID_nem_module_mx_send_free_req_queue->tail    = NULL;
   MPID_nem_module_mx_send_pending_req_queue->head = NULL;
   MPID_nem_module_mx_send_pending_req_queue->tail = NULL;
   
   MPID_nem_module_mx_recv_free_req_queue->head    = NULL;
   MPID_nem_module_mx_recv_free_req_queue->tail    = NULL;
   MPID_nem_module_mx_recv_pending_req_queue->head = NULL;
   MPID_nem_module_mx_recv_pending_req_queue->tail = NULL;
   
   for (index = 0; index < MPID_NEM_MX_REQ ; ++index)
     {
	MPID_nem_mx_req_queue_enqueue (MPID_nem_module_mx_send_free_req_queue,&MPID_nem_module_mx_send_outstanding_request[index]);
	MPID_nem_mx_req_queue_enqueue (MPID_nem_module_mx_recv_free_req_queue,&MPID_nem_module_mx_recv_outstanding_request[index]);
     }

   MPIU_CHKPMEM_COMMIT();
   fn_exit:
     return mpi_errno;
   fn_fail:
     MPIU_CHKPMEM_REAP();
     goto fn_exit;
}

/*
 int  
   MPID_nem_mx_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
	          MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		  MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;
   int index;
   
   if( MPID_nem_mem_region.ext_procs > 0)
     {
	init_mx(pg_p);
	mpi_errno = MPID_nem_mx_module_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
	if (mpi_errno) MPIU_ERR_POP (mpi_errno);
     }
   
   MPID_nem_process_recv_queue = proc_recv_queue;
   MPID_nem_process_free_queue = proc_free_queue;
   
   MPID_nem_module_mx_free_queue = &_free_queue;
   
   MPID_nem_queue_init (MPID_nem_module_mx_free_queue);
   
   for (index = 0; index < num_module_elements; ++index)
     {
	MPID_nem_queue_enqueue (MPID_nem_module_mx_free_queue, &module_elements[index]);
     }
   
   *module_free_queue = MPID_nem_module_mx_free_queue;

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
   int mpi_errno = MPI_SUCCESS;

   mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ENDPOINT_KEY, local_endpoint_id);
   if (mpi_errno != MPIU_STR_SUCCESS)
     {
	if (mpi_errno == MPIU_STR_NOMEM) 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }
	else 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }
	goto fn_exit;
     }

   mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_NIC_KEY, (char *)&local_nic_id, sizeof(uint64_t));
   if (mpi_errno != MPIU_STR_SUCCESS)
     {
	if (mpi_errno == MPIU_STR_NOMEM) 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }
	else 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }
	goto fn_exit;
     }
   
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_get_from_bc (const char *business_card, uint32_t *remote_endpoint_id, uint64_t *remote_nic_id)
{
   int mpi_errno = MPI_SUCCESS;
   int len;
   uint32_t tmp_endpoint_id;
   
   mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_ENDPOINT_KEY, &tmp_endpoint_id);
   if (mpi_errno != MPIU_STR_SUCCESS) 
     {
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   *remote_endpoint_id = (uint32_t)tmp_endpoint_id;
   
   mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_NIC_KEY, (char *)remote_nic_id, sizeof(uint64_t), &len);
   if ((mpi_errno != MPIU_STR_SUCCESS) || len != sizeof(uint64_t)) 
     {	
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   
   fn_exit:
     return mpi_errno;
   fn_fail:  
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
   int mpi_errno = MPI_SUCCESS;
   int ret;
   
   /* first make sure that our private fields in the vc fit into the area provided  */
   MPIU_Assert(sizeof(MPID_nem_mx_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

   if( MPID_nem_mem_region.ext_procs > 0)
     {
	mpi_errno = MPID_nem_mx_module_get_from_bc (business_card, &VC_FIELD(vc, remote_endpoint_id), &VC_FIELD(vc, remote_nic_id));
	/* --BEGIN ERROR HANDLING-- */   
	if (mpi_errno) 
	  {	
	     MPIU_ERR_POP (mpi_errno);
	  }   
	/* --END ERROR HANDLING-- */
	
	ret = mx_connect(MPID_nem_module_mx_local_endpoint,
			 VC_FIELD(vc, remote_nic_id),
			 VC_FIELD(vc, remote_endpoint_id),
			 MPID_nem_module_mx_filter,
			 MPID_nem_module_mx_timeout,
			 &MPID_nem_module_mx_endpoints_addr[vc->pg_rank]);
	MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_connect", "**mx_connect %s", mx_strerror (ret));
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    /* free any resources associated with this VC here */

 fn_exit:   
       return mpi_errno;
 fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_module_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}
