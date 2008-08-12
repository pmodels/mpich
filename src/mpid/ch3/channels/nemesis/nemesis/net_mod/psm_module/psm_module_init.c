/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "psm_module_impl.h"
#include "psm.h"
#include "psm_mq.h"
#include "mpid_nem_impl.h"
#include "psm_module.h"

MPID_nem_netmod_funcs_t MPIDI_nem_psm_module_funcs = {
    MPID_nem_psm_module_init,
    MPID_nem_psm_module_finalize,
    MPID_nem_psm_module_ckpt_shutdown,
    MPID_nem_psm_module_poll,
    MPID_nem_psm_module_send,
    MPID_nem_psm_module_get_business_card,
    MPID_nem_psm_module_connect_to_root,
    MPID_nem_psm_module_vc_init,
    MPID_nem_psm_module_vc_destroy,
    MPID_nem_psm_module_vc_terminate
};


#define MPIDI_CH3I_ENDPOINT_KEY "endpoint_id"
#define MPIDI_CH3I_NIC_KEY      "nic_id"
#define MPIDI_CH3I_UUID_KEY     "uuid"

psm_epid_t       MPID_nem_module_psm_local_endpoint_id;
psm_ep_t         MPID_nem_module_psm_local_endpoint;

psm_epaddr_t    *MPID_nem_module_psm_endpoint_addrs;
psm_epid_t      *MPID_nem_module_psm_endpoint_ids;

psm_uuid_t       MPID_nem_module_psm_uuid;
psm_mq_t         MPID_nem_module_psm_mq;

int              MPID_nem_module_psm_connected=0;
static int       MPID_nem_module_psm_initialized=0;



MPID_nem_psm_cell_ptr_t MPID_nem_module_psm_send_outstanding_request;
int                     MPID_nem_module_psm_send_outstanding_request_num;
MPID_nem_psm_cell_ptr_t MPID_nem_module_psm_recv_outstanding_request;
int                     MPID_nem_module_psm_recv_outstanding_request_num;

uint64_t        MPID_nem_module_psm_timeout = 0;
int             MPID_nem_module_psm_pendings_sends = 0;
int             MPID_nem_module_psm_pendings_recvs = 0 ;
int            *MPID_nem_module_psm_pendings_sends_array;
int            *MPID_nem_module_psm_pendings_recvs_array;


static MPID_nem_psm_req_queue_t _psm_send_free_req_q;
static MPID_nem_psm_req_queue_t _psm_send_pend_req_q;
static MPID_nem_psm_req_queue_t _psm_recv_free_req_q;
static MPID_nem_psm_req_queue_t _psm_recv_pend_req_q;

MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_send_free_req_queue    = &_psm_send_free_req_q;
MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_send_pending_req_queue = &_psm_send_pend_req_q;
MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_recv_free_req_queue    = &_psm_recv_free_req_q;
MPID_nem_psm_req_queue_ptr_t MPID_nem_module_psm_recv_pending_req_queue = &_psm_recv_pend_req_q;

static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_psm_free_queue = 0;

MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

#undef FUNCNAME
#define FUNCNAME init_psm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int init_psm( MPIDI_PG_t *pg_p )
{
   int         mpi_errno = MPI_SUCCESS;
   int         pmi_errno;
   
   int         index;
   int         verno_minor = PSM_VERNO_MINOR;
   int         verno_major = PSM_VERNO_MAJOR;
   psm_error_t ret;

   char        *kvs_name;
   
   char        key[MPID_NEM_MAX_KEY_VAL_LEN], get_key[MPID_NEM_MAX_KEY_VAL_LEN];
   char        val[MPID_NEM_MAX_KEY_VAL_LEN], get_val[MPID_NEM_MAX_KEY_VAL_LEN];
   int         len = MPID_NEM_MAX_KEY_VAL_LEN;
   
   MPIU_CHKPMEM_DECL(1);

   /* Fix me : mysteriously  MPIU_CHKPMEM_MALLOC fails here, when the regular MPIU_Malloc doesn't ... */
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_psm_endpoint_addrs, psm_epaddr_t *, MPID_nem_mem_region.num_procs * sizeof(psm_epaddr_t), mpi_errno, "endpoints addr");
   MPID_nem_module_psm_endpoint_ids = (psm_epid_t *)MPIU_Malloc(MPID_nem_mem_region.num_procs * sizeof(psm_epid_t));
   
   ret = psm_init(&verno_major, &verno_minor);
   MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_init", "**psm_init %s", psm_error_get_string(ret));

   mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   if (MPID_nem_mem_region.rank == 0)
   {
       char uuid_val[MPID_NEM_MAX_KEY_VAL_LEN];
       
       psm_uuid_generate(MPID_nem_module_psm_uuid);

       /* Encode the generated UUID into a string and broadcast it to others */
       ret = encode_buffer(uuid_val, len, (char*) MPID_nem_module_psm_uuid, sizeof(psm_uuid_t), &len);
       MPIU_ERR_CHKANDJUMP1 (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**encode_buffer", "**encode_buffer %d", ret);
       
       pmi_errno = PMI_KVS_Put (kvs_name, MPIDI_CH3I_UUID_KEY, uuid_val);
       MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

       pmi_errno = PMI_KVS_Commit (kvs_name);
       MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
   }
   
   pmi_errno = PMI_Barrier();
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
   
   if (MPID_nem_mem_region.rank != 0)
   {
       /* Get UUID from root and decode it */
       pmi_errno = PMI_KVS_Get (kvs_name, MPIDI_CH3I_UUID_KEY, get_val, MPID_NEM_MAX_KEY_VAL_LEN);
       MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

       ret = decode_buffer(get_val, (char*) MPID_nem_module_psm_uuid, sizeof(psm_uuid_t), &len);
       MPIU_ERR_CHKANDJUMP1 (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**decode_buffer", "**decode_buffer %d", ret);

   }


   /* Open endpoint passing the uuid generated by root */
   ret = psm_ep_open(MPID_nem_module_psm_uuid, NULL, &MPID_nem_module_psm_local_endpoint, &MPID_nem_module_psm_local_endpoint_id);
   MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_ep_open", "**psm_ep_open %s", psm_error_get_string (ret));

   /* Own endpoint */
   MPID_nem_module_psm_endpoint_ids[MPID_nem_mem_region.rank] = MPID_nem_module_psm_local_endpoint_id;

   /* Initialize message queues */
   ret = psm_mq_init(MPID_nem_module_psm_local_endpoint, PSM_MQ_ORDERMASK_ALL, NULL, 0, &MPID_nem_module_psm_mq);
   MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_mq_init", "**psm_mq_init %s", psm_error_get_string (ret));

   /* Broadcast endpoints to others */
   MPIU_Snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%lld",MPID_nem_module_psm_local_endpoint_id);
   MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "Endpointid_key[%d]", MPID_nem_mem_region.rank);

   pmi_errno = PMI_KVS_Put (kvs_name, key, val);
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

   pmi_errno = PMI_KVS_Commit (kvs_name);
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

   pmi_errno = PMI_Barrier();
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

   /* Get endpoint ids of all other processes */
   for(index = 0; index < MPID_nem_mem_region.num_procs;index++)
   {
       if (index != MPID_nem_mem_region.rank)
       {
   
           MPIU_Snprintf (get_key, MPID_NEM_MAX_KEY_VAL_LEN, "Endpointid_key[%d]", index);
           
           pmi_errno = PMI_KVS_Get(kvs_name, get_key, get_val, MPID_NEM_MAX_KEY_VAL_LEN);
           ret = sscanf(get_val, "%lld", &MPID_nem_module_psm_endpoint_ids[index]);
           MPIU_ERR_CHKANDJUMP1 (ret != 1, mpi_errno, MPI_ERR_OTHER, "**business_card", "**business_card %s", get_val);
       }
       
   }

#if 1
   /* Fix me : mysteriously  MPIU_CHKPMEM_MALLOC fails here, when the regular MPIU_Malloc doesn't ... */
   MPID_nem_module_psm_send_outstanding_request =(MPID_nem_psm_cell_ptr_t)MPIU_Malloc(MPID_NEM_PSM_REQ * sizeof(MPID_nem_psm_cell_t));
   MPID_nem_module_psm_recv_outstanding_request =(MPID_nem_psm_cell_ptr_t)MPIU_Malloc(MPID_NEM_PSM_REQ * sizeof(MPID_nem_psm_cell_t));
   MPID_nem_module_psm_pendings_recvs_array     =(int *)MPIU_Malloc(MPID_nem_mem_region.num_procs * sizeof(int));
#else
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_psm_send_outstanding_request, MPID_nem_psm_cell_ptr_t, MPID_NEM_PSM_REQ * sizeof(MPID_nem_psm_cell_t), mpi_errno, "send outstanding req");
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_psm_recv_outstanding_request, MPID_nem_psm_cell_ptr_t, MPID_NEM_PSM_REQ * sizeof(MPID_nem_psm_cell_t), mpi_errno, "recv outstanding req");
   MPIU_CHKPMEM_MALLOC (MPID_nem_module_psm_pendings_recvs_array,int *, MPID_nem_mem_region.num_procs * sizeof(int), mpi_errno, "pending recvs array");
#endif
   memset(MPID_nem_module_psm_send_outstanding_request,0,MPID_NEM_PSM_REQ*sizeof(MPID_nem_psm_cell_t));
   memset(MPID_nem_module_psm_recv_outstanding_request,0,MPID_NEM_PSM_REQ*sizeof(MPID_nem_psm_cell_t));
   for (index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
   {
       MPID_nem_module_psm_pendings_recvs_array[index] = 0;
   }
   
   MPID_nem_module_psm_send_free_req_queue->head    = NULL;
   MPID_nem_module_psm_send_free_req_queue->tail    = NULL;
   MPID_nem_module_psm_send_pending_req_queue->head = NULL;
   MPID_nem_module_psm_send_pending_req_queue->tail = NULL;
   
   MPID_nem_module_psm_recv_free_req_queue->head    = NULL;
   MPID_nem_module_psm_recv_free_req_queue->tail    = NULL;
   MPID_nem_module_psm_recv_pending_req_queue->head = NULL;
   MPID_nem_module_psm_recv_pending_req_queue->tail = NULL;
   
   for (index = 0; index < MPID_NEM_PSM_REQ ; ++index)
     {
	MPID_nem_psm_req_queue_enqueue (MPID_nem_module_psm_send_free_req_queue,&MPID_nem_module_psm_send_outstanding_request[index]);
	MPID_nem_psm_req_queue_enqueue (MPID_nem_module_psm_recv_free_req_queue,&MPID_nem_module_psm_recv_outstanding_request[index]);
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
   MPID_nem_psm_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
#define FUNCNAME MPID_nem_psm_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;
   
   psm_error_t ret;
   int index;


   if( MPID_nem_mem_region.ext_procs > 0)
   {
	init_psm(pg_p);
	if (mpi_errno) MPIU_ERR_POP (mpi_errno);
   }

      

   MPID_nem_process_recv_queue = proc_recv_queue;
   MPID_nem_process_free_queue = proc_free_queue;
   
   MPID_nem_module_psm_free_queue = &_free_queue;
   
   MPID_nem_queue_init (MPID_nem_module_psm_free_queue);
   
   for (index = 0; index < num_module_elements; ++index)
   {
	MPID_nem_queue_enqueue (MPID_nem_module_psm_free_queue, &module_elements[index]);
   }
   
   *module_free_queue = MPID_nem_module_psm_free_queue;

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
   int mpi_errno = MPI_SUCCESS;
   int i;
   psm_error_t ret;

/*    mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ENDPOINT_KEY, */
/*                                         (char *)&MPID_nem_module_psm_local_endpoint_id, sizeof(psm_epid_t)); */
/*    if (mpi_errno != MPIU_STR_SUCCESS) */
/*    { */
/*        if (mpi_errno == MPIU_STR_NOMEM)  */
/*        { */
/*            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len"); */
/*        } */
/*        else  */
/*        { */
/*            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard"); */
/*        } */
/*        goto fn_exit; */
/*    } */
   
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_get_from_bc (const char *business_card, psm_epid_t *remote_endpoint_id)
{
   int mpi_errno = MPI_SUCCESS;
   int len;
   psm_epid_t tmp_endpoint_id;
   
/*    mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_ENDPOINT_KEY, (char *)remote_endpoint_id, sizeof(psm_epid_t), &len); */
/*    if (mpi_errno != MPIU_STR_SUCCESS || len != sizeof(psm_epid_t))  */
/*    { */
/* 	/\* FIXME: create a real error string for this *\/ */
/* 	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd"); */
/*    } */
      
   fn_exit:
     return mpi_errno;
   fn_fail:  
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
   int mpi_errno = MPI_SUCCESS;
   int ret;
   
   
   /* first make sure that our private fields in the vc fit into the area provided  */
   MPIU_Assert(sizeof(MPID_nem_psm_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

   

/*    if( MPID_nem_mem_region.ext_procs > 0) */
/*    { */
/* 	mpi_errno = MPID_nem_psm_module_get_from_bc (business_card, &VC_FIELD(vc, remote_endpoint_id)); */
/* 	/\* --BEGIN ERROR HANDLING-- *\/    */
/* 	if (mpi_errno)  */
/* 	  {	 */
/* 	     MPIU_ERR_POP (mpi_errno); */
/* 	  }    */
/* 	/\* --END ERROR HANDLING-- *\/ */

/*         MPID_nem_module_psm_endpoint_ids[vc->pg_rank] = VC_FIELD(vc, remote_endpoint_id); */
/*    } */
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_psm_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    /* free any resources associated with this VC here */

    
 fn_exit:   
       return mpi_errno;
 fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_psm_module_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_exchange_endpoints
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_psm_module_exchange_endpoints(void)
{

    int          index;
    psm_error_t  ret;
    int          mpi_errno = MPI_SUCCESS;
    int          pmi_errno;
    
    int          verno_minor = PSM_VERNO_MINOR;
    int          verno_major = PSM_VERNO_MAJOR;
    
    char         *kvs_name;
    
    char         key[MPID_NEM_MAX_KEY_VAL_LEN], get_key[MPID_NEM_MAX_KEY_VAL_LEN];
    char         val[MPID_NEM_MAX_KEY_VAL_LEN], get_val[MPID_NEM_MAX_KEY_VAL_LEN];
    int          size = sizeof(psm_uuid_t) + 1;
    int          len = MPID_NEM_MAX_KEY_VAL_LEN;
    
    ret = psm_init(&verno_major, &verno_minor);
    MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_init", "**psm_init %s", psm_error_get_string(ret));
    
    mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    if (MPID_nem_mem_region.rank == 0)
    {
        char uuid_val[MPID_NEM_MAX_KEY_VAL_LEN];
        
        psm_uuid_generate(MPID_nem_module_psm_uuid);

        /* Pack UUID into character array */
/*         uuid_val = (char *)MPIU_Malloc(size); */
/*         strncpy(uuid_val, MPID_nem_module_psm_uuid, size-1); */
/*         uuid_val[size-1] = 0; */

        ret = encode_buffer(uuid_val, len, (char*) MPID_nem_module_psm_uuid, sizeof(psm_uuid_t), &len);
        MPIU_ERR_CHKANDJUMP1 (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**encode_buffer", "**encode_buffer %d", ret);
        
/*         if (mpi_errno != MPIU_STR_SUCCESS) */
/*         { */
/*             if (mpi_errno == MPIU_STR_NOMEM) */
/*             { */
/*                 MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len"); */
/*             } */
/*             else */
/*             { */
/*                 MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard"); */
/*             } */
/*             goto fn_exit; */
/*         } */

/*         memcpy(uuid_val, MPID_nem_module_psm_uuid, size-1); */
/*         memcpy(uuid_val+size, &zero, 1); */

/*         for(index = 0;index<size-1;index++) */
/*         { */
/*             if(uuid_val[index] == 0) */
/*             { */
/*                 temp=1; */
/*                 break; */
/*             } */
/*         } */

        printf("uuid = %s\n", uuid_val); 
        
        pmi_errno = PMI_KVS_Put (kvs_name, MPIDI_CH3I_UUID_KEY, uuid_val);
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
        
        pmi_errno = PMI_KVS_Commit (kvs_name);
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
    }
    
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
    
    if (MPID_nem_mem_region.rank != 0)
    {
        pmi_errno = PMI_KVS_Get (kvs_name, MPIDI_CH3I_UUID_KEY, get_val, MPID_NEM_MAX_KEY_VAL_LEN);
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

        ret = decode_buffer(get_val, (char*) MPID_nem_module_psm_uuid, sizeof(psm_uuid_t), &len);
        MPIU_ERR_CHKANDJUMP1 (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**encode_buffer", "**encode_buffer %d", ret);

/*         mpi_errno = MPIU_Str_get_binary_arg (get_val, MPIDI_CH3I_UUID_KEY, (char *)MPID_nem_module_psm_uuid, sizeof(psm_uuid_t), &len); */
/*         if (mpi_errno != MPIU_STR_SUCCESS || len != sizeof(psm_epid_t)) */
/*         { */
/*             /\* FIXME: create a real error string for this *\/ */
/*             MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd"); */
/*         } */

         printf("%d uuid = %s\n", MPID_nem_mem_region.rank, MPID_nem_module_psm_uuid); 
        
    }

/*     for(index=0;index<16;index++) */
/*     { */
/*         MPID_nem_module_psm_uuid[index] = index; */
/*     } */
    
    
    ret = psm_ep_open(MPID_nem_module_psm_uuid, NULL, &MPID_nem_module_psm_local_endpoint, &MPID_nem_module_psm_local_endpoint_id);
    MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_ep_open", "**psm_ep_open %s", psm_error_get_string (ret)); 
    
    MPID_nem_module_psm_endpoint_ids[MPID_nem_mem_region.rank] = MPID_nem_module_psm_local_endpoint_id;
    
    ret = psm_mq_init(MPID_nem_module_psm_local_endpoint, PSM_MQ_ORDERMASK_ALL, NULL, 0, &MPID_nem_module_psm_mq);
    MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_mq_init", "**psm_mq_init %s", psm_error_get_string (ret));
    
    MPIU_Snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%lld",MPID_nem_module_psm_local_endpoint_id);
    MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "Endpointid[%d]", MPID_nem_mem_region.rank);
    
    for(index = 0; index < MPID_nem_mem_region.num_procs;index++)
    {
        if (index ==  MPID_nem_mem_region.rank)
        {
            
            pmi_errno = PMI_KVS_Put (kvs_name, key, val);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
    
            pmi_errno = PMI_KVS_Commit (kvs_name);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
        }
        
        pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
        
        if (index != MPID_nem_mem_region.rank)
        {
            
            MPIU_Snprintf (get_key, MPID_NEM_MAX_KEY_VAL_LEN, "Endpointid[%d]", index);
            
            pmi_errno = PMI_KVS_Get(kvs_name, get_key, get_val, MPID_NEM_MAX_KEY_VAL_LEN);
            ret = sscanf(get_val, "%lld", &MPID_nem_module_psm_endpoint_ids[index]);
            MPIU_ERR_CHKANDJUMP1 (ret != 1, mpi_errno, MPI_ERR_OTHER, "**business_card", "**business_card %s", get_val);
        }
        
    }

    MPID_nem_module_psm_initialized = 1;
    

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
    
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_psm_module_connect (void)
{
    int         mpi_errno = MPI_SUCCESS;
    psm_error_t ret;
    psm_error_t *errors;

/*     if(!MPID_nem_module_psm_initialized) */
/*     { */
/*         ret = MPID_nem_psm_module_exchange_endpoints(); */
/*         MPIU_ERR_CHKANDJUMP1 (ret != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_psm_module_exchange_endpoints", "**MPID_nem_psm_module_exchange_endpoints %d", ret); */
/*     } */

    errors = (psm_error_t *) malloc(sizeof(psm_error_t) * MPID_nem_mem_region.num_procs);
    
    ret = psm_ep_connect(MPID_nem_module_psm_local_endpoint,
                         MPID_nem_mem_region.num_procs,
                         MPID_nem_module_psm_endpoint_ids,
                         NULL,
                         errors,
                         MPID_nem_module_psm_endpoint_addrs,
                         MPID_nem_module_psm_timeout);
    
    MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_ep_connect", "**psm_ep_connect %s", psm_error_get_string (ret));
    free(errors);

    MPID_nem_module_psm_connected = 1;

 fn_exit:
        return mpi_errno;
 fn_fail:
        goto fn_exit;
        
}
